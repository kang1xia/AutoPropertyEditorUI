#include "BoolPropertyData.h"

void UBoolPropertyData::UpdateSourceDataAndBroadcast(bool bNewValue)
{
    UpdateSourceData(bNewValue);
    OnBoolPropertyStateChanged.Broadcast(this, bIsChecked);
}

void UBoolPropertyData::UpdateSourceData(bool bNewValue)
{
    if (bIsChecked == bNewValue)
    {
        return;
    }

    if (TargetProperty && ParentStructData && bHasOverrideSwitch)
    {
        // 更新反射系统中的真实数据
        void* ValuePtr = TargetProperty->ContainerPtrToValuePtr<void>(ParentStructData);
        TargetProperty->SetPropertyValue(ValuePtr, bNewValue);
    }

    // 更新我们自己的数据对象
    bIsChecked = bNewValue;
}
