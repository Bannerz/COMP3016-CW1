#include "SoundEffects.h"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace {
std::vector<float> loadAudio(const std::string& path, const SDL_AudioSpec& output)
{
    SDL_AudioSpec source{};
    Uint8* data = nullptr;
    Uint32 length = 0;
    if (!SDL_LoadWAV(path.c_str(), &source, &data, &length)) {
        SDL_Log("Sound loading failed (%s): %s", path.c_str(), SDL_GetError());
        return {};
    }
    Uint8* converted = nullptr;
    int convertedLength = 0;
    std::vector<float> samples;
    if (SDL_ConvertAudioSamples(&source, data, static_cast<int>(length), &output,
                                &converted, &convertedLength)) {
        samples.resize(static_cast<std::size_t>(convertedLength) / sizeof(float));
        std::memcpy(samples.data(), converted, static_cast<std::size_t>(convertedLength));
    } else {
        SDL_Log("Sound conversion failed (%s): %s", path.c_str(), SDL_GetError());
    }
    SDL_free(converted);
    SDL_free(data);
    return samples;
}
}

SoundEffects::SoundEffects(const std::string& assetRoot)
{
    //A missing audio device should not stop the game from running.
    if (!SDL_InitSubSystem(SDL_INIT_AUDIO)) {
        SDL_Log("Sound unavailable: %s", SDL_GetError());
        return;
    }
    const SDL_AudioSpec output{SDL_AUDIO_F32, 2, sampleRate};
    stream.reset(SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &output, nullptr, nullptr));
    if (!stream) {
        SDL_Log("Sound unavailable: %s", SDL_GetError());
        return;
    }
    constexpr std::array filenames = {"launcher-pull.wav", "launcher-release.wav", "coin-collect.wav",
                                       "trampoline-bounce.wav", "ground-impact.wav", "ground-roll.wav",
                                       "button-hover.wav", "button-click.wav"};
    for (std::size_t i = 0; i < filenames.size(); ++i) {
        samples[i] = loadAudio(assetRoot + "audio/" + filenames[i], {SDL_AUDIO_F32, 1, sampleRate});
    }
    constexpr std::array musicFiles = {"the_mountain-8-bit-retro-522443.wav",
                                        "hitslab-retro-arcade-game-music-396890.wav"};
    for (std::size_t i = 0; i < musicFiles.size(); ++i) {
        musicTracks[i].samples = loadAudio(assetRoot + "audio/" + musicFiles[i], output);
    }
    if (!SDL_ResumeAudioStreamDevice(stream.get())) {
        SDL_Log("Sound playback unavailable: %s", SDL_GetError());
        stream.reset();
    }
}

void SoundEffects::play(Effect effect, float gain, float pitch)
{
    if (!stream || samples[static_cast<std::size_t>(effect)].empty()) return;
    if (voices.size() >= 24) voices.erase(voices.begin());
    voices.push_back({effect, 0.0, std::clamp(gain, 0.0f, 1.0f), 0.0f, std::clamp(pitch, 0.5f, 2.0f)});
}

void SoundEffects::setLoop(Effect effect, float gain, float pitch)
{
    for (auto& loop : loops) {
        if (loop.effect == effect) {
            loop.targetGain = std::clamp(gain, 0.0f, 1.0f);
            loop.pitch = std::clamp(pitch, 0.5f, 2.0f);
        }
    }
}

void SoundEffects::reset()
{
    voices.clear();
    for (auto& loop : loops) {
        loop.position = 0.0;
        loop.gain = loop.targetGain = 0.0f;
    }
    //Keep queued music and its playhead intact when resetting gameplay effects.
}

void SoundEffects::setMusic(Music music)
{
    musicTracks[0].targetGain = music == Music::Menu ? musicVolume : 0.0f;
    musicTracks[1].targetGain = music == Music::Gameplay ? musicVolume : 0.0f;
}

bool SoundEffects::isMusicPlaying() const
{
    return stream && std::any_of(musicTracks.begin(), musicTracks.end(), [](const MusicTrack& track) {
        return !track.samples.empty() && (track.gain > 0.0f || track.targetGain > 0.0f);
    });
}

void SoundEffects::setVolumes(float music, float effects, bool immediate)
{
    targetMusicGain = std::clamp(music, 0.0f, 1.0f);
    targetEffectsGain = std::clamp(effects, 0.0f, 1.0f);
    if (immediate) {
        musicGain = targetMusicGain;
        effectsGain = targetEffectsGain;
    }
}

void SoundEffects::update()
{
    if (!stream) return;
    //Keep at most 40 ms queued. Mixing on the game thread avoids callback races.
    constexpr int bufferFrames = 1920;
    const int queued = SDL_GetAudioStreamQueued(stream.get());
    if (queued < 0) return;
    constexpr int channels = 2;
    const int frames = std::max(0, bufferFrames - queued / (channels * static_cast<int>(sizeof(float))));
    std::array<float, bufferFrames * channels> buffer{};
    const auto sampleVoice = [&](Voice& voice, bool looping) {
        const auto& sound = samples[static_cast<std::size_t>(voice.effect)];
        if (sound.empty() || voice.position >= static_cast<double>(sound.size())) return 0.0f;
        const auto index = static_cast<std::size_t>(voice.position);
        const auto next = looping ? (index + 1) % sound.size() : std::min(index + 1, sound.size() - 1);
        const float fraction = static_cast<float>(voice.position - static_cast<double>(index));
        const float value = (sound[index] + (sound[next] - sound[index]) * fraction) * voice.gain;
        voice.position += voice.pitch;
        if (looping && voice.position >= static_cast<double>(sound.size())) voice.position -= static_cast<double>(sound.size());
        return value;
    };
    for (int frame = 0; frame < frames; ++frame) {
        float value = 0.0f;
        for (auto& voice : voices) value += sampleVoice(voice, false);
        for (auto& loop : loops) {
            //Smooth gain changes over a few milliseconds to avoid clicks.
            loop.gain += (loop.targetGain - loop.gain) * 0.0025f;
            value += sampleVoice(loop, true);
        }
        musicGain += (targetMusicGain - musicGain) * 0.0025f;
        effectsGain += (targetEffectsGain - effectsGain) * 0.0025f;
        if (std::abs(targetMusicGain - musicGain) < 0.00001f) musicGain = targetMusicGain;
        if (std::abs(targetEffectsGain - effectsGain) < 0.00001f) effectsGain = targetEffectsGain;
        float left = value * effectsGain;
        float right = value * effectsGain;
        for (auto& track : musicTracks) {
            if (track.samples.empty()) continue;
            //Sample-based ramps stay smooth even when the game's frame rate changes.
            constexpr float step = musicVolume / (sampleRate * musicFadeSeconds);
            if (track.gain < track.targetGain) track.gain = std::min(track.targetGain, track.gain + step);
            else if (track.gain > track.targetGain) track.gain = std::max(track.targetGain, track.gain - step);
            if (track.gain <= 0.0f) continue; //preserve the playhead while fully faded out
            const std::size_t totalFrames = track.samples.size() / channels;
            //A short edge envelope removes clicks at the file's loop boundary.
            const float edge = std::min({1.0f, static_cast<float>(track.frame) / 960.0f,
                static_cast<float>(totalFrames - 1 - track.frame) / 960.0f});
            left += track.samples[track.frame * channels] * track.gain * edge * musicGain;
            right += track.samples[track.frame * channels + 1] * track.gain * edge * musicGain;
            track.frame = (track.frame + 1) % totalFrames;
        }
        buffer[frame * channels] = 0.9f * std::tanh(left * 0.8f);
        buffer[frame * channels + 1] = 0.9f * std::tanh(right * 0.8f);
    }
    std::erase_if(voices, [&](const Voice& voice) {
        return voice.position >= static_cast<double>(samples[static_cast<std::size_t>(voice.effect)].size());
    });
    if (frames > 0 && !SDL_PutAudioStreamData(stream.get(), buffer.data(), frames * channels * static_cast<int>(sizeof(float)))) {
        SDL_Log("Sound playback failed: %s", SDL_GetError());
        stream.reset();
    }
}
