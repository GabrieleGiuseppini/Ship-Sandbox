/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-05-13
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "Buffer.h"
#include "ElementContainer.h"
#include "FixedSizeVector.h"
#include "GameParameters.h"
#include "Material.h"
#include "RenderContext.h"

#include <cassert>

namespace Physics
{

class Triangles : public ElementContainer
{
private:

    /*
    * The endpoints of a triangle.
    */
    struct Endpoints
    {
        ElementIndex PointAIndex;
        ElementIndex PointBIndex;
        ElementIndex PointCIndex;

        Endpoints(
            ElementIndex pointAIndex,
            ElementIndex pointBIndex,
            ElementIndex pointCIndex)
            : PointAIndex(pointAIndex)
            , PointBIndex(pointBIndex)
            , PointCIndex(pointCIndex)
        {}
    };


public:

    Triangles(ElementCount elementCount)
        : ElementContainer(elementCount)
        , mIsDeletedBuffer(elementCount)
        // Endpoints
        , mEndpointsBuffer(elementCount)        
    {
    }

    Triangles(Triangles && other) = default;
    
    void Add(
        ElementIndex pointAIndex,
        ElementIndex pointBIndex,
        ElementIndex pointCIndex);

    void Destroy(ElementIndex triangleElementIndex);

    void UploadElements(
        int shipId,
        RenderContext & renderContext,
        Points const & points) const;

public:

    //
    // IsDeleted
    //

    inline bool IsDeleted(ElementIndex triangleElementIndex) const
    {
        assert(triangleElementIndex < mElementCount);

        return mIsDeletedBuffer[triangleElementIndex];
    }

    //
    // Endpoints
    //

    inline ElementIndex GetPointAIndex(ElementIndex triangleElementIndex) const
    {
        assert(triangleElementIndex < mElementCount);

        return mEndpointsBuffer[triangleElementIndex].PointAIndex;
    }

    inline ElementIndex GetPointBIndex(ElementIndex triangleElementIndex) const
    {
        assert(triangleElementIndex < mElementCount);

        return mEndpointsBuffer[triangleElementIndex].PointBIndex;
    }

    inline ElementIndex GetPointCIndex(ElementIndex triangleElementIndex) const
    {
        assert(triangleElementIndex < mElementCount);

        return mEndpointsBuffer[triangleElementIndex].PointCIndex;
    }

private:

private:

    // Deletion
    Buffer<bool> mIsDeletedBuffer;

    // Endpoints
    Buffer<Endpoints> mEndpointsBuffer;
};

}
