#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MenuInputProcessor.h"
#include "DetailsPanelGenerator.generated.h"

class UListView;
class UPropertyEntryData;
class UFilterNodeData;
class UButton;
class UEditableTextBox;
class UDynamicEntryBox;
class USizeBox;
class UBorder;
class UWidgetSwitcher;

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
    void GeneratePanel(UObject* TargetObject, FName StructPropertyName);

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

private:
    UPROPERTY() TObjectPtr<UObject> WatchedObject;
    FName RootPropertyName;
    FStructProperty* WatchedRootProperty = nullptr;
    UPROPERTY(Transient) TWeakObjectPtr<class UCategoryEntry> ActiveCategoryEntry;
    UPROPERTY(Transient) TArray<TObjectPtr<class UCheckBoxEntry>> VisibleCheckBoxEntries;

    // 保存所有可能生成的属性数据
    UPROPERTY() TArray<TObjectPtr<UPropertyEntryData>> AllPropertyData;
    // 保存筛选器的树状数据
    UPROPERTY() TArray<TObjectPtr<UFilterNodeData>> FilterTreeData;

    /**
     * 一个扁平化的列表，包含了所有Category下的所有子项（属性节点）。
     * 这是我们进行搜索的数据源。
     */
    UPROPERTY(Transient)
    TArray<TObjectPtr<UFilterNodeData>> AllFilterableItems;

    // --- 数据生成 ---
    void GenerateDataFromReflection();
    void ParseStruct_Recursive(UStruct* InStructDef, void* InStructData, TMap<FName, UFilterNodeData*>& CategoryMap, int32 RecursionDepth);

    // --- UI 更新 ---
    void RefreshListView();
    void RefreshCategoryEntries();
    void RefreshSingleCategoryEntry(class UCategoryEntry* Entry);
    void OnCategoryEntryGenerated(UUserWidget& Widget);
    void OnCheckBoxEntryGenerated(UUserWidget& Widget);

    /**
    * ********************************************************************************
    * ** 过滤器
    * ********************************************************************************
    */
    UFUNCTION()
    void OnFilterChanged(UFilterNodeData* NodeData, bool bNewState);

    UFUNCTION()
    void ShowSubFilterMenu(UFilterNodeData* CategoryData, UUserWidget* HoveredEntry);

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
    void HandleCategoryToggled(UFilterNodeData* CategoryData, bool bIsChecked);

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
    void OnSearchResultClicked(UFilterNodeData* ClickedNode);

    UFUNCTION()
    void OnClearSearchClicked();

    UFUNCTION()
    void OnSearchTextChanged(const FText& Text);

    UFUNCTION()
    void ResetSearchResult();

    //...
    UFUNCTION()
    void HandleSingleValueUpdated(UPropertyEntryData* UpdatedPropertyData);

    //恢复默认值
    UFUNCTION()
    void ResetPropertyToDefault(UFilterNodeData* NodeDataToReset);

    UFUNCTION()
    bool HandleGlobalMouseDown(const FPointerEvent& MouseEvent);

    // 用于处理悬浮延迟的计时器句柄
    FTimerHandle HideMenuTimer;
    FGeometry CachedGeometry;
    bool bIsFilterPanelOpen = false;
    bool bIsSearchResultsOpen = false;
};