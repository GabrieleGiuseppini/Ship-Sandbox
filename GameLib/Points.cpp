/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-05-06
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "Physics.h"

#include <cmath>

namespace Physics {

void Points::Upload(
    int shipId,
    RenderContext & renderContext) const
{
    // TODO: once we get rid of Newtons
    ////renderContext.UploadShipPoints(
    ////    shipId,
    ////    mPositionBuffer.data(),
    ////    mLightBuffer.data(),
    ////    mWaterBufffer.data());

    renderContext.UploadShipPointsStart(shipId, mElementCount);
    
    for (ElementIndex i = 0; i < mElementCount; ++i)
    {
        renderContext.UploadShipPoint(
            shipId,
            mNewtonzBuffer[i].Position.x,
            mNewtonzBuffer[i].Position.y,
            mLightBuffer[i],
            mWaterBuffer[i]);
    }

    renderContext.UploadShipPointsEnd(shipId);
}

void Points::UploadElements(
    int shipId,
    RenderContext & renderContext) const
{
    for (ElementIndex i = 0; i < mElementCount; ++i)
    {
        if (!mIsDeletedBuffer[i])
        {
            renderContext.UploadShipElementPoint(
                shipId,
                i,
                mConnectedComponentBuffer[i].ConnectedComponentId);
        }
    }
}

void Points::Destroy(ElementIndex pointElementIndex)
{
    assert(pointElementIndex < mElementCount);

    Network & network = mNetworkBuffer[pointElementIndex];

    //
    // Destroy all springs attached to this point
    //

    for (Spring * spring : network.ConnectedSprings)
    {
        assert(!spring->IsDeleted());
        spring->Destroy(*this, pointElementIndex);
    }

    network.ConnectedSprings.clear();

    //
    // Destroy all triangles connected to this point
    //

    for (Triangle * triangle : network.ConnectedTriangles)
    {
        assert(!triangle->IsDeleted());
        triangle->Destroy(*this, pointElementIndex);
    }

    network.ConnectedTriangles.clear();

    //
    // Destroy the connected electrical element
    //

    if (nullptr != network.ConnectedElectricalElement)
    {
        network.ConnectedElectricalElement->Destroy();
        network.ConnectedElectricalElement = nullptr;
    }


    //
    // Flag ourselves as deleted
    //

    mIsDeletedBuffer[pointElementIndex] = true;
}

void Points::Breach(ElementIndex pointElementIndex)
{
    assert(pointElementIndex < mElementCount);

    //
    // Start leaking
    //

    mIsLeakingBuffer[pointElementIndex] = true;

    //
    // Destroy all of our connected triangles
    //

    Network & network = mNetworkBuffer[pointElementIndex];

    for (Triangle * triangle : network.ConnectedTriangles)
    {
        assert(!triangle->IsDeleted());
        triangle->Destroy(*this, pointElementIndex);
    }

    network.ConnectedTriangles.clear();
}

vec2f Points::CalculateMassFactor(float mass)
{
    assert(mass > 0.0f);

    static constexpr float dt = GameParameters::DynamicsSimulationStepTimeDuration<float>;
    
    return vec2f(dt * dt / mass, dt * dt / mass);
}

}