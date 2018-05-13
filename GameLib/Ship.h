/***************************************************************************************
* Original Author:		Luke Wren (wren6991@gmail.com)
* Created:				2013-04-30
* Modified By:			Gabriele Giuseppini
* Copyright:			Luke Wren (http://github.com/Wren6991),
*						Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "ElementRepository.h"
#include "GameParameters.h"
#include "MaterialDatabase.h"
#include "Physics.h"
#include "PointerContainer.h"
#include "RenderContext.h"
#include "ShipDefinition.h"
#include "Vectors.h"

#include <set>

namespace Physics
{

class Ship
{
public:

    Ship(
        int id,
        World * parentWorld);

    void Initialize(
        Points && points,
        ElementRepository<vec3f> && allPointColors,
        ElementRepository<vec2f> && allPointTextureCoordinates,
        Springs && springs,
        Triangles && triangles,
        std::vector<ElectricalElement *> && allElectricalElements,
        uint64_t currentStepSequenceNumber)
    {
        mPoints = std::move(points);
        mAllPointColors = std::move(allPointColors);
        mAllPointTextureCoordinates = std::move(allPointTextureCoordinates);
        mSprings = std::move(springs);
        mTriangles = std::move(triangles);
        mAllElectricalElements.initialize(std::move(allElectricalElements));

        mIsPointCountDirty = true;
        mAreElementsDirty = true;

        DetectConnectedComponents(currentStepSequenceNumber);
    }

    ~Ship();

    unsigned int GetId() const { return mId; }

    World const * GetParentWorld() const { return mParentWorld; }
    World * GetParentWorld() { return mParentWorld; }

    auto const & GetElectricalElements() const { return mAllElectricalElements; }
    auto & GetElectricalElements() { return mAllElectricalElements; }

    auto const & GetPoints() const { return mPoints; }
    auto & GetPoints() { return mPoints; }

    auto const & GetSprings() const { return mSprings; }
    auto & GetSprings() { return mSprings; }

    auto const & GetTriangles() const { return mTriangles; }
    auto & GetTriangles() { return mTriangles; }

    void DestroyAt(
        vec2 const & targetPos,
        float radius);

    void DrawTo(
        vec2 const & targetPos,
        float strength);

    ElementContainer::ElementIndex GetNearestPointIndexAt(
        vec2 const & targetPos,
        float radius) const;

    void Update(
        uint64_t currentStepSequenceNumber,
        GameParameters const & gameParameters);

    void Render(
        GameParameters const & gameParameters,
        RenderContext & renderContext) const;

public:

    /*
     * Invoked when an elements has been destroyed. Notifies the ship that the element
     * can be removed (at the most appropriate time) from the ship's main container
     * of element pointers, and that the pointer can be deleted.
     *
     * Implemented differently for the different elements.
     */
    template<typename TElement>
    void RegisterDestruction(TElement * element);

public:

    /////////////////////////////////////////////////////////////////////////
    // Dynamics
    /////////////////////////////////////////////////////////////////////////

    void UpdateDynamics(GameParameters const & gameParameters);

    void UpdateDrawForces(
        vec2f const & position,
        float forceStrength);

    void UpdatePointForces(GameParameters const & gameParameters);

    void UpdateSpringForces(GameParameters const & gameParameters);    

    void Integrate();

    void HandleCollisionsWithSeaFloor();

    void DetectConnectedComponents(uint64_t currentStepSequenceNumber);

    void LeakWater(GameParameters const & gameParameters);

    void GravitateWater(GameParameters const & gameParameters);

    void BalancePressure(GameParameters const & gameParameters);

    void DiffuseLight(GameParameters const & gameParameters);

private:

    unsigned int const mId;
    World * const mParentWorld;

    // All the ship elements - never removed, the repositories maintain their own size forever
    Points mPoints;
    ElementRepository<vec3f> mAllPointColors;
    ElementRepository<vec2f> mAllPointTextureCoordinates;
    Springs mSprings;
    Triangles mTriangles;

    // Parts repository
    PointerContainer<ElectricalElement> mAllElectricalElements;

    // Connected components metadata
    std::vector<std::size_t> mConnectedComponentSizes;

    // Flag remembering whether the number of points has changed
    // since the last time we delivered them to the rendering context.
    // Does not count deleted points at this moment - deleted points remain
    // in the buffer that we deliver to the rendering context
    mutable bool mIsPointCountDirty;

    // Flag remembering whether points (elements) and/or springs (incl. ropes) and/or triangles have changed
    // since the last time we delivered them to the rendering context
    mutable bool mAreElementsDirty;

    // Sinking detection
    bool mIsSinking;
    float mTotalWater;

    //
    // Draw force to apply at next iteration
    //

    struct DrawForce
    {
        vec2f Position;
        float Strength;

        DrawForce(
            vec2f position,
            float strength)
            : Position(position)
            , Strength(strength)
        {}
    };

    std::optional<DrawForce> mCurrentDrawForce;
};

template<>
inline void Ship::RegisterDestruction(ElectricalElement * /* element */)
{
    // Also tell the pointer container, he'll take care of removing the element later
    mAllElectricalElements.register_deletion();
}

}