#include "DetailsPanelGenerator.h"
#include "Components/ListView.h"
#include "Components/TreeView.h"
#include "../Data/BoolPropertyData.h"
#include "../Data/NumericPropertyData.h"
#include "../Data/FilterCategoryData.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/CanvasPanelSlot.h" 
#include "../Entry/FilterCategoryEntry.h"
#include "../Entry/BoolPropertyEntry.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "Components/DynamicEntryBox.h"
#include "Components/SizeBox.h"
#include "Components/WidgetSwitcher.h"
#include "Kismet/GameplayStatics.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Application/IInputProcessor.h"
#include "../Data/PropertyUIData.h"
#include "Kismet/KismetStringLibrary.h"

void UDetailsPanelGenerator::NativePreConstruct()
{
    Super::NativePreConstruct();

    TB_Title->SetText(Title);
}

void UDetailsPanelGenerator::NativeOnInitialized()
{
    Super::NativeOnInitialized();

    if (CategoryListView)
    {
        CategoryListView->OnEntryWidgetGenerated().AddUObject(this, &UDetailsPanelGenerator::OnCategoryEntryGenerated);
    }
    if (SubFilterListView)
    {
        SubFilterListView->OnEntryWidgetGenerated().AddUObject(this, &UDetailsPanelGenerator::OnCheckBoxEntryGenerated);
    }
    if (SearchTextBox)
    {
        SearchTextBox->OnTextChanged.AddDynamic(this, &UDetailsPanelGenerator::OnSearchTextChanged);
    }
    if (BT_Clear)
    {
        BT_Clear->OnClicked.AddDynamic(this, &UDetailsPanelGenerator::OnClearSearchClicked);
    }

    BT_Filter->OnClicked.AddDynamic(this, &ThisClass::ToggleShowCategory);
    BT_ResetEvery->OnClicked.AddDynamic(this, &ThisClass::ResetAllToDefaults);
}

void UDetailsPanelGenerator::BeginDestroy()
{
    Super::BeginDestroy();
}

void UDetailsPanelGenerator::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);
    // 每帧都将最新的几何信息缓存起来
    CachedGeometry = MyGeometry;
}

void UDetailsPanelGenerator::GeneratePanel(UObject* TargetObject, FName StructPropertyName, UDataTable* SourceData)
{
    if (!TargetObject || !MainListView || !CategoryListView)
    {
        return;
    }

    WatchedObject = TargetObject;
    RootPropertyName = StructPropertyName;
    UIDataTable = SourceData;

    TArray<FString> NameArray = UKismetStringLibrary::ParseIntoArray(UIDataTable->GetName(), TEXT("_"));
    if (NameArray.Num() == 4)
    {
        MaxRecursionDepth = FCString::Atoi(*NameArray[2]);
    }
    else
    {
        MaxRecursionDepth = 0;
    }

    FStructProperty* RootStructProperty = FindFProperty<FStructProperty>(WatchedObject->GetClass(), RootPropertyName);
    WatchedRootProperty = RootStructProperty;

    // 1. 从反射生成所有的数据对象
    GenerateDataFromReflection();

    // 2. 填充TreeView
    CategoryListView->ClearListItems();
    CategoryListView->SetListItems(TArray<UObject*>(FilterCategoryData));

    // 3. 根据初始筛选条件，刷新ListView
    RefreshListView();
}

void UDetailsPanelGenerator::GenerateDataFromReflection()
{
    AllPropertyData.Empty();
    FilterCategoryData.Empty();
    AllFilterChildItems.Empty();
    FilterCategoryMap.Empty();

    if (!WatchedRootProperty) return;

    UStruct* RootStructDef = WatchedRootProperty->Struct;
    void* RootStructData = WatchedRootProperty->ContainerPtrToValuePtr<void>(WatchedObject);

    ParseStruct_Recursive(RootStructDef, RootStructData, 0);

    for (UFilterCategoryData* FilterCategoryNode : FilterCategoryData)
    {
        // 安全检查
        if (FilterCategoryNode && FilterCategoryNode->Children.Num() > 0)
        {
            // 将这个分类下的所有子项一次性地追加到扁平化列表中
            AllFilterChildItems.Append(FilterCategoryNode->Children);
        }
    }
}

void UDetailsPanelGenerator::ParseStruct_Recursive(UStruct* InStructDef, void* InStructData, int32 RecursionDepth)
{
    if (!WidgetTree || !InStructDef || !InStructData) return;

    for (TFieldIterator<FProperty> It(InStructDef); It; ++It)
    {
        FProperty* Property = *It;
        if (!Property || !Property->HasAnyPropertyFlags(CPF_Edit)) continue;

        if (FStructProperty* StructProperty = CastField<FStructProperty>(Property))
        {
            if (RecursionDepth < MaxRecursionDepth)
            {
                void* NestedStructData = StructProperty->ContainerPtrToValuePtr<void>(InStructData);
                ParseStruct_Recursive(StructProperty->Struct, NestedStructData, RecursionDepth + 1);
            }
        }
        else if (FNumericProperty* NumericProperty = CastField<FNumericProperty>(Property))
        {
            CreateNumericNode(NumericProperty, InStructData);
        }
    }
}

void UDetailsPanelGenerator::CreateNumericNode(FNumericProperty* NumericProperty, void* ParentStructData)
{
    // ...创建对应的Number
    FPropertyUIMetadata* RowData = UIDataTable->FindRow<FPropertyUIMetadata>(NumericProperty->GetFName(), "");
    if (!RowData) return;

    UNumericPropertyData* PropData = NewObject<UNumericPropertyData>(this);
    PropData->DisplayName = RowData->DisplayName;
    PropData->TargetProperty = NumericProperty;
    PropData->ParentStructData = ParentStructData;
    PropData->MinValue = RowData->ClampMin;
    PropData->MaxValue = RowData->ClampMax;
    PropData->DefaultValue = RowData->DefaultValue;

    void* ValuePtr = NumericProperty->ContainerPtrToValuePtr<void>(ParentStructData);
    float InitialValue = NumericProperty->IsFloatingPoint() ? NumericProperty->GetFloatingPointPropertyValue(ValuePtr) : NumericProperty->GetSignedIntPropertyValue(ValuePtr);
    float ClampedInitialValue = FMath::Clamp(InitialValue, PropData->MinValue, PropData->MaxValue);
    PropData->CurrentValue = ClampedInitialValue;

    PropData->OnValueUpdated.AddDynamic(this, &UDetailsPanelGenerator::HandleSingleValueUpdated);
    AllPropertyData.Add(PropData);

    // 创建对应的筛选器
    CreateFilterNode(NumericProperty->GetFName());
}

void UDetailsPanelGenerator::CreateFilterNode(FName ParentName)
{
    if (!WatchedRootProperty) return;

    UStruct* RootStructDef = WatchedRootProperty->Struct;
    if (!RootStructDef) return;

    auto ParentProperty = RootStructDef->FindPropertyByName(ParentName);
    if (!ParentProperty) return;

    FPropertyUIMetadata* RowData = UIDataTable->FindRow<FPropertyUIMetadata>(ParentName, "");
    if (!RowData) return;

    FText DisplayName = RowData->DisplayName;
    FName CategoryString = *(RowData->Category);

    UFilterCategoryData* FilterCategoryNode = nullptr;
    if (UFilterCategoryData** FoundNode = FilterCategoryMap.Find(CategoryString))
    {
        FilterCategoryNode = *FoundNode;
    }
    else
    {
        FilterCategoryNode = NewObject<UFilterCategoryData>(this);
        FilterCategoryNode->DisplayName = FText::FromName(CategoryString);
        FilterCategoryNode->bIsChecked = false;
        FilterCategoryMap.Add(CategoryString, FilterCategoryNode);
        FilterCategoryData.Add(FilterCategoryNode);
    }

    auto InStructData = WatchedRootProperty->ContainerPtrToValuePtr<void>(WatchedObject);
    UBoolPropertyData* FilterNode = NewObject<UBoolPropertyData>(this);
    FilterNode->DisplayName = DisplayName;
    FilterNode->ParentStructData = InStructData;
    FilterNode->FilterPropertyName = ParentName;

    const FString OverrideBoolName = FString::Printf(TEXT("bOverride_%s"), *(ParentName.ToString()));
    FBoolProperty* BoolProperty = FindFProperty<FBoolProperty>(RootStructDef, *OverrideBoolName);
    if (BoolProperty)
    {
        FilterNode->bHasOverrideSwitch = true;
        FilterNode->TargetProperty = BoolProperty;
        FilterNode->bIsChecked = BoolProperty->GetPropertyValue(BoolProperty->ContainerPtrToValuePtr<void>(InStructData));
    }
    else
    {
        FilterNode->bHasOverrideSwitch = false;
        FilterNode->TargetProperty = nullptr;
        FilterNode->bIsChecked = false;
    }

    FilterNode->OnBoolPropertyStateChanged.AddDynamic(this, &UDetailsPanelGenerator::OnFilterChanged);
    FilterCategoryNode->Children.Add(FilterNode);
}

void UDetailsPanelGenerator::RefreshListView()
{
    TArray<UObject*> ItemsToShow = {};
    TMap<FName, bool> VisibilityMap = {};
    for (UFilterCategoryData* FilterCategoryNode : FilterCategoryData)
    {
        for (UBoolPropertyData* PropNode : FilterCategoryNode->Children)
        {
            if (PropNode)
            {
                VisibilityMap.Add(PropNode->FilterPropertyName, PropNode->bIsChecked);
            }
        }
    }

    for (UNumericPropertyData* PropData : AllPropertyData)
    {
        if (!PropData || !PropData->TargetProperty) continue;

        const FName PropName = PropData->TargetProperty->GetFName();
        const bool bShouldBeVisible = VisibilityMap.FindOrAdd(PropName, true);

        if (bShouldBeVisible)
        {
            ItemsToShow.Add(PropData);
        }
    }

    MainListView->SetListItems(ItemsToShow);
}

FReply UDetailsPanelGenerator::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    bool bHandle = true;

    // 1. 如果筛选器面板本就是关闭的，我们不处理这个点击。
    if (bIsFilterPanelOpen && CategoryListView && CategoryListView->GetVisibility() != ESlateVisibility::Collapsed)
    {
        // a. 获取 CategoryListView 的几何信息（相对于主面板的本地坐标）
        const FGeometry& CategoryListGeometry = CategoryListView->GetCachedGeometry();

        // b. 将鼠标的点击位置也转换到主面板的本地坐标系
        const FVector2D LocalMousePosition = InGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition());

        // c. 检查鼠标点击是否在 CategoryListView 的矩形区域内
        if (!(CategoryListGeometry.IsUnderLocation(LocalMousePosition)))
        {
            // d. 接下来，检查鼠标点击是否在【可见的】子菜单(SubFilterPopup)的矩形区域内
            if (SubFilterPopup && SubFilterPopup->GetVisibility() == ESlateVisibility::Visible)
            {
                const FGeometry& PopupGeometry = SubFilterPopup->GetCachedGeometry();
                if (PopupGeometry.IsUnderLocation(LocalMousePosition))
                {
                    // 如果是，说明点击在子菜单内部，我们也不处理
                    return FReply::Unhandled();
                }
            }

            // 关闭面板
            ToggleShowCategory();
        }
        else
        {
            bHandle = false;
        }
    }

    if (bIsSearchResultsOpen && SearchResultContainer && SearchResultContainer->GetVisibility() != ESlateVisibility::Collapsed)
    {
        const FGeometry& SearchResultContainerGeometry = SearchResultContainer->GetCachedGeometry();
        const FVector2D LocalMousePosition = InGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition());
        if (!(SearchResultContainerGeometry.IsUnderLocation(LocalMousePosition)))
        {
            ResetSearchResult();
        }
        else
        {
            bHandle = false;
        }
    }

    return bHandle ? FReply::Handled() : FReply::Unhandled();
}

void UDetailsPanelGenerator::ResetAllToDefaults()
{
    bool bAtLeastOneValueChanged = false;

    for (UNumericPropertyData* PropData : AllPropertyData)
    {
        if (PropData && PropData->TargetProperty)
        {
            if (!FMath::IsNearlyEqual(PropData->CurrentValue, PropData->DefaultValue))
            {
                const float DefaultValue = PropData->DefaultValue;
                PropData->UpdateSourceData(DefaultValue);
                bAtLeastOneValueChanged = true;
            }
        }
    }

    if (bAtLeastOneValueChanged)
    {
        MainListView->RegenerateAllEntries();
        OnRootPropertyChanged.Broadcast(WatchedObject, RootPropertyName);
    }
}

void UDetailsPanelGenerator::OnCategoryEntryGenerated(UUserWidget& Widget)
{
    if (UFilterCategoryEntry* Entry = Cast<UFilterCategoryEntry>(&Widget))
    {
        // 将我们的处理函数绑定到控件的委托上
        Entry->OnHovered.RemoveDynamic(this, &UDetailsPanelGenerator::ShowSubFilterMenu);
        Entry->OnUnhovered.RemoveDynamic(this, &UDetailsPanelGenerator::HideSubFilterMenu);
        Entry->OnToggled.RemoveDynamic(this, &UDetailsPanelGenerator::HandleFilterCategoryDataToggled);

        Entry->OnHovered.AddDynamic(this, &UDetailsPanelGenerator::ShowSubFilterMenu);
        Entry->OnUnhovered.AddDynamic(this, &UDetailsPanelGenerator::HideSubFilterMenu);
        Entry->OnToggled.AddDynamic(this, &UDetailsPanelGenerator::HandleFilterCategoryDataToggled);

        // 刷新状态
        Entry->RefreshState(Entry->GetListItem());
    }
}

void UDetailsPanelGenerator::OnCheckBoxEntryGenerated(UUserWidget& Widget)
{
    if (UBoolPropertyEntry* Entry = Cast<UBoolPropertyEntry>(&Widget))
    {
        // 将主面板的计时器控制函数，绑定到每一个CheckBoxEntry的悬浮事件上

        // 先解绑，保证安全
        Entry->OnHovered.RemoveDynamic(this, &UDetailsPanelGenerator::CancelHideMenuTimer);
        Entry->OnUnhovered.RemoveDynamic(this, &UDetailsPanelGenerator::HideSubFilterMenu);

        // 再绑定
        Entry->OnHovered.AddDynamic(this, &UDetailsPanelGenerator::CancelHideMenuTimer);
        Entry->OnUnhovered.AddDynamic(this, &UDetailsPanelGenerator::HideSubFilterMenu);
    }
}

void UDetailsPanelGenerator::ResetSearchResult()
{
    bIsSearchResultsOpen = false;
    SearchResultsBox->Reset(true);
    SearchResultContainer->SetVisibility(ESlateVisibility::Collapsed);
}

void UDetailsPanelGenerator::RefreshCategoryEntries()
{
    CategoryListView->SetListItems(TArray<UObject*>(FilterCategoryData));
}

void UDetailsPanelGenerator::RefreshEveryCategoryState()
{
    for (const auto ChildEntry : CategoryListView->GetDisplayedEntryWidgets())
    {
        if (UFilterCategoryEntry* Entry = Cast<UFilterCategoryEntry>(ChildEntry))
        {
            Entry->RefreshState(Entry->GetListItem());
        }
    }
}

void UDetailsPanelGenerator::RefreshSubFilterList()
{
    VisibleCheckBoxEntries.Empty();
    for (auto ChildWidget : SubFilterListView->GetDisplayedEntryWidgets())
    {
        if (UBoolPropertyEntry* Entry = Cast<UBoolPropertyEntry>(ChildWidget))
        {
            VisibleCheckBoxEntries.Add(Entry);
            Entry->RefreshState(Entry->GetListItem());
        }
    }
}

void UDetailsPanelGenerator::ToggleShowCategory()
{
    bIsFilterPanelOpen = !bIsFilterPanelOpen;
    bIsFilterPanelOpen ? ShowCategoryMenu() : HideCategoryMenu();
}

void UDetailsPanelGenerator::ShowCategoryMenu()
{
    CategoryListContainer->SetVisibility(ESlateVisibility::Visible);

    const FGeometry& ButtonGeometry = BT_Filter->GetCachedGeometry();
    const FGeometry& MyGeometry = GetCachedGeometry();
    if (!ButtonGeometry.GetLocalSize().IsNearlyZero())
    {
        if (UCanvasPanelSlot* CategoryListSlot = Cast<UCanvasPanelSlot>(CategoryListContainer->Slot))
        {
            CategoryListSlot->SetAnchors(FAnchors(0.0f, 0.0f, 0.0f, 0.0f));
            CategoryListSlot->SetAlignment(FVector2D(1.0f, 0.0f));

            const FVector2D ButtonAbsolutePosition = ButtonGeometry.GetAbsolutePosition();
            const FVector2D ButtonSize = ButtonGeometry.GetLocalSize();

            const FVector2D MenuDesiredSize = CategoryListContainer->GetDesiredSize();
            const FVector2D TargetAbsolutePosition = ButtonAbsolutePosition + ButtonSize;

            const FVector2D TargetLocalPosition = MyGeometry.AbsoluteToLocal(TargetAbsolutePosition);
            CategoryListSlot->SetPosition(TargetLocalPosition);
        }
    }
}

void UDetailsPanelGenerator::HideCategoryMenu()
{
    CategoryListContainer->SetVisibility(ESlateVisibility::Collapsed);
    ExecuteHideMenu();
}

void UDetailsPanelGenerator::HandleSingleValueUpdated(UNumericPropertyData* UpdatedPropertyData)
{
    if (!UpdatedPropertyData || !UpdatedPropertyData->TargetProperty || !UpdatedPropertyData->ParentStructData)
    {
        return;
    }

    // 我们从传入的数据对象中获取完成任务所需的所有信息：
    FNumericProperty* TargetProperty = UpdatedPropertyData->TargetProperty;
    FName PropertyName = TargetProperty->GetFName();
    UStruct* OwnerStruct = TargetProperty->GetOwnerStruct(); // 获取所属的结构体类型定义
    float NewValue = UpdatedPropertyData->CurrentValue;

    // 广播那个对外的、高级的委托，告诉监听者“根属性”已被修改。
    OnRootPropertyChanged.Broadcast(WatchedObject, RootPropertyName);
}

void UDetailsPanelGenerator::ResetPropertyToDefault(UBoolPropertyData* NodeDataToReset)
{
    if (!NodeDataToReset || !NodeDataToReset->TargetProperty)
    {
        return;
    }

    for (UNumericPropertyData* PropData : AllPropertyData)
    {
        if (PropData && PropData->TargetProperty)
        {
            if (PropData->TargetProperty->GetFName() == NodeDataToReset->FilterPropertyName)
            {
                const float DefaultValue = PropData->DefaultValue;
                PropData->UpdateSourceData(DefaultValue);
                break;
            }
        }
    }
}

void UDetailsPanelGenerator::OnSearchTextChanged(const FText& Text)
{
    const FString& SearchString = Text.ToString();

    if (SearchString.IsEmpty())
    {
        ResetSearchResult();
        WS_ToggleSearch->SetActiveWidgetIndex(0);
        return;
    }

    SearchResultsBox->Reset(true);
    WS_ToggleSearch->SetActiveWidgetIndex(1);

    TArray<UBoolPropertyData*> FoundItems;
    for (UBoolPropertyData* Item : AllFilterChildItems)
    {
        if (Item && Item->DisplayName.ToString().Contains(SearchString))
        {
            FoundItems.Add(Item);
        }
    }

    if (FoundItems.Num() > 0)
    {
        // 我们现在手动遍历找到的数据，并为每一项创建UI
        for (UBoolPropertyData* FoundItem : FoundItems)
        {
            // 创建一个 Entry Widget
            UBoolPropertyEntry* NewEntry = SearchResultsBox->CreateEntry<UBoolPropertyEntry>();
            if (NewEntry)
            {
                // 手动调用 NativeOnListItemObjectSet 来用数据填充UI
                // 注意：这里我们模拟了UListView的行为
                NewEntry->RefreshState(FoundItem);
                NewEntry->SetCheckBoxVisibility(false);
                // 直接在这里为新创建的控件绑定点击事件
                // 我们需要一个方法来捕获点击，最好的方式是在CheckBoxEntry上创建一个新的委托
                NewEntry->OnClicked.RemoveDynamic(this, &UDetailsPanelGenerator::OnSearchResultClicked);
                NewEntry->OnClicked.AddDynamic(this, &UDetailsPanelGenerator::OnSearchResultClicked);
            }
        }

        const FGeometry& SearchBoxGeometry = SearchTextBox->GetCachedGeometry();
        if (!SearchBoxGeometry.GetLocalSize().IsNearlyZero())
        {
            // 2. 获取最外层容器(SearchResultContainer, 也就是那个SizeBox)的 Canvas Panel Slot
            if (UCanvasPanelSlot* SearchResultSlot = Cast<UCanvasPanelSlot>(SearchResultContainer->Slot))
            {
                // 3. 计算目标位置和尺寸
                const FVector2D SearchBoxPosition = SearchBoxGeometry.GetAbsolutePosition();
                const FVector2D SearchBoxSize = SearchBoxGeometry.GetLocalSize();

                // a. 目标位置：搜索框的左下角。我们需要将绝对坐标转换为本地坐标。
                const FVector2D TargetAbsolutePosition = SearchBoxPosition + FVector2D(0.f, SearchBoxSize.Y);
                const FVector2D TargetLocalPosition = CachedGeometry.AbsoluteToLocal(TargetAbsolutePosition);

                // b. 目标尺寸：宽度与搜索框一致。
                const FVector2D TargetSize = FVector2D(SearchBoxSize.X, 0); // Y设为0，让SizeBox自适应高度

                // 4. 应用新的位置和尺寸到【最外层的SizeBox容器】上
                SearchResultSlot->SetPosition(TargetLocalPosition);
                SearchResultSlot->SetSize(TargetSize);
            }
        }

        bIsSearchResultsOpen = true;
        SearchResultContainer->SetVisibility(ESlateVisibility::Visible);
    }
    else
    {
        SearchResultContainer->SetVisibility(ESlateVisibility::Collapsed);
    }
}

void UDetailsPanelGenerator::OnSearchResultClicked(UBoolPropertyData* ClickedNode)
{
    if (ClickedNode)
    {
        ClickedNode->UpdateSourceDataAndBroadcast(!ClickedNode->bIsChecked);
        SearchTextBox->SetText(FText::GetEmpty());
        SearchResultsBox->Reset(true);
        SearchResultContainer->SetVisibility(ESlateVisibility::Collapsed);
    }
}

void UDetailsPanelGenerator::OnClearSearchClicked()
{
    if (SearchTextBox)
    {
        SearchTextBox->SetText(FText::GetEmpty());
        OnSearchTextChanged(FText::GetEmpty());
    }
}

void UDetailsPanelGenerator::HandleFilterCategoryDataToggled(UFilterCategoryData* CategoryData, bool bIsChecked)
{
    if (!CategoryData) return;

    // 1. 遍历该分类下的所有子项数据
    for (UBoolPropertyData* ChildNode : CategoryData->Children)
    {
        // 2. 更新每个子项的底层反射数据
        ChildNode->UpdateSourceDataAndBroadcast(bIsChecked);

        if (!bIsChecked)
        {
            ResetPropertyToDefault(ChildNode);
        }
    }

    RefreshSubFilterList();
    for (UBoolPropertyEntry* VisibleEntry : VisibleCheckBoxEntries)
    {
        if (VisibleEntry)
        {
            VisibleEntry->RefreshState(VisibleEntry->GetListItem());
        }
    }

    RefreshListView();
    RefreshCategoryEntries();
    RefreshEveryCategoryState();

    OnRootPropertyChanged.Broadcast(WatchedObject, RootPropertyName);
}

void UDetailsPanelGenerator::HideSubFilterMenu()
{
    // 我们不立即隐藏，而是设置一个短暂的延迟
    // 这允许用户的鼠标可以从主按钮移动到悬浮菜单上，而不会导致菜单消失
    const float HideDelay = 0.5f;
    GetWorld()->GetTimerManager().SetTimer(HideMenuTimer, this, &UDetailsPanelGenerator::ExecuteHideMenu, HideDelay, false);
}

void UDetailsPanelGenerator::CancelHideMenuTimer()
{
    GetWorld()->GetTimerManager().ClearTimer(HideMenuTimer);
}

void UDetailsPanelGenerator::ExecuteHideMenu()
{
    // 实际执行隐藏的函数
    SubFilterPopup->SetVisibility(ESlateVisibility::Collapsed);

    if (ActiveCategoryEntry.IsValid())
    {
        ActiveCategoryEntry.Get()->SetHighlight(false);
        ActiveCategoryEntry.Reset();
    }
}

void UDetailsPanelGenerator::OnFilterChanged(UBoolPropertyData* NodeData, bool bNewState)
{
    if (!bNewState)
    {
        ResetPropertyToDefault(NodeData);
    }

    // 当任何一个筛选器的状态改变时，我们只需要刷新ListView即可
    RefreshListView();

    // 需要同步更新筛选类型的状态,打开了搜索框和种类框
    if (bIsFilterPanelOpen || bIsSearchResultsOpen)
    {
        RefreshEveryCategoryState();
    }

    OnRootPropertyChanged.Broadcast(WatchedObject, RootPropertyName);
}

void UDetailsPanelGenerator::ShowSubFilterMenu(UFilterCategoryData* CategoryData, UUserWidget* HoveredEntry)
{
    CancelHideMenuTimer();
  
    if (!CategoryData || CategoryData->Children.Num() == 0 || !HoveredEntry)
    {
        SubFilterPopup->SetVisibility(ESlateVisibility::Collapsed);
        return;
    }

    // 1. 填充数据
    SubFilterListView->SetListItems(TArray<UObject*>(CategoryData->Children));

    // 2. 获取所需几何信息
    const FGeometry& EntryGeometry = HoveredEntry->GetCachedGeometry();
    if (EntryGeometry.GetLocalSize().IsNearlyZero())
    {
        SubFilterPopup->SetVisibility(ESlateVisibility::Collapsed);
        return;
    }

    // --- START OF THE SMART POSITIONING LOGIC ---

    // 3. 获取菜单自身的期望尺寸
    //    ForceLayoutPrepass() 会强制Slate计算出控件的期望尺寸，而不会立即渲染它。
    SubFilterPopup->ForceLayoutPrepass();
    const FVector2D MenuDesiredSize = SubFilterPopup->GetDesiredSize();

    // 4. 获取整个视口的尺寸
    const FVector2D ViewportSize = UWidgetLayoutLibrary::GetViewportSize(this);

    // 5. 计算初步的“理想”位置（贴着条目右侧）
    const FVector2D EntryAbsolutePosition = EntryGeometry.GetAbsolutePosition();
    const FVector2D EntryAbsoluteSize = EntryGeometry.GetAbsoluteSize();
    FVector2D TargetAbsolutePosition = EntryAbsolutePosition + FVector2D(EntryAbsoluteSize.X, 0.f);

    // 6. --- 边界检测与位置修正 ---

    // a. 检查右边界
    //    理想位置的X + 菜单宽度 是否超出了 视口宽度？
    if (TargetAbsolutePosition.X + MenuDesiredSize.X > ViewportSize.X)
    {
        // 如果超出，将菜单的水平位置修正到条目的左侧
        TargetAbsolutePosition.X = EntryAbsolutePosition.X - MenuDesiredSize.X;
    }

    // b. 检查下边界
    //    理想位置的Y + 菜单高度 是否超出了 视口高度？
    if (TargetAbsolutePosition.Y + MenuDesiredSize.Y > ViewportSize.Y)
    {
        // 如果超出，将菜单的垂直位置向上调整，使其底部与视口底部对齐
        TargetAbsolutePosition.Y = ViewportSize.Y - MenuDesiredSize.Y;
    }

    // c. 检查左边界和上边界（防止修正后的位置跑到屏幕外）
    TargetAbsolutePosition.X = FMath::Max(0.f, TargetAbsolutePosition.X);
    TargetAbsolutePosition.Y = FMath::Max(0.f, TargetAbsolutePosition.Y);

    // 7. 将最终计算出的绝对坐标，转换为我们主面板的本地坐标
    const FVector2D FinalLocalPosition = CachedGeometry.AbsoluteToLocal(TargetAbsolutePosition);

    // --- END OF THE SMART POSITIONING LOGIC ---

    // 8. 应用最终位置并显示
    if (UCanvasPanelSlot* PopupSlot = Cast<UCanvasPanelSlot>(SubFilterPopup->Slot))
    {
        PopupSlot->SetPosition(FinalLocalPosition);
    }

    SubFilterPopup->SetVisibility(ESlateVisibility::Visible);

    // (处理高亮的逻辑保持不变)
    if (ActiveCategoryEntry.IsValid())
    {
        ActiveCategoryEntry.Get()->SetHighlight(false);
    }
    ActiveCategoryEntry = TWeakObjectPtr<UFilterCategoryEntry>(Cast<UFilterCategoryEntry>(HoveredEntry));
    if (ActiveCategoryEntry.IsValid())
    {
        ActiveCategoryEntry.Get()->SetHighlight(true);
    }

    RefreshSubFilterList();
}