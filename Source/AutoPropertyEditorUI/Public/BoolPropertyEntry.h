#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/IUserObjectListEntry.h"
#include "BoolPropertyEntry.generated.h"

// 前向声明，避免在头文件中引入完整的头文件，加快编译速度
class UCheckBox;
class UTextBlock;
class UFilterNodeData;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCheckBoxEntryHovered);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCheckBoxEntryUnhovered);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEntryClicked, UFilterNodeData*, ClickedData);

/**
 * TreeView中用于显示筛选条件的条目控件。
 * 它可以代表一个分类（仅显示文本），或一个可勾选的属性（显示文本和复选框）。
 * 它实现了IUserObjectListEntry接口，使其能够被TreeView用作条目控件。
 */
UCLASS()
class AUTOPROPERTYEDITORUI_API UBoolPropertyEntry : public UUserWidget, public IUserObjectListEntry
{
    GENERATED_BODY()

public:
    /** 暴露给外部（主面板）的悬浮委托 */
    UPROPERTY(BlueprintAssignable)
    FOnCheckBoxEntryHovered OnHovered;

    /** 暴露给外部（主面板）的离开委托 */
    UPROPERTY(BlueprintAssignable)
    FOnCheckBoxEntryUnhovered OnUnhovered;

    UPROPERTY(BlueprintAssignable)
    FOnEntryClicked OnClicked;

    /**
     * 一个公开的、可在蓝图中实现的函数，用于控制此条目的高亮状态。
     * @param bIsHighlighted true表示设置为高亮状态，false表示恢复普通状态。
     */
    UFUNCTION(BlueprintImplementableEvent, Category = "Appearance")
    void SetHighlight(bool bIsHighlighted);

    UFUNCTION()
    void RefreshState(UObject* ListItemObject);

    UFUNCTION()
    void SetCheckBoxVisibility(bool bVis);

protected:
    /**
     * 【核心函数】当TreeView需要此控件显示某个数据对象时，会调用此函数。
     * 这是从IUserObjectListEntry接口继承而来的。
     * @param ListItemObject 传递给此控件的数据对象，我们期望它是一个UFilterNodeData。
     */
    virtual void NativeOnListItemObjectSet(UObject* ListItemObject) override;

    /** 在控件初始化时，绑定内部控件的事件。 */
    virtual void NativeOnInitialized() override;

    virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
    virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;
    virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

    // --- UMG控件绑定 ---
    // 这些变量名需要与您在WBP_CheckBoxEntry蓝图中的控件名完全一致。

    /** 用于显示属性或分类名称的文本框。 */
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> TitleText;

    /** 用于显示和修改bOverride_状态的复选框。 */
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UCheckBox> ValueCheckBox;

private:
    /**
     * 当ValueCheckBox的状态（勾选/未勾选）发生改变时，此函数会被调用。
     * @param bIsChecked 复选框新的状态。
     */
    UFUNCTION()
    void HandleCheckStateChanged(bool bIsChecked);

    /**
     * 保存一个对当前绑定的数据对象的引用。
     * 这样，当事件（如点击）发生时，我们知道应该操作哪个数据。
     * 标记为Transient，因为它是一个临时的运行时状态，不应被保存。
     */
    UPROPERTY(Transient)
    TObjectPtr<UFilterNodeData> LinkedData;
};