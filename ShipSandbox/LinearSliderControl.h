/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-06-15
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "SliderControl.h"

class LinearSliderControl final : public SliderControl
{
public:

    LinearSliderControl(
        wxWindow * parent,
        int width,
        int height,
        std::string const & label,
        std::function<void(float)> onValueChanged,
        float minValue,
        float maxValue,
        float currentValue);

protected:

    virtual float TickToValue(int tick) const override;

    virtual int ValueToTick(float value) const override;

private:

    float const mMinValue;
    float const mMaxValue;

    float mTickSize;
    float mValueOffset;
    float mValueAtTickZero; // Net of offset
    int mMaxTickValue;
    float mValueAtTickMax;  // Net of offset
};
