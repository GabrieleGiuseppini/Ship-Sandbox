/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-05-23
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "Physics.h"

#include "GameRandomEngine.h"

namespace Physics {

TimerBomb::TimerBomb(
    ObjectId id,
    ElementIndex springIndex,
    World & parentWorld,
    std::shared_ptr<IGameEventHandler> gameEventHandler,
    BlastHandler blastHandler,
    Points & shipPoints,
    Springs & shipSprings)
    : Bomb(
        id,
        BombType::TimerBomb,
        springIndex,
        parentWorld,
        std::move(gameEventHandler),
        blastHandler,
        shipPoints,
        shipSprings)
    , mState(State::SlowFuseBurning)
    , mNextStateTransitionTimePoint(GameWallClock::GetInstance().Now() + SlowFuseToDetonationLeadInInterval/FuseLengthStepsCount)
    , mFuseLength(0)
    , mFuseFlameFrameIndex(0)
{
    // Start slow fuse
    mGameEventHandler->OnTimerBombSlowFuseStart(
        mId,
        mParentWorld.IsUnderwater(GetPosition()));
}

bool TimerBomb::Update(
    GameWallClock::time_point now,
    GameParameters const & gameParameters)
{
    switch (mState)
    {
        case State::SlowFuseBurning:
        {
            if (now > mNextStateTransitionTimePoint)
            {
                // Check if we're done
                if (mFuseLength == FuseLengthStepsCount - 1)
                {
                    //
                    // Transition to DetonationLeadIn state
                    //

                    mState = State::DetonationLeadIn;

                    mGameEventHandler->OnTimerBombFuseStop(mId);

                    // Schedule next transition
                    mNextStateTransitionTimePoint = now + DetonationLeadInToExplosionInterval;
                }
                else
                {
                    // Shorten the fuse length
                    ++mFuseLength;

                    // Schedule next transition
                    mNextStateTransitionTimePoint = now + SlowFuseToDetonationLeadInInterval / FuseLengthStepsCount;
                }
            }

            // Calculate next fuse flame frame index
            mFuseFlameFrameIndex = GameRandomEngine::GetInstance().ChooseNew<uint32_t>(FuseFramesPerLevelCount, mFuseFlameFrameIndex);

            return true;
        }

        // TODO: other states

        default:
        {
            return true;
        }
    }

    assert(false);
    return true;
}

void TimerBomb::OnNeighborhoodDisturbed()
{
    // TODO
}

void TimerBomb::Upload(
    int shipId,
    RenderContext & renderContext) const
{
    switch (mState)
    {
        case State::SlowFuseBurning:
        {
            renderContext.UploadShipElementBomb(
                shipId,
                BombType::TimerBomb,
                RotatedTextureRenderInfo(
                    GetPosition(),
                    1.0f,
                    mRotationBaseAxis,
                    GetRotationOffsetAxis()),
                mFuseLength,                                   // Base frame
                FuseLengthStepsCount + mFuseFlameFrameIndex,   // Fuse frame
                GetConnectedComponentId());

            break;
        }

        // TODO: other states

        case State::Expired:
        {
            // No drawing
            break;
        }
    }
}

}