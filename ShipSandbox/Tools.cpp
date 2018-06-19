/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-06-17
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "Tools.h"

#include <GameLib/GameException.h>

#include <wx/bitmap.h>
#include <wx/image.h>

#include <cassert>

static constexpr int CursorStep = 30;
static constexpr int PowerBarThickness = 2;

std::vector<std::unique_ptr<wxCursor>> MakeCursors(
    std::string const & cursorName,
    int hotspotX,
    int hotspotY,
    ResourceLoader & resourceLoader)
{
    auto filepath = resourceLoader.GetCursorFilepath(cursorName);
    wxBitmap* bmp = new wxBitmap(filepath.string(), wxBITMAP_TYPE_PNG);
    if (nullptr == bmp)
    {
        throw GameException("Cannot load resource '" + filepath.string() + "'");
    }

    wxImage img = bmp->ConvertToImage();
    int const imageWidth = img.GetWidth();
    int const imageHeight = img.GetHeight();

    // Set hotspots
    img.SetOption(wxIMAGE_OPTION_CUR_HOTSPOT_X, hotspotX);
    img.SetOption(wxIMAGE_OPTION_CUR_HOTSPOT_Y, hotspotY);

    std::vector<std::unique_ptr<wxCursor>> cursors;

    // Create base
    cursors.emplace_back(std::make_unique<wxCursor>(img));

    // Create cursors for all the strengths we want to have on the power base
    static_assert(CursorStep > 0, "CursorStep is at least 1");
    static_assert(PowerBarThickness > 0, "PowerBarThickness is at least 1");

    unsigned char * data = img.GetData();
    unsigned char * alpha = img.GetAlpha();

    for (int c = 1; c <= CursorStep; ++c)
    {
        // Draw vertical line on the right, height = f(c),  0 < height <= imageHeight
        int powerHeight = static_cast<int>(floorf(
            static_cast<float>(imageHeight) * static_cast<float>(c) / static_cast<float>(CursorStep)
        ));

        // Start from top
        for (int y = imageHeight - powerHeight; y < imageHeight; ++y)
        {
            int index = imageWidth - PowerBarThickness - 1 + (imageWidth * y);

            // Thickness
            for (int t = 0; t < PowerBarThickness; ++t, ++index)
            {
                // Red:   ff3300
                // Green: 00ff00
                alpha[index] = 0xff;
                data[index * 3] = (c == CursorStep) ? 0x00 : 0xFF;
                data[index * 3 + 1] = (c == CursorStep) ? 0xFF : 0x33;
                data[index * 3 + 2] = (c == CursorStep) ? 0x00 : 0x00;
            }
        }

        cursors.emplace_back(std::make_unique<wxCursor>(img));
    }

    delete (bmp);

    return cursors;
}

std::unique_ptr<wxCursor> MakeCursor(
    std::string const & cursorName,
    int hotspotX,
    int hotspotY,
    ResourceLoader & resourceLoader)
{
    auto filepath = resourceLoader.GetCursorFilepath(cursorName);
    wxBitmap* bmp = new wxBitmap(filepath.string(), wxBITMAP_TYPE_PNG);
    if (nullptr == bmp)
    {
        throw GameException("Cannot load resource '" + filepath.string() + "'");
    }

    wxImage img = bmp->ConvertToImage();

    // Set hotspots
    img.SetOption(wxIMAGE_OPTION_CUR_HOTSPOT_X, hotspotX);
    img.SetOption(wxIMAGE_OPTION_CUR_HOTSPOT_Y, hotspotY);

    return std::make_unique<wxCursor>(img);
}

/////////////////////////////////////////////////////////////////////////
// One-Shot Tool
/////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////
// Continuous Tool
/////////////////////////////////////////////////////////////////////////

void ContinuousTool::Update(InputState const & inputState)
{
    // We apply the tool only if the left mouse button is down
    if (inputState.IsLeftMouseDown)
    {
        auto now = std::chrono::steady_clock::now();

        // Accumulate total time iff we haven't moved since last time
        if (mPreviousMouseX == inputState.MouseX
            && mPreviousMouseY == inputState.MouseY)
        {
            mCumulatedTime += std::chrono::duration_cast<std::chrono::microseconds>(now - mPreviousTimestamp);
        }
        else
        {
            // We've moved, don't accumulate but use time
            // that was built up so far
        }

        // Remember new position and timestamp
        mPreviousMouseX = inputState.MouseX;
        mPreviousMouseY = inputState.MouseY;
        mPreviousTimestamp = now;

        // Apply current tool
        ApplyTool(
            mCumulatedTime,
            inputState);
    }
}

////////////////////////////////////////////////////////////////////////
// Smash
////////////////////////////////////////////////////////////////////////

SmashTool::SmashTool(
    wxFrame * parentFrame,
    std::shared_ptr<GameController> gameController,
    ResourceLoader & resourceLoader)
    : ContinuousTool(
        ToolType::Smash,
        parentFrame,
        std::move(gameController))
    , mUpCursor(MakeCursor("smash_cursor_up", 6, 9, resourceLoader))
    , mDownCursors(MakeCursors("smash_cursor_down", 6, 9, resourceLoader))
{
}

void SmashTool::ApplyTool(
    std::chrono::microseconds const & cumulatedTime,
    InputState const & inputState)
{
    // Calculate radius multiplier
    // 0-500ms      = 1.0
    // 5000ms-+INF = 10.0

    static constexpr float MaxMultiplier = 10.0f;

    float millisecondsElapsed = static_cast<float>(
        std::chrono::duration_cast<std::chrono::milliseconds>(cumulatedTime).count());
    float radiusMultiplier = 1.0f + (MaxMultiplier - 1.0f) * std::min(1.0f, millisecondsElapsed / 5000.0f);

    // Modulate down cursor
    ModulateCursor(mDownCursors, radiusMultiplier, 1.0f, MaxMultiplier);

    // Destroy
    mGameController->DestroyAt(
        vec2f(inputState.MouseX, inputState.MouseY), 
        radiusMultiplier);
}

////////////////////////////////////////////////////////////////////////
// Grab
////////////////////////////////////////////////////////////////////////

GrabTool::GrabTool(
    wxFrame * parentFrame,
    std::shared_ptr<GameController> gameController,
    ResourceLoader & resourceLoader)
    : ContinuousTool(
        ToolType::Grab,
        parentFrame,
        std::move(gameController))
    , mUpPlusCursor(MakeCursor("drag_cursor_up_plus", 15, 15, resourceLoader))
    , mUpMinusCursor(MakeCursor("drag_cursor_up_minus", 15, 15, resourceLoader))
    , mDownPlusCursors(MakeCursors("drag_cursor_down_plus", 15, 15, resourceLoader))
    , mDownMinusCursors(MakeCursors("drag_cursor_down_minus", 15, 15, resourceLoader))
{
}

void GrabTool::ApplyTool(
    std::chrono::microseconds const & cumulatedTime,
    InputState const & inputState)
{
    // Calculate strength multiplier
    // 0-500ms      = 1.0
    // 5000ms-+INF = 20.0

    static constexpr float MaxMultiplier = 20.0f;

    float millisecondsElapsed = static_cast<float>(
        std::chrono::duration_cast<std::chrono::milliseconds>(cumulatedTime).count());
    float strengthMultiplier = 1.0f + (MaxMultiplier - 1.0f) * std::min(1.0f, millisecondsElapsed / 5000.0f);

    // Modulate down cursor
    ModulateCursor(
        inputState.IsShiftKeyDown ? mDownMinusCursors : mDownPlusCursors, 
        strengthMultiplier, 
        1.0f, 
        MaxMultiplier);

    // Draw
    mGameController->DrawTo(
        vec2f(inputState.MouseX, inputState.MouseY), 
        inputState.IsShiftKeyDown
            ? -strengthMultiplier
            : strengthMultiplier);
}

////////////////////////////////////////////////////////////////////////
// Swirl
////////////////////////////////////////////////////////////////////////

SwirlTool::SwirlTool(
    wxFrame * parentFrame,
    std::shared_ptr<GameController> gameController,
    ResourceLoader & resourceLoader)
    : ContinuousTool(
        ToolType::Grab,
        parentFrame,
        std::move(gameController))
    , mUpPlusCursor(MakeCursor("swirl_cursor_up_cw", 15, 15, resourceLoader))
    , mUpMinusCursor(MakeCursor("swirl_cursor_up_ccw", 15, 15, resourceLoader))
    , mDownPlusCursors(MakeCursors("swirl_cursor_down_cw", 15, 15, resourceLoader))
    , mDownMinusCursors(MakeCursors("swirl_cursor_down_ccw", 15, 15, resourceLoader))
{
}

void SwirlTool::ApplyTool(
    std::chrono::microseconds const & cumulatedTime,
    InputState const & inputState)
{
    // Calculate strength multiplier
    // 0-500ms      = 1.0
    // 5000ms-+INF = 20.0

    static constexpr float MaxMultiplier = 20.0f;

    float millisecondsElapsed = static_cast<float>(
        std::chrono::duration_cast<std::chrono::milliseconds>(cumulatedTime).count());
    float strengthMultiplier = 1.0f + (MaxMultiplier - 1.0f) * std::min(1.0f, millisecondsElapsed / 5000.0f);

    // Modulate down cursor
    ModulateCursor(
        inputState.IsShiftKeyDown ? mDownMinusCursors : mDownPlusCursors,
        strengthMultiplier,
        1.0f,
        MaxMultiplier);

    // Draw
    mGameController->SwirlAt(
        vec2f(inputState.MouseX, inputState.MouseY),
        inputState.IsShiftKeyDown
            ? -strengthMultiplier
            : strengthMultiplier);
}

////////////////////////////////////////////////////////////////////////
// Pin
////////////////////////////////////////////////////////////////////////

PinTool::PinTool(
    wxFrame * parentFrame,
    std::shared_ptr<GameController> gameController,
    ResourceLoader & resourceLoader)
    : OneShotTool(
        ToolType::Pin,
        parentFrame,
        std::move(gameController))
    , mCursor(MakeCursor("pin_cursor", 4, 27, resourceLoader))
{
}

////////////////////////////////////////////////////////////////////////
// TimerBomb
////////////////////////////////////////////////////////////////////////

TimerBombTool::TimerBombTool(
    wxFrame * parentFrame,
    std::shared_ptr<GameController> gameController,
    ResourceLoader & resourceLoader)
    : OneShotTool(
        ToolType::TimerBomb,
        parentFrame,
        std::move(gameController))
    , mCursor(MakeCursor("timer_bomb_cursor", 16, 19, resourceLoader))
{
}

////////////////////////////////////////////////////////////////////////
// RCBomb
////////////////////////////////////////////////////////////////////////

RCBombTool::RCBombTool(
    wxFrame * parentFrame,
    std::shared_ptr<GameController> gameController,
    ResourceLoader & resourceLoader)
    : OneShotTool(
        ToolType::RCBomb,
        parentFrame,
        std::move(gameController))
    , mCursor(MakeCursor("rc_bomb_cursor", 16, 21, resourceLoader))
{
}
