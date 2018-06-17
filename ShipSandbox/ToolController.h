/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-06-16
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "Tools.h"

#include <GameLib/GameController.h>
#include <GameLib/ResourceLoader.h>

#include <wx/frame.h>

#include <memory>
#include <vector>

class ToolController
{
public:

    ToolController(
        ToolType initialToolType,
        wxFrame * parentFrame,
        std::shared_ptr<GameController> gameController,
        ResourceLoader & resourceLoader);

    void SetTool(ToolType toolType)
    {
        assert(static_cast<size_t>(toolType) < mAllTools.size());

        // Switch tool
        mCurrentTool = mAllTools[static_cast<size_t>(toolType)].get();
        mCurrentTool->Initialize(mInputState);

        // Show its cursor
        ShowCurrentCursor();
    }

    void ShowCurrentCursor()
    {
        if (nullptr != mCurrentTool)
        {
            mCurrentTool->ShowCurrentCursor();
        }
    }

    void Update()
    {
        if (nullptr != mCurrentTool)
        {
            mCurrentTool->Update(mInputState);
        }
    }


    //
    // External event handlers
    //

    void OnMouseMove(
        int x,
        int y);

    void OnLeftMouseDown();

    void OnLeftMouseUp();

    void OnRightMouseDown();

    void OnRightMouseUp();

    void OnShiftKeyDown();

    void OnShiftKeyUp();

private:

    // Input state
    InputState mInputState;

    // Tool state
    Tool * mCurrentTool;
    std::vector<std::unique_ptr<Tool>> mAllTools; // Indexed by enum

private:
    
    wxFrame * const mParentFrame;
    std::unique_ptr<wxCursor> mMoveCursor;
    std::shared_ptr<GameController> const mGameController;
};
