// src/settings/password_manager.cpp
// Password handling and validation implementation

#include "password_manager.h"
#include "../resource.h"
#include <sstream>
#include <iomanip>
#include <algorithm>

// Global instance
PasswordManager g_passwordManager;

// Registry constants
const char* PasswordManager::REGISTRY_KEY = "SOFTWARE\\UtilityApp";
const char* PasswordManager::PASSWORD_VALUE = "PasswordHash";

PasswordManager::PasswordManager() : isPasswordSet(false) {
    LoadFromRegistry();
}

PasswordManager::~PasswordManager() {
    // Secure cleanup
    hashedPassword.clear();
}

bool PasswordManager::SetPassword(const std::string& newPassword) {
    if (newPassword.empty()) {
        ClearPassword();
        return true;
    }

    hashedPassword = HashPassword(newPassword);
    isPasswordSet = true;
    return SaveToRegistry();
}

bool PasswordManager::ValidatePassword(const std::string& inputPassword) {
    if (!isPasswordSet) return true; // No password set
    return VerifyHash(inputPassword, hashedPassword);
}

void PasswordManager::ClearPassword() {
    hashedPassword.clear();
    isPasswordSet = false;
    SaveToRegistry();
}

bool PasswordManager::LoadFromRegistry() {
    HKEY hKey;
    if (RegOpenKeyExA(HKEY_CURRENT_USER, REGISTRY_KEY, 0, KEY_READ, &hKey) != ERROR_SUCCESS) {
        return false; // No saved password
    }

    char buffer[256];
    DWORD bufferSize = sizeof(buffer);
    DWORD type;

    if (RegQueryValueExA(hKey, PASSWORD_VALUE, NULL, &type, (BYTE*)buffer, &bufferSize) == ERROR_SUCCESS) {
        if (type == REG_SZ && bufferSize > 1) {
            hashedPassword = std::string(buffer);
            isPasswordSet = true;
        }
    }

    RegCloseKey(hKey);
    return isPasswordSet;
}

bool PasswordManager::SaveToRegistry() {
    HKEY hKey;
    if (RegCreateKeyExA(HKEY_CURRENT_USER, REGISTRY_KEY, 0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL) != ERROR_SUCCESS) {
        return false;
    }

    LONG result;
    if (isPasswordSet && !hashedPassword.empty()) {
        result = RegSetValueExA(hKey, PASSWORD_VALUE, 0, REG_SZ, 
                               (const BYTE*)hashedPassword.c_str(), 
                               hashedPassword.length() + 1);
    } else {
        result = RegDeleteValueA(hKey, PASSWORD_VALUE);
        if (result == ERROR_FILE_NOT_FOUND) result = ERROR_SUCCESS; // OK if not found
    }

    RegCloseKey(hKey);
    return result == ERROR_SUCCESS;
}

void PasswordManager::InitializePasswordControls(HWND hDialog) {
    // Set placeholder text or current status
    if (isPasswordSet) {
        SetDlgItemTextA(hDialog, IDC_EDIT_PASSWORD, "••••••••");
        EnableWindow(GetDlgItem(hDialog, IDC_BUTTON_CLEAR_PASSWORD), TRUE);
    } else {
        SetDlgItemTextA(hDialog, IDC_EDIT_PASSWORD, "");
        EnableWindow(GetDlgItem(hDialog, IDC_BUTTON_CLEAR_PASSWORD), FALSE);
    }
}

bool PasswordManager::HandlePasswordChange(HWND hDialog, int editControlId) {
    char buffer[256];
    GetDlgItemTextA(hDialog, editControlId, buffer, sizeof(buffer));
    
    std::string newPassword(buffer);
    bool success = SetPassword(newPassword);
    
    // Clear the edit control for security
    SetDlgItemTextA(hDialog, editControlId, "");
    
    // Update UI
    InitializePasswordControls(hDialog);
    
    return success;
}

bool PasswordManager::HandlePasswordValidation(HWND hDialog, int editControlId) {
    char buffer[256];
    GetDlgItemTextA(hDialog, editControlId, buffer, sizeof(buffer));
    
    std::string inputPassword(buffer);
    bool valid = ValidatePassword(inputPassword);
    
    // Clear the edit control for security
    SetDlgItemTextA(hDialog, editControlId, "");
    
    return valid;
}

std::string PasswordManager::HashPassword(const std::string& password) {
    // Use Windows CryptoAPI for secure hashing
    HCRYPTPROV hProv = 0;
    HCRYPTHASH hHash = 0;
    
    if (!CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) {
        return "";
    }
    
    if (!CryptCreateHash(hProv, CALG_SHA_256, 0, 0, &hHash)) {
        CryptReleaseContext(hProv, 0);
        return "";
    }
    
    if (!CryptHashData(hHash, (const BYTE*)password.c_str(), password.length(), 0)) {
        CryptDestroyHash(hHash);
        CryptReleaseContext(hProv, 0);
        return "";
    }
    
    DWORD hashSize = 0;
    DWORD dataSize = sizeof(DWORD);
    CryptGetHashParam(hHash, HP_HASHSIZE, (BYTE*)&hashSize, &dataSize, 0);
    
    BYTE* hashData = new BYTE[hashSize];
    CryptGetHashParam(hHash, HP_HASHVAL, hashData, &hashSize, 0);
    
    // Convert to hex string
    std::stringstream ss;
    for (DWORD i = 0; i < hashSize; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hashData[i];
    }
    
    delete[] hashData;
    CryptDestroyHash(hHash);
    CryptReleaseContext(hProv, 0);
    
    return ss.str();
}

bool PasswordManager::VerifyHash(const std::string& password, const std::string& hash) {
    return HashPassword(password) == hash;
}
