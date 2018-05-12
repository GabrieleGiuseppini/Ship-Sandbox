/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-05-06
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "Buffer.h"
#include "ElementContainer.h"
#include "FixedSizeVector.h"
#include "GameParameters.h"
#include "Material.h"
#include "RenderContext.h"
#include "Vectors.h"

#include <cassert>

namespace Physics
{

class Points : public ElementContainer
{
private:

    /*
     * The elements connected to a point.
     */
    struct Network
    {
        // 8 neighbours and 1 rope spring, when this is a rope endpoint
        FixedSizeVector<Spring *, 8U + 1U> ConnectedSprings; 

        FixedSizeVector<Triangle *, 8U> ConnectedTriangles;

        ElectricalElement * ConnectedElectricalElement;

        Network()
            : ConnectedElectricalElement(nullptr)
        {}
    };

public:

    Points(ElementCount elementCount)
        : ElementContainer(elementCount)
        , mIsDeletedBuffer(elementCount)
        , mMaterialBuffer(elementCount)
        // Dynamics
        , mPositionBuffer(elementCount)
        , mVelocityBuffer(elementCount)
        , mForceBuffer(elementCount)
        , mMassFactorBuffer(elementCount)
        , mMassBuffer(elementCount)
        // Water dynamics
        , mBuoyancyBuffer(elementCount)        
        , mWaterBuffer(elementCount)
        , mIsLeakingBuffer(elementCount)
        // Electrical dynamics
        , mLightBuffer(elementCount)
        // Structure
        , mNetworkBuffer(elementCount)
        // Connected component
        , mConnectedComponentIdBuffer(elementCount)
        , mCurrentConnectedComponentDetectionStepSequenceNumberBuffer(elementCount)
    {
    }

    Points(Points && other) = default;

    // TODOREMOVE: only for phase I, this is needed for ship ::Initialize() which will go, substituted
    // by ship cctor()
    Points & operator=(Points && other) = default;

    void Add(
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

    void Destroy(ElementIndex pointElementIndex);

    void UploadMutableGraphicalAttributes(
        int shipId,
        RenderContext & renderContext) const;

    void UploadElements(
        int shipId,
        RenderContext & renderContext) const;

public:

    //
    // IsDeleted
    //

    inline bool IsDeleted(ElementIndex pointElementIndex) const
    {
        assert(pointElementIndex < mElementCount);

        return mIsDeletedBuffer[pointElementIndex];
    }


    //
    // Material
    //

    inline Material const * GetMaterial(ElementIndex pointElementIndex) const
    {
        assert(pointElementIndex < mElementCount);

        return mMaterialBuffer[pointElementIndex];
    }

    //
    // Dynamics
    //

    vec2f const & GetPosition(ElementIndex pointElementIndex) const
    {
        assert(pointElementIndex < mElementCount);

        return mPositionBuffer[pointElementIndex];
    }

    vec2f & GetPosition(ElementIndex pointElementIndex) 
    {
        assert(pointElementIndex < mElementCount);

        return mPositionBuffer[pointElementIndex];
    }

    vec2f const & GetVelocity(ElementIndex pointElementIndex) const
    {
        assert(pointElementIndex < mElementCount);

        return mVelocityBuffer[pointElementIndex];
    }

    vec2f & GetVelocity(ElementIndex pointElementIndex)
    {
        assert(pointElementIndex < mElementCount);

        return mVelocityBuffer[pointElementIndex];
    }

    vec2f const & GetForce(ElementIndex pointElementIndex) const
    {
        assert(pointElementIndex < mElementCount);

        return mForceBuffer[pointElementIndex];
    }

    vec2f & GetForce(ElementIndex pointElementIndex)
    {
        assert(pointElementIndex < mElementCount);

        return mForceBuffer[pointElementIndex];
    }

    vec2f const & GetMassFactor(ElementIndex pointElementIndex) const
    {
        assert(pointElementIndex < mElementCount);

        return mMassFactorBuffer[pointElementIndex];
    }

    float GetMass(ElementIndex pointElementIndex) const
    {
        assert(pointElementIndex < mElementCount);

        return mMassBuffer[pointElementIndex];
    }

    //
    // Water dynamics
    //

    inline float GetBuoyancy(ElementIndex pointElementIndex) const
    {
        assert(pointElementIndex < mElementCount);

        return mBuoyancyBuffer[pointElementIndex];
    }

    inline float GetWater(ElementIndex pointElementIndex) const
    {
        assert(pointElementIndex < mElementCount);

        return mWaterBuffer[pointElementIndex];
    }

    inline float & GetWater(ElementIndex pointElementIndex)
    {
        assert(pointElementIndex < mElementCount);

        return mWaterBuffer[pointElementIndex];
    }

    float GetExternalWaterPressure(
        ElementIndex pointElementIndex,
        float waterLevel,
        GameParameters const & gameParameters) const
    {
        // Negative Y == under water line
        if (GetPosition(pointElementIndex).y < waterLevel)
        {
            return gameParameters.GravityMagnitude * (waterLevel - GetPosition(pointElementIndex).y) * 0.1f;  // 0.1 = scaling constant, represents 1/ship width
        }
        else
        {
            return 0.0f;
        }
    }

    inline bool IsLeaking(ElementIndex pointElementIndex) const
    {
        assert(pointElementIndex < mElementCount);

        return mIsLeakingBuffer[pointElementIndex];
    }

    inline void SetLeaking(ElementIndex pointElementIndex)
    {
        assert(pointElementIndex < mElementCount);

        mIsLeakingBuffer[pointElementIndex] = true;
    }

    //
    // Electrical dynamics
    //

    inline float GetLight(ElementIndex pointElementIndex) const
    {
        assert(pointElementIndex < mElementCount);

        return mLightBuffer[pointElementIndex];
    }

    inline float & GetLight(ElementIndex pointElementIndex)
    {
        assert(pointElementIndex < mElementCount);

        return mLightBuffer[pointElementIndex];
    }

    //
    // Network
    //

    inline auto const & GetConnectedSprings(ElementIndex pointElementIndex) const
    {
        assert(pointElementIndex < mElementCount);

        return mNetworkBuffer[pointElementIndex].ConnectedSprings;
    }

    inline void AddConnectedSpring(
        ElementIndex pointElementIndex,
        Spring * spring)
    {
        assert(pointElementIndex < mElementCount);

        mNetworkBuffer[pointElementIndex].ConnectedSprings.push_back(spring);
    }

    inline void RemoveConnectedSpring(
        ElementIndex pointElementIndex,
        Spring * spring)
    {
        assert(pointElementIndex < mElementCount);
        
        bool found = mNetworkBuffer[pointElementIndex].ConnectedSprings.erase_first(spring);

        assert(found);
        (void)found;
    }

    inline auto const & GetConnectedTriangles(ElementIndex pointElementIndex) const
    {
        assert(pointElementIndex < mElementCount);

        return mNetworkBuffer[pointElementIndex].ConnectedTriangles;
    }

    inline void AddConnectedTriangle(
        ElementIndex pointElementIndex,
        Triangle * triangle)
    {
        assert(pointElementIndex < mElementCount);

        mNetworkBuffer[pointElementIndex].ConnectedTriangles.push_back(triangle);
    }

    inline void RemoveConnectedTriangle(
        ElementIndex pointElementIndex,
        Triangle * triangle)
    {
        assert(pointElementIndex < mElementCount);

        bool found = mNetworkBuffer[pointElementIndex].ConnectedTriangles.erase_first(triangle);

        assert(found);
        (void)found;
    }

    inline ElectricalElement * GetConnectedElectricalElement(ElementIndex pointElementIndex) const
    {
        assert(pointElementIndex < mElementCount);

        return mNetworkBuffer[pointElementIndex].ConnectedElectricalElement;
    }

    inline void SetConnectedElectricalElement(
        ElementIndex pointElementIndex,
        ElectricalElement * electricalElement)
    {
        assert(pointElementIndex < mElementCount);
        assert(nullptr == mNetworkBuffer[pointElementIndex].ConnectedElectricalElement);

        mNetworkBuffer[pointElementIndex].ConnectedElectricalElement = electricalElement;
    }

    void Breach(ElementIndex pointElementIndex);

    //
    // Connected component
    //

    inline uint32_t GetConnectedComponentId(ElementIndex pointElementIndex) const
    {
        assert(pointElementIndex < mElementCount);

        return mConnectedComponentIdBuffer[pointElementIndex];
    }

    inline void SetConnectedComponentId(
        ElementIndex pointElementIndex,
        uint32_t connectedComponentId) 
    { 
        assert(pointElementIndex < mElementCount);

        mConnectedComponentIdBuffer[pointElementIndex] = connectedComponentId;
    }

    inline uint64_t GetCurrentConnectedComponentDetectionStepSequenceNumber(ElementIndex pointElementIndex) const
    {
        assert(pointElementIndex < mElementCount);

        return mCurrentConnectedComponentDetectionStepSequenceNumberBuffer[pointElementIndex];
    }

    inline void SetCurrentConnectedComponentDetectionStepSequenceNumber(
        ElementIndex pointElementIndex,
        uint64_t connectedComponentDetectionStepSequenceNumber) 
    { 
        assert(pointElementIndex < mElementCount);

        mCurrentConnectedComponentDetectionStepSequenceNumberBuffer[pointElementIndex] =
            connectedComponentDetectionStepSequenceNumber;
    }

private:

    static vec2f CalculateMassFactor(float mass);

private:

    // Deletion
    Buffer<bool> mIsDeletedBuffer;

    // Material
    Buffer<Material const *> mMaterialBuffer;

    //
    // Dynamics
    //

    Buffer<vec2f> mPositionBuffer;
    Buffer<vec2f> mVelocityBuffer;
    Buffer<vec2f> mForceBuffer;
    Buffer<vec2f> mMassFactorBuffer;
    Buffer<float> mMassBuffer;

    //
    // Water dynamics
    //

    Buffer<float> mBuoyancyBuffer;
    
    // Total quantity of water, 0.0->+INF (== internal water pressure)
    Buffer<float> mWaterBuffer;

    Buffer<bool> mIsLeakingBuffer;

    //
    // Electrical dynamics
    //

    // Total illumination, 0.0->1.0
    Buffer<float> mLightBuffer;

    //
    // Structure
    //

    Buffer<Network> mNetworkBuffer;

    //
    // Connected component
    //

    Buffer<uint32_t> mConnectedComponentIdBuffer; // Connected component IDs start from 1
    Buffer<uint64_t> mCurrentConnectedComponentDetectionStepSequenceNumberBuffer;
};

}