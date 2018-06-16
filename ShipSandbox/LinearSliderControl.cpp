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
    //
    // Calculate number of ticks and tick size, i.e. value delta for each tick, as follows:
    //
    // NumberOfTicks * TickSize = Max - Min
    //
    // With: TickSize = 1/2**n
    //
    
    // Start with an approximate number of ticks
    float n = floorf(logf(100.0f / (maxValue - minValue)) / logf(2.0f));
    mTickSize = 1.0 / powf(2.0, n);

    // Now calculate the real number of ticks
    float numberOfTicks = ceilf((maxValue - minValue) / mTickSize);

    // Re-adjust min: calc min at tick 0 (exclusive of offset), and offset to add to slider's value
    mValueOffset = floorf(minValue / mTickSize) * mTickSize;
    mValueAtTickZero = minValue - mValueOffset;
    assert(mValueAtTickZero < mTickSize);

    // Store maximum tick value and maximum value (exclusive of offset) at that tick
    float theoreticalMaxValue = mValueOffset + numberOfTicks * mTickSize;
    assert(theoreticalMaxValue - maxValue < mTickSize);
    mMaxTickValue = static_cast<int>(numberOfTicks);
    mValueAtTickMax = maxValue;

    this->Initialize(
        static_cast<int>(numberOfTicks),
        currentValue);
}

float LinearSliderControl::TickToValue(int tick) const
{
    float sliderValue;

    if (tick == 0)
    {
        sliderValue = mValueAtTickZero;
    }
    else if (tick == mMaxTickValue)
    {
        sliderValue = mValueAtTickMax;
    }
    else
    {
        sliderValue = mTickSize * static_cast<float>(tick);
    }

    return mValueOffset + sliderValue;
}

int LinearSliderControl::ValueToTick(float value) const
{
    value -= mValueOffset;

    if (value <= mValueAtTickZero)
    {
        return 0;
    }
    else if (value >= mValueAtTickMax)
    {
        return mMaxTickValue;
    }
    else
    {
        return static_cast<int>(floorf((value - mValueAtTickZero) / mTickSize));
    }
}
