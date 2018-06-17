/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-06-17
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <GameLib/GameController.h>
#include <GameLib/ResourceLoader.h>

#include <wx/cursor.h>
#include <wx/frame.h>

#include <chrono>
#include <memory>
#include <vector>

std::vector<std::unique_ptr<wxCursor>> MakeCursors(
    std::string const & cursorName,
    int hotspotX,
    int hotspotY,
    ResourceLoader & resourceLoader);

std::unique_ptr<wxCursor> MakeCursor(
    std::string const & cursorName,
    int hotspotX,
    int hotspotY,
    ResourceLoader & resourceLoader);

enum class ToolType
{
    Smash = 0,
    Grab = 1,
    Pin = 2,
    TimerBomb = 3,
    RCBomb = 4
};

struct InputState
{
    bool IsLeftMouseDown;
    bool IsRightMouseDown;
    bool IsShiftKeyDown;
    int MouseX;
    int MouseY;
    int PreviousMouseX;
    int PreviousMouseY;

    InputState()
        : IsLeftMouseDown(false)
        , IsRightMouseDown(false)
        , IsShiftKeyDown(false)
        , MouseX(0)
        , MouseY(0)
        , PreviousMouseX(0)
        , PreviousMouseY(0)
    {
    }
};

/*
 * Base abstract class of all tools.
 */ 
class Tool
{
public:

    virtual ~Tool() {}

    ToolType GetToolType() const { return mToolType; }

    virtual void Initialize(InputState const & inputState) = 0;

    virtual void Update(InputState const & inputState) = 0;

    virtual void OnMouseMove(InputState const & inputState) = 0;
    virtual void OnLeftMouseDown(InputState const & inputState) = 0;
    virtual void OnLeftMouseUp(InputState const & inputState) = 0;
    virtual void OnShiftKeyDown(InputState const & inputState) = 0;
    virtual void OnShiftKeyUp(InputState const & inputState) = 0;

    virtual void ShowCurrentCursor() = 0;

protected:

    Tool(
        ToolType toolType,
        wxFrame * parentFrame,
        std::shared_ptr<GameController> gameController)
        : mToolType(toolType)
        , mParentFrame(parentFrame)
        , mGameController(gameController)
    {}

    wxFrame * const mParentFrame;
    std::shared_ptr<GameController> const mGameController;

private:

    ToolType const mToolType;
};

/*
 * Base class of tools that shoot once.
 */
class OneShotTool : public Tool
{
public:

    virtual ~OneShotTool() {}

    virtual void Update(InputState const & /*inputState*/) override {}

    virtual void OnMouseMove(InputState const & /*inputState*/) override {}
    virtual void OnShiftKeyDown(InputState const & /*inputState*/) override {}
    virtual void OnShiftKeyUp(InputState const & /*inputState*/) override {}

    virtual void ShowCurrentCursor() override
    {
        assert(nullptr != mParentFrame);
        assert(nullptr != mCurrentCursor);

        mParentFrame->SetCursor(*mCurrentCursor);
    }

protected:

    OneShotTool(
        ToolType toolType,
        wxFrame * parentFrame,
        std::shared_ptr<GameController> gameController)
        : Tool(
            toolType,
            parentFrame,
            std::move(gameController))
        , mCurrentCursor(nullptr)
    {}

protected:

    wxCursor * mCurrentCursor;
};

/*
 * Base class of tools that apply continuously.
 */
class ContinuousTool : public Tool
{
public:

    virtual ~ContinuousTool() {}

    virtual void Initialize(InputState const & /*inputState*/) override
    {
        // Reset current cursor
        mCurrentCursor = mCursors[0].get();
    }

    virtual void Update(InputState const & inputState) override;

    virtual void OnMouseMove(InputState const & /*inputState*/) override {}
    
    virtual void OnLeftMouseDown(InputState const & inputState) override
    {
        // Initialize the continuous tool state
        mPreviousMouseX = inputState.MouseX;
        mPreviousMouseY = inputState.MouseY;
        mPreviousTimestamp = std::chrono::steady_clock::now();
        mCumulatedTime = std::chrono::microseconds(0);

        // Set current cursor
        mCurrentCursor = mCursors[0].get();
        ShowCurrentCursor();
    }

    virtual void OnLeftMouseUp(InputState const & /*inputState*/) override
    {
        // Set current cursor
        mCurrentCursor = mCursors[0].get();
        ShowCurrentCursor();
    }

    virtual void OnShiftKeyDown(InputState const & /*inputState*/) override {}
    virtual void OnShiftKeyUp(InputState const & /*inputState*/) override {}

    virtual void ShowCurrentCursor() override
    {
        assert(nullptr != mParentFrame);
        assert(nullptr != mCurrentCursor);

        mParentFrame->SetCursor(*mCurrentCursor);
    }

protected:

    ContinuousTool(
        ToolType toolType,
        std::vector<std::unique_ptr<wxCursor>> cursors,
        wxFrame * parentFrame,
        std::shared_ptr<GameController> gameController)
        : Tool(
            toolType,
            parentFrame,
            std::move(gameController))
        , mCursors(std::move(cursors))
        , mCurrentCursor(nullptr)
        , mPreviousMouseX(-1)
        , mPreviousMouseY(-1)
        , mPreviousTimestamp(std::chrono::steady_clock::now())
        , mCumulatedTime(0)
    {}

    void ModulateCursor(
        float strength,
        float minStrength,
        float maxStrength)
    {
        // Calculate cursor index (cursor 0 is the base, we don't use it here)
        size_t const numberOfCursors = (mCursors.size() - 1);
        size_t cursorIndex = 1u + static_cast<size_t>(
            floorf(
                (strength - minStrength) / (maxStrength - minStrength) * static_cast<float>(numberOfCursors - 1)));

        // Set cursor
        mCurrentCursor = mCursors[cursorIndex].get();

        // Display cursor
        ShowCurrentCursor();
    }

    virtual void ApplyTool(
        std::chrono::microseconds const & cumulatedTime,
        InputState const & inputState) = 0;

private:

    // Cursor 0 is base; cursor 1 up to size-1 are strength-based
    std::vector<std::unique_ptr<wxCursor>> mCursors;

    // The currently-selected cursor that will be shown
    wxCursor * mCurrentCursor;

    //
    // Tool state
    //

    // Previous mouse position and time when we looked at it
    int mPreviousMouseX;
    int mPreviousMouseY;
    std::chrono::steady_clock::time_point mPreviousTimestamp;

    // The total accumulated press time - the proxy for the strength of the tool
    std::chrono::microseconds mCumulatedTime;
};

//////////////////////////////////////////////////////////////////////////////////////////
// Tools
//////////////////////////////////////////////////////////////////////////////////////////

class SmashTool final : public ContinuousTool
{
public:

    SmashTool(
        wxFrame * parentFrame,
        std::shared_ptr<GameController> gameController,
        ResourceLoader & resourceLoader);

protected:

    virtual void ApplyTool(
        std::chrono::microseconds const & cumulatedTime,
        InputState const & inputState) override;
};

class GrabTool final : public ContinuousTool
{
public:

    GrabTool(
        wxFrame * parentFrame,
        std::shared_ptr<GameController> gameController,
        ResourceLoader & resourceLoader);

protected:

    virtual void ApplyTool(
        std::chrono::microseconds const & cumulatedTime,
        InputState const & inputState) override;
};

class PinTool final : public OneShotTool
{
public:

    PinTool(
        wxFrame * parentFrame,
        std::shared_ptr<GameController> gameController,
        ResourceLoader & resourceLoader);

public:

    virtual void Initialize(InputState const & inputState) override
    {
        // Reset cursor
        assert(!!mUpCursor && !!mDownCursor);
        mCurrentCursor = inputState.IsLeftMouseDown ? mDownCursor.get() : mUpCursor.get();
    }

    virtual void OnLeftMouseDown(InputState const & inputState) override
    {
        // Switch cursor to down
        mCurrentCursor = mDownCursor.get();
        ShowCurrentCursor();

        // Toggle pin
        mGameController->TogglePinAt(vec2f(inputState.MouseX, inputState.MouseY));
    }

    virtual void OnLeftMouseUp(InputState const & /*inputState*/) override
    {
        // Switch cursor to up
        mCurrentCursor = mUpCursor.get();
        ShowCurrentCursor();
    }

private:

    std::unique_ptr<wxCursor> mUpCursor;
    std::unique_ptr<wxCursor> mDownCursor;
};

class TimerBombTool final : public OneShotTool
{
public:

    TimerBombTool(
        wxFrame * parentFrame,
        std::shared_ptr<GameController> gameController,
        ResourceLoader & resourceLoader);

public:

    virtual void Initialize(InputState const & /*inputState*/) override
    {
        // Reset cursor
        assert(!!mCursor);
        mCurrentCursor = mCursor.get();
    }

    virtual void OnLeftMouseDown(InputState const & inputState) override
    {
        // Toggle bomb
        mGameController->ToggleTimerBombAt(vec2f(inputState.MouseX, inputState.MouseY));
    }

    virtual void OnLeftMouseUp(InputState const & /*inputState*/) override {}

private:

    std::unique_ptr<wxCursor> mCursor;
};

class RCBombTool final : public OneShotTool
{
public:

    RCBombTool(
        wxFrame * parentFrame,
        std::shared_ptr<GameController> gameController,
        ResourceLoader & resourceLoader);

public:

    virtual void Initialize(InputState const & /*inputState*/) override
    {
        // Reset cursor
        assert(!!mCursor);
        mCurrentCursor = mCursor.get();
    }

    virtual void OnLeftMouseDown(InputState const & inputState) override
    {
        // Toggle bomb
        mGameController->ToggleRCBombAt(vec2f(inputState.MouseX, inputState.MouseY));
    }

    virtual void OnLeftMouseUp(InputState const & /*inputState*/) override {}

private:

    std::unique_ptr<wxCursor> mCursor;
};