#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/IUserObjectListEntry.h"
#include "FilterCategoryEntry.generated.h"

class UTextBlock;
class UBoolPropertyData;
class UCheckBox;
class UFilterCategoryData;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnFilterCategoryHovered, UFilterCategoryData*, FilterCategoryData, UUserWidget*, HoveredEntry);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnFilterCategoryUnhovered);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnFilterCategoryToggled, UFilterCategoryData*, FilterCategoryData, bool, bIsChecked);

UCLASS()
class AUTOPROPERTYEDITORUI_API UFilterCategoryEntry : public UUserWidget, public IUserObjectListEntry
{
    GENERATED_BODY()

public:
    
    UPROPERTY(BlueprintAssignable) FOnFilterCategoryHovered OnHovered;
    UPROPERTY(BlueprintAssignable) FOnFilterCategoryUnhovered OnUnhovered;
    UPROPERTY(BlueprintAssignable) FOnFilterCategoryToggled OnToggled;

    /**
     * 一个公开的函数，用于控制此条目的高亮状态。
     * @param bIsHighlighted true表示设置为高亮状态，false表示恢复普通状态。
     */
    UFUNCTION(BlueprintImplementableEvent, Category = "Appearance")
    void SetHighlight(bool bIsHighlighted);

    UFUNCTION()
    void RefreshState(UObject* ListItemObject);

protected:
    // 实现 IUserObjectListEntry 的核心函数
    virtual void NativeOnListItemObjectSet(UObject* ListItemObject) override;

    virtual void NativeOnInitialized() override;

    // 覆盖鼠标事件函数
    virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
    virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;

    // 绑定到UMG蓝图中的文本控件
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> CategoryNameText;

    UPROPERTY(meta = (BindWidget)) 
    TObjectPtr<UCheckBox> MainCheckBox;

private:
    // 保存对此条目所代表的数据的引用
    UPROPERTY(Transient)
    TObjectPtr<UFilterCategoryData> LinkedData;

    UFUNCTION()
    void HandleCheckStateChanged(bool bIsChecked);
};