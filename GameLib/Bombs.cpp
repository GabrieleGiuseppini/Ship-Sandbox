/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-05-23
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "Physics.h"

namespace Physics {

void Bombs::Update()
{
    // Run through all bombs and invoke Update() on each;
    // remove those bombs that have expired
    for (auto it = mCurrentBombs.begin(); it != mCurrentBombs.end(); /* incremented in loop */)
    {
        bool hasExpired = (*it)->Update();
        if (hasExpired)
        {
            it = mCurrentBombs.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

void Bombs::DetonateRCBombs()
{
    // TODO
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