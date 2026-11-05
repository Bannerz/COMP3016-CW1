#pragma once

#include <SDL3/SDL.h>
#include <array>
#include <cstddef>
#include <memory>
#include <string>
#include <vector>

//Mix short effects, responsive loops and crossfading music into one stereo stream.
class SoundEffects {
public:
    enum class Effect : std::size_t { Pull, Release, Coin, Trampoline, Impact, Roll, ButtonHover, ButtonClick, Count };
    enum class Music { None, Menu, Gameplay };
    explicit SoundEffects(const std::string& assetRoot);
    void play(Effect effect, float gain = 1.0f, float pitch = 1.0f);
    void setLoop(Effect effect, float gain, float pitch = 1.0f);
    void update();
    void reset();
    void setMusic(Music music);
    bool isMusicPlaying() const;
    void setVolumes(float music, float effects, bool immediate = false);

private:
    struct Voice {
        Effect effect;
        double position = 0.0;
        float gain = 0.0f;
        float targetGain = 0.0f;
        float pitch = 1.0f;
    };
    struct MusicTrack {
        std::vector<float> samples; //interleaved stereo at the mixer sample rate
        std::size_t frame = 0;
        float gain = 0.0f;
        float targetGain = 0.0f;
    };
    static constexpr int sampleRate = 48000;
    using StreamPtr = std::unique_ptr<SDL_AudioStream, decltype(&SDL_DestroyAudioStream)>;
    StreamPtr stream{nullptr, SDL_DestroyAudioStream};
    std::array<std::vector<float>, static_cast<std::size_t>(Effect::Count)> samples;
    std::vector<Voice> voices;
    std::array<Voice, 2> loops{{{Effect::Pull}, {Effect::Roll}}};
    std::array<MusicTrack, 2> musicTracks;
    float musicGain = 1.0f;
    float effectsGain = 1.0f;
    float targetMusicGain = 1.0f;
    float targetEffectsGain = 1.0f;
    static constexpr float musicVolume = 0.32f;
    static constexpr float musicFadeSeconds = 0.8f;
};
