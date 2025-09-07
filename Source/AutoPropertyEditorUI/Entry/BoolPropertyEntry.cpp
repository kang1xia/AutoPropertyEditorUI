#include "BoolPropertyEntry.h"
#include "../Data/BoolPropertyData.h"
#include "Components/CheckBox.h"
#include "Components/TextBlock.h"

void UBoolPropertyEntry::NativeOnInitialized()
{
    Super::NativeOnInitialized();

    // 确保复选框控件有效，然后将其OnCheckStateChanged事件绑定到我们的处理函数上。
    if (ValueCheckBox)
    {
        ValueCheckBox->OnCheckStateChanged.AddDynamic(this, &UBoolPropertyEntry::HandleCheckStateChanged);
    }
}

void UBoolPropertyEntry::RefreshState(UObject* ListItemObject)
{
    // 1. 将传入的通用UObject*安全地转换为我们期望的UFilterNodeData*类型。
    LinkedData = Cast<UBoolPropertyData>(ListItemObject);
    if (!LinkedData)
    {
        // 如果转换失败，隐藏此条目以避免显示错误信息。
        SetVisibility(ESlateVisibility::Collapsed);
        return;
    }

    // 确保条目可见（因为可能会被复用）。
    SetVisibility(ESlateVisibility::Visible);

    // 2. 使用数据对象中的信息来配置UI元素。
    TitleText->SetText(LinkedData->DisplayName);

    // 3. 根据数据对象的类型，决定是否显示复选框。
    // 分类节点（Category）不应该有复选框。
    if (LinkedData->NodeType == EFilterNodeType::Category)
    {
        ValueCheckBox->SetVisibility(ESlateVisibility::Hidden);
    }
    else // 属性节点（Property）应该有复选框。
    {
        ValueCheckBox->SetVisibility(ESlateVisibility::Visible);

        // 设置复选框的初始状态，注意不要触发绑定的事件。
        // SetIsChecked函数不会触发OnCheckStateChanged事件。
        ValueCheckBox->SetIsChecked(LinkedData->bIsChecked);
    }
}

void UBoolPropertyEntry::SetCheckBoxVisibility(bool bVis)
{
    ValueCheckBox->SetVisibility(bVis ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
}

void UBoolPropertyEntry::NativeOnListItemObjectSet(UObject* ListItemObject)
{
    RefreshState(ListItemObject);
}

void UBoolPropertyEntry::HandleCheckStateChanged(bool bIsChecked)
{
    // 当用户点击复选框时：
    // 1. 确保我们有一个有效的数据对象与之关联。
    if (LinkedData)
    {
        LinkedData->UpdateSourceDataAndBroadcast(bIsChecked);
    }
}

void UBoolPropertyEntry::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    Super::NativeOnMouseEnter(InGeometry, InMouseEvent);
    SetHighlight(true);
    OnHovered.Broadcast();
}

// 【新增】当鼠标离开此控件时，同样调用蓝图实现的SetHighlight事件
void UBoolPropertyEntry::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
    Super::NativeOnMouseLeave(InMouseEvent);
    SetHighlight(false);
    OnUnhovered.Broadcast();
}

FReply UBoolPropertyEntry::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
    {
        // 广播点击事件，并把自己关联的数据传递出去
        OnClicked.Broadcast(LinkedData);
        // 返回 Handled，表示我们已经处理了这个点击
        return FReply::Handled();
    }

    return FReply::Unhandled();
}
