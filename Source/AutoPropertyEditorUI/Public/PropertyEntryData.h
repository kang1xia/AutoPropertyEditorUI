#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "UObject/UnrealType.h"
#include "PropertyEntryData.generated.h"

UCLASS(BlueprintType)
class AUTOPROPERTYEDITORUI_API UPropertyEntryData : public UObject
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintReadOnly, Category = "Property Entry")
    FText DisplayName;

    UPROPERTY(BlueprintReadOnly, Category = "Property Entry")
    float MinValue;

    UPROPERTY(BlueprintReadOnly, Category = "Property Entry")
    float MaxValue;

    UPROPERTY(BlueprintReadWrite, Category = "Property Entry")
    float CurrentValue;

    UPROPERTY(BlueprintReadWrite, Category = "Property Entry")
    float DefaultValue;

    DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnValueUpdated, UPropertyEntryData*, Emitter);
    UPROPERTY(BlueprintAssignable)
    FOnValueUpdated OnValueUpdated;

    // --- C++ Only Data ---
    FNumericProperty* TargetProperty = nullptr;

    void* ParentStructData = nullptr;

    void UpdateSourceData(float NewValue);
};