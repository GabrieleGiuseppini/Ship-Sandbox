/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-05-13
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "Buffer.h"
#include "ElementContainer.h"

#include <cassert>
#include <memory>

namespace Physics
{

class ElectricalElements : public ElementContainer
{
public:

    ElectricalElements(ElementCount elementCount)
        : ElementContainer(elementCount)
        , mIsDeletedBuffer(elementCount)
        , mElectricalElementBuffer(elementCount)        
    {
    }

    ElectricalElements(ElectricalElements && other) = default;

    void Add(std::unique_ptr<ElectricalElement> electricalElement);

    void Destroy(ElementIndex electricalElementIndex);

public:

    inline bool IsDeleted(ElementIndex electricalElementIndex) const
    {
        assert(electricalElementIndex < mElementCount);

        return mIsDeletedBuffer[electricalElementIndex];
    }

    inline ElectricalElement * GetElectricalElement(ElementIndex electricalElementIndex) const
    {
        assert(electricalElementIndex < mElementCount);

        return mElectricalElementBuffer[electricalElementIndex].get();
    }

private:

private:

    // Deletion
    Buffer<bool> mIsDeletedBuffer;

    // Endpoints
    Buffer<std::unique_ptr<ElectricalElement>> mElectricalElementBuffer;
};

}
