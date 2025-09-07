#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/IUserObjectListEntry.h" 
#include "NumericPropertyEntry.generated.h"

class USlider;
class UTextBlock;
class UButton;

UCLASS()
class AUTOPROPERTYEDITORUI_API UNumericPropertyEntry : public UUserWidget, public IUserObjectListEntry
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintImplementableEvent, Category = "Appearance")
    void SetRowStyle(bool bIsEvenRow);

protected:
    // 来自 IUserObjectListEntry 的核心函数
    virtual void NativeOnListItemObjectSet(UObject* ListItemObject) override;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> TitleText;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> ValueText;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> BT_Reset;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<USlider> ValueSlider;

    virtual void NativeOnInitialized() override;

private:
    UFUNCTION()
    void HandleSliderValueChanged(float InValue);

    UFUNCTION()
    void HandleResetClicked();

    // 保存对数据对象的引用，以便在事件发生时使用
    UPROPERTY(Transient) // 避免被垃圾回收
    TObjectPtr<class UNumericPropertyData> LinkedData;
};