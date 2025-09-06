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
