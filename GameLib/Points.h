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
#include "IGameEventHandler.h"
#include "Material.h"
#include "RenderContext.h"
#include "Vectors.h"

#include <cassert>
#include <vector>

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
        FixedSizeVector<ElementIndex, 8U + 1U> ConnectedSprings; 

        FixedSizeVector<ElementIndex, 8U> ConnectedTriangles;

        ElementIndex ConnectedElectricalElement;

        Network()
            : ConnectedElectricalElement(NoneElementIndex)
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
        // Pinning
        , mIsPinnedBuffer(elementCount)
        // Immutable render attributes
        , mColorBuffer(elementCount)
        , mTextureCoordinatesBuffer(elementCount)
        , mAreImmutableRenderAttributesUploaded(false)
    {
    }

    Points(Points && other) = default;
    
    void Add(
        vec2 const & position,
        Material const * material,
        float buoyancy,
        vec3f const & color,
        vec2f const & textureCoordinates);

    void Destroy(
        ElementIndex pointElementIndex,
        Springs & springs,
        Triangles & triangles,
        ElectricalElements & electricalElements);

    void Breach(
        ElementIndex pointElementIndex,
        Triangles & triangles);    

    void Upload(
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

    float * restrict GetPositionBufferAsFloat()
    {
        return reinterpret_cast<float *>(mPositionBuffer.data());
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

    float * restrict GetVelocityBufferAsFloat()
    {
        return reinterpret_cast<float *>(mVelocityBuffer.data());
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

    float * restrict GetForceBufferAsFloat()
    {
        return reinterpret_cast<float *>(mForceBuffer.data());
    }

    vec2f const & GetMassFactor(ElementIndex pointElementIndex) const
    {
        assert(pointElementIndex < mElementCount);

        return mMassFactorBuffer[pointElementIndex];
    }

    float * restrict GetMassFactorBufferAsFloat()
    {
        return reinterpret_cast<float *>(mMassFactorBuffer.data());
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
        ElementIndex springElementIndex)
    {
        assert(pointElementIndex < mElementCount);

        mNetworkBuffer[pointElementIndex].ConnectedSprings.push_back(springElementIndex);
    }

    inline void RemoveConnectedSpring(
        ElementIndex pointElementIndex,
        ElementIndex springElementIndex)
    {
        assert(pointElementIndex < mElementCount);
        
        bool found = mNetworkBuffer[pointElementIndex].ConnectedSprings.erase_first(springElementIndex);

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
        ElementIndex triangleElementIndex)
    {
        assert(pointElementIndex < mElementCount);

        mNetworkBuffer[pointElementIndex].ConnectedTriangles.push_back(triangleElementIndex);
    }

    inline void RemoveConnectedTriangle(
        ElementIndex pointElementIndex,
        ElementIndex triangleElementIndex)
    {
        assert(pointElementIndex < mElementCount);

        bool found = mNetworkBuffer[pointElementIndex].ConnectedTriangles.erase_first(triangleElementIndex);

        assert(found);
        (void)found;
    }

    inline ElementIndex GetConnectedElectricalElement(ElementIndex pointElementIndex) const
    {
        assert(pointElementIndex < mElementCount);

        return mNetworkBuffer[pointElementIndex].ConnectedElectricalElement;
    }

    inline void SetConnectedElectricalElement(
        ElementIndex pointElementIndex,
        ElementIndex electricalElementIndex)
    {
        assert(pointElementIndex < mElementCount);
        assert(NoneElementIndex == mNetworkBuffer[pointElementIndex].ConnectedElectricalElement);

        mNetworkBuffer[pointElementIndex].ConnectedElectricalElement = electricalElementIndex;
    }

    //
    // Pinning
    //

    inline bool IsPinned(ElementIndex pointElementIndex) const
    {
        assert(pointElementIndex < mElementCount);

        return mIsPinnedBuffer[pointElementIndex];
    }

    void Pin(ElementIndex pointElementIndex) 
    {
        assert(pointElementIndex < mElementCount);
    
        mIsPinnedBuffer[pointElementIndex] = true;

        // Zero-out mass factor and velocity, freezing point
        mMassFactorBuffer[pointElementIndex] = vec2f(0.0f, 0.0f);
        mVelocityBuffer[pointElementIndex] = vec2f(0.0f, 0.0f);
    }

    void UnPin(ElementIndex pointElementIndex)
    {
        assert(pointElementIndex < mElementCount);
    
        mIsPinnedBuffer[pointElementIndex] = false;

        // Re-populate its mass factor, thawing point
        mMassFactorBuffer[pointElementIndex] = CalculateMassFactor(mMassBuffer[pointElementIndex]);
    }

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

    //
    // Pinning
    //

    Buffer<bool> mIsPinnedBuffer;

    //
    // Immutable render attributes
    //

    Buffer<vec3f> mColorBuffer;
    Buffer<vec2f> mTextureCoordinatesBuffer;

    // Flag remembering whether or not we've already uploaded
    // the immutable render attributes
    bool mutable mAreImmutableRenderAttributesUploaded;
};

}