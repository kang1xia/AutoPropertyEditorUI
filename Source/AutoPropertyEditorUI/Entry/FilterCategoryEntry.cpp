#include "FilterCategoryEntry.h"
#include "Components/TextBlock.h"
#include "Components/CheckBox.h"
#include "../Data/FilterCategoryData.h"

void UFilterCategoryEntry::RefreshState(UObject* ListItemObject)
{
    LinkedData = Cast<UFilterCategoryData>(ListItemObject);
    if (LinkedData)
    {
        CategoryNameText->SetText(LinkedData->DisplayName);

        const ECheckBoxState CurrentState = LinkedData->GetCheckStateFromChildren();
        MainCheckBox->SetCheckedState(CurrentState);
    }
}

void UFilterCategoryEntry::NativeOnListItemObjectSet(UObject* ListItemObject)
{
    RefreshState(ListItemObject);
}

void UFilterCategoryEntry::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    if (MainCheckBox)
    {
        MainCheckBox->OnCheckStateChanged.AddDynamic(this, &UFilterCategoryEntry::HandleCheckStateChanged);
    }
}

void UFilterCategoryEntry::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    Super::NativeOnMouseEnter(InGeometry, InMouseEvent);
    // 当鼠标进入时，广播 OnHovered 委托，并把自己的数据和鼠标事件信息传递出去
    OnHovered.Broadcast(LinkedData, this);
}

void UFilterCategoryEntry::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
    Super::NativeOnMouseLeave(InMouseEvent);
    // 当鼠标离开时，广播 OnUnhovered 委托
    OnUnhovered.Broadcast();
}

void UFilterCategoryEntry::HandleCheckStateChanged(bool bIsChecked)
{
    OnToggled.Broadcast(LinkedData, bIsChecked);
}
