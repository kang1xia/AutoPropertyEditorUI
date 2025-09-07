#include "FilterCategoryData.h"
#include "BoolPropertyData.h"

void UFilterCategoryData::UpdateSourceDataAndBroadcast(bool bNewValue)
{
    if (bIsChecked == bNewValue)
    {
        return;
    }

    // ���������Լ������ݶ���
    bIsChecked = bNewValue;

    OnFilterCategoryStateChanged.Broadcast(this, bIsChecked);
}

ECheckBoxState UFilterCategoryData::GetCheckStateFromChildren() const
{
    // ��ʼ��״̬׷�ٱ���
    int32 CheckedCount = 0;

    // ���������ӽڵ㣬ͳ�ƹ�ѡ������
    for (const UBoolPropertyData* Child : Children)
    {
        if (Child && Child->bIsChecked)
        {
            CheckedCount++;
        }
    }

    // ����ͳ�ƽ�����ж�����״̬
    if (CheckedCount == 0)
    {
        // ���û���κ������ѡ -> ȫ��ѡ
        return ECheckBoxState::Unchecked;
    }
    else if (CheckedCount == Children.Num())
    {
        // ��������������ѡ -> ȫѡ
        return ECheckBoxState::Checked;
    }
    else
    {
        // ���� (���ֱ���ѡ) -> δȷ��
        return ECheckBoxState::Undetermined;
    }
}
