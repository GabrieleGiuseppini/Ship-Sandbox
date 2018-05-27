/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-03-08
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <GameLib/IGameEventHandler.h>
#include <GameLib/ResourceLoader.h>
#include <GameLib/TupleKeys.h>
#include <GameLib/Utils.h>

#include <SFML/Audio.hpp>

#include <chrono>
#include <memory>
#include <string>
#include <vector>

class SoundController : public IGameEventHandler
{
public:

    SoundController(
        std::shared_ptr<ResourceLoader> resourceLoader,
        ProgressCallback const & progressCallback);

	virtual ~SoundController();

    void SetPaused(bool isPaused);

    void SetMute(bool isMute);

    void SetVolume(float volume);

    void HighFrequencyUpdate();

    void LowFrequencyUpdate();

    void Reset();

public:

    //
    // Game event handlers
    //

    virtual void OnDestroy(
        Material const * material,
        bool isUnderwater,
        unsigned int size) override;

    virtual void OnDraw() override;

    virtual void OnPinToggled(
        bool isPinned,
        bool isUnderwater) override;

    virtual void OnStress(
        Material const * material,
        bool isUnderwater,
        unsigned int size) override;

    virtual void OnBreak(
        Material const * material,
        bool isUnderwater,
        unsigned int size) override;

    virtual void OnSinkingBegin(unsigned int shipId) override;

    virtual void OnBombPlaced(
        BombType bombType,
        bool isUnderwater) override;

    virtual void OnBombRemoved(
        BombType bombType,
        std::optional<bool> isUnderwater) override;

    virtual void OnBombExplosion(
        bool isUnderwater,
        unsigned int size) override;

    virtual void OnRCBombPing(
        bool isUnderwater,
        unsigned int size) override;

private:

    enum class SoundType
    {
        Break,
        Destroy,
        PinPoint,
        UnpinPoint,
        Draw,
        Stress,
        BombAttached,
        BombDetached,
        RCBombPing,
        Explosion
    };

    static SoundType StrToSoundType(std::string const & str)
    {
        std::string lstr = Utils::ToLower(str);

        if (lstr == "break")
            return SoundType::Break;
        else if (lstr == "destroy")
            return SoundType::Destroy;
        else if (lstr == "draw")
            return SoundType::Draw;
        else if (lstr == "pinpoint")
            return SoundType::PinPoint;
        else if (lstr == "unpinpoint")
            return SoundType::UnpinPoint;
        else if (lstr == "stress")
            return SoundType::Stress;
        else if (lstr == "bombattached")
            return SoundType::BombAttached;
        else if (lstr == "bombdetached")
            return SoundType::BombDetached;
        else if (lstr == "rcbombping")
            return SoundType::RCBombPing;
        else if (lstr == "explosion")
            return SoundType::Explosion;
        else
            throw GameException("Unrecognized SoundType \"" + str + "\"");
    }

    enum class SizeType : int
    {
        Min = 0,

        Small = 0,
        Medium = 1,
        Large = 2,

        Max = 2
    };

    static SizeType StrToSizeType(std::string const & str)
    {
        std::string lstr = Utils::ToLower(str);

        if (lstr == "small")
            return SizeType::Small;
        else if (lstr == "medium")
            return SizeType::Medium;
        else if (lstr == "large")
            return SizeType::Large;
        else
            throw GameException("Unrecognized SizeType \"" + str + "\"");
    }

private:

    struct SoundInfo
    {
        std::vector<std::unique_ptr<sf::SoundBuffer>> SoundBuffers;
        size_t LastPlayedSoundIndex;

        SoundInfo()
            : SoundBuffers()
            , LastPlayedSoundIndex(0u)
        {
        }
    };

    struct PlayingSound
    {
        SoundType Type;
        std::unique_ptr<sf::Sound> Sound;
        std::chrono::steady_clock::time_point StartedTimestamp;

        PlayingSound(
            SoundType type,
            std::unique_ptr<sf::Sound> sound,
            std::chrono::steady_clock::time_point startedTimestamp)
            : Type(type)
            , Sound(std::move(sound))
            , StartedTimestamp(startedTimestamp)
        {
        }
    };

private:

    void PlayMSUSound(
        SoundType soundType,
        Material const * material,
        unsigned int size,
        bool isUnderwater,
        float volume);

    void PlayUSound(
        SoundType soundType,
        bool isUnderwater,
        float volume);

    void ChooseAndPlaySound(
        SoundType soundType,
        SoundInfo & soundInfo,
        float volume);

    void ScavengeStoppedSounds();

    void ScavengeOldestSound(SoundType soundType);    

private:

    std::shared_ptr<ResourceLoader> mResourceLoader;

    float mCurrentVolume;

    //
    // Sounds
    //

    static constexpr size_t MaxPlayingSounds{ 100 };
    static constexpr std::chrono::milliseconds MinDeltaTimeSound{ 100 };

    unordered_tuple_map<
        std::tuple<SoundType, Material::SoundProperties::SoundElementType, SizeType, bool>,
        SoundInfo> mMSUSoundBuffers;

    unordered_tuple_map<
        std::tuple<SoundType, bool>,
        SoundInfo> mUSoundBuffers;

    std::unique_ptr<sf::SoundBuffer> mDrawSoundBuffer;
    std::unique_ptr<sf::Sound> mDrawSound;

    std::vector<PlayingSound> mCurrentlyPlayingSounds;

    //
    // Music
    //

    sf::Music mSinkingMusic;
};
