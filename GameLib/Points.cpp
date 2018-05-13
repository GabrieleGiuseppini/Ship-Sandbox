/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-05-06
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "Physics.h"

#include <cmath>

namespace Physics {

void Points::Add(
    vec2 const & position,
    Material const * material,
    float buoyancy)
{
    mIsDeletedBuffer.emplace_back(false);

    mMaterialBuffer.emplace_back(material);

    mPositionBuffer.emplace_back(position);
    mVelocityBuffer.emplace_back(vec2f(0.0f, 0.0f));
    mForceBuffer.emplace_back(vec2f(0.0f, 0.0f));
    mMassFactorBuffer.emplace_back(CalculateMassFactor(material->Mass));
    mMassBuffer.emplace_back(material->Mass);

    mBuoyancyBuffer.emplace_back(buoyancy);
    mIsLeakingBuffer.emplace_back(false);
    mWaterBuffer.emplace_back(0.0f);

    mLightBuffer.emplace_back(0.0f);

    mNetworkBuffer.emplace_back();

    mConnectedComponentIdBuffer.emplace_back(0u);
    mCurrentConnectedComponentDetectionStepSequenceNumberBuffer.emplace_back(0u);
}

void Points::Destroy(
    ElementIndex pointElementIndex,
    Springs & springs,
    Triangles & triangles,
    ElectricalElements & electricalElements)
{
    assert(pointElementIndex < mElementCount);

    Network & pointNetwork = mNetworkBuffer[pointElementIndex];

    //
    // Destroy all springs attached to this point
    //

    for (auto springIndex : pointNetwork.ConnectedSprings)
    {
        assert(!springs.IsDeleted(springIndex));
        springs.Destroy(springIndex, pointElementIndex, *this, triangles);
    }

    pointNetwork.ConnectedSprings.clear();

    //
    // Destroy all triangles connected to this point
    //

    for (auto triangleIndex : pointNetwork.ConnectedTriangles)
    {
        assert(!triangles.IsDeleted(triangleIndex));
        triangles.Destroy(triangleIndex, pointElementIndex, *this);
    }

    pointNetwork.ConnectedTriangles.clear();

    //
    // Destroy the connected electrical element
    //

    if (NoneElementIndex != pointNetwork.ConnectedElectricalElement)
    {
        electricalElements.Destroy(pointNetwork.ConnectedElectricalElement);
    }


    //
    // Flag ourselves as deleted
    //

    mIsDeletedBuffer[pointElementIndex] = true;
}

void Points::Breach(
    ElementIndex pointElementIndex,
    Triangles & triangles)
{
    assert(pointElementIndex < mElementCount);

    //
    // Start leaking
    //

    mIsLeakingBuffer[pointElementIndex] = true;

    //
    // Destroy all of our connected triangles
    //

    Network & pointNetwork = mNetworkBuffer[pointElementIndex];

    for (auto triangleIndex : pointNetwork.ConnectedTriangles)
    {
        assert(!triangles.IsDeleted(triangleIndex));
        triangles.Destroy(triangleIndex, pointElementIndex, *this);
    }

    pointNetwork.ConnectedTriangles.clear();
}

void Points::UploadMutableGraphicalAttributes(
    int shipId,
    RenderContext & renderContext) const
{
    renderContext.UploadShipPoints(
        shipId,  
        mElementCount,
        reinterpret_cast<float const *>(mPositionBuffer.data()),
        mLightBuffer.data(),
        mWaterBuffer.data());
}

void Points::UploadElements(
    int shipId,
    RenderContext & renderContext) const
{
    for (ElementIndex i : *this)
    {
        if (!mIsDeletedBuffer[i])
        {
            renderContext.UploadShipElementPoint(
                shipId,
                i,
                mConnectedComponentIdBuffer[i]);
        }
    }
}

vec2f Points::CalculateMassFactor(float mass)
{
    assert(mass > 0.0f);

    static constexpr float dt = GameParameters::DynamicsSimulationStepTimeDuration<float>;
    
    return vec2f(dt * dt / mass, dt * dt / mass);
}

}