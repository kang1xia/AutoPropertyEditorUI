#include "PopulateDataTool.h"
#include "PropertyUIData.h"
#include "UObject/UnrealType.h"
#include "Engine/DataTable.h"
#include "Widgets/SWindow.h"
#include "Widgets/SWidget.h"
#include "Editor.h"
#include "Editor/ContentBrowser/Public/ContentBrowserModule.h"
#include "Editor/ContentBrowser/Public/IContentBrowserSingleton.h"
#include "EditorUtilitiesBlueprintFunctionLibrary.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Factories/DataTableFactory.h"

#define LOCTEXT_NAMESPACE "PopulateDataTool"

void UPopulateDataTool::GenerateData()
{
#if WITH_EDITOR
    if (!SourceStruct) {
        UE_LOG(LogTemp, Error, TEXT("Property Data Tool: Source Struct is not set!"));
        return;
    }

    bool bSucceed = CreateDataTableAsset();
    if (bSucceed)
    {
        PopulateDataTableRecursive(SourceStruct, TEXT(""), TEXT(""), 0); // Start recursion
    }

    FString Message = bSucceed ? TEXT("创建资产成功") : TEXT("创建资产失败");
    UEditorUtilitiesBlueprintFunctionLibrary::PopMessageDialog(bSucceed ? EAppMsgCategory::Success : EAppMsgCategory::Error, Message);
#endif
}

void UPopulateDataTool::SelectedUnrealContentFolder()
{
    FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");

    // 1. Create the configuration for the Path Picker.
    FPathPickerConfig PathPickerConfig;

    // 2. Set the delegate that will be called when a path is picked.
    PathPickerConfig.OnPathSelected = FOnPathSelected::CreateUObject(this, &UPopulateDataTool::OnPathSelected);

    // 3. Create the Path Picker widget itself using the correct function from your list.
    TSharedRef<SWidget> PathPickerWidget = ContentBrowserModule.Get().CreatePathPicker(PathPickerConfig);

    // 4. Create and show a modal dialog window to host the Path Picker widget.
    TSharedRef<SWindow> DialogWindow = SNew(SWindow)
        .Title(LOCTEXT("SelectFolderDialogTitle", "Select Folder"))
        .SizingRule(ESizingRule::UserSized)
        .ClientSize(FVector2D(400, 400))
        .SupportsMaximize(false)
        .SupportsMinimize(false)
        [
            PathPickerWidget
        ];

    // Store a weak pointer to the window so our delegate can close it.
    PickerDialogWindow = DialogWindow;

    // Show the modal window. The code execution stops here until the window is closed.
    GEditor->EditorAddModalWindow(DialogWindow);
}

#if WITH_EDITOR
void UPopulateDataTool::PopulateDataTableRecursive(UStruct* TargetStruct, const FString& PathPrefix, const FString& CategoryPrefix, int32 CurrentDepth)
{
    if (!DynamicDT || !TargetStruct) return;

    TArray<uint8> DefaultStructData;
    DefaultStructData.AddUninitialized(TargetStruct->GetStructureSize());
    TargetStruct->InitializeStruct(DefaultStructData.GetData());

    // --- Process Numeric Properties at current level ---
    for (TFieldIterator<FProperty> It(TargetStruct); It; ++It)
    {
        FProperty* Property = *It;
        if (Property && Property->HasAnyPropertyFlags(CPF_Edit))
        {
            const FName RowName = FName(*(PathPrefix.IsEmpty() ? Property->GetName() : FString::Printf(TEXT("%s.%s"), *PathPrefix, *Property->GetName())));
            if (FNumericProperty* NumericProperty = CastField<FNumericProperty>(Property))
            {
                FPropertyUIMetadata NewRow;
                FString CategoryString = NumericProperty->GetMetaData(TEXT("Category"));
                if (CategoryString.IsEmpty())
                {
                    CategoryString = TEXT("Default");
                }

                NewRow.Category = CategoryPrefix.IsEmpty() ? CategoryString : FString::Printf(TEXT("%s|%s"), *CategoryPrefix, *CategoryString);
                NewRow.DisplayName = NumericProperty->GetDisplayNameText();

                NewRow.DisplayName = NumericProperty->GetDisplayNameText();
                float MinValue = 0.f;
                float MaxValue = 0.f;
                if (NumericProperty->HasMetaData(TEXT("UIMin")) && NumericProperty->HasMetaData(TEXT("UIMax")))
                {
                    MinValue = FCString::Atof(*NumericProperty->GetMetaData(TEXT("UIMin")));
                    MaxValue = FCString::Atof(*NumericProperty->GetMetaData(TEXT("UIMax")));
                }
                else if (NumericProperty->HasMetaData(TEXT("ClampMin")) && NumericProperty->HasMetaData(TEXT("ClampMax")))
                {
                    MinValue = FCString::Atof(*NumericProperty->GetMetaData(TEXT("ClampMin")));
                    MaxValue = FCString::Atof(*NumericProperty->GetMetaData(TEXT("ClampMax")));
                }
                else
                {
                    // 如果都找不到，提供一个安全的默认值
                    MinValue = 0.f;
                    MaxValue = 1.f;
                }

                NewRow.ClampMin = MinValue;
                NewRow.ClampMax = MaxValue;

                void* DefaultValuePtr = NumericProperty->ContainerPtrToValuePtr<void>(DefaultStructData.GetData());
                if (NumericProperty->IsFloatingPoint()) 
                {
                    NewRow.DefaultValue = FMath::Clamp(NumericProperty->GetFloatingPointPropertyValue(DefaultValuePtr), MinValue, MaxValue);
                }
                else 
                {
                    NewRow.DefaultValue = FMath::Clamp(NumericProperty->GetSignedIntPropertyValue(DefaultValuePtr), MinValue, MaxValue);
                }

                DynamicDT->AddRow(NumericProperty->GetFName(), NewRow);
            }
        }
    }

    // --- Recurse into Nested Structs if depth allows ---
    if (CurrentDepth < MaxRecursionDepth)
    {
        for (TFieldIterator<FStructProperty> It(TargetStruct); It; ++It)
        {
            FStructProperty* StructProperty = *It;
            if (StructProperty && StructProperty->HasAnyPropertyFlags(CPF_Edit))
            {
                const FString NextPathPrefix = PathPrefix.IsEmpty() ? StructProperty->GetName() : FString::Printf(TEXT("%s.%s"), *PathPrefix, *StructProperty->GetName());
                const FString NextCategoryPrefix = CategoryPrefix.IsEmpty() ? StructProperty->GetDisplayNameText().ToString() : FString::Printf(TEXT("%s|%s"), *CategoryPrefix, *StructProperty->GetDisplayNameText().ToString());

                PopulateDataTableRecursive(StructProperty->Struct, NextPathPrefix, NextCategoryPrefix, CurrentDepth + 1);
            }
        }
    }
}
void UPopulateDataTool::OnPathSelected(const FString& Path)
{
    SelectedDirectory = Path;

    // Close the dialog window
    if (PickerDialogWindow.IsValid())
    {
        PickerDialogWindow.Pin()->RequestDestroyWindow();
    }
}

bool UPopulateDataTool::CreateDataTableAsset()
{
    const FString DT_Name = FString::Printf(TEXT("DT_%s_%d_MetaData"), *SourceStruct->GetName(), MaxRecursionDepth);
    const FString PackagePath = FString::Printf(TEXT("%s/%s"), *SelectedDirectory, *DT_Name);

    DynamicDT = LoadObject<UDataTable>(nullptr, *PackagePath);

    if (!DynamicDT)
    {
        // 3. 【核心】如果不存在，就创建一个新的
        UE_LOG(LogTemp, Log, TEXT("Data Table '%s' not found. Creating a new one..."), *DT_Name);

        // a. 创建一个包 (Package) 来存放我们的新资产
        UPackage* Package = CreatePackage(*PackagePath);
        if (!Package) return false;

        // b. 在这个包里，创建一个UDataTable工厂
        UDataTableFactory* DataTableFactory = NewObject<UDataTableFactory>();
        DataTableFactory->Struct = FPropertyUIMetadata::StaticStruct(); // 设置行结构

        // c. 使用工厂来创建真正的数据表资产
        DynamicDT = (UDataTable*)DataTableFactory->FactoryCreateNew(
            UDataTable::StaticClass(),
            Package,
            *DT_Name,
            RF_Public | RF_Standalone,
            nullptr,
            GWarn
        );

        if (DynamicDT)
        {
            // d. 通知资产注册表我们创建了一个新资产
            FAssetRegistryModule::AssetCreated(DynamicDT);
            // 标记包为已修改，以便编辑器提示保存
            Package->MarkPackageDirty();
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("Failed to create new Data Table."));
            return false;
        }
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("Found existing Data Table '%s'. It will be updated."), *DT_Name);
    }

    return true;
}
#endif

#undef LOCTEXT_NAMESPACE