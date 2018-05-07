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
#include "Vectors.h"

#include <cassert>

namespace Physics
{

class Points : ElementContainer
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

        // TODOHERE: others
    }

public:

    //
    // IsDeleted
    //

    inline bool const * GetIsDeletedBuffer() const
    {
        return mIsDeletedBuffer.data();
    }

    inline bool IsDeleted(ElementIndex pointElementIndex) const
    {
        assert(pointElementIndex < mElementCount);
        return mIsDeletedBuffer[pointElementIndex];
    }

    // TODO: caller must also ensure that Ship::mAreElementsDirty is set
    void Destroy(ElementIndex pointElementIndex);

    //
    // Material
    //

    inline Material const * const * GetMaterialBuffer() const
    {
        return mMaterialBuffer.data();
    }

    //
    // Dynamics
    //

    inline Newtonz * GetNewtonzBuffer()
    {
        return mNewtonzBuffer.data();
    }

    inline float const * GetMassBuffer() const
    {
        return mMassBuffer.data();
    }

    //
    // Water dynamics
    //

    inline float const * GetBuoyancyBuffer() const
    {
        return mBuoyancyBuffer.data();
    }

    inline float * GetWaterBuffer()
    {
        return mWaterBuffer.data();
    }

    inline bool * GetIsLeakingBuffer()
    {
        return mIsLeakingBuffer.data();
    }

    //
    // Electrical dynamics
    //

    inline float * GetLightBuffer()
    {
        return mLightBuffer.data();
    }

    //
    // Network
    //

    inline Network const * GetNetworkBuffer() const
    {
        return mNetworkBuffer.data();
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

    inline void SetConnectedElectricalElement(
        ElementIndex pointElementIndex,
        ElectricalElement * electricalElement)
    {
        assert(pointElementIndex < mElementCount);
        assert(nullptr == mConnectedElectricalElement);

        mNetworkBuffer[pointElementIndex].ConnectedElectricalElement = electricalElement;
    }


    //
    // Connected component
    //

    inline ConnectedComponent const * GetConnectedComponentBuffer() const
    {
        return mConnectedComponentBuffer.data();
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