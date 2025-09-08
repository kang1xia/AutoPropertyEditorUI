#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DetailsPanelGenerator.generated.h"

class UListView;
class UNumericPropertyData;
class UBoolPropertyData;
class UFilterCategoryData;
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
    virtual UNumericPropertyData* CreateNumericNode(FNumericProperty* NumericProperty, void* ParentStructData);

    // 会创建一个类别和一个筛选器（BOOL）
    virtual UFilterCategoryData* CreateFilterCategoryNode(UStruct* ParentStruct, void* ParentStructData, FName ParentName);

private:
    // 观测对象
    UPROPERTY() TObjectPtr<UObject> WatchedObject;

    // 被观测对象中的那个属性名称
    UPROPERTY() FName RootPropertyName;

    // 这个属性对应的结构体对象
    FStructProperty* WatchedRootProperty = nullptr;

    // 当前观察的种类Entry
    UPROPERTY(Transient) TWeakObjectPtr<class UFilterCategoryEntry> ActiveFilterCategoryEntry;

    // 当前被展示的Bool类型的Entry（过滤器）
    UPROPERTY(Transient) TArray<TObjectPtr<class UBoolPropertyEntry>> VisibleBoolPropertyEntries;

    // 所有Numeric类型的数据
    UPROPERTY() TArray<TObjectPtr<UNumericPropertyData>> EveryNumericPropertyDatas;

    // 所有Bool类型的数据（扮演过滤器）
    TArray<TObjectPtr<UBoolPropertyData>> EveryBoolPropertyDatas;

    // 所有种类数据
    TArray<TObjectPtr<UFilterCategoryData>> FilterCategoryData;
    TMap<FName, UFilterCategoryData*> FilterCategoryMap;

    // --- UI 更新 ---
    void RefreshListView();
    void RefreshCategoryEntries();
    void RefreshEveryCategoryState();
    void RefreshSubFilterList();
    void OnCategoryEntryGenerated(UUserWidget& Widget);
    void OnCheckBoxEntryGenerated(UUserWidget& Widget);

    /**
    * ********************************************************************************
    * ** Bool类型,过滤器
    * ********************************************************************************
    */
    UFUNCTION()
    void OnFilterChanged(UBoolPropertyData* NodeData, bool bNewState);

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
    void HandleFilterCategoryDataToggled(UFilterCategoryData* CategoryData, bool bIsChecked);

    UFUNCTION()
    void ShowSubFilterMenu(UFilterCategoryData* CategoryData, UUserWidget* HoveredEntry);

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