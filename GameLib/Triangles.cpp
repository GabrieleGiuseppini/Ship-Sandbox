/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-05-13
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "Physics.h"

namespace Physics {

void Triangles::Add(
    ElementIndex pointAIndex,
    ElementIndex pointBIndex,
    ElementIndex pointCIndex)
{
    mIsDeletedBuffer.emplace_back(false);

    mEndpointsBuffer.emplace_back(pointAIndex, pointBIndex, pointCIndex);
}

void Triangles::Destroy(
    ElementIndex triangleElementIndex,
    ElementIndex sourcePointElementIndex,
    Points & points)
{
    assert(triangleElementIndex < mElementCount);

    Endpoints const & endpoints = mEndpointsBuffer[triangleElementIndex];

    assert(!points.IsDeleted(endpoints.PointAIndex));
    assert(!points.IsDeleted(endpoints.PointBIndex));
    assert(!points.IsDeleted(endpoints.PointCIndex));

    // Remove ourselves from our endpoints
    if (endpoints.PointAIndex != sourcePointElementIndex)
        points.RemoveConnectedTriangle(endpoints.PointAIndex, triangleElementIndex);
    if (endpoints.PointBIndex != sourcePointElementIndex)
        points.RemoveConnectedTriangle(endpoints.PointBIndex, triangleElementIndex);
    if (endpoints.PointCIndex != sourcePointElementIndex)
        points.RemoveConnectedTriangle(endpoints.PointCIndex, triangleElementIndex);

    //
    // Flag ourselves as deleted
    //

    mIsDeletedBuffer[triangleElementIndex] = true;
}

void Triangles::UploadElements(
    int shipId,
    RenderContext & renderContext,
    Points const & points) const
{
    for (ElementIndex i : *this)
    {
        if (!mIsDeletedBuffer[i])
        {
            assert(points.GetConnectedComponentId(GetPointAIndex(i)) == points.GetConnectedComponentId(GetPointBIndex(i))
                && points.GetConnectedComponentId(GetPointAIndex(i)) == points.GetConnectedComponentId(GetPointCIndex(i)));

            renderContext.UploadShipElementTriangle(
                shipId,
                GetPointAIndex(i),
                GetPointBIndex(i),
                GetPointCIndex(i),
                points.GetConnectedComponentId(GetPointAIndex(i)));
        }
    }
}

}