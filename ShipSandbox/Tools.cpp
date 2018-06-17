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
        MakeCursors("smash_cursor", 1, 16, resourceLoader),
        parentFrame,
        std::move(gameController))
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

    // Modulate cursor
    ModulateCursor(radiusMultiplier, 1.0f, MaxMultiplier);

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
        MakeCursors("drag_cursor", 15, 15, resourceLoader),
        parentFrame,
        std::move(gameController))
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

    // Modulate cursor
    ModulateCursor(strengthMultiplier, 1.0f, MaxMultiplier);

    // Draw
    mGameController->DrawTo(
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
{
    // Load cursors
    mUpCursor = MakeCursor("pin_up_cursor", 4, 27, resourceLoader);
    // TODO: have different one
    mDownCursor = MakeCursor("pin_up_cursor", 4, 27, resourceLoader);
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
{
    // Load cursor
    mCursor = MakeCursor("timer_bomb_cursor", 16, 19, resourceLoader);
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
{
    // Load cursor
    mCursor = MakeCursor("rc_bomb_cursor", 16, 21, resourceLoader);
}
