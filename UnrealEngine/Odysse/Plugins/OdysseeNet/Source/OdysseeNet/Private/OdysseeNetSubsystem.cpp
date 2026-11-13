#include "OdysseeNetSubsystem.h"

#include "OdysseeNetSettings.h"
#include "OdysseeRemotePlayer.h"
#include "OdysseeUnits.h"
#include "OdysseeWorldSubsystem.h"

#include "Engine/GameInstance.h"
#include "Engine/World.h"

#include "odyssee/core/Config.hpp"
#include "odyssee/net/Address.hpp"
#include "odyssee/net/ControlMessages.hpp"
#include "odyssee/world/WorldFingerprint.hpp"

#include <chrono>
#include <string>
#include <utility>

DEFINE_LOG_CATEGORY_STATIC(LogOdysseeNet, Log, All);

#define LOCTEXT_NAMESPACE "OdysseeNet"

namespace
{
	using Clock = std::chrono::steady_clock;

	/** « ip » ou « ip:port » → adresse du serveur (port par défaut sinon). */
	std::optional<odyssee::net::Address> ParseAddress(const FString& Text)
	{
		FString Ip = Text.TrimStartAndEnd();
		int32 Port = odyssee::kDefaultPort;
		FString PortText;
		if (Ip.Split(TEXT(":"), &Ip, &PortText))
		{
			if (!PortText.IsNumeric())
			{
				return std::nullopt;
			}
			Port = FCString::Atoi(*PortText);
		}
		if (Port <= 0 || Port > 65535)
		{
			return std::nullopt;
		}
		return odyssee::net::Address::parse(std::string(TCHAR_TO_UTF8(*Ip)),
			static_cast<std::uint16_t>(Port));
	}

	std::string ToUtf8(const FString& Text)
	{
		return std::string(TCHAR_TO_UTF8(*Text.TrimStartAndEnd()));
	}
}

bool UOdysseeNetSubsystem::Connect(const FString& Address, const FString& PlayerName,
	const FString& InviteCode)
{
	Disconnect();

	const auto Server = ParseAddress(Address);
	if (!Server)
	{
		SetConnection(EOdysseeConnection::Failed,
			LOCTEXT("BadAddress", "Adresse invalide (exemple : 89.92.152.61:58121)"));
		return false;
	}
	Link = odyssee::client::ServerLink::open(ToUtf8(PlayerName), *Server, ToUtf8(InviteCode));
	if (!Link)
	{
		SetConnection(EOdysseeConnection::Failed, LOCTEXT("NoSocket", "Impossible d'ouvrir un socket UDP"));
		return false;
	}

	++SessionId; // la prédiction repart de zéro
	++SnapshotVersion;
	Remote = odyssee::client::Interpolation{};
	Link->connect(Clock::now());
	UE_LOG(LogOdysseeNet, Log, TEXT("Connexion à %s…"), *Address);
	SetConnection(EOdysseeConnection::Connecting, LOCTEXT("Connecting", "Connexion…"));
	return true;
}

void UOdysseeNetSubsystem::Disconnect()
{
	if (Link)
	{
		Link->disconnect();
		Link->poll(Clock::now()); // prévient le serveur : la place se libère tout de suite
		Link.reset();
	}
	Latest.reset();
	RemoveRemotePlayers();
	if (Connection == EOdysseeConnection::Connecting || Connection == EOdysseeConnection::Connected)
	{
		SetConnection(EOdysseeConnection::Offline, LOCTEXT("Offline", "Hors ligne"));
	}
}

void UOdysseeNetSubsystem::Deinitialize()
{
	Disconnect();
	Super::Deinitialize();
}

int32 UOdysseeNetSubsystem::GetPingMs() const
{
	if (!Link || !Link->session() || !Link->session()->roundTripTime())
	{
		return -1;
	}
	const auto RoundTrip = *Link->session()->roundTripTime();
	return static_cast<int32>(std::chrono::duration_cast<std::chrono::milliseconds>(RoundTrip).count());
}

float UOdysseeNetSubsystem::GetLossPercent() const
{
	return Link ? static_cast<float>(Link->traffic(Clock::now()).incomingLossPercent) : 0.0f;
}

namespace
{
	float VitalFraction(std::uint8_t Percent)
	{
		return static_cast<float>(Percent) / static_cast<float>(odyssee::net::kMaxVital);
	}
}

float UOdysseeNetSubsystem::GetHealth() const
{
	return Latest ? VitalFraction(Latest->vitals.health) : 1.0f;
}

float UOdysseeNetSubsystem::GetHunger() const
{
	return Latest ? VitalFraction(Latest->vitals.hunger) : 1.0f;
}

float UOdysseeNetSubsystem::GetThirst() const
{
	return Latest ? VitalFraction(Latest->vitals.thirst) : 1.0f;
}

std::optional<odyssee::net::PlayerId> UOdysseeNetSubsystem::GetPlayerId() const
{
	return Link ? Link->playerId() : std::nullopt;
}

void UOdysseeNetSubsystem::SendInputs(const odyssee::net::InputBatch& Batch)
{
	if (Link && Link->connected())
	{
		Link->sendInputs(Batch, Clock::now());
	}
}

void UOdysseeNetSubsystem::Tick(float DeltaTime)
{
	if (!Link)
	{
		return;
	}
	Link->poll(Clock::now());
	KeepSnapshots();
	UpdateConnectionState();
	if (Link) // une connexion perdue vient peut-être d'être fermée
	{
		Remote.advance(static_cast<double>(DeltaTime) * odyssee::kTickRate);
		ShowRemotePlayers(DeltaTime);
	}
}

void UOdysseeNetSubsystem::KeepSnapshots()
{
	for (auto& Snapshot : Link->takeSnapshots())
	{
		Remote.push(Snapshot); // tous, même en retard : ils servent à interpoler
		if (!Latest || Snapshot.serverTick > Latest->serverTick)
		{
			Latest = std::move(Snapshot);
			++SnapshotVersion;
		}
	}
}

void UOdysseeNetSubsystem::UpdateConnectionState()
{
	using odyssee::net::ConnectionState;
	switch (Link->state())
	{
	case ConnectionState::Connected:
		if (Connection != EOdysseeConnection::Connected && !HasServerMap())
		{
			Disconnect();
			SetConnection(EOdysseeConnection::Failed,
				LOCTEXT("OtherMap", "Carte différente de celle du serveur : mettez le jeu à jour"));
			return;
		}
		if (Connection != EOdysseeConnection::Connected)
		{
			UE_LOG(LogOdysseeNet, Log, TEXT("Connecté : joueur %d"), static_cast<int32>(*Link->playerId()));
			SetConnection(EOdysseeConnection::Connected, LOCTEXT("Connected", "Connecté"));
		}
		return;
	case ConnectionState::Refused:
	{
		const auto Reason = Link->refuseReason();
		const FString Why = Reason ? FString(UTF8_TO_TCHAR(std::string(odyssee::net::describe(*Reason)).c_str()))
								   : FString(TEXT("raison inconnue"));
		Disconnect();
		SetConnection(EOdysseeConnection::Failed,
			FText::Format(LOCTEXT("Refused", "Connexion refusée : {0}"), FText::FromString(Why)));
		return;
	}
	case ConnectionState::TimedOut:
		Disconnect();
		SetConnection(EOdysseeConnection::Failed,
			LOCTEXT("TimedOut", "Le serveur ne répond pas (adresse, port ou code ?)"));
		return;
	case ConnectionState::Closed:
		Disconnect();
		SetConnection(EOdysseeConnection::Failed, LOCTEXT("Closed", "Déconnecté par le serveur"));
		return;
	case ConnectionState::Idle:
	case ConnectionState::Requesting:
	case ConnectionState::Answering:
		return;
	}
}

void UOdysseeNetSubsystem::SetConnection(EOdysseeConnection NewState, const FText& NewStatus)
{
	Connection = NewState;
	Status = NewStatus;
	if (NewState == EOdysseeConnection::Failed)
	{
		UE_LOG(LogOdysseeNet, Warning, TEXT("%s"), *NewStatus.ToString());
	}
	OnConnectionChanged.Broadcast(NewState, NewStatus);
}

void UOdysseeNetSubsystem::ShowRemotePlayers(float DeltaTime)
{
	TSet<int32> Seen;
	for (const auto& Entity : Remote.sample(GetPlayerId()))
	{
		const int32 Id = static_cast<int32>(Entity.id);
		Seen.Add(Id);
		TObjectPtr<AOdysseeRemotePlayer>* Found = RemotePlayers.Find(Id);
		AOdysseeRemotePlayer* Player = Found && IsValid(*Found) ? Found->Get() : SpawnRemotePlayer(Id);
		if (Player)
		{
			const FVector Feet(Entity.position[0] * OdysseeUnits::CentimetersPerUnit,
				Entity.position[1] * OdysseeUnits::CentimetersPerUnit,
				Entity.position[2] * OdysseeUnits::CentimetersPerUnit);
			Player->ShowAt(Feet, static_cast<float>(Entity.yawDegrees), DeltaTime);
		}
	}
	for (auto It = RemotePlayers.CreateIterator(); It; ++It)
	{
		if (!Seen.Contains(It.Key()))
		{
			if (IsValid(It.Value()))
			{
				It.Value()->Destroy(); // parti, ou hors de vue
			}
			It.RemoveCurrent();
		}
	}
}

AOdysseeRemotePlayer* UOdysseeNetSubsystem::SpawnRemotePlayer(int32 Id)
{
	UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;
	if (!World)
	{
		return nullptr;
	}
	UClass* Class = GetDefault<UOdysseeNetSettings>()->RemotePlayerClass.LoadSynchronous();
	if (!Class)
	{
		Class = AOdysseeRemotePlayer::StaticClass();
	}
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AOdysseeRemotePlayer* Player = World->SpawnActor<AOdysseeRemotePlayer>(Class, FTransform::Identity, Params);
	if (Player)
	{
		Player->PlayerId = Id;
		RemotePlayers.Add(Id, Player);
	}
	return Player;
}

void UOdysseeNetSubsystem::RemoveRemotePlayers()
{
	for (auto& Entry : RemotePlayers)
	{
		if (IsValid(Entry.Value))
		{
			Entry.Value->Destroy();
		}
	}
	RemotePlayers.Reset();
}

bool UOdysseeNetSubsystem::HasServerMap() const
{
	const auto ServerMap = Link->serverWorldFingerprint();
	const UOdysseeWorldSubsystem* WorldMap = GetGameInstance()->GetSubsystem<UOdysseeWorldSubsystem>();
	const std::uint64_t OwnMap = WorldMap && WorldMap->GetMap()
		? WorldMap->GetMap()->world().fingerprint
		: odyssee::world::kNoMapFingerprint;
	return ServerMap && *ServerMap == OwnMap;
}

TStatId UOdysseeNetSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UOdysseeNetSubsystem, STATGROUP_Tickables);
}

ETickableTickType UOdysseeNetSubsystem::GetTickableTickType() const
{
	// L'objet par défaut de la classe (CDO) ne doit pas tourner : seulement le vrai sous-système.
	return IsTemplate() ? ETickableTickType::Never : ETickableTickType::Always;
}

#undef LOCTEXT_NAMESPACE
