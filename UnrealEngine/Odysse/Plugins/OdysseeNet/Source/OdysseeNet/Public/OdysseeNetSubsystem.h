// Connexion au serveur de jeu (T027) : poignée de main, snapshots, autres joueurs.
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"

#include "odyssee/client/Interpolation.hpp"
#include "odyssee/client/ServerLink.hpp"
#include "odyssee/net/GameMessages.hpp"

#include <cstdint>
#include <optional>

#include "OdysseeNetSubsystem.generated.h"

class AOdysseeRemotePlayer;

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

/**
 * Le lien avec le serveur, pour toute la durée du jeu (il survit aux changements de carte).
 *
 * - Connect() ouvre la partie ; à chaque image, le lien envoie et reçoit (client::ServerLink).
 * - Les snapshots reçus nourrissent la prédiction du joueur local (UOdysseeMovementComponent, qui
 *   envoie aussi ses inputs par SendInputs) et l'affichage des autres joueurs, interpolés avec un
 *   léger retard (T033) : un AOdysseeRemotePlayer par joueur.
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

	/** Quitte la partie (le serveur est prévenu) et retire les autres joueurs. */
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

	UPROPERTY(BlueprintAssignable, Category = "Odyssee|Reseau")
	FOdysseeConnectionChanged OnConnectionChanged;

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

	virtual void Deinitialize() override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual ETickableTickType GetTickableTickType() const override;
	virtual bool IsTickableWhenPaused() const override { return true; }

private:
	void KeepSnapshots();
	void UpdateConnectionState();
	void SetConnection(EOdysseeConnection NewState, const FText& NewStatus);
	void ShowRemotePlayers(float DeltaTime);
	AOdysseeRemotePlayer* SpawnRemotePlayer(int32 Id);
	void RemoveRemotePlayers();

	std::optional<odyssee::client::ServerLink> Link;
	std::optional<odyssee::net::Snapshot> Latest;
	odyssee::client::Interpolation Remote;
	uint64 SnapshotVersion = 0;
	uint32 SessionId = 0;

	EOdysseeConnection Connection = EOdysseeConnection::Offline;
	FText Status;

	UPROPERTY(Transient)
	TMap<int32, TObjectPtr<AOdysseeRemotePlayer>> RemotePlayers;
};
