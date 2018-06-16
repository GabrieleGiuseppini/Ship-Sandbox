/***************************************************************************************
* Original Author:		Luke Wren (wren6991@gmail.com)
* Created:				2013-04-30
* Modified By:			Gabriele Giuseppini
* Copyright:			Luke Wren (http://github.com/Wren6991),
*						Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "SliderControl.h"

#include <GameLib/GameController.h>

#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/dialog.h>
#include <wx/radiobox.h>

#include <memory>

class SettingsDialog : public wxDialog
{
public:

    SettingsDialog(
        wxWindow * parent,
        std::shared_ptr<GameController> gameController);

    virtual ~SettingsDialog();

    void Open();

private:

    void OnQuickWaterFixCheckBoxClick(wxCommandEvent & event);
    void OnShipRenderModeRadioBox(wxCommandEvent & event);
    void OnShowStressCheckBoxClick(wxCommandEvent & event);

    void OnOkButton(wxCommandEvent & event);
    void OnApplyButton(wxCommandEvent & event);

private:

    // Controls

    std::unique_ptr<SliderControl> mStiffnessSlider;
    std::unique_ptr<SliderControl> mStrengthSlider;
    std::unique_ptr<SliderControl> mBuoyancySlider;
    std::unique_ptr<SliderControl> mWaterPressureSlider;
    std::unique_ptr<SliderControl> mWaveHeightSlider;
    std::unique_ptr<SliderControl> mWaterTransparencySlider;
    std::unique_ptr<SliderControl> mLightDiffusionSlider;
    std::unique_ptr<SliderControl> mSeaDepthSlider;
    std::unique_ptr<SliderControl> mDestroyRadiusSlider;
    std::unique_ptr<SliderControl> mBombBlastRadiusSlider;

    wxCheckBox * mQuickWaterFixCheckBox;
    wxRadioBox * mShipRenderModeRadioBox;
	wxCheckBox* mShowStressCheckBox;

	wxButton * mOkButton;
	wxButton * mCancelButton;
	wxButton * mApplyButton;

private:

    void ReadSettings();

	void ApplySettings();

private:

	wxWindow * const mParent;
	std::shared_ptr<GameController> mGameController;
};
