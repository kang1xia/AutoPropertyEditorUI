#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DetailsPanelGenerator.generated.h"

class UListView;
class UNumericPropertyData;
class UBoolPropertyData;
class UButton;
class UEditableTextBox;
class UDynamicEntryBox;
class USizeBox;
class UBorder;
class UWidgetSwitcher;
class UDataTable;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnRootPropertyChanged, UObject*, TargetObject, FName, RootPropertyName);

UCLASS()
class AUTOPROPERTYEDITORUI_API UDetailsPanelGenerator : public UUserWidget
{
    GENERATED_BODY()

    friend class FMenuInputProcessor;

public:
    UPROPERTY(BlueprintAssignable, Category = "Details Panel | Events")
    FOnRootPropertyChanged OnRootPropertyChanged;

    UFUNCTION(BlueprintCallable, Category = "Details Panel")
    void GeneratePanel(UObject* TargetObject, FName StructPropertyName, UDataTable* SourceData);

    UFUNCTION(BlueprintCallable, Category = "Details Panel | Actions")
    void ResetAllToDefaults();

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Details Panel")
    FText Title;

protected:
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UListView> MainListView;

    UPROPERTY(meta = (BindWidget)) 
    TObjectPtr<UListView> CategoryListView;

    UPROPERTY(meta = (BindWidget)) 
    TObjectPtr<class UBorder> SubFilterPopup;

    UPROPERTY(meta = (BindWidget)) 
    TObjectPtr<UListView> SubFilterListView;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> BT_Filter;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> BT_ResetEvery;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<class UTextBlock> TB_Title;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UEditableTextBox> SearchTextBox;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UDynamicEntryBox> SearchResultsBox;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UWidget> SearchResultContainer;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UWidgetSwitcher> WS_ToggleSearch;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> BT_Clear;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UWidget> CategoryListContainer;

    virtual void NativePreConstruct() override;
    virtual void NativeOnInitialized() override;
    virtual void BeginDestroy() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
    virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

    virtual void GenerateDataFromReflection();
    virtual void ParseStruct_Recursive(UStruct* InStructDef, void* InStructData, int32 RecursionDepth);
    virtual void CreateNumericNode(FNumericProperty* NumericProperty, void* ParentStructData);
    virtual void CreateFilterNode(FName ParentName);

private:
    UPROPERTY() TObjectPtr<UObject> WatchedObject;
    FName RootPropertyName;
    FStructProperty* WatchedRootProperty = nullptr;
    UPROPERTY(Transient) TWeakObjectPtr<class UFilterCategoryEntry> ActiveCategoryEntry;
    UPROPERTY(Transient) TArray<TObjectPtr<class UBoolPropertyEntry>> VisibleCheckBoxEntries;

    // 保存所有可能生成的属性数据
    UPROPERTY() TArray<TObjectPtr<UNumericPropertyData>> AllPropertyData;

    // 种类筛选器需要的内容
    TArray<TObjectPtr<UBoolPropertyData>> FilterTreeData;
    // 所有种类下的子项
    TArray<TObjectPtr<UBoolPropertyData>> AllFilterChildItems;
    // 对应种类下的子项
    TMap<FName, UBoolPropertyData*> CategoryMap;

    // --- UI 更新 ---
    void RefreshListView();
    void RefreshCategoryEntries();
    void RefreshEveryCategoryState();
    void RefreshSubFilterList();
    void OnCategoryEntryGenerated(UUserWidget& Widget);
    void OnCheckBoxEntryGenerated(UUserWidget& Widget);

    /**
    * ********************************************************************************
    * ** 过滤器
    * ********************************************************************************
    */
    UFUNCTION()
    void OnFilterChanged(UBoolPropertyData* NodeData, bool bNewState);

    UFUNCTION()
    void ShowSubFilterMenu(UBoolPropertyData* CategoryData, UUserWidget* HoveredEntry);

    UFUNCTION()
    void CancelHideMenuTimer();

    UFUNCTION()
    void HideSubFilterMenu();

    UFUNCTION()
    void ExecuteHideMenu();

    /**
    * ********************************************************************************
    * ** 类别
    * ********************************************************************************
    */
    UFUNCTION()
    void HandleCategoryToggled(UBoolPropertyData* CategoryData, bool bIsChecked);

    UFUNCTION()
    void ToggleShowCategory();

    void ShowCategoryMenu();
    void HideCategoryMenu();

    /**
    * ********************************************************************************
    * ** 搜索
    * ********************************************************************************
    */
    UFUNCTION()
    void OnSearchResultClicked(UBoolPropertyData* ClickedNode);

    UFUNCTION()
    void OnClearSearchClicked();

    UFUNCTION()
    void OnSearchTextChanged(const FText& Text);

    UFUNCTION()
    void ResetSearchResult();

    //...
    UFUNCTION()
    void HandleSingleValueUpdated(UNumericPropertyData* UpdatedPropertyData);

    //恢复默认值
    UFUNCTION()
    void ResetPropertyToDefault(UBoolPropertyData* NodeDataToReset);

    // 用于处理悬浮延迟的计时器句柄
    FTimerHandle HideMenuTimer;
    FGeometry CachedGeometry;
    bool bIsFilterPanelOpen = false;
    bool bIsSearchResultsOpen = false;
    TObjectPtr<UDataTable> UIDataTable;
    int32 MaxRecursionDepth = 1;
};