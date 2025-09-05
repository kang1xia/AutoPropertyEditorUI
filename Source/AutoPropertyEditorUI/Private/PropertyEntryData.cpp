#include "PropertyEntryData.h"

void UPropertyEntryData::UpdateSourceData(float NewValue)
{
    if (TargetProperty && ParentStructData)
    {
        void* ValuePtr = TargetProperty->ContainerPtrToValuePtr<void>(ParentStructData);
        if (TargetProperty->IsFloatingPoint())
        {
            TargetProperty->SetFloatingPointPropertyValue(ValuePtr, NewValue);
        }
        else
        {
            TargetProperty->SetIntPropertyValue(ValuePtr, static_cast<int64>(NewValue));
        }
        CurrentValue = NewValue;
    }
}