#include "token_encryption.h"

#include <openssl/evp.h>
#include <openssl/rand.h>

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <vector>

namespace {

static const int IV_LEN = 12;
static const int TAG_LEN = 16;
static const int KEY_LEN = 32;
static const std::string ENC_PREFIX = "ENC:";

// Simple base64 encode/decode (RFC 4648, no line wrapping)
static const std::string BASE64_CHARS =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

std::string base64Encode(const unsigned char *data, size_t len) {
    std::string ret;
    int i = 0;
    unsigned char char3[3], char4[4];
    while (len--) {
        char3[i++] = *(data++);
        if (i == 3) {
            char4[0] = (char3[0] & 0xfc) >> 2;
            char4[1] = ((char3[0] & 0x03) << 4) + ((char3[1] & 0xf0) >> 4);
            char4[2] = ((char3[1] & 0x0f) << 2) + ((char3[2] & 0xc0) >> 6);
            char4[3] = char3[2] & 0x3f;
            for (unsigned char j : char4) ret += BASE64_CHARS[j];
            i = 0;
        }
    }
    if (i) {
        for (int j = i; j < 3; j++) char3[j] = '\0';
        char4[0] = (char3[0] & 0xfc) >> 2;
        char4[1] = ((char3[0] & 0x03) << 4) + ((char3[1] & 0xf0) >> 4);
        char4[2] = ((char3[1] & 0x0f) << 2) + ((char3[2] & 0xc0) >> 6);
        for (int j = 0; j < i + 1; j++) ret += BASE64_CHARS[char4[j]];
        while (i++ < 3) ret += '=';
    }
    return ret;
}

std::vector<unsigned char> base64Decode(const std::string &encoded) {
    std::vector<unsigned char> ret;
    int i = 0;
    size_t in_ = 0;
    unsigned char char3[3], char4[4];
    auto isBase64 = [](unsigned char c) -> bool { return isalnum(c) || c == '+' || c == '/'; };
    size_t inLen = encoded.size();
    while (inLen-- && encoded[in_] != '=' && isBase64(encoded[in_])) {
        char4[i++] = encoded[in_++];
        if (i == 4) {
            for (unsigned char &j : char4) {
                auto pos = BASE64_CHARS.find(j);
                j = (pos == std::string::npos) ? 0 : static_cast<unsigned char>(pos);
            }
            char3[0] = (char4[0] << 2) + ((char4[1] & 0x30) >> 4);
            char3[1] = ((char4[1] & 0x0f) << 4) + ((char4[2] & 0x3c) >> 2);
            char3[2] = ((char4[2] & 0x03) << 6) + char4[3];
            for (unsigned char &j : char3) ret.push_back(j);
            i = 0;
        }
    }
    if (i) {
        for (int j = i; j < 4; j++) char4[j] = 0;
        for (unsigned char &j : char4) {
            auto pos = BASE64_CHARS.find(j);
            j = (pos == std::string::npos) ? 0 : static_cast<unsigned char>(pos);
        }
        char3[0] = (char4[0] << 2) + ((char4[1] & 0x30) >> 4);
        char3[1] = ((char4[1] & 0x0f) << 4) + ((char4[2] & 0x3c) >> 2);
        char3[2] = ((char4[2] & 0x03) << 6) + char4[3];
        for (int j = 0; j < i - 1; j++) ret.push_back(char3[j]);
    }
    return ret;
}

// Reads MEAL_PREP_TOKEN_KEY env var (64 hex chars) into key[32].
// Returns false if not set or invalid.
bool getKey(unsigned char key[KEY_LEN]) {
    const char *keyHex = std::getenv("MEAL_PREP_TOKEN_KEY");
    if (!keyHex) return false;
    size_t hexLen = std::strlen(keyHex);
    if (hexLen != static_cast<size_t>(KEY_LEN * 2)) return false;
    for (int i = 0; i < KEY_LEN; i++) {
        char byte[3] = {keyHex[i * 2], keyHex[i * 2 + 1], '\0'};
        char *end = nullptr;
        long val = std::strtol(byte, &end, 16);
        if (end != byte + 2) return false;
        key[i] = static_cast<unsigned char>(val);
    }
    return true;
}

}  // namespace

namespace TokenEncryption {

std::string encrypt(const std::string &plaintext) {
    unsigned char key[KEY_LEN];
    if (!getKey(key)) {
        std::cerr << "Warning: MEAL_PREP_TOKEN_KEY not set — storing tokens unencrypted.\n";
        return plaintext;
    }

    unsigned char iv[IV_LEN];
    if (RAND_bytes(iv, IV_LEN) != 1) return plaintext;

    std::vector<unsigned char> ciphertext(plaintext.size() + EVP_MAX_BLOCK_LENGTH);
    unsigned char tag[TAG_LEN];
    int len = 0, cipherLen = 0;

    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx) return plaintext;

    bool ok = EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, nullptr, nullptr) == 1 &&
              EVP_EncryptInit_ex(ctx, nullptr, nullptr, key, iv) == 1 &&
              EVP_EncryptUpdate(ctx, ciphertext.data(), &len,
                                reinterpret_cast<const unsigned char *>(plaintext.data()),
                                static_cast<int>(plaintext.size())) == 1;

    if (!ok) {
        EVP_CIPHER_CTX_free(ctx);
        return plaintext;
    }
    cipherLen = len;

    if (EVP_EncryptFinal_ex(ctx, ciphertext.data() + len, &len) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return plaintext;
    }
    cipherLen += len;
    EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, TAG_LEN, tag);
    EVP_CIPHER_CTX_free(ctx);

    // Pack: iv (12 bytes) + ciphertext + tag (16 bytes), then base64-encode
    std::vector<unsigned char> packed;
    packed.insert(packed.end(), iv, iv + IV_LEN);
    packed.insert(packed.end(), ciphertext.begin(), ciphertext.begin() + cipherLen);
    packed.insert(packed.end(), tag, tag + TAG_LEN);

    return ENC_PREFIX + base64Encode(packed.data(), packed.size());
}

std::string decrypt(const std::string &stored) {
    if (stored.size() < ENC_PREFIX.size() || stored.substr(0, ENC_PREFIX.size()) != ENC_PREFIX) {
        return stored;  // Legacy plaintext — return as-is
    }

    unsigned char key[KEY_LEN];
    if (!getKey(key)) {
        std::cerr << "Error: MEAL_PREP_TOKEN_KEY not set — cannot decrypt stored tokens.\n";
        return "";
    }

    auto packed = base64Decode(stored.substr(ENC_PREFIX.size()));
    if (packed.size() < static_cast<size_t>(IV_LEN + TAG_LEN)) return "";

    unsigned char iv[IV_LEN];
    memcpy(iv, packed.data(), IV_LEN);

    size_t cipherLen = packed.size() - IV_LEN - TAG_LEN;
    unsigned char tag[TAG_LEN];
    memcpy(tag, packed.data() + IV_LEN + cipherLen, TAG_LEN);

    std::vector<unsigned char> plaintext(cipherLen + EVP_MAX_BLOCK_LENGTH);
    int len = 0, plaintextLen = 0;

    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx) return "";

    bool ok = EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, nullptr, nullptr) == 1 &&
              EVP_DecryptInit_ex(ctx, nullptr, nullptr, key, iv) == 1 &&
              EVP_DecryptUpdate(ctx, plaintext.data(), &len, packed.data() + IV_LEN,
                                static_cast<int>(cipherLen)) == 1;

    if (!ok) {
        EVP_CIPHER_CTX_free(ctx);
        return "";
    }
    plaintextLen = len;

    EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, TAG_LEN, tag);
    if (EVP_DecryptFinal_ex(ctx, plaintext.data() + plaintextLen, &len) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        std::cerr << "Error: Token decryption failed — authentication tag mismatch.\n";
        return "";
    }
    plaintextLen += len;
    EVP_CIPHER_CTX_free(ctx);

    return std::string(plaintext.begin(), plaintext.begin() + plaintextLen);
}

}  // namespace TokenEncryption
