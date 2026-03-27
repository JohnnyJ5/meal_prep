#pragma once
#include <string>

// AES-256-GCM encryption for OAuth tokens stored in SQLite.
// Key is read from the MEAL_PREP_TOKEN_KEY environment variable (64 hex chars = 32 bytes).
// If the env var is not set, tokens are stored/returned as plaintext with a warning.
// Encrypted values are prefixed with "ENC:" to distinguish them from legacy plaintext.
namespace TokenEncryption {
std::string encrypt(const std::string &plaintext);
std::string decrypt(const std::string &stored);
} // namespace TokenEncryption
