// Fill out your copyright notice in the Description page of Project Settings.

#include "EditorUtilitiesLibrary.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "FileHelpers.h" 
#include "UObject/SavePackage.h"
#include "Misc/MessageDialog.h"

UDataAsset* UEditorUtilitiesLibrary::CreateDataAsset(const FString& PackagePathName, const UClass* DataAssetClass)
{
	if (DataAssetClass != nullptr)
	{
		UPackage* PackageNew = CreatePackage(*PackagePathName);

		UDataAsset* DataAsset = NewObject<UDataAsset>(PackageNew, DataAssetClass, *FPaths::GetBaseFilename(PackagePathName), EObjectFlags::RF_Public | EObjectFlags::RF_Standalone);
		FAssetRegistryModule::AssetCreated(DataAsset);

		if (DataAsset != nullptr)
		{
			FString const PackageName = PackageNew->GetName();
			FString const PackageFileName = FPackageName::LongPackageNameToFilename(PackageName, FPackageName::GetAssetPackageExtension());

			FSavePackageArgs SavePackageArgs;
			SavePackageArgs.TopLevelFlags = RF_Public | RF_Standalone;
			SavePackageArgs.Error = GError;
			SavePackageArgs.SaveFlags = SAVE_NoError;
			UPackage::SavePackage(PackageNew, nullptr, *PackageFileName, SavePackageArgs);
			if (DataAsset->MarkPackageDirty())
			{
				PackageNew->SetDirtyFlag(true);
				return DataAsset;
			}
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Load class is failed!"));
	}
	return nullptr;
}

void UEditorUtilitiesLibrary::PopMessageDialog(EAppMsgCategory Category, const FString& Content)
{
	FMessageDialog::Open(Category, EAppMsgType::Ok, FText::FromString(FString::Printf(TEXT("%s"), *Content)));
}
