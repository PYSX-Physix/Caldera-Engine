#pragma once
#include <string>
#include <unordered_map>
#include <vector>

class AudioClip {
public:
    AudioClip(const std::string& filePath);
    ~AudioClip();

    void Play();
    void Stop();
    void SetVolume(float volume);
    float GetVolume() const;

private:
    std::string filePath;
    float volume;
};

class AudioManager {
public:
    AudioManager();
    ~AudioManager();

    void Initialize();
    void Shutdown();

    void PlaySound(const std::string& soundName);
    void StopSound(const std::string& soundName);
    void SetSoundVolume(const std::string& soundName, float volume);

private:
    std::unordered_map<std::string, AudioClip*> audioClips;
    std::vector<AudioClip*> activeClips;
};
