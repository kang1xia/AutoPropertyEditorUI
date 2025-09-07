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

    // 委托：当勾选状态改变时广播
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnFilterCategoryStateChanged, UFilterCategoryData*, FilterCategoryData, bool, bIsChecked);
    UPROPERTY(BlueprintAssignable)
    FOnFilterCategoryStateChanged OnFilterCategoryStateChanged;

    UPROPERTY(BlueprintReadOnly, Category = "Filter Node")
    TArray<TObjectPtr<UBoolPropertyData>> Children;

    // C++函数，用于更新真实数据
    void UpdateSourceDataAndBroadcast(bool bNewValue);

    /**
     * 一个函数，专门用于Category类型的节点，
     * 它会根据其子节点的状态，计算出自己应该处于的“三态”。
     * @return 返回 ECheckBoxState 枚举值 (Unchecked, Checked, or Undetermined).
     */
    ECheckBoxState GetCheckStateFromChildren() const;
};