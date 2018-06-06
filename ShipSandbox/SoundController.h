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
        ObjectId bombId,
        BombType bombType,
        bool isUnderwater) override;

    virtual void OnBombRemoved(
        ObjectId bombId,
        BombType bombType,
        std::optional<bool> isUnderwater) override;

    virtual void OnBombExplosion(
        bool isUnderwater,
        unsigned int size) override;

    virtual void OnRCBombPing(
        bool isUnderwater,
        unsigned int size) override;

    virtual void OnTimerBombSlowFuseStart(
        ObjectId bombId,
        bool isUnderwater) override;

    virtual void OnTimerBombFastFuseStart(
        ObjectId bombId,
        bool isUnderwater) override;

    virtual void OnTimerBombFuseStop(
        ObjectId bombId) override;

    virtual void OnTimerBombDefused(
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
        TimerBombSlowFuse,
        TimerBombFastFuse,
        TimerBombDefused,
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
        else if (lstr == "timerbombslowfuse")
            return SoundType::TimerBombSlowFuse;
        else if (lstr == "timerbombfastfuse")
            return SoundType::TimerBombFastFuse;
        else if (lstr == "timerbombdefused")
            return SoundType::TimerBombDefused;
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

    struct MultipleSoundChoiceInfo
    {
        std::vector<std::unique_ptr<sf::SoundBuffer>> SoundBuffers;
        size_t LastPlayedSoundIndex;

        MultipleSoundChoiceInfo()
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

    struct SingleContinuousSound
    {
        std::unique_ptr<sf::SoundBuffer> SoundBuffer;
        std::unique_ptr<sf::Sound> Sound;

        SingleContinuousSound()
            : SoundBuffer()
            , Sound()
        {
        }

        void Initialize(std::unique_ptr<sf::SoundBuffer> soundBuffer)
        {
            assert(!SoundBuffer && !Sound);

            SoundBuffer = std::move(soundBuffer);
            Sound = std::make_unique<sf::Sound>();
            Sound->setBuffer(*SoundBuffer);
            Sound->setLoop(true);
        }

        void Start()
        {
            if (!!Sound)
            {
                if (sf::Sound::Status::Playing != Sound->getStatus())
                    Sound->play();
            }
        }

        void SetPaused(bool isPaused)
        {
            if (!!Sound)
            {
                if (isPaused)
                {
                    if (sf::Sound::Status::Playing == Sound->getStatus())
                        Sound->pause();
                }
                else
                {
                    if (sf::Sound::Status::Paused == Sound->getStatus())
                        Sound->play();
                }
            }
        }

        void Stop()
        {
            if (!!Sound)
            {
                if (sf::Sound::Status::Stopped != Sound->getStatus())
                    Sound->stop();
            }
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
        MultipleSoundChoiceInfo & multipleSoundChoiceInfo,
        float volume);

    void ScavengeStoppedSounds();

    void ScavengeOldestSound(SoundType soundType);    


private:

    std::shared_ptr<ResourceLoader> mResourceLoader;

    float mCurrentVolume;


    //
    // State
    //

    bool mIsInDraw;


    //
    // One-Shot sounds
    //

    static constexpr size_t MaxPlayingSounds{ 100 };
    static constexpr std::chrono::milliseconds MinDeltaTimeSound{ 100 };

    unordered_tuple_map<
        std::tuple<SoundType, Material::SoundProperties::SoundElementType, SizeType, bool>,
        MultipleSoundChoiceInfo> mMSUSoundBuffers;

    unordered_tuple_map<
        std::tuple<SoundType, bool>,
        MultipleSoundChoiceInfo> mUSoundBuffers;

    std::vector<PlayingSound> mCurrentlyPlayingSounds;

    //
    // Continuous sounds
    //

    SingleContinuousSound mDrawSound;
    SingleContinuousSound mTimerBombSlowFuseSound;
    SingleContinuousSound mTimerBombFastFuseSound;


    //
    // Music
    //

    sf::Music mSinkingMusic;
};
