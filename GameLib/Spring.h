/***************************************************************************************
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

    enum class Characteristics : uint8_t
    {
        None    = 0,
        Hull    = 1,    // Does not take water
        Rope    = 2     // Ropes are drawn differently
    };

public:

	Spring(
		Ship * parentShip,
        Points & points,
        ElementContainer::ElementIndex pointAIndex,
        ElementContainer::ElementIndex pointBIndex,
        Characteristics characteristics,
		Material const * material)
		: Spring(
			parentShip,
            points,
            pointAIndex,
            pointBIndex,
			(points.GetPosition(pointAIndex) - points.GetPosition(pointBIndex)).length(),
            characteristics,
			material)
	{
	}

	Spring(
		Ship * parentShip,
        Points & points,
        ElementContainer::ElementIndex pointAIndex,
        ElementContainer::ElementIndex pointBIndex,
        float restLength,
        Characteristics characteristics,
		Material const *material);

    void Destroy(
        Points & points, 
        ElementContainer::ElementIndex sourcePointElementIndex);

    /*
     * Calculates the current strain - due to tension or compression - and acts depending on it.
     *
     * Returns true if the spring got broken.
     */
    inline bool UpdateStrain(        
        GameParameters const & gameParameters,
        Points & points,
        IGameEventHandler * gameEventHandler)
    {
        float const strain = CalculateStrain(points);
        float const effectiveStrength = gameParameters.StrengthAdjustment * mMaterial->Strength;
        if (strain > effectiveStrength)
        {
            // It's broken!
            this->Destroy(points, ElementContainer::NoneElementIndex);

            // Notify
            gameEventHandler->OnBreak(
                mMaterial, 
                GetParentShip()->GetParentWorld()->IsUnderwater(points.GetPosition(mPointAIndex)),
                1);

            return true;
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
                    GetParentShip()->GetParentWorld()->IsUnderwater(points.GetPosition(mPointAIndex)),
                    1);
            }
        }
        else
        {
            // Just fine
            mIsStressed = false;
        }

        return false;
    }

	inline bool IsStressed() const
	{
        return mIsStressed;
	}

    inline ElementContainer::ElementIndex GetPointAIndex() const { return mPointAIndex; }
    inline ElementContainer::ElementIndex GetPointBIndex() const { return mPointBIndex; }

    inline float GetRestLength() const { return mRestLength; }
    inline float GetStiffnessCoefficient() const { return mStiffnessCoefficient; }
    inline float GetDampingCoefficient() const { return mDampingCoefficient; }

    inline bool IsHull() const;
    inline bool IsRope() const;

	inline Material const * GetMaterial() const { return mMaterial; };

    inline float GetWaterPermeability() const { return mWaterPermeability; }

private:

    static float CalculateStiffnessCoefficient(
        Points const & points,
        ElementContainer::ElementIndex pointAIndex,
        ElementContainer::ElementIndex pointBIndex,
        float springStiffness);

    static float CalculateDampingCoefficient(
        Points const & points,
        ElementContainer::ElementIndex pointAIndex,
        ElementContainer::ElementIndex pointBIndex);

    // Strain: 
    // 0  = no tension nor compression
    // >0 = tension or compression, symmetrical
    inline float CalculateStrain(Points const & points) const
    {
        float dx = (points.GetPosition(mPointAIndex) - points.GetPosition(mPointBIndex)).length();
        return fabs(this->mRestLength - dx) / this->mRestLength;
    }

private:

    ElementContainer::ElementIndex const mPointAIndex;
    ElementContainer::ElementIndex const mPointBIndex;

    float const mRestLength;
    float mStiffnessCoefficient;
    float mDampingCoefficient;

    Characteristics const mCharacteristics;    
	Material const * const mMaterial;

    // Water propagates through this spring according to this value;
    // 0.0 make water not propagate
    float mWaterPermeability;

    // State variable that tracks when we enter and exit the stressed state
    bool mIsStressed;
};

}

constexpr Physics::Spring::Characteristics operator&(Physics::Spring::Characteristics a, Physics::Spring::Characteristics b)
{
    return static_cast<Physics::Spring::Characteristics>(static_cast<int>(a) & static_cast<int>(b));
}

inline bool Physics::Spring::IsHull() const 
{ 
    return Physics::Spring::Characteristics::None != (mCharacteristics & Physics::Spring::Characteristics::Hull); 
}

inline bool Physics::Spring::IsRope() const 
{ 
    return Physics::Spring::Characteristics::None != (mCharacteristics & Physics::Spring::Characteristics::Rope); 
}
