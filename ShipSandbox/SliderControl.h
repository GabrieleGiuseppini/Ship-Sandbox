/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-06-15
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <wx/slider.h>
#include <wx/textctrl.h>
#include <wx/wx.h>

#include <functional>
#include <memory>
#include <string>

/*
 * This control incorporates a slider and a textbox showing the 
 * current mapped float value of the slider.
 *
 * This is the base class of concrete implementations, each
 * providing a different logic for mapping slider positions
 * to float vaues.
 */
class SliderControl : public wxPanel
{
public:

    SliderControl(
        wxWindow * parent,
        int width,
        int height,
        std::string const & label,
        std::function<void(float)> onValueChanged);
    
    virtual ~SliderControl();

    void Initialize(
        int numberOfTicks,
        float currentValue);

    float GetValue() const
    {
        return TickToValue(mSlider->GetValue());
    }

protected:

    virtual float TickToValue(int tick) const = 0;

    virtual int ValueToTick(float value) const = 0;

    void SetTick(int tick)
    {
        mSlider->SetValue(tick);
    }

private:

    void OnSliderScroll(wxScrollEvent & event);

private:

    std::unique_ptr<wxSlider> mSlider;
    std::unique_ptr<wxTextCtrl> mTextCtrl;

    std::function<void(float)> const mOnValueChanged;
};
