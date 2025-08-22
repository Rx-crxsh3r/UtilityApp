// src/audio_manager.cpp
// Simple audio manager implementation

#include "audio_manager.h"
#include <mmsystem.h>

// Global instance
AudioManager* AudioManager::instance = nullptr;
AudioManager* g_audioManager = nullptr;

AudioManager::AudioManager() : audioEnabled(true) {
    // Default to assets/notif.wav
    audioPath = "resources\\notif.wav";
}

AudioManager::~AudioManager() {
    instance = nullptr;
}

AudioManager* AudioManager::GetInstance() {
    if (!instance) {
        instance = new AudioManager();
    }
    return instance;
}

void AudioManager::Initialize() {
    // Check if audio file exists
    DWORD fileAttributes = GetFileAttributesA(audioPath.c_str());
    if (fileAttributes == INVALID_FILE_ATTRIBUTES) {
        // File doesn't exist, disable audio
        audioEnabled = false;
    }
}

void AudioManager::PlayNotificationSound(NotificationSoundType soundType) {
    if (!audioEnabled) return;
    
    // For both work/break and USB device notifications, we use the same sound file
    // You can extend this to use different sounds for different types if needed
    
    // Play the sound asynchronously using PlaySound
    PlaySoundA(audioPath.c_str(), NULL, SND_FILENAME | SND_ASYNC | SND_NODEFAULT);
}

// Helper functions
void InitializeAudio() {
    if (!g_audioManager) {
        g_audioManager = AudioManager::GetInstance();
        g_audioManager->Initialize();
    }
}

void CleanupAudio() {
    if (g_audioManager) {
        delete g_audioManager;
        g_audioManager = nullptr;
    }
}

void PlayNotificationSound(NotificationSoundType soundType) {
    if (g_audioManager) {
        g_audioManager->PlayNotificationSound(soundType);
    }
}
