#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/IUserObjectListEntry.h"
#include "FilterCategoryEntry.generated.h"

class UTextBlock;
class UFilterNodeData;
class UCheckBox;

// 声明两个委托，用于通知主面板鼠标进入和离开的事件
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCategoryHovered, UFilterNodeData*, CategoryData, UUserWidget*, HoveredEntry);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCategoryUnhovered);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCategoryToggled, UFilterNodeData*, CategoryData, bool, bIsChecked);

UCLASS()
class AUTOPROPERTYEDITORUI_API UFilterCategoryEntry : public UUserWidget, public IUserObjectListEntry
{
    GENERATED_BODY()

public:
    // 暴露给外部（主面板）的委托
    UPROPERTY(BlueprintAssignable)
    FOnCategoryHovered OnHovered;

    UPROPERTY(BlueprintAssignable)
    FOnCategoryUnhovered OnUnhovered;

    UPROPERTY(BlueprintAssignable)
    FOnCategoryToggled OnToggled;

    /**
     * 一个公开的函数，用于控制此条目的高亮状态。
     * @param bIsHighlighted true表示设置为高亮状态，false表示恢复普通状态。
     */
    UFUNCTION(BlueprintImplementableEvent, Category = "Appearance")
    void SetHighlight(bool bIsHighlighted);

    UFUNCTION()
    void RefreshState(UObject* ListItemObject);

    /** 一个帮助函数，用于获取此条目所代表的数据 */
    UFilterNodeData* GetLinkedData() const { return LinkedData; }

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
    TObjectPtr<UFilterNodeData> LinkedData;

    UFUNCTION()
    void HandleCheckStateChanged(bool bIsChecked);
};