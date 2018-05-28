/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-05-23
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "Physics.h"

namespace Physics {

void Bombs::Update(GameParameters const & gameParameters)
{
    auto now = GameWallClock::GetInstance().Now();

    // Run through all bombs and invoke Update() on each;
    // remove those bombs that have expired
    for (auto it = mCurrentBombs.begin(); it != mCurrentBombs.end(); /* incremented in loop */)
    {
        bool isActive = (*it)->Update(now, gameParameters);
        if (!isActive)
        {
            //
            // Bomb has expired
            //

            // Bomb has been detached already
            assert(!(*it)->GetAttachedPointIndex());

            // Notify (soundless) removal
            mGameEventHandler->OnBombRemoved(
                (*it)->GetType(),
                std::nullopt);

            // Remove it from the container
            it = mCurrentBombs.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

void Bombs::OnPointDestroyed(ElementContainer::ElementIndex pointElementIndex)
{
    auto squareNeighborhoodRadius = GameParameters::BombNeighborhoodRadius * GameParameters::BombNeighborhoodRadius;

    auto neighborhoodCenter = mShipPoints.GetPosition(pointElementIndex);

    for (auto & bomb : mCurrentBombs)
    {
        // Check if the bomb is attached to this point
        auto bombPoint = bomb->GetAttachedPointIndex();
        if (!!bombPoint && *bombPoint == pointElementIndex)
        {
            // Detach bomb
            bomb->DetachFromPointIfAttached();
        }

        // Check if the bomb is within the neighborhood of the disturbed point
        float squareBombDistance = (bomb->GetPosition() - neighborhoodCenter).squareLength();
        if (squareBombDistance < squareNeighborhoodRadius)
        {
            // Tel the bomb that its neighborhood has been disturbed
            bomb->OnNeighborhoodDisturbed();
        }
    }
}

void Bombs::OnSpringDestroyed(ElementContainer::ElementIndex springElementIndex)
{
    auto squareNeighborhoodRadius = GameParameters::BombNeighborhoodRadius * GameParameters::BombNeighborhoodRadius;

    auto neighborhoodCenter =
        (mShipPoints.GetPosition(mShipSprings.GetPointAIndex(springElementIndex)) 
        + mShipPoints.GetPosition(mShipSprings.GetPointBIndex(springElementIndex))) / 2.0f;

    for (auto & bomb : mCurrentBombs)
    {
        // Check if the bomb is within the neighborhood of the disturbed center
        float squareBombDistance = (bomb->GetPosition() - neighborhoodCenter).squareLength();
        if (squareBombDistance < squareNeighborhoodRadius)
        {
            // Tel the bomb that its neighborhood has been disturbed
            bomb->OnNeighborhoodDisturbed();
        }
    }
}

void Bombs::DetonateRCBombs()
{
    for (auto & bomb : mCurrentBombs)
    {
        if (BombType::RCBomb == bomb->GetType())
        {
            RCBomb * rcb = dynamic_cast<RCBomb *>(bomb.get());
            rcb->Detonate();
        }
    }
}

void Bombs::Upload(
    int shipId,
    RenderContext & renderContext) const
{
    renderContext.UploadShipElementBombsStart(
        shipId,
        mCurrentBombs.size());

    for (auto & bomb : mCurrentBombs)
    {
        auto const & position = bomb->GetPosition();

        renderContext.UploadShipElementBomb(
            shipId,
            position.x,
            position.y,            
            bomb->GetRenderScale(),
            bomb->GetType(),            
            bomb->GetRenderFrameIndex(),            
            bomb->GetConnectedComponentId());
    }

    renderContext.UploadShipElementBombsEnd(shipId);
}

}