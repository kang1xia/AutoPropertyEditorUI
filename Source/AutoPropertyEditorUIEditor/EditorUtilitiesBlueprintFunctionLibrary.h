// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "EditorUtilitiesBlueprintFunctionLibrary.generated.h"

/**
 * 
 */
UCLASS()
class AUTOPROPERTYEDITORUIEDITOR_API UEditorUtilitiesBlueprintFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "EditorUtilities")
	static UDataAsset* CreateDataAsset(const FString& PackagePathName, const UClass* DataAssetClass);

	UFUNCTION(BlueprintCallable, Category = "EditorUtilities")
	static void PopMessageDialog(EAppMsgCategory Category, const FString& Content);
};
