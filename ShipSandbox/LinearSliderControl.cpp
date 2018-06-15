/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-06-15
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "LinearSliderControl.h"

#include <cassert>

LinearSliderControl::LinearSliderControl(
    wxWindow * parent,
    int width,
    int height,
    std::string const & label,
    std::function<void(float)> onValueChanged,
    float minValue,
    float maxValue,
    float currentValue)
    : SliderControl(
        parent,
        width,
        height,
        label,
        std::move(onValueChanged))
    , mMinValue(minValue)
    , mMaxValue(maxValue)
{
    this->Initialize(
        100, // TODO
        currentValue);
}

float LinearSliderControl::TickToValue(int tick) const
{
    // TODO
    return mMinValue + (static_cast<float>(tick) / static_cast<float>(100)) * (mMaxValue - mMinValue);
}

int LinearSliderControl::ValueToTick(float value) const
{
    // TODO
    return static_cast<int>(roundf((value - mMinValue) / (mMaxValue - mMinValue) * static_cast<float>(100)));
}
