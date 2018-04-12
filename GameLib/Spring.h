﻿/***************************************************************************************
* Original Author:		Luke Wren (wren6991@gmail.com)
* Created:				2013-04-30
* Modified By:			Gabriele Giuseppini
* Copyright:			Luke Wren (http://github.com/Wren6991),
*						Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "IGameEventHandler.h"
#include "Material.h"
#include "GameParameters.h"
#include "Physics.h"
#include "Vectors.h"

namespace Physics
{

class Spring : public ShipElement<Spring>
{
public:

    enum class Characteristics
    {
        Hull    = 1,    // Does not take water
        Rope    = 2     // Ropes are drawn differently
    };

public:

	Spring(
		Ship * parentShip,
		Point * a,
		Point * b,
        Characteristics characteristics,
		Material const * material)
		: Spring(
			parentShip,
			a,
			b,			
			(a->GetPosition() - b->GetPosition()).length(),
            characteristics,
			material)
	{
	}

	Spring(
		Ship * parentShip,
		Point * a,
		Point * b,
		float restLength,
        Characteristics characteristics,
		Material const *material);

    void Destroy(Point const * pointSource);

    /*
     * Calculates the current strain - due to tension or compression - and acts depending on it.
     */
    inline void UpdateStrain(        
        GameParameters const & gameParameters,
        IGameEventHandler * gameEventHandler)
    {
        float const effectiveStrength = gameParameters.StrengthAdjustment * mMaterial->Strength;

        float strain = GetStrain();
        if (strain > effectiveStrength)
        {
            // It's broken!
            this->Destroy(nullptr);

            // Notify
            gameEventHandler->OnBreak(
                mMaterial, 
                GetParentShip()->GetParentWorld()->IsUnderwater(*mPointA, gameParameters),
                1);
        }
        else if (strain > 0.25f * effectiveStrength)
        {
            // It's stressed!
            if (!mIsStressed)
            {
                mIsStressed = true;

                // Notify
                gameEventHandler->OnStress(
                    mMaterial, 
                    GetParentShip()->GetParentWorld()->IsUnderwater(*mPointA, gameParameters),
                    1);
            }
        }
        else
        {
            // Just fine
            mIsStressed = false;
        }
    }

	inline bool IsStressed() const
	{
        return mIsStressed;
	}

    inline Point * GetPointA() { return mPointA; }
	inline Point const * GetPointA() const { return mPointA; }

    inline Point * GetPointB() { return mPointB; }
	inline Point const * GetPointB() const { return mPointB; }

    inline float GetRestLength() const { return mRestLength; }

    inline bool IsHull() const { return 0 != (static_cast<int>(mCharacteristics) & static_cast<int>(Characteristics::Hull)); }
    inline bool IsRope() const { return 0 != (static_cast<int>(mCharacteristics) & static_cast<int>(Characteristics::Rope)); }

	inline Material const * GetMaterial() const { return mMaterial; };


private:

    // Strain: 
    // 0  = no tension nor compression
    // >0 = tension or compression, symmetrical
    inline float GetStrain() const
    {
        float dx = (mPointA->GetPosition() - mPointB->GetPosition()).length();
        return fabs(this->mRestLength - dx) / this->mRestLength;
    }

private:

	Point * const mPointA;
	Point * const mPointB;
	
    float const mRestLength;
    Characteristics const mCharacteristics;
	Material const * const mMaterial;

    // State variable that tracks when we enter and exit the stressed state
    bool mIsStressed;
};

}
