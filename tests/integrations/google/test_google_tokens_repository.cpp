#include <gtest/gtest.h>

#include <cstdlib>
#include <memory>

#include "core/db/db_connection.h"
#include "core/db/schema.h"
#include "integrations/google/google_tokens_repository.h"

class GoogleTokensRepositoryTest : public ::testing::Test {
   protected:
    void SetUp() override {
        unsetenv("MEAL_PREP_TOKEN_KEY");
        conn = std::make_shared<DbConnection>(":memory:");
        initializeSchema(*conn);
        repo = std::make_unique<GoogleTokensRepository>(conn);
    }
    void TearDown() override { unsetenv("MEAL_PREP_TOKEN_KEY"); }

    std::shared_ptr<DbConnection> conn;
    std::unique_ptr<GoogleTokensRepository> repo;
};

TEST_F(GoogleTokensRepositoryTest, SaveAndGetGoogleTokensPlaintext) {
    EXPECT_TRUE(repo->saveTokens("access-abc", "refresh-xyz", 9999));

    std::string accessToken, refreshToken;
    int64_t expiryTime;
    EXPECT_TRUE(repo->getTokens(accessToken, refreshToken, expiryTime));
    EXPECT_EQ(accessToken, "access-abc");
    EXPECT_EQ(refreshToken, "refresh-xyz");
    EXPECT_EQ(expiryTime, 9999);
}

TEST_F(GoogleTokensRepositoryTest, GetGoogleTokensNotFound) {
    std::string accessToken, refreshToken;
    int64_t expiryTime;
    EXPECT_FALSE(repo->getTokens(accessToken, refreshToken, expiryTime));
}

TEST_F(GoogleTokensRepositoryTest, SaveGoogleTokensEmptyRefresh) {
    EXPECT_TRUE(repo->saveTokens("access-only", "", 12345));

    std::string accessToken, refreshToken;
    int64_t expiryTime;
    EXPECT_TRUE(repo->getTokens(accessToken, refreshToken, expiryTime));
    EXPECT_EQ(accessToken, "access-only");
    EXPECT_EQ(refreshToken, "");
    EXPECT_EQ(expiryTime, 12345);
}

TEST_F(GoogleTokensRepositoryTest, SaveAndGetGoogleTokensEncrypted) {
    setenv("MEAL_PREP_TOKEN_KEY",
           "0102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f20", 1);

    EXPECT_TRUE(repo->saveTokens("secret-access", "secret-refresh", 42));

    std::string accessToken, refreshToken;
    int64_t expiryTime;
    EXPECT_TRUE(repo->getTokens(accessToken, refreshToken, expiryTime));
    EXPECT_EQ(accessToken, "secret-access");
    EXPECT_EQ(refreshToken, "secret-refresh");
    EXPECT_EQ(expiryTime, 42);
}

TEST_F(GoogleTokensRepositoryTest, SaveGoogleTokensOverwritesPrevious) {
    repo->saveTokens("old-access", "old-refresh", 100);
    repo->saveTokens("new-access", "new-refresh", 200);

    std::string accessToken, refreshToken;
    int64_t expiryTime;
    repo->getTokens(accessToken, refreshToken, expiryTime);
    EXPECT_EQ(accessToken, "new-access");
    EXPECT_EQ(expiryTime, 200);
}
