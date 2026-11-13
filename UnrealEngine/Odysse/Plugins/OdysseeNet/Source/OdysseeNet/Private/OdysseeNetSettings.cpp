#include "OdysseeNetSettings.h"

#include "OdysseeRemotePlayer.h"
#include "OdysseeResourceActor.h"

UOdysseeNetSettings::UOdysseeNetSettings()
{
	RemotePlayerClass = AOdysseeRemotePlayer::StaticClass();
	ResourceClass = AOdysseeResourceActor::StaticClass();
}
