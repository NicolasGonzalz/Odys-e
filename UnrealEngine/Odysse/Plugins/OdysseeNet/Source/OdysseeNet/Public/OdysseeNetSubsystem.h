// Connexion au serveur de jeu (T027) : poignée de main, snapshots, autres joueurs.
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"

#include "odyssee/client/Interpolation.hpp"
#include "odyssee/client/ServerLink.hpp"
#include "odyssee/data/Catalog.hpp"
#include "odyssee/net/GameEvents.hpp"
#include "odyssee/net/GameMessages.hpp"

#include <cstdint>
#include <optional>

#include "OdysseeNetSubsystem.generated.h"

class AOdysseeRemotePlayer;
class UOdysseeResourceSync;

UENUM(BlueprintType)
enum class EOdysseeConnection : uint8
{
	Offline,    ///< pas de partie en ligne
	Connecting, ///< poignée de main en cours
	Connected,
	Failed,     ///< refusée ou perdue : voir GetStatusText
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOdysseeConnectionChanged, EOdysseeConnection, State,
	const FText&, Status);

/** Issue d'une fabrication (mêmes valeurs que net::CraftOutcome). */
UENUM(BlueprintType)
enum class EOdysseeCraftOutcome : uint8
{
	Started = 1,
	Done = 2,
	Busy = 3,
	NeedStation = 4,
	MissingIngredients = 5,
	UnknownRecipe = 6,
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOdysseeCraftChanged, FName, RecipeKey,
	EOdysseeCraftOutcome, Outcome, float, Seconds);

/** Une recette (data/recipes.txt), prête à afficher dans l'interface d'artisanat (T048). */
USTRUCT(BlueprintType)
struct FOdysseeRecipe
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Odyssee|Artisanat")
	FName Key;

	/** Nom de l'objet fabriqué. */
	UPROPERTY(BlueprintReadOnly, Category = "Odyssee|Artisanat")
	FText Name;

	/** « hand », « workbench » ou « forge ». */
	UPROPERTY(BlueprintReadOnly, Category = "Odyssee|Artisanat")
	FName Station;

	/** « 1 Bâton, 2 Pierre… » */
	UPROPERTY(BlueprintReadOnly, Category = "Odyssee|Artisanat")
	FText Ingredients;

	UPROPERTY(BlueprintReadOnly, Category = "Odyssee|Artisanat")
	float Seconds = 0.0f;
};

/** Une case de l'inventaire du joueur local (T045), prête à afficher. */
USTRUCT(BlueprintType)
struct FOdysseeInventorySlot
{
	GENERATED_BODY()

	/** Identifiant stable de l'objet (`stone_axe`), vide si la case est vide. */
	UPROPERTY(BlueprintReadOnly, Category = "Odyssee|Inventaire")
	FName ItemKey;

	UPROPERTY(BlueprintReadOnly, Category = "Odyssee|Inventaire")
	FText Name;

	UPROPERTY(BlueprintReadOnly, Category = "Odyssee|Inventaire")
	int32 Count = 0;

	/** Nom de base du modèle et de l'icône (`hache_pierre`, d'après data/items.txt). */
	UPROPERTY(BlueprintReadOnly, Category = "Odyssee|Inventaire")
	FString IconName;
};

/**
 * Le lien avec le serveur, pour toute la durée du jeu (il survit aux changements de carte).
 *
 * - Connect() ouvre la partie ; à chaque image, le lien envoie et reçoit (client::ServerLink).
 * - Les snapshots reçus nourrissent la prédiction du joueur local (UOdysseeMovementComponent, qui
 *   envoie aussi ses inputs par SendInputs) et l'affichage des autres joueurs, interpolés avec un
 *   léger retard (T033) : un AOdysseeRemotePlayer par joueur.
 * - Les ressources récoltables du dernier snapshot sont montrées telles quelles, sans interpolation
 *   (T044) : un AOdysseeResourceActor par ressource (UOdysseeResourceSync).
 */
UCLASS()
class ODYSSEENET_API UOdysseeNetSubsystem : public UGameInstanceSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

public:
	/**
	 * Rejoint un serveur. `Address` : « 89.92.152.61 » ou « 89.92.152.61:58121 » (port 27015 par
	 * défaut). Faux si l'adresse est invalide (raison dans GetStatusText).
	 */
	UFUNCTION(BlueprintCallable, Category = "Odyssee|Reseau")
	bool Connect(const FString& Address, const FString& PlayerName, const FString& InviteCode);

	/** Quitte la partie (le serveur est prévenu) et retire les autres joueurs et les ressources. */
	UFUNCTION(BlueprintCallable, Category = "Odyssee|Reseau")
	void Disconnect();

	UFUNCTION(BlueprintPure, Category = "Odyssee|Reseau")
	EOdysseeConnection GetConnection() const { return Connection; }

	/** « Connexion… », « Connecté », ou pourquoi ça a échoué. */
	UFUNCTION(BlueprintPure, Category = "Odyssee|Reseau")
	FText GetStatusText() const { return Status; }

	/** Aller-retour mesuré, en ms (-1 tant qu'il n'est pas connu). */
	UFUNCTION(BlueprintPure, Category = "Odyssee|Reseau")
	int32 GetPingMs() const;

	/** Paquets du serveur perdus sur la dernière seconde, en %. */
	UFUNCTION(BlueprintPure, Category = "Odyssee|Reseau")
	float GetLossPercent() const;

	/** Besoins vitaux du joueur local, de 0 à 1, calculés par le serveur (T051). Pleins hors ligne. */
	UFUNCTION(BlueprintPure, Category = "Odyssee|Survie")
	float GetHealth() const;

	UFUNCTION(BlueprintPure, Category = "Odyssee|Survie")
	float GetHunger() const;

	UFUNCTION(BlueprintPure, Category = "Odyssee|Survie")
	float GetThirst() const;

	UPROPERTY(BlueprintAssignable, Category = "Odyssee|Reseau")
	FOdysseeConnectionChanged OnConnectionChanged;

	/** L'inventaire du joueur (25 cases, les 5 premières = barre d'outils), tel que le serveur
	 *  l'a envoyé. Vide avant la première réception. */
	UFUNCTION(BlueprintPure, Category = "Odyssee|Inventaire")
	TArray<FOdysseeInventorySlot> GetInventory() const;

	/** Demande au serveur de déplacer une case vers une autre (échange ou regroupement). Le
	 *  serveur vérifie, puis renvoie l'inventaire. */
	UFUNCTION(BlueprintCallable, Category = "Odyssee|Inventaire")
	void RequestMoveItem(int32 From, int32 To);

	/** Toutes les recettes du jeu (T047), dans l'ordre de data/recipes.txt. */
	UFUNCTION(BlueprintPure, Category = "Odyssee|Artisanat")
	TArray<FOdysseeRecipe> GetRecipes() const;

	/** Demande au serveur de fabriquer une recette : la réponse arrive par OnCraftChanged. */
	UFUNCTION(BlueprintCallable, Category = "Odyssee|Artisanat")
	void RequestCraft(FName RecipeKey);

	/** Réponse du serveur : fabrication commencée (durée), finie, ou refusée (pourquoi). */
	UPROPERTY(BlueprintAssignable, Category = "Odyssee|Artisanat")
	FOdysseeCraftChanged OnCraftChanged;

	// --- Pour la prédiction (C++) ---------------------------------------------------------------

	bool IsConnected() const { return Connection == EOdysseeConnection::Connected; }
	std::optional<odyssee::net::PlayerId> GetPlayerId() const;
	/** Dernier snapshot reçu (le plus récent du serveur). */
	const std::optional<odyssee::net::Snapshot>& GetLatestSnapshot() const { return Latest; }
	/** Change à chaque nouveau snapshot, et à chaque nouvelle connexion (repartir de zéro). */
	uint64 GetSnapshotVersion() const { return SnapshotVersion; }
	uint32 GetSessionId() const { return SessionId; }
	void SendInputs(const odyssee::net::InputBatch& Batch);

	// --- FTickableGameObject ---------------------------------------------------------------------

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual ETickableTickType GetTickableTickType() const override;
	virtual bool IsTickableWhenPaused() const override { return true; }

private:
	void KeepSnapshots();
	void KeepEvents();
	const odyssee::data::Catalog* FindCatalog() const;
	FName RecipeKeyOf(std::uint16_t Recipe) const;
	void UpdateConnectionState();
	/** Vrai si le serveur annonce la même carte que celle chargée ici (UOdysseeWorldSubsystem). */
	bool HasServerMap() const;
	void SetConnection(EOdysseeConnection NewState, const FText& NewStatus);
	void ShowRemotePlayers(float DeltaTime);
	AOdysseeRemotePlayer* SpawnRemotePlayer(int32 Id);
	void RemoveRemotePlayers();
	void ShowResources();

	std::optional<odyssee::client::ServerLink> Link;
	std::optional<odyssee::net::Snapshot> Latest;
	std::optional<odyssee::net::InventoryContents> Inventory;
	odyssee::client::Interpolation Remote;
	uint64 SnapshotVersion = 0;
	uint32 SessionId = 0;
	uint64 ShownResources = 0; ///< SnapshotVersion dont les ressources sont montrées

	EOdysseeConnection Connection = EOdysseeConnection::Offline;
	FText Status;

	UPROPERTY(Transient)
	TMap<int32, TObjectPtr<AOdysseeRemotePlayer>> RemotePlayers;

	UPROPERTY(Transient)
	TObjectPtr<UOdysseeResourceSync> Resources;
};
