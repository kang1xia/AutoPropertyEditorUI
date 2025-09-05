#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Misc/Paths.h"
#include "PopulateDataTool.generated.h"

class UDataTable;
class SWindow;

UCLASS(Blueprintable, meta = (DisplayName = "Property Data Exporter Tool"))
class AUTOPROPERTYEDITORUIEDITOR_API UPopulateDataTool : public UObject
{
    GENERATED_BODY()

public:
    /**
     * The Struct to export property data from.
     * You can select any UStruct available in the engine here!
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings", meta = (RequiredAssetDataTags = "RowStructure=/Script/CoreUObject.ScriptStruct"))
    TObjectPtr<UScriptStruct> SourceStruct;

    /**
     * How many levels of nested structs to recurse into.
     * 0 = Top level only.
     * 1 = Recurse one level deep (e.g., into FPostProcessSettings).
     * 2 = Recurse two levels deep.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings", meta = (ClampMin = "0", ClampMax = "5"))
    int32 MaxRecursionDepth = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
    FString SelectedDirectory = TEXT("/Game");

    /** Press this button to generate/update the Data Table. */
    UFUNCTION(BlueprintCallable, CallInEditor, Category = "Actions", meta = (DisplayName = "生成"))
    void GenerateData();

    UFUNCTION(BlueprintCallable, CallInEditor, Category = "Actions", meta = (DisplayName = "选择路径"))
    void SelectedUnrealContentFolder();

private:
#if WITH_EDITOR
    /** The recursive helper function. */
    void PopulateDataTableRecursive(UStruct* TargetStruct, const FString& PathPrefix, const FString& CategoryPrefix, int32 CurrentDepth);
#endif

    TWeakPtr<SWindow> PickerDialogWindow;

    UPROPERTY()
    TObjectPtr<UDataTable> DynamicDT = nullptr;

    UFUNCTION()
    void OnPathSelected(const FString& Path);

    bool CreateDataTableAsset();
};