#include "MenuInputProcessor.h"
#include "DetailsPanelGenerator.h" 
#include "Framework/Application/SlateApplication.h"
#include "Components/Border.h"
#include "Components/Button.h" 
#include "Widgets/SWidget.h"
#include "Input/Events.h"

FMenuInputProcessor::FMenuInputProcessor(UDetailsPanelGenerator* InOwnerWidget)
    : OwnerWidget(InOwnerWidget)
{
}

void FMenuInputProcessor::Tick(const float DeltaTime, FSlateApplication& SlateApp, TSharedRef<ICursor> Cursor)
{
    // We don't need to do anything in Tick.
}

bool FMenuInputProcessor::HandleMouseButtonDownEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent)
{
    return false;
}