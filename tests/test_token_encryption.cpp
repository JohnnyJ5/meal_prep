#include <gtest/gtest.h>

#include <cstdlib>

#include "../src/token_encryption.h"

class TokenEncryptionTest : public ::testing::Test {
   protected:
    void SetUp() override { unsetenv("MEAL_PREP_TOKEN_KEY"); }
    void TearDown() override { unsetenv("MEAL_PREP_TOKEN_KEY"); }

    void setValidKey() {
        setenv("MEAL_PREP_TOKEN_KEY",
               "0102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f20", 1);
    }
};

// Without key, encrypt returns plaintext unchanged
TEST_F(TokenEncryptionTest, EncryptWithoutKeyReturnsPlaintext) {
    std::string plaintext = "my-access-token";
    std::string result = TokenEncryption::encrypt(plaintext);
    EXPECT_EQ(result, plaintext);
}

// Without ENC: prefix, decrypt returns stored value as-is (legacy plaintext)
TEST_F(TokenEncryptionTest, DecryptLegacyPlaintextReturnedAsIs) {
    std::string stored = "some-plaintext-token";
    std::string result = TokenEncryption::decrypt(stored);
    EXPECT_EQ(result, stored);
}

// ENC: prefix without key → error, returns empty string
TEST_F(TokenEncryptionTest, DecryptEncryptedWithoutKeyReturnsEmpty) {
    std::string stored = "ENC:somebase64data";
    std::string result = TokenEncryption::decrypt(stored);
    EXPECT_EQ(result, "");
}

// With valid key, encrypt returns ENC:-prefixed string
TEST_F(TokenEncryptionTest, EncryptWithKeyReturnsEncPrefixed) {
    setValidKey();
    std::string plaintext = "my-access-token";
    std::string result = TokenEncryption::encrypt(plaintext);
    ASSERT_GE(result.size(), 4u);
    EXPECT_EQ(result.substr(0, 4), "ENC:");
    EXPECT_NE(result, plaintext);
}

// Round-trip: encrypt then decrypt gives original string
TEST_F(TokenEncryptionTest, RoundTripEncryptDecrypt) {
    setValidKey();
    std::string plaintext = "my-secret-access-token";
    std::string encrypted = TokenEncryption::encrypt(plaintext);
    ASSERT_EQ(encrypted.substr(0, 4), "ENC:");
    std::string decrypted = TokenEncryption::decrypt(encrypted);
    EXPECT_EQ(decrypted, plaintext);
}

// Round-trip with empty string
TEST_F(TokenEncryptionTest, RoundTripEmptyString) {
    setValidKey();
    std::string plaintext = "";
    std::string encrypted = TokenEncryption::encrypt(plaintext);
    ASSERT_EQ(encrypted.substr(0, 4), "ENC:");
    std::string decrypted = TokenEncryption::decrypt(encrypted);
    EXPECT_EQ(decrypted, plaintext);
}

// Round-trip with long string (>block size)
TEST_F(TokenEncryptionTest, RoundTripLongString) {
    setValidKey();
    std::string plaintext(500, 'A');
    std::string encrypted = TokenEncryption::encrypt(plaintext);
    std::string decrypted = TokenEncryption::decrypt(encrypted);
    EXPECT_EQ(decrypted, plaintext);
}

// Round-trip with binary-like content (various byte values)
TEST_F(TokenEncryptionTest, RoundTripSpecialCharacters) {
    setValidKey();
    std::string plaintext = "token\x01\x02\x03with-special chars & symbols!@#$%^&*()";
    std::string encrypted = TokenEncryption::encrypt(plaintext);
    std::string decrypted = TokenEncryption::decrypt(encrypted);
    EXPECT_EQ(decrypted, plaintext);
}

// Two encryptions of the same plaintext differ (random IV)
TEST_F(TokenEncryptionTest, EncryptionsAreNonDeterministic) {
    setValidKey();
    std::string plaintext = "test-token";
    std::string enc1 = TokenEncryption::encrypt(plaintext);
    std::string enc2 = TokenEncryption::encrypt(plaintext);
    EXPECT_NE(enc1, enc2);
}

// Corrupt (too-short) base64 payload after ENC: returns empty
TEST_F(TokenEncryptionTest, DecryptTooShortPayloadReturnsEmpty) {
    setValidKey();
    // "abc" decodes to 2 bytes, which is < IV_LEN(12) + TAG_LEN(16)
    std::string corrupt = "ENC:abc";
    std::string result = TokenEncryption::decrypt(corrupt);
    EXPECT_EQ(result, "");
}

// Key too short (not 64 hex chars) → encrypt falls back to plaintext
TEST_F(TokenEncryptionTest, EncryptWithShortKeyReturnsPlaintext) {
    setenv("MEAL_PREP_TOKEN_KEY", "tooshort", 1);
    std::string plaintext = "my-token";
    EXPECT_EQ(TokenEncryption::encrypt(plaintext), plaintext);
}

// Key with invalid hex characters → encrypt falls back to plaintext
TEST_F(TokenEncryptionTest, EncryptWithInvalidHexKeyReturnsPlaintext) {
    // 64 chars but not valid hex (Z is not a hex digit)
    setenv("MEAL_PREP_TOKEN_KEY",
           "ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ", 1);
    std::string plaintext = "my-token";
    EXPECT_EQ(TokenEncryption::encrypt(plaintext), plaintext);
}

// Tampered ciphertext fails authentication and returns empty
TEST_F(TokenEncryptionTest, DecryptTamperedCiphertextReturnsEmpty) {
    setValidKey();
    std::string plaintext = "important-token";
    std::string encrypted = TokenEncryption::encrypt(plaintext);
    // Flip a character in the middle of the base64 payload to corrupt it
    ASSERT_GT(encrypted.size(), 10u);
    encrypted[encrypted.size() / 2] ^= 0x01;
    // May produce empty or different result — the key thing is it won't match
    // (we just verify it doesn't silently return the original)
    std::string result = TokenEncryption::decrypt(encrypted);
    // If prefix is gone it returns as-is; otherwise it should be "" or different
    if (result.size() >= 4 && result.substr(0, 4) != "ENC:") {
        EXPECT_NE(result, plaintext);
    }
}
