// Copyright Epic Games, Inc. All Rights Reserved.

#include "Actions_ContextEffectsLibrary.h"
#include "GASShooter/Feedback/ContextEffect/LyraContextEffectsLibrary.h"

class UClass;

#define LOCTEXT_NAMESPACE "AssetTypeActions"

UClass* FActions_ContextEffectsLibrary::GetSupportedClass() const
{
	return ULyraContextEffectsLibrary::StaticClass();
}

#undef LOCTEXT_NAMESPACE
