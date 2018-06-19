/***************************************************************************************
* Original Author:		Luke Wren (wren6991@gmail.com)
* Created:				2013-04-30
* Modified By:			Gabriele Giuseppini
* Copyright:			Luke Wren (http://github.com/Wren6991),
*						Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "CircularList.h"
#include "GameParameters.h"
#include "GameTypes.h"
#include "MaterialDatabase.h"
#include "Physics.h"
#include "RenderContext.h"
#include "ShipDefinition.h"
#include "Vectors.h"

#include <optional>
#include <vector>

namespace Physics
{

class Ship
{
public:

    Ship(
        int id,
        World & parentWorld,
        std::shared_ptr<IGameEventHandler> gameEventHandler,
        Points && points,
        Springs && springs,
        Triangles && triangles,
        ElectricalElements && electricalElements,
        uint64_t currentStepSequenceNumber);

    ~Ship();

    unsigned int GetId() const { return mId; }

    World const & GetParentWorld() const { return mParentWorld; }
    World & GetParentWorld() { return mParentWorld; }

    auto const & GetPoints() const { return mPoints; }
    auto & GetPoints() { return mPoints; }

    auto const & GetSprings() const { return mSprings; }
    auto & GetSprings() { return mSprings; }

    auto const & GetTriangles() const { return mTriangles; }
    auto & GetTriangles() { return mTriangles; }

    auto const & GetElectricalElements() const { return mElectricalElements; }
    auto & GetElectricalElements() { return mElectricalElements; }

    void DestroyAt(
        vec2 const & targetPos,
        float radius);

    void SawThrough(
        vec2 const & startPos,
        vec2 const & endPos);

    void DrawTo(
        vec2 const & targetPos,
        float strength);

    void SwirlAt(
        vec2 const & targetPos,
        float strength);

    bool TogglePinAt(
        vec2 const & targetPos,
        GameParameters const & gameParameters);

    bool ToggleTimerBombAt(
        vec2 const & targetPos,
        GameParameters const & gameParameters);

    bool ToggleRCBombAt(
        vec2 const & targetPos,
        GameParameters const & gameParameters);

    void DetonateRCBombs();

    ElementIndex GetNearestPointIndexAt(
        vec2 const & targetPos,
        float radius) const;

    void Update(
        uint64_t currentStepSequenceNumber,
        GameParameters const & gameParameters);

    void Render(
        GameParameters const & gameParameters,
        RenderContext & renderContext) const;

public:

    /////////////////////////////////////////////////////////////////////////
    // Dynamics
    /////////////////////////////////////////////////////////////////////////

    void UpdateDynamics(GameParameters const & gameParameters);

    void UpdateDrawForces(
        vec2f const & position,
        float forceStrength);

    void UpdateSwirlForces(
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

    void BombBlastHandler(
        vec2f const & blastPosition,
        ConnectedComponentId connectedComponentId,
        int blastSequenceNumber,
        int blastSequenceCount,
        GameParameters const & gameParameters);

    void PointDestroyHandler(ElementIndex pointElementIndex);

    void SpringDestroyHandler(ElementIndex springElementIndex);

    void TriangleDestroyHandler(ElementIndex triangleElementIndex);

    void ElectricalElementDestroyHandler(ElementIndex electricalElementIndex);

private:

    unsigned int const mId;
    World & mParentWorld;
    std::shared_ptr<IGameEventHandler> mGameEventHandler;

    // All the ship elements - never removed, the repositories maintain their own size forever
    Points mPoints;
    Springs mSprings;
    Triangles mTriangles;
    ElectricalElements mElectricalElements;

    // Connected components metadata
    std::vector<std::size_t> mConnectedComponentSizes;

    // Flag remembering whether points (elements) and/or springs (incl. ropes) and/or triangles have changed
    // since the last step.
    // When this flag is set, we'll re-detect connected components and re-upload elements
    // to the rendering context
    bool mutable mAreElementsDirty;

    // Sinking detection
    bool mIsSinking;
    float mTotalWater;


    //
    // Pinned points
    //

    // The current set of pinned points
    CircularList<ElementIndex, GameParameters::MaxPinnedPoints> mCurrentPinnedPoints;

    // Flag remembering whether the set of pinned points has changed since the last step
    bool mutable mArePinnedPointsDirty;


    //
    // Bombs
    //

    Bombs mBombs;


    //
    // Tool force to apply at next iteration
    //

    struct ToolForce
    {
        vec2f Position;
        float Strength;
        bool IsRadial;

        ToolForce(
            vec2f position,
            float strength,
            bool isRadial)
            : Position(position)
            , Strength(strength)
            , IsRadial(isRadial)
        {}
    };

    std::optional<ToolForce> mCurrentToolForce;
};

}