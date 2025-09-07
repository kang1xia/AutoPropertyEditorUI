#include "NumericPropertyEntry.h"
#include "../Data/NumericPropertyData.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Components/ListView.h"
#include "Components/Button.h"

void UNumericPropertyEntry::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    if (ValueSlider)
    {
        ValueSlider->OnValueChanged.AddDynamic(this, &UNumericPropertyEntry::HandleSliderValueChanged);
        BT_Reset->OnClicked.AddDynamic(this, &UNumericPropertyEntry::HandleResetClicked);
    }
}

void UNumericPropertyEntry::NativeOnListItemObjectSet(UObject* ListItemObject)
{
    // 1. 将通用UObject转换为我们的特定数据类型
    LinkedData = Cast<UNumericPropertyData>(ListItemObject);
    if (!LinkedData) return;

    // 2. 使用数据对象来配置UI
    TitleText->SetText(LinkedData->DisplayName);
    ValueSlider->SetMinValue(LinkedData->MinValue);
    ValueSlider->SetMaxValue(LinkedData->MaxValue);
    ValueSlider->SetValue(LinkedData->CurrentValue);
    ValueText->SetText(FText::FromString(FString::Printf(TEXT("%.1f"), LinkedData->CurrentValue)));

    if (const UListView* OwningListView = Cast<UListView>(GetOwningListView()))
    {
        // 向ListView查询此条目数据(ListItemObject)对应的索引号。
        const int32 ItemIndex = OwningListView->GetIndexForItem(ListItemObject);

        // 判断索引是否有效，以及是否为偶数。
        // (索引从0开始，所以第1行是偶数，第2行是奇数，以此类推)
        const bool bIsEvenRow = (ItemIndex != INDEX_NONE) && (ItemIndex % 2 == 0);

        // 调用我们在.h中声明的蓝图事件，将结果传递给蓝图去处理视觉效果。
        SetRowStyle(bIsEvenRow);
    }
}

void UNumericPropertyEntry::HandleSliderValueChanged(float InValue)
{
    // 当UI变化时，通过数据对象来更新源数据
    if (LinkedData)
    {
        ValueSlider->SetValue(InValue);
        ValueText->SetText(FText::FromString(FString::Printf(TEXT("%.1f"), InValue)));

        LinkedData->UpdateSourceData(InValue);
        LinkedData->OnValueUpdated.Broadcast(LinkedData);
    }
}

void UNumericPropertyEntry::HandleResetClicked()
{
    ValueSlider->SetValue(LinkedData->DefaultValue);
    ValueText->SetText(FText::FromString(FString::Printf(TEXT("%.1f"), LinkedData->DefaultValue)));
}
