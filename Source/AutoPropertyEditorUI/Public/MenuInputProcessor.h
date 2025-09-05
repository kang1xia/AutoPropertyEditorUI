#pragma once

#include "CoreMinimal.h"
#include "Framework/Application/IInputProcessor.h"

class UDetailsPanelGenerator;

/**
 * A custom input processor to detect clicks outside of a specific widget.
 */
class FMenuInputProcessor : public IInputProcessor
{
public:
    // We pass the widget we want to monitor to the constructor.
    FMenuInputProcessor(UDetailsPanelGenerator* InOwnerWidget);

    // IInputProcessor interface
    virtual void Tick(const float DeltaTime, FSlateApplication& SlateApp, TSharedRef<ICursor> Cursor) override;
    virtual bool HandleMouseButtonDownEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent) override;

    // We don't need to handle these other events
    virtual bool HandleKeyDownEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent) override { return false; }
    virtual bool HandleKeyUpEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent) override { return false; }
    virtual bool HandleAnalogInputEvent(FSlateApplication& SlateApp, const FAnalogInputEvent& InAnalogInputEvent) override { return false; }
    virtual bool HandleMouseMoveEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent) override { return false; }
    virtual bool HandleMouseButtonUpEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent) override { return false; }
    virtual bool HandleMouseButtonDoubleClickEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent) override { return false; }

private:
    // A weak pointer is crucial here to prevent dangling pointers if the widget is destroyed.
    TWeakObjectPtr<UDetailsPanelGenerator> OwnerWidget;
};