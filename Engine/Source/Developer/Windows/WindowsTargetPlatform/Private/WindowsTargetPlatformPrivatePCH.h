// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once


/* Private dependencies
 *****************************************************************************/

#include "Core.h"
#include "WindowsPlatformProperties.h"

#if WITH_ENGINE
	#include "Engine.h"
	#include "TextureCompressorModule.h"
#endif

#include "TargetPlatform.h"
#include "TargetPlatformBase.h"

#include "AllowWindowsPlatformTypes.h"
	#include <TlHelp32.h>
#include "HideWindowsPlatformTypes.h"


/* Private includes
 *****************************************************************************/

#include "WindowsTargetSettings.h"
#include "LocalPcTargetDevice.h"
#include "GenericWindowsTargetPlatform.h"
