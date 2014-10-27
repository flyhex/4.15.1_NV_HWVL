// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "UnrealEd.h"
#include "SlateBasics.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Kismet2/KismetDebugUtilities.h"
#include "Kismet2/DebuggerCommands.h"
#include "BlueprintUtilities.h"

#include "LauncherServices.h"
#include "ISettingsModule.h"
#include "TargetDeviceServices.h"
#include "IMainFrameModule.h"

#include "EngineAnalytics.h"
#include "Runtime/Analytics/Analytics/Public/Interfaces/IAnalyticsProvider.h"

#include "GameProjectGenerationModule.h"
#include "ProjectTargetPlatformEditor.h"
#include "PlatformInfo.h"

//@TODO: Remove this dependency
#include "Editor/LevelEditor/Public/LevelEditor.h"
#include "Editor/LevelEditor/Public/ILevelViewport.h"

#include "EditorAnalytics.h"
#include "MessageLog.h"


#define LOCTEXT_NAMESPACE "DebuggerCommands"


// Put internal callbacks that we don't need to expose here in order to avoid unnecessary build dependencies outside of this module
class FInternalPlayWorldCommandCallbacks : public FPlayWorldCommandCallbacks
{
public:

	// Play In
	static void RepeatLastPlay_Clicked();
	static bool RepeatLastPlay_CanExecute();
	static FText GetRepeatLastPlayToolTip();
	static FSlateIcon GetRepeatLastPlayIcon();

	static void Simulate_Clicked();
	static bool Simulate_CanExecute();
	static bool Simulate_IsChecked();

	static void PlayInViewport_Clicked();
	static bool PlayInViewport_CanExecute();
	static void PlayInEditorFloating_Clicked();
	static bool PlayInEditorFloating_CanExecute();
	static void PlayInNewProcess_Clicked( bool MobilePreview );
	static bool PlayInNewProcess_CanExecute();
	static bool PlayInModeIsChecked( EPlayModeType PlayMode );

	static bool PlayInLocation_CanExecute( EPlayModeLocations Location );
	static void PlayInLocation_Clicked( EPlayModeLocations Location );
	static bool PlayInLocation_IsChecked( EPlayModeLocations Location );

	static void PlayInSettings_Clicked();

	// Launch On
	static void HandleLaunchOnDeviceActionExecute(FString DevicedId, FString DeviceName);
	static bool HandleLaunchOnDeviceActionCanExecute(FString DeviceName);
	static bool HandleLaunchOnDeviceActionIsChecked(FString DeviceName);

	// No Device
	static void HandleNoDeviceFoundActionExecute() {}
	static bool HandleNoDeviceFoundActionCanExecute() { return false; }

	static void HandleShowSDKTutorial(FString PlatformName, FString NotInstalledDocLink);

	static void RepeatLastLaunch_Clicked();
	static bool RepeatLastLaunch_CanExecute();
	static FText GetRepeatLastLaunchToolTip();
	static FSlateIcon GetRepeatLastLaunchIcon();
	static void OpenLauncher_Clicked();
	static void OpenDeviceManager_Clicked();

	static FSlateIcon GetResumePlaySessionImage();
	static FText GetResumePlaySessionToolTip();
	static void ResumePlaySession_Clicked();
	static void StopPlaySession_Clicked();
	static void PausePlaySession_Clicked();
	static void SingleFrameAdvance_Clicked();

	static void ShowCurrentStatement_Clicked();
	static void StepInto_Clicked();

	static void TogglePlayPause_Clicked();

	static void PossessEjectPlayer_Clicked();
	static bool CanPossessEjectPlayer();
	static FText GetPossessEjectLabel();
	static FText GetPossessEjectTooltip();
	static FSlateIcon GetPossessEjectImage();

	static bool HasPlayWorld();
	static bool HasPlayWorldAndPaused();
	static bool HasPlayWorldAndRunning();

	static bool IsStoppedAtBreakpoint();

	static bool CanShowNonPlayWorldOnlyActions();

	static int32 GetNumberOfClients();
	static void SetNumberOfClients(int32 NumClients, ETextCommit::Type CommitInfo);

	static void OnToggleDedicatedServerPIE();
	static bool OnIsDedicatedServerPIEEnabled();

protected:

	/**
	 * Adds a message to the message log.
	 *
	 * @param Text The main message text.
	 * @param Detail The detailed description.
	 * @param TutorialLink A link to an associated tutorial.
	 */
	static void AddMessageLog( const FText& Text, const FText& Detail, const FString& TutorialLink );

	/**
	 * Checks whether the specified platform has a default device that can be launched on.
	 *
	 * @param PlatformName - The name of the platform to check.
	 *
	 * @return true if the platform can be played on, false otherwise.
	 */
	static bool CanLaunchOnDevice(const FString& DeviceName);

	/**
	 * Starts a game session on the default device of the specified platform.
	 *
	 * @param PlatformName - The name of the platform to play the game on.
	 */
	static void LaunchOnDevice(const FString& DeviceId, const FString& DeviceName);

	/** Get the player start location to use when starting PIE */
	static EPlayModeLocations GetPlayModeLocation();

	/** checks to see if we have everything needed to launch a build to device */
	static bool IsReadyToLaunchOnDevice(FString DeviceId);
};


/**
 * Called to leave K2 debugging mode                   
 */
static void LeaveDebuggingMode()
{
	if( GUnrealEd->PlayWorld != NULL )
	{
		GUnrealEd->PlayWorld->bDebugPauseExecution = false;
	}

	// Focus the game view port 
	FModuleManager::GetModuleChecked<FLevelEditorModule>("LevelEditor").FocusPIEViewport();

	// Tell the application to stop ticking in this stack frame
	FSlateApplication::Get().LeaveDebuggingMode( FKismetDebugUtilities::IsSingleStepping() );
}


//////////////////////////////////////////////////////////////////////////
// FPlayWorldCommands

TSharedPtr<FUICommandList> FPlayWorldCommands::GlobalPlayWorldActions;

FPlayWorldCommands::FPlayWorldCommands()
	: TCommands<FPlayWorldCommands>("PlayWorld", LOCTEXT("PlayWorld", "Play World (PIE/SIE)"), "MainFrame", FEditorStyle::GetStyleSetName())
{
	ULevelEditorPlaySettings* PlaySettings = GetMutableDefault<ULevelEditorPlaySettings>();

	// initialize default Play device
	if (PlaySettings->LastExecutedLaunchName.IsEmpty())
	{
		FString RunningPlatformName = GetTargetPlatformManagerRef().GetRunningTargetPlatform()->PlatformName();
		FString PlayPlatformName;

		if (RunningPlatformName == TEXT("Windows"))
		{
			PlayPlatformName = TEXT("WindowsNoEditor");
		}
		else if (RunningPlatformName == TEXT("Mac"))
		{
			PlayPlatformName = TEXT("MacNoEditor");
		}

		if (!PlayPlatformName.IsEmpty())
		{
			ITargetPlatform* PlayPlatform = GetTargetPlatformManagerRef().FindTargetPlatform(PlayPlatformName);

			if (PlayPlatform != nullptr)
			{
				ITargetDevicePtr PlayDevice = PlayPlatform->GetDefaultDevice();

				if (PlayDevice.IsValid())
				{
					PlaySettings->LastExecutedLaunchDevice = PlayDevice->GetId().ToString();
					PlaySettings->LastExecutedLaunchName = PlayDevice->GetName();
					PlaySettings->SaveConfig();
				}
			}
		}
	}
}


void FPlayWorldCommands::RegisterCommands()
{
	// SIE
	UI_COMMAND( Simulate, "Simulate", "Start simulating the game", EUserInterfaceActionType::Check, FInputGesture( EKeys::S, EModifierKey::Alt ) );

	// PIE
	UI_COMMAND( RepeatLastPlay, "Play", "Launches a game preview session in the same mode as the last game preview session launched from the Game Preview Modes dropdown next to the Play button on the level editor toolbar", EUserInterfaceActionType::Button, FInputGesture( EKeys::P, EModifierKey::Alt ) )
	UI_COMMAND( PlayInViewport, "Selected Viewport", "Play this level in the active level editor viewport", EUserInterfaceActionType::Check, FInputGesture() );
	UI_COMMAND( PlayInEditorFloating, "New Editor Window", "Play this level in a new window", EUserInterfaceActionType::Check, FInputGesture() );
	UI_COMMAND( PlayInMobilePreview, "Mobile Preview", "Play this level as a mobile device preview (runs in its own process)", EUserInterfaceActionType::Check, FInputGesture() );
	UI_COMMAND( PlayInNewProcess, "Standalone Game", "Play this level in a new window that runs in its own process", EUserInterfaceActionType::Check, FInputGesture() );
	UI_COMMAND( PlayInCameraLocation, "Current Camera Location", "Spawn the player at the current camera location", EUserInterfaceActionType::RadioButton, FInputGesture() );
	UI_COMMAND( PlayInDefaultPlayerStart, "Default Player Start", "Spawn the player at the map's default player start", EUserInterfaceActionType::RadioButton, FInputGesture() );
	UI_COMMAND( PlayInNetworkSettings, "Network Settings...", "Open the settings for the 'Play In' feature", EUserInterfaceActionType::Button, FInputGesture() );
	UI_COMMAND( PlayInNetworkDedicatedServer, "Run Dedicated Server", "If enabled, a dedicated server will be launched and connected to behind the scenes.", EUserInterfaceActionType::ToggleButton, FInputGesture() );
	UI_COMMAND( PlayInSettings, "Advanced Settings...", "Open the settings for the 'Play In' feature", EUserInterfaceActionType::Button, FInputGesture() );

	// SIE & PIE controls
	UI_COMMAND( StopPlaySession, "Stop", "Stop simulation", EUserInterfaceActionType::Button, FInputGesture() );
	UI_COMMAND( ResumePlaySession, "Resume", "Resume simulation", EUserInterfaceActionType::Button, FInputGesture() );
	UI_COMMAND( PausePlaySession, "Pause", "Pause simulation", EUserInterfaceActionType::Button, FInputGesture() );
	UI_COMMAND( SingleFrameAdvance, "Frame Skip", "Advances a single frame", EUserInterfaceActionType::Button, FInputGesture() );
	UI_COMMAND( TogglePlayPauseOfPlaySession, "Toggle Play/Pause", "Resume playing if paused, or pause if playing", EUserInterfaceActionType::Button, FInputGesture( EKeys::Pause ) );
	UI_COMMAND( PossessEjectPlayer, "Possess or Eject Player", "Possesses or ejects the player from the camera", EUserInterfaceActionType::Button, FInputGesture( EKeys::F8 ) );
	UI_COMMAND( ShowCurrentStatement, "Find Node", "Show the current node", EUserInterfaceActionType::Button, FInputGesture() );
	UI_COMMAND( StepInto, "Step", "Step to the next node to be executed", EUserInterfaceActionType::Button, FInputGesture( EKeys::F10) );

	// Launch
	UI_COMMAND( RepeatLastLaunch, "Launch", "Launches the game on the device as the last session launched from the dropdown next to the Play on Device button on the level editor toolbar", EUserInterfaceActionType::Button, FInputGesture( EKeys::P, EModifierKey::Alt | EModifierKey::Shift ) )
	UI_COMMAND( OpenDeviceManager, "Device Manager...", "View and manage connected devices.", EUserInterfaceActionType::Button, FInputGesture() );
}


void FPlayWorldCommands::BindGlobalPlayWorldCommands()
{
	check( !GlobalPlayWorldActions.IsValid() );

	GlobalPlayWorldActions = MakeShareable( new FUICommandList );

	const FPlayWorldCommands& Commands = FPlayWorldCommands::Get();
	FUICommandList& ActionList = *GlobalPlayWorldActions;

	// SIE
	ActionList.MapAction( Commands.Simulate,
		FExecuteAction::CreateStatic( &FInternalPlayWorldCommandCallbacks::Simulate_Clicked ),
		FCanExecuteAction::CreateStatic( &FInternalPlayWorldCommandCallbacks::Simulate_CanExecute ),
		FIsActionChecked::CreateStatic( &FInternalPlayWorldCommandCallbacks::PlayInModeIsChecked, PlayMode_Simulate ),
		FIsActionButtonVisible::CreateStatic( &FInternalPlayWorldCommandCallbacks::CanShowNonPlayWorldOnlyActions )
		);

	// PIE
	ActionList.MapAction( Commands.RepeatLastPlay,
		FExecuteAction::CreateStatic( &FInternalPlayWorldCommandCallbacks::RepeatLastPlay_Clicked ),
		FCanExecuteAction::CreateStatic( &FInternalPlayWorldCommandCallbacks::RepeatLastPlay_CanExecute ),
		FIsActionChecked(),
		FIsActionButtonVisible::CreateStatic( &FInternalPlayWorldCommandCallbacks::CanShowNonPlayWorldOnlyActions )
		);

	ActionList.MapAction( Commands.PlayInViewport,
		FExecuteAction::CreateStatic( &FInternalPlayWorldCommandCallbacks::PlayInViewport_Clicked ),
		FCanExecuteAction::CreateStatic( &FInternalPlayWorldCommandCallbacks::PlayInViewport_CanExecute ),
		FIsActionChecked::CreateStatic( &FInternalPlayWorldCommandCallbacks::PlayInModeIsChecked, PlayMode_InViewPort ),
		FIsActionButtonVisible::CreateStatic( &FInternalPlayWorldCommandCallbacks::CanShowNonPlayWorldOnlyActions )
		);

	ActionList.MapAction( Commands.PlayInEditorFloating,
		FExecuteAction::CreateStatic( &FInternalPlayWorldCommandCallbacks::PlayInEditorFloating_Clicked ),
		FCanExecuteAction::CreateStatic( &FInternalPlayWorldCommandCallbacks::PlayInEditorFloating_CanExecute ),
		FIsActionChecked::CreateStatic( &FInternalPlayWorldCommandCallbacks::PlayInModeIsChecked, PlayMode_InEditorFloating ),
		FIsActionButtonVisible::CreateStatic( &FInternalPlayWorldCommandCallbacks::CanShowNonPlayWorldOnlyActions )
		);

	ActionList.MapAction( Commands.PlayInMobilePreview,
		FExecuteAction::CreateStatic( &FInternalPlayWorldCommandCallbacks::PlayInNewProcess_Clicked, true ),
		FCanExecuteAction::CreateStatic( &FInternalPlayWorldCommandCallbacks::PlayInNewProcess_CanExecute ),
		FIsActionChecked::CreateStatic( &FInternalPlayWorldCommandCallbacks::PlayInModeIsChecked, PlayMode_InMobilePreview ),
		FIsActionButtonVisible::CreateStatic( &FInternalPlayWorldCommandCallbacks::CanShowNonPlayWorldOnlyActions )
		);

	ActionList.MapAction( Commands.PlayInNewProcess,
		FExecuteAction::CreateStatic( &FInternalPlayWorldCommandCallbacks::PlayInNewProcess_Clicked, false ),
		FCanExecuteAction::CreateStatic( &FInternalPlayWorldCommandCallbacks::PlayInNewProcess_CanExecute ),
		FIsActionChecked::CreateStatic( &FInternalPlayWorldCommandCallbacks::PlayInModeIsChecked, PlayMode_InNewProcess ),
		FIsActionButtonVisible::CreateStatic( &FInternalPlayWorldCommandCallbacks::CanShowNonPlayWorldOnlyActions )
		);

	ActionList.MapAction( Commands.PlayInCameraLocation,
		FExecuteAction::CreateStatic( &FInternalPlayWorldCommandCallbacks::PlayInLocation_Clicked, PlayLocation_CurrentCameraLocation ),
		FCanExecuteAction::CreateStatic( &FInternalPlayWorldCommandCallbacks::PlayInLocation_CanExecute, PlayLocation_CurrentCameraLocation ),
		FIsActionChecked::CreateStatic( &FInternalPlayWorldCommandCallbacks::PlayInLocation_IsChecked, PlayLocation_CurrentCameraLocation ),
		FIsActionButtonVisible::CreateStatic( &FInternalPlayWorldCommandCallbacks::CanShowNonPlayWorldOnlyActions )
		);

	ActionList.MapAction( Commands.PlayInDefaultPlayerStart,
		FExecuteAction::CreateStatic( &FInternalPlayWorldCommandCallbacks::PlayInLocation_Clicked, PlayLocation_DefaultPlayerStart ),
		FCanExecuteAction::CreateStatic( &FInternalPlayWorldCommandCallbacks::PlayInLocation_CanExecute, PlayLocation_DefaultPlayerStart ),
		FIsActionChecked::CreateStatic( &FInternalPlayWorldCommandCallbacks::PlayInLocation_IsChecked, PlayLocation_DefaultPlayerStart ),
		FIsActionButtonVisible::CreateStatic( &FInternalPlayWorldCommandCallbacks::CanShowNonPlayWorldOnlyActions )
		);

	ActionList.MapAction( Commands.PlayInSettings,
		FExecuteAction::CreateStatic( &FInternalPlayWorldCommandCallbacks::PlayInSettings_Clicked )
		);

	// Launch
	ActionList.MapAction( Commands.OpenDeviceManager,
		FExecuteAction::CreateStatic( &FInternalPlayWorldCommandCallbacks::OpenDeviceManager_Clicked )
		);

	ActionList.MapAction( Commands.RepeatLastLaunch,
		FExecuteAction::CreateStatic( &FInternalPlayWorldCommandCallbacks::RepeatLastLaunch_Clicked ),
		FCanExecuteAction::CreateStatic( &FInternalPlayWorldCommandCallbacks::RepeatLastLaunch_CanExecute ),
		FIsActionChecked(),
		FIsActionButtonVisible::CreateStatic( &FInternalPlayWorldCommandCallbacks::CanShowNonPlayWorldOnlyActions )
		);


	// Stop play session
	ActionList.MapAction( Commands.StopPlaySession,
		FExecuteAction::CreateStatic( &FInternalPlayWorldCommandCallbacks::StopPlaySession_Clicked ),
		FCanExecuteAction::CreateStatic( &FInternalPlayWorldCommandCallbacks::HasPlayWorld ),
		FIsActionChecked(),
		FIsActionButtonVisible::CreateStatic( &FInternalPlayWorldCommandCallbacks::HasPlayWorld )
		);

	// Play, Pause, Toggle between play and pause
	ActionList.MapAction( Commands.ResumePlaySession,
		FExecuteAction::CreateStatic( &FInternalPlayWorldCommandCallbacks::ResumePlaySession_Clicked ),
		FCanExecuteAction::CreateStatic( &FInternalPlayWorldCommandCallbacks::HasPlayWorldAndPaused ),
		FIsActionChecked(),
		FIsActionButtonVisible::CreateStatic( &FInternalPlayWorldCommandCallbacks::HasPlayWorldAndPaused )
		);

	ActionList.MapAction( Commands.PausePlaySession,
		FExecuteAction::CreateStatic( &FInternalPlayWorldCommandCallbacks::PausePlaySession_Clicked ),
		FCanExecuteAction::CreateStatic( &FInternalPlayWorldCommandCallbacks::HasPlayWorldAndRunning ),
		FIsActionChecked(),
		FIsActionButtonVisible::CreateStatic( &FInternalPlayWorldCommandCallbacks::HasPlayWorldAndRunning )
		);

	ActionList.MapAction( Commands.SingleFrameAdvance,
		FExecuteAction::CreateStatic( &FInternalPlayWorldCommandCallbacks::SingleFrameAdvance_Clicked ),
		FCanExecuteAction::CreateStatic( &FInternalPlayWorldCommandCallbacks::HasPlayWorldAndPaused ),
		FIsActionChecked(),
		FIsActionChecked::CreateStatic( &FInternalPlayWorldCommandCallbacks::HasPlayWorldAndPaused )
		);

	ActionList.MapAction( Commands.TogglePlayPauseOfPlaySession,
		FExecuteAction::CreateStatic( &FInternalPlayWorldCommandCallbacks::TogglePlayPause_Clicked ),
		FCanExecuteAction::CreateStatic( &FInternalPlayWorldCommandCallbacks::HasPlayWorld ),
		FIsActionChecked(),
		FIsActionButtonVisible::CreateStatic( &FInternalPlayWorldCommandCallbacks::HasPlayWorld )
		);

	// Toggle PIE/SIE, Eject (PIE->SIE), and Possess (SIE->PIE)
	ActionList.MapAction( Commands.PossessEjectPlayer,
		FExecuteAction::CreateStatic( &FInternalPlayWorldCommandCallbacks::PossessEjectPlayer_Clicked ),
		FCanExecuteAction::CreateStatic( &FInternalPlayWorldCommandCallbacks::CanPossessEjectPlayer ),
		FIsActionChecked(),
		FIsActionButtonVisible::CreateStatic( &FInternalPlayWorldCommandCallbacks::CanPossessEjectPlayer )
		);

	// Breakpoint-only commands
	ActionList.MapAction( Commands.ShowCurrentStatement,
		FExecuteAction::CreateStatic( &FInternalPlayWorldCommandCallbacks::ShowCurrentStatement_Clicked ),
		FCanExecuteAction::CreateStatic( &FInternalPlayWorldCommandCallbacks::IsStoppedAtBreakpoint ),
		FIsActionChecked(),
		FIsActionChecked::CreateStatic( &FInternalPlayWorldCommandCallbacks::IsStoppedAtBreakpoint )
		);

	ActionList.MapAction( Commands.StepInto,
		FExecuteAction::CreateStatic( &FInternalPlayWorldCommandCallbacks::StepInto_Clicked ),
		FCanExecuteAction::CreateStatic( &FInternalPlayWorldCommandCallbacks::IsStoppedAtBreakpoint ),
		FIsActionChecked(),
		FIsActionChecked::CreateStatic( &FInternalPlayWorldCommandCallbacks::IsStoppedAtBreakpoint )
		);

	ActionList.MapAction( Commands.PlayInNetworkDedicatedServer,
		FExecuteAction::CreateStatic( &FInternalPlayWorldCommandCallbacks::OnToggleDedicatedServerPIE ),
		FCanExecuteAction(),
		FIsActionChecked::CreateStatic( &FInternalPlayWorldCommandCallbacks::OnIsDedicatedServerPIEEnabled ) 
		);
}


void FPlayWorldCommands::BuildToolbar( FToolBarBuilder& ToolbarBuilder, bool bIncludeLaunchButtonAndOptions )
{
	// Play
	ToolbarBuilder.AddToolBarButton( 
		FPlayWorldCommands::Get().RepeatLastPlay, 
		NAME_None, 
		LOCTEXT("RepeatLastPlay", "Play"),
		TAttribute< FText >::Create( TAttribute< FText >::FGetter::CreateStatic( &FInternalPlayWorldCommandCallbacks::GetRepeatLastPlayToolTip ) ),
		TAttribute< FSlateIcon >::Create( TAttribute< FSlateIcon >::FGetter::CreateStatic( &FInternalPlayWorldCommandCallbacks::GetRepeatLastPlayIcon ) ),
		FName(TEXT("LevelToolbarPlay"))
	);

	// Play combo box
	FUIAction SpecialPIEOptionsMenuAction;
	SpecialPIEOptionsMenuAction.IsActionVisibleDelegate = FIsActionButtonVisible::CreateStatic( &FInternalPlayWorldCommandCallbacks::CanShowNonPlayWorldOnlyActions );

	ToolbarBuilder.AddComboButton(
		SpecialPIEOptionsMenuAction,
		FOnGetContent::CreateStatic( &GeneratePlayMenuContent, GlobalPlayWorldActions.ToSharedRef() ),
		LOCTEXT( "PlayCombo_Label", "Play Options" ),
		LOCTEXT( "PIEComboToolTip", "Options for Play in Editor (PIE)" ),
		FSlateIcon(FEditorStyle::GetStyleSetName(), "LevelEditor.RepeatLastPlay"),
		true
	);

	if (bIncludeLaunchButtonAndOptions)
	{
		// Launch
		ToolbarBuilder.AddToolBarButton( 
			FPlayWorldCommands::Get().RepeatLastLaunch, 
			NAME_None, 
			LOCTEXT("RepeatLastLaunch", "Launch"),
			TAttribute< FText >::Create( TAttribute< FText >::FGetter::CreateStatic( &FInternalPlayWorldCommandCallbacks::GetRepeatLastLaunchToolTip ) ),
			TAttribute< FSlateIcon >::Create( TAttribute< FSlateIcon >::FGetter::CreateStatic( &FInternalPlayWorldCommandCallbacks::GetRepeatLastLaunchIcon ) ),
			FName(TEXT("RepeatLastLaunch"))
		);

		// Launch combo box
		FUIAction LaunchMenuAction;
		LaunchMenuAction.IsActionVisibleDelegate = FIsActionButtonVisible::CreateStatic( &FInternalPlayWorldCommandCallbacks::CanShowNonPlayWorldOnlyActions );

		ToolbarBuilder.AddComboButton(
			LaunchMenuAction,
			FOnGetContent::CreateStatic( &GenerateLaunchMenuContent, GlobalPlayWorldActions.ToSharedRef() ),
			LOCTEXT( "LaunchCombo_Label", "Launch Options" ),
			LOCTEXT( "PODComboToolTip", "Options for launching on a device" ),
			FSlateIcon(FEditorStyle::GetStyleSetName(), "LevelEditor.RepeatLastLaunch"),
			true
		);
	}

	// Resume/pause toggle (only one will be visible, and only in PIE/SIE)
	ToolbarBuilder.AddToolBarButton(FPlayWorldCommands::Get().ResumePlaySession, NAME_None, TAttribute<FText>(),
		TAttribute<FText>::Create(TAttribute<FText>::FGetter::CreateStatic(&FInternalPlayWorldCommandCallbacks::GetResumePlaySessionToolTip)),
		TAttribute<FSlateIcon>::Create(TAttribute<FSlateIcon>::FGetter::CreateStatic(&FInternalPlayWorldCommandCallbacks::GetResumePlaySessionImage)),
		FName(TEXT("ResumePlaySession"))
	);

	ToolbarBuilder.AddToolBarButton(FPlayWorldCommands::Get().PausePlaySession, NAME_None, TAttribute<FText>(), TAttribute<FText>(), TAttribute<FSlateIcon>(), FName(TEXT("PausePlaySession")));
	ToolbarBuilder.AddToolBarButton(FPlayWorldCommands::Get().SingleFrameAdvance, NAME_None, TAttribute<FText>(), TAttribute<FText>(), TAttribute<FSlateIcon>(), FName(TEXT("SingleFrameAdvance")));

	// Stop
	ToolbarBuilder.AddToolBarButton(FPlayWorldCommands::Get().StopPlaySession, NAME_None, TAttribute<FText>(), TAttribute<FText>(), TAttribute<FSlateIcon>(), FName(TEXT("StopPlaySession")));

	// Eject/possess toggle
	ToolbarBuilder.AddToolBarButton(FPlayWorldCommands::Get().PossessEjectPlayer, NAME_None, 
		TAttribute<FText>::Create(TAttribute<FText>::FGetter::CreateStatic(&FInternalPlayWorldCommandCallbacks::GetPossessEjectLabel)),
		TAttribute<FText>::Create(TAttribute<FText>::FGetter::CreateStatic(&FInternalPlayWorldCommandCallbacks::GetPossessEjectTooltip)),
		TAttribute<FSlateIcon>::Create(TAttribute<FSlateIcon>::FGetter::CreateStatic(&FInternalPlayWorldCommandCallbacks::GetPossessEjectImage)),
		FName(TEXT("PossessEjectPlayer"))
	);

	// Single-stepping only buttons
	ToolbarBuilder.AddToolBarButton(FPlayWorldCommands::Get().ShowCurrentStatement, NAME_None, TAttribute<FText>(), TAttribute<FText>(), TAttribute<FSlateIcon>(), FName(TEXT("ShowCurrentStatement")));
	ToolbarBuilder.AddToolBarButton(FPlayWorldCommands::Get().StepInto, NAME_None, TAttribute<FText>(), TAttribute<FText>(), TAttribute<FSlateIcon>(), FName(TEXT("StepInto")));
}

TSharedRef< SWidget > FPlayWorldCommands::GeneratePlayMenuContent( TSharedRef<FUICommandList> InCommandList )
{
	struct FLocal
	{
		static void AddPlayModeMenuEntry( FMenuBuilder& MenuBuilder, EPlayModeType PlayMode )
		{
			TSharedPtr<FUICommandInfo> PlayModeCommand;

			switch(PlayMode)
			{
			case PlayMode_InEditorFloating:
				PlayModeCommand = FPlayWorldCommands::Get().PlayInEditorFloating;
				break;

			case PlayMode_InMobilePreview:
				PlayModeCommand = FPlayWorldCommands::Get().PlayInMobilePreview;
				break;

			case PlayMode_InNewProcess:
				PlayModeCommand = FPlayWorldCommands::Get().PlayInNewProcess;
				break;

			case PlayMode_InViewPort:
				PlayModeCommand = FPlayWorldCommands::Get().PlayInViewport;
				break;

			case PlayMode_Simulate:
				PlayModeCommand = FPlayWorldCommands::Get().Simulate;
				break;
			}

			if (PlayModeCommand.IsValid())
			{
				MenuBuilder.AddMenuEntry(PlayModeCommand);
			}
		}
	};

	const bool bShouldCloseWindowAfterMenuSelection = true;
	FMenuBuilder MenuBuilder( bShouldCloseWindowAfterMenuSelection, InCommandList );

	// play in view port
	MenuBuilder.BeginSection("LevelEditorPlayModes", LOCTEXT("PlayButtonModesSection", "Modes"));
	{
		FLocal::AddPlayModeMenuEntry(MenuBuilder, PlayMode_InViewPort);
		FLocal::AddPlayModeMenuEntry(MenuBuilder, PlayMode_InMobilePreview);
		FLocal::AddPlayModeMenuEntry(MenuBuilder, PlayMode_InEditorFloating);
		FLocal::AddPlayModeMenuEntry(MenuBuilder, PlayMode_InNewProcess);
		FLocal::AddPlayModeMenuEntry(MenuBuilder, PlayMode_Simulate);
	}
	MenuBuilder.EndSection();

	// tip section
	MenuBuilder.BeginSection("LevelEditorPlayTip");
	{
		MenuBuilder.AddWidget( 
			SNew( STextBlock )
				.ColorAndOpacity(FSlateColor::UseSubduedForeground())
				.Text(LOCTEXT("PlayInTip", "Launching a game preview with a different mode will change your default 'Play' mode in the toolbar"))
				.WrapTextAt(250),
			FText::GetEmpty() );
	}
	MenuBuilder.EndSection();

	// player start selection
	MenuBuilder.BeginSection("LevelEditorPlayPlayerStart", LOCTEXT("PlayButtonLocationSection", "Spawn player at..."));
	{
		MenuBuilder.AddMenuEntry(FPlayWorldCommands::Get().PlayInCameraLocation);
		MenuBuilder.AddMenuEntry(FPlayWorldCommands::Get().PlayInDefaultPlayerStart);
	}
	MenuBuilder.EndSection();

	// Basic network options
	if ( GetDefault<ULevelEditorPlaySettings>()->RunUnderOneProcess )
	{
		MenuBuilder.BeginSection("LevelEditorPlayInWindowNetwork", LOCTEXT("LevelEditorPlayInWindowNetworkSection", "Network Settings"));
		{
			TSharedRef<SWidget> NumPlayers = SNew(SSpinBox<int32>)
				.MinValue(1)
				.MaxValue(64)
				.ToolTipText(LOCTEXT( "NumberOfClientsToolTip", "Number of clients to spawn in the PIE session" ))
				.Value(FInternalPlayWorldCommandCallbacks::GetNumberOfClients())
				.OnValueCommitted_Static(&FInternalPlayWorldCommandCallbacks::SetNumberOfClients);
			
			MenuBuilder.AddWidget(NumPlayers, LOCTEXT( "NumberOfClientsMenuWidget", "Number of Clients" ));
		}
		
		MenuBuilder.AddMenuEntry( FPlayWorldCommands::Get().PlayInNetworkDedicatedServer );

		MenuBuilder.EndSection();
	}

	// settings
	MenuBuilder.BeginSection("LevelEditorPlaySettings");
	{
		MenuBuilder.AddMenuEntry(FPlayWorldCommands::Get().PlayInSettings);
	}
	MenuBuilder.EndSection();

	return MenuBuilder.MakeWidget();
}

TSharedRef< SWidget > FPlayWorldCommands::GenerateLaunchMenuContent( TSharedRef<FUICommandList> InCommandList )
{
	const bool bShouldCloseWindowAfterMenuSelection = true;
	FMenuBuilder MenuBuilder(bShouldCloseWindowAfterMenuSelection, InCommandList);

	TArray<PlatformInfo::FVanillaPlatformEntry> VanillaPlatforms = PlatformInfo::BuildPlatformHierarchy(PlatformInfo::EPlatformFilter::All);

	VanillaPlatforms.Sort([](const PlatformInfo::FVanillaPlatformEntry& One, const PlatformInfo::FVanillaPlatformEntry& Two) -> bool
	{
		return One.PlatformInfo->DisplayName.CompareTo(Two.PlatformInfo->DisplayName) < 0;
	});

	// shared devices section
	TSharedPtr<ITargetDeviceServicesModule> TargetDeviceServicesModule = StaticCastSharedPtr<ITargetDeviceServicesModule>(FModuleManager::Get().LoadModule(TEXT("TargetDeviceServices")));
	IProjectTargetPlatformEditorModule& ProjectTargetPlatformEditorModule = FModuleManager::LoadModuleChecked<IProjectTargetPlatformEditorModule>("ProjectTargetPlatformEditor");

	TArray<FString> PlatformsToMaybeInstallLinksFor;
	PlatformsToMaybeInstallLinksFor.Add(TEXT("Android"));
	PlatformsToMaybeInstallLinksFor.Add(TEXT("IOS"));
	PlatformsToMaybeInstallLinksFor.Add(TEXT("Linux"));
	TArray<FString> PlatformsToCheckFlavorsFor;
	PlatformsToCheckFlavorsFor.Add(TEXT("Android"));
	PlatformsToCheckFlavorsFor.Add(TEXT("IOS"));
	TArray<FName> PlatformsWithNoDevices;
	TArray<PlatformInfo::FPlatformInfo> PlatformsToAddInstallLinksFor;

	MenuBuilder.BeginSection("LevelEditorLaunchDevices", LOCTEXT("LaunchButtonDevicesSection", "Devices"));
	{
		for (const PlatformInfo::FVanillaPlatformEntry& VanillaPlatform : VanillaPlatforms)
		{
			// for the Editor we are only interested in launching standalone games
			if (VanillaPlatform.PlatformInfo->PlatformType != PlatformInfo::EPlatformType::Game || !VanillaPlatform.PlatformInfo->bEnabledForUse || (!VanillaPlatform.PlatformInfo->bEnabledInBinary && FRocketSupport::IsRocket()))
			{
				continue;
			}

			if (VanillaPlatform.PlatformInfo->SDKStatus == PlatformInfo::EPlatformSDKStatus::Installed)
			{
				// for each platform...
				TArray<ITargetDeviceProxyPtr> DeviceProxies;
				TargetDeviceServicesModule->GetDeviceProxyManager()->GetProxies(VanillaPlatform.PlatformInfo->VanillaPlatformName, false, DeviceProxies);
					
				// if this platform had no devices, but we want to show an extra option if not installed right
				if (DeviceProxies.Num() == 0)
				{
					if (PlatformsWithNoDevices.Find(VanillaPlatform.PlatformInfo->VanillaPlatformName) == INDEX_NONE)
					{
						// add an entry with a no devices found
						PlatformsWithNoDevices.Add(VanillaPlatform.PlatformInfo->VanillaPlatformName);
					}
				}
				else
				{
					// for each proxy...
					for (auto DeviceProxyIt = DeviceProxies.CreateIterator(); DeviceProxyIt; ++DeviceProxyIt)
					{
						ITargetDeviceProxyPtr DeviceProxy = *DeviceProxyIt;

						// ... create an action...
						FUIAction LaunchDeviceAction(
							FExecuteAction::CreateStatic(&FInternalPlayWorldCommandCallbacks::HandleLaunchOnDeviceActionExecute, DeviceProxy->GetTargetDeviceId(NAME_None), DeviceProxy->GetName()),
							FCanExecuteAction::CreateStatic(&FInternalPlayWorldCommandCallbacks::HandleLaunchOnDeviceActionCanExecute, DeviceProxy->GetName()),
							FIsActionChecked::CreateStatic(&FInternalPlayWorldCommandCallbacks::HandleLaunchOnDeviceActionIsChecked, DeviceProxy->GetName())
							);

						// ... generate display label...
						FFormatNamedArguments LabelArguments;
						LabelArguments.Add(TEXT("DeviceName"), FText::FromString(DeviceProxy->GetName()));

						if (!DeviceProxy->IsConnected())
						{
							LabelArguments.Add(TEXT("HostUser"), LOCTEXT("DisconnectedHint", " [Disconnected]"));
						}
						else if (DeviceProxy->GetHostUser() != FPlatformProcess::UserName(true))
						{
							LabelArguments.Add(TEXT("HostUser"), FText::FromString(DeviceProxy->GetHostUser()));
						}
						else
						{
							LabelArguments.Add(TEXT("HostUser"), FText::GetEmpty());
						}

						FText Label = FText::Format(LOCTEXT("LaunchDeviceLabel", "{DeviceName}{HostUser}"), LabelArguments);

						// ... generate tooltip text
						FFormatNamedArguments TooltipArguments;
						TooltipArguments.Add(TEXT("DeviceID"), FText::FromString(DeviceProxy->GetName()));
						TooltipArguments.Add(TEXT("DisplayName"), VanillaPlatform.PlatformInfo->DisplayName);
						FText Tooltip = FText::Format(LOCTEXT("LaunchDeviceToolTipText", "Launch the game on this {DisplayName} device ({DeviceID})"), TooltipArguments);

						FProjectStatus ProjectStatus;
						if (IProjectManager::Get().QueryStatusForCurrentProject(ProjectStatus) && !ProjectStatus.IsTargetPlatformSupported(VanillaPlatform.PlatformInfo->VanillaPlatformName)) 
						{
							FText TooltipLine2 = FText::Format(LOCTEXT("LaunchDevicePlatformWarning", "{DisplayName} is not listed as a target platform for this project, so may not run as expected."), TooltipArguments);
							Tooltip = FText::Format(FText::FromString(TEXT("{0}\n\n{1}")), Tooltip, TooltipLine2);
						}

						// ... and add a menu entry
						MenuBuilder.AddMenuEntry(
							LaunchDeviceAction, 
							ProjectTargetPlatformEditorModule.MakePlatformMenuItemWidget(*VanillaPlatform.PlatformInfo, true, Label),
							NAME_None, 
							Tooltip, 
							EUserInterfaceActionType::Check
							);
					}
				}
			}
			else
			{
				// if the platform wasn't installed, we'll add a menu item later (we never care about code in this case, since we don't compile)
				if (PlatformsToMaybeInstallLinksFor.Find(VanillaPlatform.PlatformInfo->VanillaPlatformName.ToString()) != INDEX_NONE)
				{
					PlatformsToAddInstallLinksFor.Add(*(VanillaPlatform.PlatformInfo));
				}
			}
		}
	}
	MenuBuilder.EndSection();

	if (PlatformsWithNoDevices.Num() > 0)
	{
		MenuBuilder.BeginSection("NoDevices");
		{
			for (int32 PlatformIndex = 0; PlatformIndex < PlatformsWithNoDevices.Num(); PlatformIndex++)
			{
				const PlatformInfo::FPlatformInfo* PlatformInfo = PlatformInfo::FindVanillaPlatformInfo(PlatformsWithNoDevices[PlatformIndex]);

				// ... generate display label...
				FFormatNamedArguments LabelArguments;
				LabelArguments.Add(TEXT("DisplayName"), PlatformInfo->DisplayName);

				FText Label = FText::Format(LOCTEXT("NoDeviceLabel", "{DisplayName} - No Devices Found"), LabelArguments);

				// ... create an action...
				FUIAction NoDeviceAction(
					FExecuteAction::CreateStatic(&FInternalPlayWorldCommandCallbacks::HandleNoDeviceFoundActionExecute),
					FCanExecuteAction::CreateStatic(&FInternalPlayWorldCommandCallbacks::HandleNoDeviceFoundActionCanExecute)
					);

				// ... generate tooltip text
				FFormatNamedArguments TooltipArguments;
				TooltipArguments.Add(TEXT("DisplayName"), PlatformInfo->DisplayName);
				FText Tooltip = FText::Format(LOCTEXT("LaunchDeviceToolTipText", "Found no connected devices for {DisplayName}"), TooltipArguments);

				// ... and add a menu entry
				MenuBuilder.AddMenuEntry(
					NoDeviceAction, 
					ProjectTargetPlatformEditorModule.MakePlatformMenuItemWidget(*PlatformInfo, true, Label), 
					NAME_None, 
					Tooltip, 
					EUserInterfaceActionType::Check
					);
			}
		}
		MenuBuilder.EndSection();
	}

	// tip section
	MenuBuilder.BeginSection("LevelEditorLaunchHint");
	{
		MenuBuilder.AddWidget( 
			SNew( STextBlock )
				.ColorAndOpacity(FSlateColor::UseSubduedForeground())
				.Text(LOCTEXT("ZoomToFitHorizontal", "Launching a game on a different device will change your default 'Launch' device in the toolbar"))
				.WrapTextAt(250),
			FText::GetEmpty()
		);
	}
	MenuBuilder.EndSection();

	if (PlatformsToAddInstallLinksFor.Num() > 0)
	{
		MenuBuilder.BeginSection("SDKUninstalledTutorials");
		{
			for (int32 PlatformIndex = 0; PlatformIndex < PlatformsToAddInstallLinksFor.Num(); PlatformIndex++)
			{
				const PlatformInfo::FPlatformInfo& Platform = PlatformsToAddInstallLinksFor[PlatformIndex];

				FUIAction Action(FExecuteAction::CreateStatic(&FInternalPlayWorldCommandCallbacks::HandleShowSDKTutorial, Platform.DisplayName.ToString(), Platform.SDKTutorial));

				FFormatNamedArguments LabelArguments;
				LabelArguments.Add(TEXT("PlatformName"), Platform.DisplayName);
				FText Label = FText::Format(LOCTEXT("LaunchPlatformLabel", "{PlatformName} Support"), LabelArguments);


				MenuBuilder.AddMenuEntry( 
					Label,
					LOCTEXT("PlatformSDK", "Show information on setting up the platform tools"),
					FSlateIcon(FEditorStyle::GetStyleSetName(), "LevelEditor.BrowseDocumentation"),
					Action,
					NAME_None,
					EUserInterfaceActionType::Button);
			}
		}
		MenuBuilder.EndSection();
	}

	// options section
	MenuBuilder.BeginSection("LevelEditorLaunchOptions");
	{
		MenuBuilder.AddMenuEntry( FPlayWorldCommands::Get().OpenDeviceManager,
			NAME_None,
			TAttribute<FText>(),
			TAttribute<FText>(),
			FSlateIcon(FEditorStyle::GetStyleSetName(), "DeviceDetails.TabIcon")
			);
		
		ProjectTargetPlatformEditorModule.AddOpenProjectTargetPlatformEditorMenuItem(MenuBuilder);
	}
	MenuBuilder.EndSection();

	return MenuBuilder.MakeWidget();
}


//////////////////////////////////////////////////////////////////////////
// FPlayWorldCommandCallbacks

void FPlayWorldCommandCallbacks::StartPlayFromHere()
{
	// Is a PIE session already running?  If so we close it first
	if( GUnrealEd->PlayWorld != NULL )
	{
		GUnrealEd->EndPlayMap();
	}

	UClass* const PlayerStartClass = GUnrealEd->PlayFromHerePlayerStartClass ? (UClass*)GUnrealEd->PlayFromHerePlayerStartClass : APlayerStart::StaticClass();

	// Figure out the start location of the player
	UCapsuleComponent*	DefaultCollisionComponent = CastChecked<UCapsuleComponent>(PlayerStartClass->GetDefaultObject<AActor>()->GetRootComponent());
	FVector				CollisionExtent = FVector(DefaultCollisionComponent->GetScaledCapsuleRadius(),DefaultCollisionComponent->GetScaledCapsuleRadius(),DefaultCollisionComponent->GetScaledCapsuleHalfHeight());
	FVector				StartLocation = GEditor->ClickLocation + GEditor->ClickPlane * (FVector::BoxPushOut(GEditor->ClickPlane,CollisionExtent) + 0.1f);

	FRotator StartRotation = FRotator::ZeroRotator;

	FLevelEditorModule& LevelEditorModule = FModuleManager::GetModuleChecked<FLevelEditorModule>( "LevelEditor");

	TSharedPtr<ILevelViewport> ActiveLevelViewport = LevelEditorModule.GetFirstActiveViewport();

	const bool bSimulateInEditor = false;
	if( ActiveLevelViewport.IsValid() )
	{
		if( ActiveLevelViewport->GetLevelViewportClient().IsPerspective() )
		{
			StartRotation = ActiveLevelViewport->GetLevelViewportClient().GetViewRotation();
		}

		// If there is an active level view port, play the game in it.
		GUnrealEd->RequestPlaySession(false, ActiveLevelViewport, bSimulateInEditor, &StartLocation, &StartRotation, -1, false );
	}
	else
	{
		// No active level view port, spawn a new window to play in.
		GUnrealEd->RequestPlaySession(false, NULL, bSimulateInEditor, &StartLocation, NULL );
	}
}


bool FPlayWorldCommandCallbacks::IsInSIE()
{
	return GEditor->bIsSimulatingInEditor;
}


bool FPlayWorldCommandCallbacks::IsInPIE()
{
	return (GEditor->PlayWorld != NULL) && (!GEditor->bIsSimulatingInEditor);
}


bool FPlayWorldCommandCallbacks::IsInSIE_AndRunning()
{
	return IsInSIE() && ((GEditor->PlayWorld == NULL) || !(GEditor->PlayWorld->bDebugPauseExecution));
}


bool FPlayWorldCommandCallbacks::IsInPIE_AndRunning()
{
	return IsInPIE() && ((GEditor->PlayWorld == NULL) || !(GEditor->PlayWorld->bDebugPauseExecution));
}


//////////////////////////////////////////////////////////////////////////
// FInternalPlayWorldCommandCallbacks

FText FInternalPlayWorldCommandCallbacks::GetPossessEjectLabel()
{
	if ( IsInPIE() )
	{
		return LOCTEXT( "EjectLabel", "Eject" );
	}
	else if ( IsInSIE() )
	{
		return LOCTEXT( "PossessLabel", "Possess" );
	}
	else
	{
		return LOCTEXT( "ToggleBetweenPieAndSIELabel", "Toggle Between PIE and SIE" );
	}
}


FText FInternalPlayWorldCommandCallbacks::GetPossessEjectTooltip()
{
	if ( IsInPIE() )
	{
		return LOCTEXT( "EjectToolTip", "Detaches from the player controller, allowing regular editor controls" );
	}
	else if ( IsInSIE() )
	{
		return LOCTEXT( "PossessToolTip", "Attaches to the player controller, allowing normal gameplay controls" );
	}
	else
	{
		return LOCTEXT( "ToggleBetweenPieAndSIEToolTip", "Toggles the current play session between play in editor and simulate in editor" );
	}
}


FSlateIcon FInternalPlayWorldCommandCallbacks::GetPossessEjectImage()
{
	if ( IsInPIE() )
	{
		return FSlateIcon(FEditorStyle::GetStyleSetName(), "PlayWorld.EjectFromPlayer");
	}
	else if ( IsInSIE() )
	{
		return FSlateIcon(FEditorStyle::GetStyleSetName(), "PlayWorld.PossessPlayer");
	}
	else
	{
		return FSlateIcon();
	}
}


void SetLastExecutedPlayMode(EPlayModeType PlayMode)
{
	ULevelEditorPlaySettings* PlaySettings = GetMutableDefault<ULevelEditorPlaySettings>();
	PlaySettings->LastExecutedPlayModeType = PlayMode;
	PlaySettings->SaveConfig();
}


void FInternalPlayWorldCommandCallbacks::Simulate_Clicked()
{
	// Is a simulation session already running?  If so, do nothing
	if( HasPlayWorld() && GUnrealEd->bIsSimulatingInEditor )
	{
		return;
	}

	SetLastExecutedPlayMode( PlayMode_Simulate );

	FLevelEditorModule& LevelEditorModule = FModuleManager::GetModuleChecked<FLevelEditorModule>( TEXT("LevelEditor") );

	TSharedPtr<ILevelViewport> ActiveLevelViewport = LevelEditorModule.GetFirstActiveViewport();
	if( ActiveLevelViewport.IsValid() )
	{
		// Start a new simulation session!
		if( !HasPlayWorld() )
		{
			if( FEngineAnalytics::IsAvailable() )
			{
				FEngineAnalytics::GetProvider().RecordEvent( TEXT("Editor.Usage.SimulateInEditor") );
			}

			GUnrealEd->RequestPlaySession(false, ActiveLevelViewport, true/*bSimulateInEditor*/, NULL, NULL, -1, false );
		}
		else
		{
			GUnrealEd->RequestToggleBetweenPIEandSIE();
		}
	}
}


bool FInternalPlayWorldCommandCallbacks::Simulate_CanExecute()
{
	// Can't simulate while already simulating; PIE is fine as we toggle to simulate
	return !(HasPlayWorld() && GUnrealEd->bIsSimulatingInEditor);
}


bool FInternalPlayWorldCommandCallbacks::Simulate_IsChecked()
{
	return HasPlayWorld() && GUnrealEd->bIsSimulatingInEditor;
}


const TSharedRef < FUICommandInfo > GetLastPlaySessionCommand()
{
	const ULevelEditorPlaySettings* PlaySettings = GetDefault<ULevelEditorPlaySettings>();

	const FPlayWorldCommands& Commands = FPlayWorldCommands::Get();
	TSharedRef < FUICommandInfo > Command = Commands.PlayInViewport.ToSharedRef();

	switch( PlaySettings->LastExecutedPlayModeType )
	{
	case PlayMode_InViewPort:		
		Command = Commands.PlayInViewport.ToSharedRef();				
		break;

	case PlayMode_InEditorFloating:			
		Command = Commands.PlayInEditorFloating.ToSharedRef();			
		break;

	case PlayMode_InMobilePreview:
		Command = Commands.PlayInMobilePreview.ToSharedRef();
		break;

	case PlayMode_InNewProcess:
		Command = Commands.PlayInNewProcess.ToSharedRef();
		break;
	case PlayMode_Simulate:
		Command = Commands.Simulate.ToSharedRef();
	}

	return Command;
}



/** Report PIE usage to engine analytics */
void RecordLastExecutedPlayMode()
{
	if (FEngineAnalytics::IsAvailable())
	{
		const ULevelEditorPlaySettings* PlaySettings = GetDefault<ULevelEditorPlaySettings>();

		// play location
		FString PlayLocationString;

		switch (PlaySettings->LastExecutedPlayModeLocation)
		{
		case PlayLocation_CurrentCameraLocation:
			PlayLocationString = TEXT("CurrentCameraLocation");
			break;

		case PlayLocation_DefaultPlayerStart:
			PlayLocationString = TEXT("DefaultPlayerStart");
			break;

		default:
			PlayLocationString = TEXT("<UNKNOWN>");
		}

		// play mode
		FString PlayModeString;

		switch (PlaySettings->LastExecutedPlayModeType)
		{
		case PlayMode_InViewPort:
			PlayModeString = TEXT("InViewPort");
			break;

		case PlayMode_InEditorFloating:
			PlayModeString = TEXT("InEditorFloating");
			break;

		case PlayMode_InMobilePreview:
			PlayModeString = TEXT("InMobilePreview");
			break;

		case PlayMode_InNewProcess:
			PlayModeString = TEXT("InNewProcess");
			break;

		case PlayMode_Simulate:
			PlayModeString = TEXT("Simulate");
			break;

		default:
			PlayModeString = TEXT("<UNKNOWN>");
		}

		FEngineAnalytics::GetProvider().RecordEvent(TEXT("Editor.Usage.PIE"), TEXT("PlayLocation"), PlayModeString);
		FEngineAnalytics::GetProvider().RecordEvent(TEXT("Editor.Usage.PIE"), TEXT("PlayMode"), PlayModeString);
	}	
}


void SetLastExecutedLaunchMode( ELaunchModeType LaunchMode )
{
	ULevelEditorPlaySettings* PlaySettings = GetMutableDefault<ULevelEditorPlaySettings>();
	PlaySettings->LastExecutedLaunchModeType = LaunchMode;
	PlaySettings->SaveConfig();
}


void FInternalPlayWorldCommandCallbacks::RepeatLastPlay_Clicked()
{
	FPlayWorldCommands::GlobalPlayWorldActions->ExecuteAction( GetLastPlaySessionCommand() );
}


bool FInternalPlayWorldCommandCallbacks::RepeatLastPlay_CanExecute()
{
	return FPlayWorldCommands::GlobalPlayWorldActions->CanExecuteAction( GetLastPlaySessionCommand() );
}


FText FInternalPlayWorldCommandCallbacks::GetRepeatLastPlayToolTip()
{
	return GetLastPlaySessionCommand()->GetDescription();
}


FSlateIcon FInternalPlayWorldCommandCallbacks::GetRepeatLastPlayIcon()
{
	return GetLastPlaySessionCommand()->GetIcon();
}


void FInternalPlayWorldCommandCallbacks::PlayInViewport_Clicked( )
{
	FLevelEditorModule& LevelEditorModule = FModuleManager::GetModuleChecked<FLevelEditorModule>( TEXT("LevelEditor") );

	/** Set PlayInViewPort as the last executed play command */
	const FPlayWorldCommands& Commands = FPlayWorldCommands::Get();

	SetLastExecutedPlayMode( PlayMode_InViewPort );

	RecordLastExecutedPlayMode();

	TSharedPtr<ILevelViewport> ActiveLevelViewport = LevelEditorModule.GetFirstActiveViewport();

	const bool bAtPlayerStart = (GetPlayModeLocation() == PlayLocation_DefaultPlayerStart);
	const bool bSimulateInEditor = false;

	// Make sure we can find a path to the view port.  This will fail in cases where the view port widget
	// is in a backgrounded tab, etc.  We can't currently support starting PIE in a backgrounded tab
	// due to how PIE manages focus and requires event forwarding from the application.
	if (ActiveLevelViewport.IsValid() && FSlateApplication::Get().FindWidgetWindow(ActiveLevelViewport->AsWidget()).IsValid())
	{
		const FVector* StartLoc = NULL;
		const FRotator* StartRot = NULL;
		if( !bAtPlayerStart )
		{
			// Start the player where the camera is if not forcing from player start
			StartLoc = &ActiveLevelViewport->GetLevelViewportClient().GetViewLocation();
			StartRot = &ActiveLevelViewport->GetLevelViewportClient().GetViewRotation();
		}

		// @todo UE4: Not supported yet
		const bool bUseMobilePreview = false;
		const int32 DestinationConsoleIndex = -1;

		if( !HasPlayWorld() )
		{
			// If there is an active level view port, play the game in it.
			GUnrealEd->RequestPlaySession( bAtPlayerStart, ActiveLevelViewport, bSimulateInEditor, StartLoc, StartRot, DestinationConsoleIndex, bUseMobilePreview );
		}
		else
		{
			// There is already a play world active which means simulate in editor is happening
			// Toggle to pie 
			check( !GIsPlayInEditorWorld );
			GUnrealEd->RequestToggleBetweenPIEandSIE();
		}
	}
	else
	{
		// No active level view port, spawn a new window to play in.
		GUnrealEd->RequestPlaySession( bAtPlayerStart, NULL, bSimulateInEditor );
	}
}

bool FInternalPlayWorldCommandCallbacks::PlayInViewport_CanExecute()
{
	// Allow PIE if we don't already have a play session or the play session is simulate in editor (which we can toggle to PIE)
	return !HasPlayWorld() || GUnrealEd->bIsSimulatingInEditor;
}


void FInternalPlayWorldCommandCallbacks::PlayInEditorFloating_Clicked( )
{
	FLevelEditorModule& LevelEditorModule = FModuleManager::GetModuleChecked<FLevelEditorModule>( TEXT("LevelEditor") );

	SetLastExecutedPlayMode( PlayMode_InEditorFloating );

	// Is a PIE session already running?  If not, then we'll kick off a new one
	if( !HasPlayWorld() )
	{
		RecordLastExecutedPlayMode();

		const bool bAtPlayerStart = (GetPlayModeLocation() == PlayLocation_DefaultPlayerStart);
		const bool bSimulateInEditor = false;

		const FVector* StartLoc = NULL;
		const FRotator* StartRot = NULL;

		if ( !bAtPlayerStart )
		{
			TSharedPtr<ILevelViewport> ActiveLevelViewport = LevelEditorModule.GetFirstActiveViewport();

			// Make sure we can find a path to the view port.  This will fail in cases where the view port widget
			// is in a backgrounded tab, etc.  We can't currently support starting PIE in a backgrounded tab
			// due to how PIE manages focus and requires event forwarding from the application.
			if( ActiveLevelViewport.IsValid() &&
				FSlateApplication::Get().FindWidgetWindow( ActiveLevelViewport->AsWidget() ).IsValid() )
			{
				// Start the player where the camera is if not forcing from player start
				StartLoc = &ActiveLevelViewport->GetLevelViewportClient().GetViewLocation();
				StartRot = &ActiveLevelViewport->GetLevelViewportClient().GetViewRotation();
			}
		}

		// Spawn a new window to play in.
		GUnrealEd->RequestPlaySession(bAtPlayerStart, NULL, bSimulateInEditor, StartLoc, StartRot );
	}
	else
	{
		// Terminate existing session
		GUnrealEd->EndPlayMap();
	}
}


bool FInternalPlayWorldCommandCallbacks::PlayInEditorFloating_CanExecute()
{
	return !HasPlayWorld() || !GUnrealEd->bIsSimulatingInEditor;
}


void FInternalPlayWorldCommandCallbacks::PlayInNewProcess_Clicked( bool MobilePreview )
{
	if (MobilePreview)
	{
		SetLastExecutedPlayMode(PlayMode_InMobilePreview);
	}
	else
	{
		SetLastExecutedPlayMode(PlayMode_InNewProcess);
	}

	if( !HasPlayWorld() )
	{
		RecordLastExecutedPlayMode();

		const FVector* StartLoc = NULL;
		const FRotator* StartRot = NULL;

		const bool bAtPlayerStart = (GetPlayModeLocation() == PlayLocation_DefaultPlayerStart);

		if (!bAtPlayerStart)
		{
			FLevelEditorModule& LevelEditorModule = FModuleManager::GetModuleChecked<FLevelEditorModule>(TEXT("LevelEditor"));
			TSharedPtr<ILevelViewport> ActiveLevelViewport = LevelEditorModule.GetFirstActiveViewport();

			if ( ActiveLevelViewport.IsValid() && FSlateApplication::Get().FindWidgetWindow( ActiveLevelViewport->AsWidget() ).IsValid() )
			{
				StartLoc = &ActiveLevelViewport->GetLevelViewportClient().GetViewLocation();
				StartRot = &ActiveLevelViewport->GetLevelViewportClient().GetViewRotation();
			}
		}

		// Spawn a new window to play in.
		GUnrealEd->RequestPlaySession(StartLoc, StartRot, MobilePreview);
	}
	else
	{
		GUnrealEd->EndPlayMap();
	}
}


bool FInternalPlayWorldCommandCallbacks::PlayInNewProcess_CanExecute()
{
	return true;
}


bool FInternalPlayWorldCommandCallbacks::PlayInModeIsChecked( EPlayModeType PlayMode )
{
	return (PlayMode == GetDefault<ULevelEditorPlaySettings>()->LastExecutedPlayModeType);
}


bool FInternalPlayWorldCommandCallbacks::PlayInLocation_CanExecute( EPlayModeLocations Location )
{
	switch (Location)
	{
	case PlayLocation_CurrentCameraLocation:
		return true;

	case PlayLocation_DefaultPlayerStart:
		return (GEditor->CheckForPlayerStart() != nullptr);

	default:
		return false;
	}
}


void FInternalPlayWorldCommandCallbacks::PlayInLocation_Clicked( EPlayModeLocations Location )
{
	ULevelEditorPlaySettings* PlaySettings = GetMutableDefault<ULevelEditorPlaySettings>();
	PlaySettings->LastExecutedPlayModeLocation = Location;
	PlaySettings->SaveConfig();
}


bool FInternalPlayWorldCommandCallbacks::PlayInLocation_IsChecked( EPlayModeLocations Location )
{
	switch (Location)
	{
	case PlayLocation_CurrentCameraLocation:
		return ((GetDefault<ULevelEditorPlaySettings>()->LastExecutedPlayModeLocation == PlayLocation_CurrentCameraLocation) || (GEditor->CheckForPlayerStart() == nullptr));

	case PlayLocation_DefaultPlayerStart:
		return ((GetDefault<ULevelEditorPlaySettings>()->LastExecutedPlayModeLocation == PlayLocation_DefaultPlayerStart) && (GEditor->CheckForPlayerStart() != nullptr));
	}

	return false;
}


void FInternalPlayWorldCommandCallbacks::PlayInSettings_Clicked()
{
	FModuleManager::LoadModuleChecked<ISettingsModule>("Settings").ShowViewer("Editor", "LevelEditor", "PlayIn");
}


void FInternalPlayWorldCommandCallbacks::OpenDeviceManager_Clicked()
{
	FGlobalTabmanager::Get()->InvokeTab(FTabId("DeviceManager"));
}


void FInternalPlayWorldCommandCallbacks::OpenLauncher_Clicked()
{
	//FGlobalTabmanager::Get()->InvokeTab(FName("ProjectLauncher"));
}


void FInternalPlayWorldCommandCallbacks::RepeatLastLaunch_Clicked()
{
	const ULevelEditorPlaySettings* PlaySettings = GetDefault<ULevelEditorPlaySettings>();

	switch (PlaySettings->LastExecutedLaunchModeType)
	{
	case LaunchMode_OnDevice:
		if (IsReadyToLaunchOnDevice(PlaySettings->LastExecutedLaunchDevice))
		{
			LaunchOnDevice(PlaySettings->LastExecutedLaunchDevice, PlaySettings->LastExecutedLaunchName);
		}
		break;

	default:
		break;
	}	
}


bool FInternalPlayWorldCommandCallbacks::RepeatLastLaunch_CanExecute()
{
	const ULevelEditorPlaySettings* PlaySettings = GetDefault<ULevelEditorPlaySettings>();

	switch (PlaySettings->LastExecutedLaunchModeType)
	{
	case LaunchMode_OnDevice:
		return CanLaunchOnDevice(PlaySettings->LastExecutedLaunchName);

	default:
		return false;
	}
}


FText FInternalPlayWorldCommandCallbacks::GetRepeatLastLaunchToolTip()
{
	const ULevelEditorPlaySettings* PlaySettings = GetDefault<ULevelEditorPlaySettings>();

	switch (PlaySettings->LastExecutedLaunchModeType)
	{
	case LaunchMode_OnDevice:
		if (CanLaunchOnDevice(PlaySettings->LastExecutedLaunchName))
		{
			FFormatNamedArguments Arguments;
			Arguments.Add(TEXT("DeviceName"), FText::FromString(PlaySettings->LastExecutedLaunchName));

			return FText::Format( LOCTEXT("RepeatLaunchTooltip", "Launch this level on {DeviceName}"), Arguments);
		}

		break;

	default:
		break;
	}

	return LOCTEXT("RepeatLaunchSelectOptionToolTip", "Select a play-on target from the combo menu");
}


FSlateIcon FInternalPlayWorldCommandCallbacks::GetRepeatLastLaunchIcon()
{
	const ULevelEditorPlaySettings* PlaySettings = GetDefault<ULevelEditorPlaySettings>();

	// @todo gmp: add play mode specific icons
	switch (PlaySettings->LastExecutedLaunchModeType)
	{
	case LaunchMode_OnDevice:
		break;
	}

	static FName RepeatLastLaunchIcon("PlayWorld.RepeatLastLaunch");

	return FSlateIcon(FEditorStyle::GetStyleSetName(), RepeatLastLaunchIcon );
}

bool FInternalPlayWorldCommandCallbacks::IsReadyToLaunchOnDevice(FString DeviceId)
{
	int32 Index = 0;
	DeviceId.FindChar(TEXT('@'), Index);
	FString PlatformName = DeviceId.Left(Index);

	const PlatformInfo::FPlatformInfo* const PlatformInfo = PlatformInfo::FindPlatformInfo(FName(*PlatformName));
	check(PlatformInfo);

	FGameProjectGenerationModule& GameProjectModule = FModuleManager::LoadModuleChecked<FGameProjectGenerationModule>(TEXT("GameProjectGeneration"));
	bool bHasCode = GameProjectModule.Get().ProjectHasCodeFiles();

	if (PlatformInfo->SDKStatus == PlatformInfo::EPlatformSDKStatus::NotInstalled)
	{
		IMainFrameModule& MainFrameModule = FModuleManager::GetModuleChecked<IMainFrameModule>(TEXT("MainFrame"));
		MainFrameModule.BroadcastMainFrameSDKNotInstalled(PlatformInfo->TargetPlatformName.ToString(), PlatformInfo->SDKTutorial);
		TArray<FAnalyticsEventAttribute> ParamArray;
		ParamArray.Add(FAnalyticsEventAttribute(TEXT("Time"), 0.0));
		FEditorAnalytics::ReportEvent(TEXT("Editor.LaunchOn.Failed"), PlatformInfo->TargetPlatformName.ToString(), bHasCode, EAnalyticsErrorCodes::SDKNotFound, ParamArray);
		return false;
	}

	const ITargetPlatform* Platform = GetTargetPlatformManager()->FindTargetPlatform(PlatformName);
	if (Platform)
	{
		FString NotInstalledTutorialLink;
		FString ProjectPath = FPaths::IsProjectFilePathSet() ? FPaths::ConvertRelativePathToFull(FPaths::GetProjectFilePath()) : FPaths::RootDir() / FApp::GetGameName() / FApp::GetGameName() + TEXT(".uproject");
		int32 Result = Platform->CheckRequirements(ProjectPath, bHasCode, NotInstalledTutorialLink);
		
		// report to analytics
		FEditorAnalytics::ReportBuildRequirementsFailure(TEXT("Editor.LaunchOn.Failed"), PlatformName, bHasCode, Result);

		// report to message log
		bool UnrecoverableError = false;

		if ((Result & ETargetPlatformReadyStatus::SDKNotFound) != 0)
		{
			AddMessageLog(
				LOCTEXT("SdkNotFoundMessage", "Software Development Kit (SDK) not found."),
				FText::Format(LOCTEXT("SdkNotFoundMessageDetail", "Please install the SDK for the {0} target platform!"), Platform->DisplayName()),
				NotInstalledTutorialLink
			);

			UnrecoverableError = true;
		}

		if ((Result & ETargetPlatformReadyStatus::ProvisionNotFound) != 0)
		{
			AddMessageLog(
				LOCTEXT("ProvisionNotFoundMessage", "Provision not found."),
				LOCTEXT("ProvisionNotFoundMessageDetail", "A provision is required for deploying your app to the device."),
				NotInstalledTutorialLink
			);

			UnrecoverableError = true;
		}

		if ((Result & ETargetPlatformReadyStatus::SigningKeyNotFound) != 0)
		{
			AddMessageLog(
				LOCTEXT("SigningKeyNotFoundMessage", "Signing key not found."),
				LOCTEXT("SigningKeyNotFoundMessageDetail", "The app could not be digitally signed, because the signing key is not configured."),
				NotInstalledTutorialLink
			);

			UnrecoverableError = true;
		}

		if (UnrecoverableError)
		{
			return false;
		}

		// report to main frame
		switch (Result)
		{
#if PLATFORM_WINDOWS
		case ETargetPlatformReadyStatus::CodeUnsupported:
			// show the message
			FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("IOSNotSupported", "Sorry, launching on a device is currently not supported for code-based iOS projects. This feature will be available in a future release.") );
			return false;

		case ETargetPlatformReadyStatus::PluginsUnsupported:
			// show the message
			FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("IOSNotSupported", "Sorry, launching on a device is currently not supported for content based projects with third-party plugins. This feature will be available in a future release.") );
			return false;
#endif
		default:
			break;
		}
	}
	else
	{
		const PlatformInfo::FPlatformInfo* PlatformInfo = PlatformInfo::FindPlatformInfo(FName(*PlatformName));
		check(PlatformInfo);

		IMainFrameModule& MainFrameModule = FModuleManager::GetModuleChecked<IMainFrameModule>(TEXT("MainFrame"));
		MainFrameModule.BroadcastMainFrameSDKNotInstalled(PlatformInfo->TargetPlatformName.ToString(), PlatformInfo->SDKTutorial);
		return false;
	}

	return true;
}

void FInternalPlayWorldCommandCallbacks::HandleLaunchOnDeviceActionExecute( FString DeviceId, FString DeviceName )
{
	if (IsReadyToLaunchOnDevice(DeviceId))
	{
		ULevelEditorPlaySettings* PlaySettings = GetMutableDefault<ULevelEditorPlaySettings>();

		PlaySettings->LastExecutedLaunchModeType = LaunchMode_OnDevice;
		PlaySettings->LastExecutedLaunchDevice = DeviceId;
		PlaySettings->LastExecutedLaunchName = DeviceName;
		PlaySettings->SaveConfig();

		LaunchOnDevice(DeviceId, DeviceName);
	}

}


bool FInternalPlayWorldCommandCallbacks::HandleLaunchOnDeviceActionCanExecute(FString DeviceName)
{
	return CanLaunchOnDevice(DeviceName);
}


bool FInternalPlayWorldCommandCallbacks::HandleLaunchOnDeviceActionIsChecked(FString DeviceName)
{
	return (DeviceName == GetDefault<ULevelEditorPlaySettings>()->LastExecutedLaunchName);
}


void FInternalPlayWorldCommandCallbacks::HandleShowSDKTutorial( FString PlatformName, FString NotInstalledDocLink )
{
	// broadcast this, and assume someone will pick it up
	IMainFrameModule& MainFrameModule = FModuleManager::GetModuleChecked<IMainFrameModule>(TEXT("MainFrame"));
	MainFrameModule.BroadcastMainFrameSDKNotInstalled(PlatformName, NotInstalledDocLink);
}

FSlateIcon FInternalPlayWorldCommandCallbacks::GetResumePlaySessionImage()
{
	if ( IsInPIE() )
	{
		return FSlateIcon(FEditorStyle::GetStyleSetName(), "PlayWorld.ResumePlaySession");
	}
	else if ( IsInSIE() )
	{
		return FSlateIcon(FEditorStyle::GetStyleSetName(), "PlayWorld.Simulate");
	}
	else
	{
		return FSlateIcon();
	}
}


FText FInternalPlayWorldCommandCallbacks::GetResumePlaySessionToolTip()
{
	if ( IsInPIE() )
	{
		return LOCTEXT("ResumePIE", "Resume play-in-editor session");
	}
	else if ( IsInSIE() )
	{
		return LOCTEXT("ResumeSIE", "Resume simulation");
	}
	else
	{
		return FText();
	}
}


void FInternalPlayWorldCommandCallbacks::ResumePlaySession_Clicked()
{
	if (HasPlayWorld())
	{
		LeaveDebuggingMode();
		GUnrealEd->PlaySessionResumed();
	}
}


void FInternalPlayWorldCommandCallbacks::PausePlaySession_Clicked()
{
	if (HasPlayWorld())
	{
		GUnrealEd->PlayWorld->bDebugPauseExecution = true;
		GUnrealEd->PlaySessionPaused();
	}
}


void FInternalPlayWorldCommandCallbacks::SingleFrameAdvance_Clicked()
{
	// We want to function just like Single stepping where we will stop at a breakpoint if one is encountered but we also want to stop after 1 tick if a breakpoint is not encountered.
	FKismetDebugUtilities::RequestSingleStepping();
	if (HasPlayWorld())
	{
		GUnrealEd->PlayWorld->bDebugFrameStepExecution = true;
		LeaveDebuggingMode();
		GUnrealEd->PlaySessionSingleStepped();
	}
}


void FInternalPlayWorldCommandCallbacks::StopPlaySession_Clicked()
{
	if (HasPlayWorld())
	{
		GEditor->RequestEndPlayMap();
		LeaveDebuggingMode();
	}
}


void FInternalPlayWorldCommandCallbacks::ShowCurrentStatement_Clicked()
{
	UEdGraphNode* CurrentInstruction = FKismetDebugUtilities::GetCurrentInstruction();
	if (CurrentInstruction != NULL)
	{
		FKismetEditorUtilities::BringKismetToFocusAttentionOnObject(CurrentInstruction);
	}
}


void FInternalPlayWorldCommandCallbacks::StepInto_Clicked()
{
	FKismetDebugUtilities::RequestSingleStepping();
	if (HasPlayWorld())
	{
		LeaveDebuggingMode();
		GUnrealEd->PlaySessionSingleStepped();
	}
}


void FInternalPlayWorldCommandCallbacks::TogglePlayPause_Clicked()
{
	if (HasPlayWorld())
	{
		if (GUnrealEd->PlayWorld->bDebugPauseExecution)
		{
			LeaveDebuggingMode();
			GUnrealEd->PlaySessionResumed();
		}
		else
		{
			GUnrealEd->PlayWorld->bDebugPauseExecution = true;
			GUnrealEd->PlaySessionPaused();
		}
	}
}


bool FInternalPlayWorldCommandCallbacks::CanShowNonPlayWorldOnlyActions()
{
	return !HasPlayWorld();
}


bool FInternalPlayWorldCommandCallbacks::HasPlayWorld()
{
	return GEditor->PlayWorld != NULL;
}


int32 FInternalPlayWorldCommandCallbacks::GetNumberOfClients()
{
	return GetDefault<ULevelEditorPlaySettings>()->PlayNumberOfClients;
}


void FInternalPlayWorldCommandCallbacks::SetNumberOfClients(int32 NumClients, ETextCommit::Type CommitInfo)
{
	ULevelEditorPlaySettings* PlayInSettings = Cast<ULevelEditorPlaySettings>(ULevelEditorPlaySettings::StaticClass()->GetDefaultObject());
	PlayInSettings->PlayNumberOfClients = NumClients;
}


void FInternalPlayWorldCommandCallbacks::OnToggleDedicatedServerPIE()
{
	ULevelEditorPlaySettings* PlayInSettings = Cast<ULevelEditorPlaySettings>(ULevelEditorPlaySettings::StaticClass()->GetDefaultObject());
	PlayInSettings->PlayNetDedicated = !PlayInSettings->PlayNetDedicated;
}


bool FInternalPlayWorldCommandCallbacks::OnIsDedicatedServerPIEEnabled()
{
	return GetDefault<ULevelEditorPlaySettings>()->PlayNetDedicated;
}


bool FInternalPlayWorldCommandCallbacks::HasPlayWorldAndPaused()
{
	return HasPlayWorld() && GUnrealEd->PlayWorld->bDebugPauseExecution;
}


bool FInternalPlayWorldCommandCallbacks::HasPlayWorldAndRunning()
{
	return HasPlayWorld() && !GUnrealEd->PlayWorld->bDebugPauseExecution;
}


bool FInternalPlayWorldCommandCallbacks::IsStoppedAtBreakpoint()
{
	return GIntraFrameDebuggingGameThread;
}


void FInternalPlayWorldCommandCallbacks::PossessEjectPlayer_Clicked()
{
	GEditor->RequestToggleBetweenPIEandSIE();
}


bool FInternalPlayWorldCommandCallbacks::CanPossessEjectPlayer()
{
	if ((IsInSIE() || IsInPIE()) && !IsStoppedAtBreakpoint())
	{
		for (auto It = GUnrealEd->SlatePlayInEditorMap.CreateIterator(); It; ++It)
		{
			return It.Value().DestinationSlateViewport.IsValid();
		}
	}
	return false;
}


void FInternalPlayWorldCommandCallbacks::AddMessageLog( const FText& Text, const FText& Detail, const FString& TutorialLink )
{
	TSharedRef<FTokenizedMessage> Message = FTokenizedMessage::Create(EMessageSeverity::Error);
	Message->AddToken(FTextToken::Create(Text));
	Message->AddToken(FTextToken::Create(Detail));
	Message->AddToken(FTutorialToken::Create(TutorialLink));
	Message->AddToken(FDocumentationToken::Create(TEXT("Platforms/iOS/QuickStart/6")));

	FMessageLog MessageLog("PackagingResults");
	MessageLog.AddMessage(Message);
	MessageLog.Open();
}


bool FInternalPlayWorldCommandCallbacks::CanLaunchOnDevice(const FString& DeviceName)
{
	if (!GUnrealEd->IsPlayingViaLauncher())
	{
		TSharedPtr<ITargetDeviceServicesModule> TargetDeviceServicesModule = StaticCastSharedPtr<ITargetDeviceServicesModule>(FModuleManager::Get().LoadModule(TEXT("TargetDeviceServices")));

		if (TargetDeviceServicesModule.IsValid())
		{
			ITargetDeviceProxyPtr DeviceProxy = TargetDeviceServicesModule->GetDeviceProxyManager()->FindProxy(DeviceName);

			return (DeviceProxy.IsValid() && DeviceProxy->IsConnected());
		}
	}
	return false;
}


void FInternalPlayWorldCommandCallbacks::LaunchOnDevice( const FString& DeviceId, const FString& DeviceName )
{
	FTargetDeviceId TargetDeviceId;
	if (FTargetDeviceId::Parse(DeviceId, TargetDeviceId))
	{
		if (FModuleManager::LoadModuleChecked<IProjectTargetPlatformEditorModule>("ProjectTargetPlatformEditor").ShowUnsupportedTargetWarning(*TargetDeviceId.GetPlatformName()))
		{
			GUnrealEd->RequestPlaySession(DeviceId, DeviceName);
		}
	}
}


EPlayModeLocations FInternalPlayWorldCommandCallbacks::GetPlayModeLocation()
{
	// We can't use PlayLocation_DefaultPlayerStart without a player start position
	return GEditor->CheckForPlayerStart() 
		? static_cast<EPlayModeLocations>(GetDefault<ULevelEditorPlaySettings>()->LastExecutedPlayModeLocation) 
		: PlayLocation_CurrentCameraLocation;
}

#undef LOCTEXT_NAMESPACE
