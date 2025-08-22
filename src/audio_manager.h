// src/audio_manager.h
// Simple audio manager for notification sounds

#pragma once
#include <windows.h>
#include <string>

enum NotificationSoundType {
    SOUND_WORK_BREAK = 0,
    SOUND_USB_DEVICE = 1
};

class AudioManager {
private:
    static AudioManager* instance;
    std::string audioPath;
    bool audioEnabled;
    
public:
    AudioManager();
    ~AudioManager();
    
    static AudioManager* GetInstance();
    
    // Initialize audio system
    void Initialize();
    
    // Play notification sound
    void PlayNotificationSound(NotificationSoundType soundType);
    
    // Enable/disable audio
    void SetAudioEnabled(bool enabled) { audioEnabled = enabled; }
    bool IsAudioEnabled() const { return audioEnabled; }
    
    // Set audio file path
    void SetAudioPath(const std::string& path) { audioPath = path; }
};

// Global audio manager instance
extern AudioManager* g_audioManager;

// Helper functions
void InitializeAudio();
void CleanupAudio();
void PlayNotificationSound(NotificationSoundType soundType);
