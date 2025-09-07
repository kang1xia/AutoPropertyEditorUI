#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "PropertyUIData.generated.h"

// 【核心】定义数据表的行结构，它必须继承自FTableRowBase
USTRUCT(BlueprintType)
struct AUTOPROPERTYEDITORUI_API FPropertyUIMetadata : public FTableRowBase
{
    GENERATED_BODY()

public:
    /** The category this property belongs to (e.g., "Lens", "Depth of Field"). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI Metadata")
    FString Category;

    /** The user-facing display name for this property. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI Metadata")
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI Metadata")
    float DefaultValue = 0.0f;

    /** The minimum value for the slider. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI Metadata")
    float ClampMin = 0.0f;

    /** The maximum value for the slider. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI Metadata")
    float ClampMax = 100.0f;
};