/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-06-16
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "ToolController.h"

#include <GameLib/Vectors.h>

#include <cassert>


ToolController::ToolController(
    ToolType initialToolType,
    wxFrame * parentFrame,
    std::shared_ptr<GameController> gameController,
    ResourceLoader & resourceLoader)
    : mInputState()
    , mCurrentTool(nullptr)
    , mAllTools()
    , mParentFrame(parentFrame)
    , mMoveCursor()
    , mGameController(gameController)
{
    //
    // Initialize all tools
    //

    mAllTools.emplace_back(
        std::make_unique<SmashTool>(
            parentFrame,
            gameController,
            resourceLoader));

    mAllTools.emplace_back(
        std::make_unique<SawTool>(
            parentFrame,
            gameController,
            resourceLoader));

    mAllTools.emplace_back(
        std::make_unique<GrabTool>(
            parentFrame,
            gameController,
            resourceLoader));

    mAllTools.emplace_back(
        std::make_unique<SwirlTool>(
            parentFrame,
            gameController,
            resourceLoader));

    mAllTools.emplace_back(
        std::make_unique<PinTool>(
            parentFrame,
            gameController,
            resourceLoader));

    mAllTools.emplace_back(
        std::make_unique<TimerBombTool>(
            parentFrame,
            gameController,
            resourceLoader));

    mAllTools.emplace_back(
        std::make_unique<RCBombTool>(
            parentFrame,
            gameController,
            resourceLoader));

    // Prepare own cursor(s)
    mMoveCursor = MakeCursor("move_cursor", 15, 15, resourceLoader);

    // Set current tool
    this->SetTool(initialToolType);
}

void ToolController::OnMouseMove(
    int x,
    int y)
{
    // Update input state
    mInputState.PreviousMouseX = mInputState.MouseX;
    mInputState.PreviousMouseY = mInputState.MouseY;
    mInputState.MouseX = x;
    mInputState.MouseY = y;
    
    // Perform action
    if (mInputState.IsRightMouseDown)
    {
        // Perform our move tool

        // Pan (opposite direction)
        vec2 screenOffset = vec2(mInputState.PreviousMouseX, mInputState.PreviousMouseY) - vec2(mInputState.MouseX, mInputState.MouseY);
        mGameController->PanImmediate(screenOffset);
    }
    else
    {
        // Perform current tool's action
        if (nullptr != mCurrentTool)
        {
            mCurrentTool->OnMouseMove(mInputState);
        }
    }
}

void ToolController::OnLeftMouseDown()
{
    // Update input state
    mInputState.IsLeftMouseDown = true;

    // Perform current tool's action
    if (nullptr != mCurrentTool)
    {
        mCurrentTool->OnLeftMouseDown(mInputState);
    }
}

void ToolController::OnLeftMouseUp()
{
    // Update input state
    mInputState.IsLeftMouseDown = false;

    // Perform current tool's action
    if (nullptr != mCurrentTool)
    {
        mCurrentTool->OnLeftMouseUp(mInputState);
    }
}

void ToolController::OnRightMouseDown()
{
    // Update input state
    mInputState.IsRightMouseDown = true;

    // Show our move cursor 
    mParentFrame->SetCursor(*mMoveCursor);
}

void ToolController::OnRightMouseUp()
{
    // Update input state
    mInputState.IsRightMouseDown = false;

    if (nullptr != mCurrentTool)
    {        
        // Show tool's cursor again, since we moved out of Move
        mCurrentTool->ShowCurrentCursor();
    }
}

void ToolController::OnShiftKeyDown()
{
    // Update input state
    mInputState.IsShiftKeyDown = true;

    // Perform current tool's action
    if (nullptr != mCurrentTool)
    {
        mCurrentTool->OnShiftKeyDown(mInputState);
    }
}

void ToolController::OnShiftKeyUp()
{
    // Update input state
    mInputState.IsShiftKeyDown = false;

    // Perform current tool's action
    if (nullptr != mCurrentTool)
    {
        mCurrentTool->OnShiftKeyUp(mInputState);
    }
}
