/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-03-08
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "SoundController.h"

#include <GameLib/GameException.h>
#include <GameLib/GameRandomEngine.h>
#include <GameLib/Log.h>
#include <GameLib/Material.h>

#include <cassert>
#include <limits>
#include <regex>

SoundController::SoundController(
    std::shared_ptr<ResourceLoader> resourceLoader,
    ProgressCallback const & progressCallback)
    : mResourceLoader(std::move(resourceLoader))
    , mCurrentVolume(100.0f)
    , mMSUSoundBuffers()
    , mUSoundBuffers()
    , mDrawSoundBuffer()
    , mDrawSound()
    , mCurrentlyPlayingSounds()
    , mSinkingMusic()
{    
    //
    // Initialize Music
    //

    if (!mSinkingMusic.openFromFile(mResourceLoader->GetMusicFilepath("sinking_ship").string()))
    {
        throw GameException("Cannot load \"sinking_ship\" music");
    }

    mSinkingMusic.setLoop(true);
    mSinkingMusic.setVolume(20);


    //
    // Initialize Sounds
    //

    auto soundNames = mResourceLoader->GetSoundNames();
    for (size_t i = 0; i < soundNames.size(); ++i)
    {
        std::string const & soundName = soundNames[i];

        // Notify progress
        progressCallback(static_cast<float>(i + 1) / static_cast<float>(soundNames.size()), "Loading sounds...");
        

        //
        // Load sound buffer
        //

        std::unique_ptr<sf::SoundBuffer> soundBuffer = std::make_unique<sf::SoundBuffer>();
        if (!soundBuffer->loadFromFile(mResourceLoader->GetSoundFilepath(soundName).string()))
        {
            throw GameException("Cannot load sound \"" + soundName + "\"");
        }



        //
        // Parse filename
        //        

        std::regex soundTypeRegex(R"(([^_]+)(?:_.+)?)");
        std::smatch soundTypeMatch;
        if (!std::regex_match(soundName, soundTypeMatch, soundTypeRegex))
        {
            throw GameException("Sound filename \"" + soundName + "\" is not recognized");
        }

        assert(soundTypeMatch.size() == 1 + 1);
        SoundType soundType = StrToSoundType(soundTypeMatch[1].str());
        if (soundType == SoundType::Draw)
        {
            mDrawSoundBuffer = std::move(soundBuffer);
            mDrawSound = std::make_unique<sf::Sound>();
            mDrawSound->setBuffer(*mDrawSoundBuffer);
        }
        else if (soundType == SoundType::Break || soundType == SoundType::Destroy || soundType == SoundType::Stress)
        {
            //
            // MSU sound
            //

            std::regex msuRegex(R"(([^_]+)_([^_]+)_([^_]+)_(?:(underwater)_)?\d+)");
            std::smatch msuMatch;
            if (!std::regex_match(soundName, msuMatch, msuRegex))
            {
                throw GameException("MSU sound filename \"" + soundName + "\" is not recognized");
            }

            assert(msuMatch.size() == 1 + 4);

            // 1. Parse SoundElementType
            Material::SoundProperties::SoundElementType soundElementType = Material::SoundProperties::StrToSoundElementType(msuMatch[2].str());

            // 2. Parse Size
            SizeType sizeType = StrToSizeType(msuMatch[3].str());

            // 3. Parse Underwater
            bool isUnderwater;
            if (msuMatch[4].matched)
            {
                assert(msuMatch[4].str() == "underwater");
                isUnderwater = true;
            }
            else
            {
                isUnderwater = false;
            }


            //
            // Store sound buffer
            //

            mMSUSoundBuffers[std::make_tuple(soundType, soundElementType, sizeType, isUnderwater)]
                .SoundBuffers.emplace_back(std::move(soundBuffer));
        }
        else
        {
            //
            // U sound
            //

            std::regex uRegex(R"(([^_]+)_(?:(underwater)_)?\d+)");
            std::smatch uMatch;
            if (!std::regex_match(soundName, uMatch, uRegex))
            {
                throw GameException("U sound filename \"" + soundName + "\" is not recognized");
            }

            assert(uMatch.size() == 1 + 2);

            // 1. Parse Underwater
            bool isUnderwater;
            if (uMatch[2].matched)
            {
                assert(uMatch[2].str() == "underwater");
                isUnderwater = true;
            }
            else
            {
                isUnderwater = false;
            }


            //
            // Store sound buffer
            //

            mUSoundBuffers[std::make_tuple(soundType, isUnderwater)]
                .SoundBuffers.emplace_back(std::move(soundBuffer));
        }
    }
}

SoundController::~SoundController()
{
    Reset();
}

void SoundController::SetPaused(bool isPaused)
{
    if (!!mDrawSound)
    {
        if (isPaused)
        {
            if (sf::Sound::Status::Playing == mDrawSound->getStatus())
                mDrawSound->pause();
        }
        else
        {
            if (sf::Sound::Status::Paused == mDrawSound->getStatus())
                mDrawSound->play();
        }
    }

    for (auto const & playingSound : mCurrentlyPlayingSounds)
    {
        if (isPaused)
        {
            if (sf::Sound::Status::Playing == playingSound.Sound->getStatus())
                playingSound.Sound->pause();
        }
        else
        {
            if (sf::Sound::Status::Paused == playingSound.Sound->getStatus())
                playingSound.Sound->play();
        }
    }

    if (isPaused)
    {
        if (sf::Sound::Status::Playing == mSinkingMusic.getStatus())
            mSinkingMusic.pause();
    }
    else
    {
        if (sf::Sound::Status::Paused == mSinkingMusic.getStatus())
            mSinkingMusic.play();
    }
}

void SoundController::SetMute(bool isMute)
{
    if (isMute)
        sf::Listener::setGlobalVolume(0.0f);
    else
        sf::Listener::setGlobalVolume(mCurrentVolume);
}

void SoundController::SetVolume(float volume)
{
    mCurrentVolume = volume;
    sf::Listener::setGlobalVolume(mCurrentVolume);
}

void SoundController::HighFrequencyUpdate()
{
}

void SoundController::LowFrequencyUpdate()
{
    //
    // Scavenge stopped sounds
    //

    ScavengeStoppedSounds();
}

void SoundController::Reset()
{
    //
    // Stop music
    //

    mSinkingMusic.stop();


    //
    // Stop and clear all sounds
    //

    for (auto & playingSound : mCurrentlyPlayingSounds)
    {
        assert(!!playingSound.Sound);
        if (sf::Sound::Status::Playing == playingSound.Sound->getStatus())
        {
            playingSound.Sound->stop();
        }
    }

    mCurrentlyPlayingSounds.clear();
}

///////////////////////////////////////////////////////////////////////////////////////

void SoundController::OnDestroy(
    Material const * material,
    bool isUnderwater,
    unsigned int size)
{
    assert(nullptr != material);

    PlayMSUSound(
        SoundType::Destroy, 
        material, 
        size, 
        isUnderwater,
        50.0f);
}

void SoundController::OnDraw()
{
    if (!!mDrawSound && sf::Sound::Status::Paused != mDrawSound->getStatus())
    {
        if (sf::Sound::Status::Playing != mDrawSound->getStatus())
        {
            mDrawSound->play();
        }
        else if (mDrawSound->getPlayingOffset() > sf::seconds(0.7f))
        {
            mDrawSound->setPlayingOffset(sf::seconds(0.1f));
        }
    }
}

void SoundController::OnPinToggled(
    bool isPinned,
    bool isUnderwater)
{
    PlayUSound(
        isPinned ? SoundType::PinPoint : SoundType::UnpinPoint, 
        isUnderwater,
        100.0f);
}

void SoundController::OnStress(
    Material const * material,
    bool isUnderwater,
    unsigned int size)
{
    assert(nullptr != material);

    PlayMSUSound(
        SoundType::Stress, 
        material, 
        size, 
        isUnderwater,
        50.0f);
}

void SoundController::OnBreak(
    Material const * material,
    bool isUnderwater,
    unsigned int size)
{
    assert(nullptr != material);

    PlayMSUSound(
        SoundType::Break, 
        material, 
        size, 
        isUnderwater,
        50.0f);
}

void SoundController::OnSinkingBegin(unsigned int /*shipId*/)
{
    if (sf::SoundSource::Status::Playing != mSinkingMusic.getStatus())
    {
        mSinkingMusic.play();
    }
}

void SoundController::OnBombPlaced(
    BombType /*bombType*/,
    bool isUnderwater) 
{
    PlayUSound(
        SoundType::BombAttached, 
        isUnderwater,
        100.0f);
}

void SoundController::OnBombRemoved(
    BombType /*bombType*/,
    std::optional<bool> isUnderwater)
{
    if (!!isUnderwater)
    {
        PlayUSound(
            SoundType::BombDetached,
            *isUnderwater,
            100.0f);
    }
}

void SoundController::OnBombExplosion(
    bool isUnderwater,
    unsigned int size)
{
    PlayUSound(
        SoundType::Explosion, 
        isUnderwater,
        std::max(
            100.0f,
            30.0f * size));
}

void SoundController::OnRCBombPing(
    bool isUnderwater,
    unsigned int size)
{
    PlayUSound(
        SoundType::RCBombPing, 
        isUnderwater,
        std::max(
            100.0f,
            30.0f * size));
}

///////////////////////////////////////////////////////////////////////////////////////

void SoundController::PlayMSUSound(
    SoundType soundType,
    Material const * material,
    unsigned int size,
    bool isUnderwater,
    float volume)
{
    assert(nullptr != material);

    if (!material->Sound)
        return;

    // Convert size
    SizeType sizeType;
    if (size < 2)
        sizeType = SizeType::Small;
    else if (size < 10)
        sizeType = SizeType::Medium;
    else
        sizeType = SizeType::Large;

    LogMessage("MSUSound: <", 
        static_cast<int>(soundType), 
        ",", 
        static_cast<int>(material->Sound->ElementType), 
        ",", 
        static_cast<int>(sizeType), 
        ",", 
        static_cast<int>(isUnderwater),
        ">");

    //
    // Find vector
    //
    
    auto it = mMSUSoundBuffers.find(std::make_tuple(soundType, material->Sound->ElementType, sizeType, isUnderwater));

    if (it == mMSUSoundBuffers.end())
    {
        // Find a smaller one
        for (int s = static_cast<int>(sizeType) - 1; s >= static_cast<int>(SizeType::Min); --s)
        {
            it = mMSUSoundBuffers.find(std::make_tuple(soundType, material->Sound->ElementType, static_cast<SizeType>(s), isUnderwater));
            if (it != mMSUSoundBuffers.end())
            {
                break;
            }
        }
    }

    if (it == mMSUSoundBuffers.end())
    {
        // Find this or smaller size with different underwater
        for (int s = static_cast<int>(sizeType); s >= static_cast<int>(SizeType::Min); --s)
        {
            it = mMSUSoundBuffers.find(std::make_tuple(soundType, material->Sound->ElementType, static_cast<SizeType>(s), !isUnderwater));
            if (it != mMSUSoundBuffers.end())
            {
                break;
            }
        }
    }

    if (it == mMSUSoundBuffers.end())
    {
        // No luck
        return;
    }

    
    //
    // Play sound
    //

    ChooseAndPlaySound(
        soundType,
        it->second,
        volume);
}

void SoundController::PlayUSound(
    SoundType soundType,
    bool isUnderwater,
    float volume)
{
    LogMessage("USound: <",
        static_cast<int>(soundType),
        ",",
        static_cast<int>(isUnderwater),
        ">");

    //
    // Find vector
    //

    auto it = mUSoundBuffers.find(std::make_tuple(soundType, isUnderwater));
    if (it == mUSoundBuffers.end())
    {
        // Find different underwater
        it = mUSoundBuffers.find(std::make_tuple(soundType, !isUnderwater));
    }

    if (it == mUSoundBuffers.end())
    {
        // No luck
        return;
    }


    //
    // Play sound
    //

    ChooseAndPlaySound(
        soundType,
        it->second,
        volume);
}

void SoundController::ChooseAndPlaySound(
    SoundType soundType,
    SoundInfo & soundInfo,
    float volume)
{
    auto const now = std::chrono::steady_clock::now();

    //
    // Choose sound buffer
    //

    sf::SoundBuffer * chosenSoundBuffer = nullptr;

    assert(!soundInfo.SoundBuffers.empty());
    if (1 == soundInfo.SoundBuffers.size())
    {
        // Nothing to choose
        chosenSoundBuffer = soundInfo.SoundBuffers[0].get();
    }
    else
    {
        assert(soundInfo.SoundBuffers.size() >= 2);

        // Choose randomly, but avoid choosing the last-chosen sound again
        size_t chosenSoundIndex = GameRandomEngine::GetInstance().ChooseNew(
            soundInfo.SoundBuffers.size(), 
            soundInfo.LastPlayedSoundIndex);

        chosenSoundBuffer = soundInfo.SoundBuffers[chosenSoundIndex].get();

        soundInfo.LastPlayedSoundIndex = chosenSoundIndex;
    }

    assert(nullptr != chosenSoundBuffer);

    //
    // Make sure there isn't already a sound with this sound buffer that started
    // playing too recently;
    // if there is, adjust its volume
    //

    for (auto const & currentlyPlayingSound : mCurrentlyPlayingSounds)
    {
        assert(!!currentlyPlayingSound.Sound);
        if (currentlyPlayingSound.Sound->getBuffer() == chosenSoundBuffer
            && std::chrono::duration_cast<std::chrono::milliseconds>(now - currentlyPlayingSound.StartedTimestamp) < MinDeltaTimeSound)
        {
            currentlyPlayingSound.Sound->setVolume(
                std::max(
                    100.0f,
                    currentlyPlayingSound.Sound->getVolume() + volume));

            return;
        }
    }


    //
    // Make sure there's room for this sound
    //

    if (mCurrentlyPlayingSounds.size() >= MaxPlayingSounds)
    {
        ScavengeStoppedSounds();

        if (mCurrentlyPlayingSounds.size() >= MaxPlayingSounds)
        { 
            // Need to stop sound that's been playing for the longest
            ScavengeOldestSound(soundType);
        }
    }

    assert(mCurrentlyPlayingSounds.size() < MaxPlayingSounds);



    //
    // Create and play sound
    //

    std::unique_ptr<sf::Sound> sound = std::make_unique<sf::Sound>();
    sound->setBuffer(*chosenSoundBuffer);

    sound->setVolume(volume);
    sound->play();    

    mCurrentlyPlayingSounds.emplace_back(
        soundType,
        std::move(sound),
        now);
}

void SoundController::ScavengeStoppedSounds()
{
    for (size_t i = 0; i < mCurrentlyPlayingSounds.size(); /*incremented in loop*/)
    {
        assert(!!mCurrentlyPlayingSounds[i].Sound);
        if (sf::Sound::Status::Stopped == mCurrentlyPlayingSounds[i].Sound->getStatus())
        {
            // Scavenge
            mCurrentlyPlayingSounds.erase(mCurrentlyPlayingSounds.begin() + i);
        }
        else
        {
            ++i;
        }
    }
}

void SoundController::ScavengeOldestSound(SoundType soundType)
{
    assert(!mCurrentlyPlayingSounds.empty());

    size_t iOldestSound = std::numeric_limits<size_t>::max();
    auto oldestSoundStartTimestamp = std::chrono::steady_clock::time_point::max();
    size_t iOldestSoundForType = std::numeric_limits<size_t>::max();
    auto oldestSoundForTypeStartTimestamp = std::chrono::steady_clock::time_point::max();    
    for (size_t i = 0; i < mCurrentlyPlayingSounds.size(); ++i)
    {
        if (mCurrentlyPlayingSounds[i].StartedTimestamp < oldestSoundStartTimestamp)
        {
            iOldestSound = i;
            oldestSoundStartTimestamp = mCurrentlyPlayingSounds[i].StartedTimestamp;
        }

        if (mCurrentlyPlayingSounds[i].StartedTimestamp < oldestSoundForTypeStartTimestamp
            && mCurrentlyPlayingSounds[i].Type == soundType)
        {
            iOldestSoundForType = i;
            oldestSoundForTypeStartTimestamp = mCurrentlyPlayingSounds[i].StartedTimestamp;
        }
    }

    size_t iStop;
    if (oldestSoundForTypeStartTimestamp != std::chrono::steady_clock::time_point::max())
    {
        iStop = iOldestSoundForType;
    }
    else 
    {
        assert((oldestSoundStartTimestamp != std::chrono::steady_clock::time_point::max()));
        iStop = iOldestSound;
    }

    assert(!!mCurrentlyPlayingSounds[iStop].Sound);
    mCurrentlyPlayingSounds[iStop].Sound->stop();
    mCurrentlyPlayingSounds.erase(mCurrentlyPlayingSounds.begin() + iStop);
}
