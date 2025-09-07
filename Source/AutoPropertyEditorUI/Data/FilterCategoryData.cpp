#include "FilterCategoryData.h"
#include "BoolPropertyData.h"

void UFilterCategoryData::UpdateSourceDataAndBroadcast(bool bNewValue)
{
    if (bIsChecked == bNewValue)
    {
        return;
    }

    // 更新我们自己的数据对象
    bIsChecked = bNewValue;

    OnFilterCategoryStateChanged.Broadcast(this, bIsChecked);
}

ECheckBoxState UFilterCategoryData::GetCheckStateFromChildren() const
{
    // 初始化状态追踪变量
    int32 CheckedCount = 0;

    // 遍历所有子节点，统计勾选的数量
    for (const UBoolPropertyData* Child : Children)
    {
        if (Child && Child->bIsChecked)
        {
            CheckedCount++;
        }
    }

    // 根据统计结果，判断最终状态
    if (CheckedCount == 0)
    {
        // 如果没有任何子项被勾选 -> 全不选
        return ECheckBoxState::Unchecked;
    }
    else if (CheckedCount == Children.Num())
    {
        // 如果所有子项都被勾选 -> 全选
        return ECheckBoxState::Checked;
    }
    else
    {
        // 否则 (部分被勾选) -> 未确定
        return ECheckBoxState::Undetermined;
    }
}
