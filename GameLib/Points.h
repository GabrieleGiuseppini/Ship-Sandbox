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
public:

    // TODO: this is just a test to check whether 4 small buffers are really
    // better than one bigger buffer
    struct Newtonz
    {
        vec2f Position;
        vec2f Velocity;
        vec2f Force;
        vec2f MassFactor;

        Newtonz(
            vec2f position,
            vec2f velocity,
            vec2f force,
            vec2f massFactor)
            : Position(position)
            , Velocity(velocity)
            , Force(force)
            , MassFactor(massFactor)
        {}
    };

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

    struct ConnectedComponent
    {
        size_t ConnectedComponentId; // Starts from 1
        size_t CurrentConnectedComponentDetectionStepSequenceNumber;

        ConnectedComponent()
            : ConnectedComponentId(0u)
            , CurrentConnectedComponentDetectionStepSequenceNumber(0u)
        {}
    };

public:

    Points(ElementCount elementCount)
        : ElementContainer(elementCount)
        , mIsDeletedBuffer(elementCount)
        , mMaterialBuffer(elementCount)
        // Dynamics
        , mNewtonzBuffer(elementCount)
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
        , mConnectedComponentBuffer(elementCount)
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

        mNewtonzBuffer.emplace_back(
            position,
            vec2f(0.0f, 0.0f),
            vec2f(0.0f, 0.0f),
            CalculateMassFactor(material->Mass));
        mMassBuffer.emplace_back(material->Mass);

        mBuoyancyBuffer.emplace_back(buoyancy);
        mIsLeakingBuffer.emplace_back(false);
        mWaterBuffer.emplace_back(0.0f);

        mLightBuffer.emplace_back(0.0f);

        mNetworkBuffer.emplace_back();

        mConnectedComponentBuffer.emplace_back();
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

    inline Newtonz * GetNewtonzBuffer()
    {
        return mNewtonzBuffer.data();
    }

    vec2f const & GetPosition(ElementIndex pointElementIndex) const
    {
        assert(pointElementIndex < mElementCount);

        return mNewtonzBuffer[pointElementIndex].Position;
    }

    vec2f & GetPosition(ElementIndex pointElementIndex) 
    {
        assert(pointElementIndex < mElementCount);

        return mNewtonzBuffer[pointElementIndex].Position;
    }

    vec2f const & GetVelocity(ElementIndex pointElementIndex) const
    {
        assert(pointElementIndex < mElementCount);

        return mNewtonzBuffer[pointElementIndex].Velocity;
    }

    vec2f & GetVelocity(ElementIndex pointElementIndex)
    {
        assert(pointElementIndex < mElementCount);

        return mNewtonzBuffer[pointElementIndex].Velocity;
    }

    vec2f const & GetForce(ElementIndex pointElementIndex) const
    {
        assert(pointElementIndex < mElementCount);

        return mNewtonzBuffer[pointElementIndex].Force;
    }

    vec2f & GetForce(ElementIndex pointElementIndex)
    {
        assert(pointElementIndex < mElementCount);

        return mNewtonzBuffer[pointElementIndex].Force;
    }

    vec2f const & GetMassFactor(ElementIndex pointElementIndex) const
    {
        assert(pointElementIndex < mElementCount);

        return mNewtonzBuffer[pointElementIndex].MassFactor;
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

    inline uint64_t GetConnectedComponentId(ElementIndex pointElementIndex) const
    {
        assert(pointElementIndex < mElementCount);

        return mConnectedComponentBuffer[pointElementIndex].ConnectedComponentId;
    }

    inline void SetConnectedComponentId(
        ElementIndex pointElementIndex,
        uint64_t connectedComponentId) 
    { 
        assert(pointElementIndex < mElementCount);

        mConnectedComponentBuffer[pointElementIndex].ConnectedComponentId = connectedComponentId; 
    }

    inline uint64_t GetCurrentConnectedComponentDetectionStepSequenceNumber(ElementIndex pointElementIndex) const
    {
        assert(pointElementIndex < mElementCount);

        return mConnectedComponentBuffer[pointElementIndex].CurrentConnectedComponentDetectionStepSequenceNumber;
    }

    inline void SetCurrentConnectedComponentDetectionStepSequenceNumber(
        ElementIndex pointElementIndex,
        uint64_t connectedComponentDetectionStepSequenceNumber) 
    { 
        assert(pointElementIndex < mElementCount);

        mConnectedComponentBuffer[pointElementIndex].CurrentConnectedComponentDetectionStepSequenceNumber = 
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

    Buffer<Newtonz> mNewtonzBuffer;

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

    Buffer<ConnectedComponent> mConnectedComponentBuffer;
};

}