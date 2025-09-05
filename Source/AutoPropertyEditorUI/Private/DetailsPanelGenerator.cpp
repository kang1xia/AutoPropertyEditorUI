#include "DetailsPanelGenerator.h"
#include "Components/ListView.h"
#include "Components/TreeView.h"
#include "FilterNodeData.h"
#include "PropertyEntryData.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/CanvasPanelSlot.h" 
#include "CategoryEntry.h"
#include "CheckBoxEntry.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "Components/DynamicEntryBox.h"
#include "Components/SizeBox.h"
#include "Components/WidgetSwitcher.h"

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

void UDetailsPanelGenerator::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);
    // 每帧都将最新的几何信息缓存起来
    CachedGeometry = MyGeometry;
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

void UDetailsPanelGenerator::GeneratePanel(UObject* TargetObject, FName StructPropertyName)
{
    if (!TargetObject || !MainListView || !CategoryListView)
    {
        return;
    }

    WatchedObject = TargetObject;
    RootPropertyName = StructPropertyName;

    FStructProperty* RootStructProperty = FindFProperty<FStructProperty>(WatchedObject->GetClass(), RootPropertyName);
    WatchedRootProperty = RootStructProperty;

    // 1. 从反射生成所有的数据对象
    GenerateDataFromReflection();

    // 2. 填充TreeView
    CategoryListView->ClearListItems();
    CategoryListView->SetListItems(TArray<UObject*>(FilterTreeData));

    // 3. 根据初始筛选条件，刷新ListView
    RefreshListView();
}

void UDetailsPanelGenerator::ResetAllToDefaults()
{
    //UE_LOG(LogTemp, Log, TEXT("--- 正在重置所有已覆盖的属性值为默认值 ---"));

    bool bAtLeastOneValueChanged = false;

    for (UPropertyEntryData* PropData : AllPropertyData)
    {
        if (PropData && PropData->TargetProperty)
        {
            FString OverrideBoolName = FString::Printf(TEXT("bOverride_%s"), *PropData->TargetProperty->GetName());
            FBoolProperty* OverrideProperty = FindFProperty<FBoolProperty>(PropData->TargetProperty->GetOwnerStruct(), *OverrideBoolName);

            if (OverrideProperty && OverrideProperty->GetPropertyValue(OverrideProperty->ContainerPtrToValuePtr<void>(PropData->ParentStructData)))
            {
                // 只有当值真的需要被重置时，才标记为已改变
                if (!FMath::IsNearlyEqual(PropData->CurrentValue, PropData->DefaultValue))
                {
                    const float DefaultValue = PropData->DefaultValue;
                    PropData->UpdateSourceData(DefaultValue);
                    bAtLeastOneValueChanged = true;
                    //UE_LOG(LogTemp, Log, TEXT("属性 '%s' 已被重置为默认值: %f"), *PropData->TargetProperty->GetName(), DefaultValue);
                }
            }
        }
    }

    // --- START OF THE FINAL FIX ---

    // 2. 在所有数据都被重置后，我们需要强制刷新UI
    //    这个检查是可选的，但可以避免在没有任何值改变时进行不必要的UI刷新
    if (bAtLeastOneValueChanged)
    {
        // 【核心修正】调用 RegenerateAllEntries()
        // 这个函数会强制ListView销毁所有现有条目控件，并重新创建它们。
        // 这将保证每个新创建的SliderEntry都会调用NativeOnListItemObjectSet，
        // 从而读取到已经被我们重置回默认值的最新数据。
        MainListView->RegenerateAllEntries();

        // 3. 最后，发出总的通知
        OnRootPropertyChanged.Broadcast(WatchedObject, RootPropertyName);
    }
}

void UDetailsPanelGenerator::OnCategoryEntryGenerated(UUserWidget& Widget)
{
    if (UCategoryEntry* Entry = Cast<UCategoryEntry>(&Widget))
    {
        // 将我们的处理函数绑定到控件的委托上
        Entry->OnHovered.RemoveDynamic(this, &UDetailsPanelGenerator::ShowSubFilterMenu);
        Entry->OnUnhovered.RemoveDynamic(this, &UDetailsPanelGenerator::HideSubFilterMenu);
        Entry->OnToggled.RemoveDynamic(this, &UDetailsPanelGenerator::HandleCategoryToggled);

        Entry->OnHovered.AddDynamic(this, &UDetailsPanelGenerator::ShowSubFilterMenu);
        Entry->OnUnhovered.AddDynamic(this, &UDetailsPanelGenerator::HideSubFilterMenu);
        Entry->OnToggled.AddDynamic(this, &UDetailsPanelGenerator::HandleCategoryToggled);
    }
}

void UDetailsPanelGenerator::OnCheckBoxEntryGenerated(UUserWidget& Widget)
{
    if (UCheckBoxEntry* Entry = Cast<UCheckBoxEntry>(&Widget))
    {
        // 将主面板的计时器控制函数，绑定到每一个CheckBoxEntry的悬浮事件上

        // 先解绑，保证安全
        Entry->OnHovered.RemoveDynamic(this, &UDetailsPanelGenerator::CancelHideMenuTimer);
        Entry->OnUnhovered.RemoveDynamic(this, &UDetailsPanelGenerator::HideSubFilterMenu);

        // 再绑定
        Entry->OnHovered.AddDynamic(this, &UDetailsPanelGenerator::CancelHideMenuTimer);
        Entry->OnUnhovered.AddDynamic(this, &UDetailsPanelGenerator::HideSubFilterMenu);

        VisibleCheckBoxEntries.Add(Entry);
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
    // ListView没有一个简单的方法来“刷新”现有条目。
    // 最简单可靠的方法是重新设置列表项，这会强制重建所有条目并调用NativeOnListItemObjectSet
    CategoryListView->SetListItems(TArray<UObject*>(FilterTreeData));

    //// 由于重新设置列表项会销毁旧控件，我们需要重新绑定新控件的委托
    //// 幸运的是，ListView会在下一帧才销毁，所以我们在这里重新绑定
    //CategoryListView->OnEntryWidgetGenerated().Clear(); // 清除旧的绑定，防止重复
    //CategoryListView->OnEntryWidgetGenerated().AddUObject(this, &UDetailsPanelGenerator::OnCategoryEntryGenerated);
}

void UDetailsPanelGenerator::RefreshSingleCategoryEntry(UCategoryEntry* Entry)
{
    if (Entry)
    {
        // 根据其子项的最新数据状态，重新计算并显示正确的“三态”。
        Entry->RefreshState(Entry->GetListItem());
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

void UDetailsPanelGenerator::HandleSingleValueUpdated(UPropertyEntryData* UpdatedPropertyData)
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

void UDetailsPanelGenerator::ResetPropertyToDefault(UFilterNodeData* NodeDataToReset)
{
    // 1. --- 安全检查 ---
    // 确保传入的数据节点和它包含的属性指针都是有效的。
    if (!NodeDataToReset || !NodeDataToReset->TargetProperty)
    {
        return;
    }

    // 2. --- 推导属性名 ---
    // 获取布尔属性的名称 (例如 "bOverride_WhiteTemp")。
    FString BoolPropertyName = NodeDataToReset->TargetProperty->GetName();

    // FString::RemoveFromStart会移除前缀并返回一个bool值表示是否成功。
    // 这是一个完美的检查，确保我们只处理我们关心的 bOverride_ 属性。
    if (!BoolPropertyName.RemoveFromStart(TEXT("bOverride_")))
    {
        // 如果属性名不以"bOverride_"开头，这不是我们应该处理的情况，直接返回。
        return;
    }

    // 现在 BoolPropertyName 变量的值变成了 "WhiteTemp"。
    // 我们用它来创建一个FName，用于后续的比较。
    const FName NumericPropertyName(*BoolPropertyName);

    // 3. --- 查找并重置 ---
    // 遍历我们缓存的所有数值属性的数据列表 (AllPropertyData)。
    for (UPropertyEntryData* PropData : AllPropertyData)
    {
        // 再次进行安全检查，确保列表中的数据是有效的。
        if (PropData && PropData->TargetProperty)
        {
            // 比较当前数值属性的名称是否与我们推导出的名称相匹配。
            if (PropData->TargetProperty->GetFName() == NumericPropertyName)
            {
                // 4. --- 找到了！执行重置操作 ---

                // a. 从数据对象中获取我们在一开始就保存好的默认值。
                const float DefaultValue = PropData->DefaultValue;

                // b. 调用该数据对象自己的UpdateSourceData函数，
                //    这个函数已经包含了所有必要的反射逻辑，可以将默认值写回到底层内存中。
                PropData->UpdateSourceData(DefaultValue);

                // c. (可选但推荐) 打印一条日志，方便调试，确认操作已执行。
                //UE_LOG(LogTemp, Log, TEXT("属性 '%s' 已被重置为默认值: %f"), *NumericPropertyName.ToString(), DefaultValue);

                // d. 任务已完成，无需继续遍历，跳出循环。
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

    TArray<UFilterNodeData*> FoundItems;
    for (UFilterNodeData* Item : AllFilterableItems)
    {
        if (Item && Item->DisplayName.ToString().Contains(SearchString))
        {
            FoundItems.Add(Item);
        }
    }

    if (FoundItems.Num() > 0)
    {
        // 【核心修改】我们现在手动遍历找到的数据，并为每一项创建UI
        for (UFilterNodeData* FoundItem : FoundItems)
        {
            // 创建一个 Entry Widget
            UCheckBoxEntry* NewEntry = SearchResultsBox->CreateEntry<UCheckBoxEntry>();
            if (NewEntry)
            {
                // 手动调用 NativeOnListItemObjectSet 来用数据填充UI
                // 注意：这里我们模拟了UListView的行为
                NewEntry->RefreshState(FoundItem);

                // 【核心修改】直接在这里为新创建的控件绑定点击事件
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

void UDetailsPanelGenerator::OnSearchResultClicked(UFilterNodeData* ClickedNode)
{
    if (ClickedNode)
    {
        ClickedNode->UpdateSourceDataAndBroadcast(true);
        SearchTextBox->SetText(FText::GetEmpty());
        SearchResultsBox->Reset(true);
        SearchResultsBox->SetVisibility(ESlateVisibility::Collapsed);
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

void UDetailsPanelGenerator::HandleCategoryToggled(UFilterNodeData* CategoryData, bool bIsChecked)
{
    if (!CategoryData) return;

    // 1. 遍历该分类下的所有子项数据
    for (UFilterNodeData* ChildNode : CategoryData->Children)
    {
        // 2. 更新每个子项的底层反射数据
        ChildNode->UpdateSourceDataAndBroadcast(bIsChecked);
    }

    for (UCheckBoxEntry* VisibleEntry : VisibleCheckBoxEntries)
    {
        // 强制它用其关联的最新数据来刷新自己的外观。
        if (VisibleEntry)
        {
            VisibleEntry->RefreshState(VisibleEntry->GetListItem());
        }
    }

    bool bValuesChanged = false;
    for (UFilterNodeData* ChildNode : CategoryData->Children)
    {
        // 先只更新数据，不广播
        ChildNode->bIsChecked = bIsChecked;
        if (ChildNode->TargetProperty && ChildNode->ParentStructData)
        {
            void* ValuePtr = ChildNode->TargetProperty->ContainerPtrToValuePtr<void>(ChildNode->ParentStructData);
            ChildNode->TargetProperty->SetPropertyValue(ValuePtr, bIsChecked);
        }

        if (!bIsChecked)
        {
            ResetPropertyToDefault(ChildNode);
            bValuesChanged = true;
        }
    }

    RefreshListView();
    RefreshCategoryEntries();

    OnRootPropertyChanged.Broadcast(WatchedObject, RootPropertyName);
}

void UDetailsPanelGenerator::HideSubFilterMenu()
{
    // 我们不立即隐藏，而是设置一个短暂的延迟
    // 这允许用户的鼠标可以从主按钮移动到悬浮菜单上，而不会导致菜单消失
    const float HideDelay = 1.0f;
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

    VisibleCheckBoxEntries.Empty();

    if (ActiveCategoryEntry.IsValid())
    {
        ActiveCategoryEntry.Get()->SetHighlight(false);
        ActiveCategoryEntry.Reset();
    }
}

void UDetailsPanelGenerator::GenerateDataFromReflection()
{
    AllPropertyData.Empty();
    FilterTreeData.Empty();
    AllFilterableItems.Empty();

    FStructProperty* RootStructProperty = FindFProperty<FStructProperty>(WatchedObject->GetClass(), RootPropertyName);
    if (!RootStructProperty) return;

    UStruct* RootStructDef = RootStructProperty->Struct;
    void* RootStructData = RootStructProperty->ContainerPtrToValuePtr<void>(WatchedObject);
    TMap<FName, UFilterNodeData*> CategoryMap;

    ParseStruct_Recursive(RootStructDef, RootStructData, CategoryMap, 0);

    for (UFilterNodeData* CategoryNode : FilterTreeData)
    {
        // 安全检查
        if (CategoryNode && CategoryNode->Children.Num() > 0)
        {
            // 将这个分类下的所有子项一次性地追加到扁平化列表中
            AllFilterableItems.Append(CategoryNode->Children);
        }
    }
}

void UDetailsPanelGenerator::ParseStruct_Recursive(UStruct* InStructDef, void* InStructData, TMap<FName, UFilterNodeData*>& CategoryMap, int32 RecursionDepth)
{
    if (!InStructDef || !InStructData || RecursionDepth > 1) return;

    for (TFieldIterator<FProperty> It(InStructDef); It; ++It)
    {
        FProperty* Property = *It;
        if (!Property || !Property->HasAnyPropertyFlags(CPF_Edit)) continue;

        if (FBoolProperty* BoolProperty = CastField<FBoolProperty>(Property))
        {
            if (BoolProperty->GetName().StartsWith("bOverride_"))
            {
                // --- START OF FINAL FIX ---

                FString ParamName = BoolProperty->GetName();
                ParamName.RemoveFromStart(TEXT("bOverride_"));
                FProperty* RealProperty = InStructDef->FindPropertyByName(*ParamName);

                // 【优化】前置条件检查：
                // 1. 必须找到对应的真实属性 (RealProperty != nullptr)
                // 2. 并且这个真实属性的类型必须是数值类型 (FNumericProperty)
                // 只有同时满足这两个条件，我们才为它创建筛选器条目。
                if (!RealProperty || !RealProperty->IsA<FNumericProperty>())
                {
                    // 如果不满足条件，就直接跳过这个 bOverride_ 开关，
                    // 不为它创建任何UI。
                    continue;
                }

                FText DisplayName;
                FString CategoryString;

                if (RealProperty)
                {
                    // 理想情况：找到了对应的真实属性，从它那里获取信息
                    DisplayName = RealProperty->GetDisplayNameText();
                    CategoryString = RealProperty->GetMetaData(TEXT("Category"));
                }
                else
                {
                    // 降级处理：没找到真实属性，就从 bOverride_... 布尔属性自身获取信息
                    DisplayName = BoolProperty->GetDisplayNameText();
                    if (DisplayName.IsEmpty()) // 如果DisplayName为空，就用参数名
                    {
                        DisplayName = FText::FromString(ParamName);
                    }
                    CategoryString = BoolProperty->GetMetaData(TEXT("Category"));
                }

                if (CategoryString.IsEmpty())
                {
                    CategoryString = TEXT("Default"); // 默认分类
                }
                FName Category = FName(*CategoryString);

                // 查找或创建分类节点
                UFilterNodeData* CategoryNode = nullptr;
                if (UFilterNodeData** FoundNode = CategoryMap.Find(Category))
                {
                    CategoryNode = *FoundNode;
                }
                else
                {
                    CategoryNode = NewObject<UFilterNodeData>(this);
                    CategoryNode->NodeType = EFilterNodeType::Category;
                    CategoryNode->DisplayName = FText::FromString(CategoryString);
                    CategoryMap.Add(Category, CategoryNode);
                    FilterTreeData.Add(CategoryNode);
                }

                // 创建属性节点
                UFilterNodeData* FilterNode = NewObject<UFilterNodeData>(this);
                FilterNode->NodeType = EFilterNodeType::Property;
                FilterNode->DisplayName = DisplayName;
                FilterNode->TargetProperty = BoolProperty;
                FilterNode->ParentStructData = InStructData;
                FilterNode->bIsChecked = BoolProperty->GetPropertyValue(BoolProperty->ContainerPtrToValuePtr<void>(InStructData));

                FilterNode->OnStateChanged.AddDynamic(this, &UDetailsPanelGenerator::OnFilterChanged);

                CategoryNode->Children.Add(FilterNode);

                // --- END OF FINAL FIX ---
            }
        }
        else if (FNumericProperty* NumericProperty = CastField<FNumericProperty>(Property))
        {
            UPropertyEntryData* PropData = NewObject<UPropertyEntryData>(this);
            PropData->DisplayName = NumericProperty->GetDisplayNameText();
            PropData->TargetProperty = NumericProperty;
            PropData->ParentStructData = InStructData;

            void* ValuePtr = NumericProperty->ContainerPtrToValuePtr<void>(InStructData);
            float InitialValue = NumericProperty->IsFloatingPoint() ? NumericProperty->GetFloatingPointPropertyValue(ValuePtr) : NumericProperty->GetSignedIntPropertyValue(ValuePtr);

            if (NumericProperty->HasMetaData(TEXT("UIMin")) && NumericProperty->HasMetaData(TEXT("UIMax")))
            {
                PropData->MinValue = FCString::Atof(*NumericProperty->GetMetaData(TEXT("UIMin")));
                PropData->MaxValue = FCString::Atof(*NumericProperty->GetMetaData(TEXT("UIMax")));
            }
            else if (NumericProperty->HasMetaData(TEXT("ClampMin")) && NumericProperty->HasMetaData(TEXT("ClampMax")))
            {
                PropData->MinValue = FCString::Atof(*NumericProperty->GetMetaData(TEXT("ClampMin")));
                PropData->MaxValue = FCString::Atof(*NumericProperty->GetMetaData(TEXT("ClampMax")));
            }
            else
            {
                // 如果都找不到，提供一个安全的默认值
                PropData->MinValue = 0.f;
                PropData->MaxValue = 1.f;
            }

            float ClampedInitialValue = FMath::Clamp(InitialValue, PropData->MinValue, PropData->MaxValue);

            PropData->CurrentValue = ClampedInitialValue;
            PropData->DefaultValue = ClampedInitialValue;

            PropData->OnValueUpdated.AddDynamic(this, &UDetailsPanelGenerator::HandleSingleValueUpdated);
            AllPropertyData.Add(PropData);
        }
        else if (FStructProperty* StructProperty = CastField<FStructProperty>(Property))
        {
            // ... (这部分逻辑保持之前修正后的版本)
            FString OverrideBoolName = FString::Printf(TEXT("bOverride_%s"), *StructProperty->GetName());
            FBoolProperty* OverrideProperty = FindFProperty<FBoolProperty>(InStructDef, *OverrideBoolName);
            if (OverrideProperty && !OverrideProperty->GetPropertyValue(OverrideProperty->ContainerPtrToValuePtr<void>(InStructData)))
            {
                continue;
            }
            void* NestedStructData = StructProperty->ContainerPtrToValuePtr<void>(InStructData);
            ParseStruct_Recursive(StructProperty->Struct, NestedStructData, CategoryMap, RecursionDepth + 1);
        }
    }
}

void UDetailsPanelGenerator::RefreshListView()
{
    TArray<UObject*> ItemsToShow;

    for (UPropertyEntryData* PropData : AllPropertyData)
    {
        if (!PropData || !PropData->TargetProperty) continue;

        // 构造与之对应的 override 开关的名字
        FString OverrideBoolName = FString::Printf(TEXT("bOverride_%s"), *PropData->TargetProperty->GetName());

        FBoolProperty* OverrideProperty = FindFProperty<FBoolProperty>(PropData->TargetProperty->GetOwnerStruct(), *OverrideBoolName);

        if (!OverrideProperty)
        {
            ItemsToShow.Add(PropData);
            continue;
        }

        bool bIsChecked = OverrideProperty->GetPropertyValue(OverrideProperty->ContainerPtrToValuePtr<void>(PropData->ParentStructData));

        if (bIsChecked)
        {
            ItemsToShow.Add(PropData);
        }
    }

    MainListView->SetListItems(ItemsToShow);
}

void UDetailsPanelGenerator::OnFilterChanged(UFilterNodeData* NodeData, bool bNewState)
{
    bool bValueChanged = false;

    if (!bNewState)
    {
        ResetPropertyToDefault(NodeData);
        bValueChanged = true;
    }

    // 当任何一个筛选器的状态改变时，我们只需要刷新ListView即可
    RefreshListView();

    if (ActiveCategoryEntry.IsValid())
    {
        RefreshSingleCategoryEntry(ActiveCategoryEntry.Get());
    }
    //RefreshCategoryEntries();

    OnRootPropertyChanged.Broadcast(WatchedObject, RootPropertyName);
}

void UDetailsPanelGenerator::ShowSubFilterMenu(UFilterNodeData* CategoryData, UUserWidget* HoveredEntry)
{
    VisibleCheckBoxEntries.Empty();
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
    ActiveCategoryEntry = TWeakObjectPtr<UCategoryEntry>(Cast<UCategoryEntry>(HoveredEntry));
    if (ActiveCategoryEntry.IsValid())
    {
        ActiveCategoryEntry.Get()->SetHighlight(true);
    }
}