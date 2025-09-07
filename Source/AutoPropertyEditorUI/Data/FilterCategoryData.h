#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Types/SlateEnums.h"
#include "FilterCategoryData.generated.h"

class UBoolPropertyData;

UCLASS(BlueprintType)
class AUTOPROPERTYEDITORUI_API UFilterCategoryData : public UObject
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintReadOnly, Category = "Filter Node")
    FText DisplayName;

    UPROPERTY(BlueprintReadWrite, Category = "Filter Node")
    bool bIsChecked;

    // ί�У�����ѡ״̬�ı�ʱ�㲥
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnFilterCategoryStateChanged, UFilterCategoryData*, FilterCategoryData, bool, bIsChecked);
    UPROPERTY(BlueprintAssignable)
    FOnFilterCategoryStateChanged OnFilterCategoryStateChanged;

    UPROPERTY(BlueprintReadOnly, Category = "Filter Node")
    TArray<TObjectPtr<UBoolPropertyData>> Children;

    // C++���������ڸ�����ʵ����
    void UpdateSourceDataAndBroadcast(bool bNewValue);

    /**
     * һ��������ר������Category���͵Ľڵ㣬
     * ����������ӽڵ��״̬��������Լ�Ӧ�ô��ڵġ���̬����
     * @return ���� ECheckBoxState ö��ֵ (Unchecked, Checked, or Undetermined).
     */
    ECheckBoxState GetCheckStateFromChildren() const;
};