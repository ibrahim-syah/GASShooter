#pragma once

#include "Engine.h"
#include "Modules/ModuleInterface.h"
#include "Modules/ModuleManager.h"
#include "UnrealEd.h"
#include "Private/Actions_ContextEffectsLibrary.h"

DECLARE_LOG_CATEGORY_EXTERN(GASShooterEditor, All, All)

class FGASShooterEditorModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
private:
    TSharedPtr<FActions_ContextEffectsLibrary> LyraContextEffectsLibraryAssetTypeActions;
};