#include "GASShooterEditor.h"
#include "Modules/ModuleManager.h"
#include "Modules/ModuleInterface.h"

IMPLEMENT_GAME_MODULE(FGASShooterEditorModule, GASShooterEditor);

DEFINE_LOG_CATEGORY(GASShooterEditor)

#define LOCTEXT_NAMESPACE "GASShooterEditor"

void FGASShooterEditorModule::StartupModule()
{
    UE_LOG(GASShooterEditor, Warning, TEXT("GASShooterEditor: Log Started"));
    LyraContextEffectsLibraryAssetTypeActions = MakeShared<FActions_ContextEffectsLibrary>();
    FAssetToolsModule::GetModule().Get().RegisterAssetTypeActions(LyraContextEffectsLibraryAssetTypeActions.ToSharedRef());
}

void FGASShooterEditorModule::ShutdownModule()
{
    UE_LOG(GASShooterEditor, Warning, TEXT("GASShooterEditor: Log Ended"));
    if (!FModuleManager::Get().IsModuleLoaded("AssetTools")) return;
    FAssetToolsModule::GetModule().Get().UnregisterAssetTypeActions(LyraContextEffectsLibraryAssetTypeActions.ToSharedRef());
}

#undef LOCTEXT_NAMESPACE