// Copyright Epic Games, Inc. All Rights Reserved.

#include "ContextFXLibraryFactory.h"

#include "GASShooter/Feedback/ContextEffect/LyraContextEffectsLibrary.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ContextFXLibraryFactory)

class FFeedbackContext;
class UClass;
class UObject;

UContextFXLibraryFactory::UContextFXLibraryFactory(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SupportedClass = ULyraContextEffectsLibrary::StaticClass();

	bCreateNew = true;
	bEditorImport = false;
	bEditAfterNew = true;
}

UObject* UContextFXLibraryFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	ULyraContextEffectsLibrary* LyraContextEffectsLibrary = NewObject<ULyraContextEffectsLibrary>(InParent, Name, Flags);

	return LyraContextEffectsLibrary;
}