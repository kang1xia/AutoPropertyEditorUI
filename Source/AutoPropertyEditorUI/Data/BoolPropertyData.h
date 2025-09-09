#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Types/SlateEnums.h"
#include "BoolPropertyData.generated.h"

UCLASS(BlueprintType)
class AUTOPROPERTYEDITORUI_API UBoolPropertyData : public UObject
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintReadOnly, Category = "Filter Node")
    FText DisplayName;

    UPROPERTY(BlueprintReadWrite, Category = "Filter Node")
    bool bIsChecked;

    UPROPERTY(BlueprintReadWrite, Category = "Filter Node")
    FName FilterPropertyName;

    // 委托：当勾选状态改变时广播
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnBoolPropertyStateChanged, UBoolPropertyData*, EmitterNode, bool, bIsChecked);
    UPROPERTY(BlueprintAssignable)
    FOnBoolPropertyStateChanged OnBoolPropertyStateChanged;

    UPROPERTY(BlueprintReadOnly, Category = "Filter Node")
    TArray<TObjectPtr<UBoolPropertyData>> Children;

    // --- C++ Only Data ---
    FBoolProperty* TargetProperty = nullptr;
    void* ParentStructData = nullptr;
    bool bHasOverrideSwitch = false;

    // C++函数，用于更新真实数据
    void UpdateSourceDataAndBroadcast(bool bNewValue);
    void UpdateSourceData(bool bNewValue);
};