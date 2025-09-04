// src/features/lock_input/password_manager.h
// Password management for secure unlock

#pragma once
#include <windows.h>
#include <string>

class PasswordManager {
private:
    std::string hashedPassword;
    bool isPasswordSet;
    static const char* REGISTRY_KEY;
    static const char* PASSWORD_VALUE;

public:
    PasswordManager();
    ~PasswordManager();

    // Password operations
    bool SetPassword(const std::string& newPassword);
    bool ValidatePassword(const std::string& inputPassword);
    bool HasPassword() const { return isPasswordSet; }
    void ClearPassword();

    // Registry operations
    bool LoadFromRegistry();
    bool SaveToRegistry();

    // UI helpers
    void InitializePasswordControls(HWND hDialog);
    bool HandlePasswordChange(HWND hDialog, int editControlId);
    bool HandlePasswordValidation(HWND hDialog, int editControlId);

private:
    std::string HashPassword(const std::string& password);
    bool VerifyHash(const std::string& password, const std::string& hash);
};

extern PasswordManager g_passwordManager;
