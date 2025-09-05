#include "CategoryEntry.h"
#include "FilterNodeData.h"
#include "Components/TextBlock.h"
#include "Components/CheckBox.h"

void UCategoryEntry::RefreshState(UObject* ListItemObject)
{
    LinkedData = Cast<UFilterNodeData>(ListItemObject);
    if (LinkedData)
    {
        CategoryNameText->SetText(LinkedData->DisplayName);

        const ECheckBoxState CurrentState = LinkedData->GetCheckStateFromChildren();
        MainCheckBox->SetCheckedState(CurrentState);

        //// 根据子项的状态，决定“全选”复选框的初始状态
        //// 逻辑：如果所有子项都被勾选，则为勾选状态；只要有一个子项未勾选，则为未勾选状态。
        //bool bAllChildrenChecked = true;
        //if (LinkedData->Children.Num() > 0)
        //{
        //    for (const UFilterNodeData* Child : LinkedData->Children)
        //    {
        //        if (!Child->bIsChecked)
        //        {
        //            bAllChildrenChecked = false;
        //            break;
        //        }
        //    }
        //}
        //else
        //{
        //    bAllChildrenChecked = false; // 如果没有子项，也算未勾选
        //}

        //MainCheckBox->SetIsChecked(bAllChildrenChecked);
    }
}

void UCategoryEntry::NativeOnListItemObjectSet(UObject* ListItemObject)
{
    RefreshState(ListItemObject);
}

void UCategoryEntry::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    if (MainCheckBox)
    {
        MainCheckBox->OnCheckStateChanged.AddDynamic(this, &UCategoryEntry::HandleCheckStateChanged);
    }
}

void UCategoryEntry::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    Super::NativeOnMouseEnter(InGeometry, InMouseEvent);
    // 当鼠标进入时，广播 OnHovered 委托，并把自己的数据和鼠标事件信息传递出去
    OnHovered.Broadcast(LinkedData, this);
}

void UCategoryEntry::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
    Super::NativeOnMouseLeave(InMouseEvent);
    // 当鼠标离开时，广播 OnUnhovered 委托
    OnUnhovered.Broadcast();
}

void UCategoryEntry::HandleCheckStateChanged(bool bIsChecked)
{
    OnToggled.Broadcast(LinkedData, bIsChecked);
}
