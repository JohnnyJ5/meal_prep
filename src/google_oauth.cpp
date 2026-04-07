#include "google_oauth.h"

#include <crow.h>
#include <curl/curl.h>

#include <chrono>
#include <iomanip>
#include <random>
#include <sstream>
#include <utility>

#include "curl_utils.h"

namespace {
std::string generateRandomState() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint32_t> dis(0, UINT32_MAX);
    std::ostringstream oss;
    for (int i = 0; i < 4; ++i) {
        oss << std::hex << std::setw(8) << std::setfill('0') << dis(gen);
    }
    return oss.str();
}
}  // namespace

GoogleOAuth::GoogleOAuth(const Config &config, std::shared_ptr<DBManager> dbManager)
    : d_config(config), d_dbManager(std::move(dbManager)) {}

std::string GoogleOAuth::getAuthUrl() {
    std::lock_guard<std::mutex> lock(d_tokenMutex);
    d_pendingState = generateRandomState();
    std::stringstream ss;
    ss << "https://accounts.google.com/o/oauth2/v2/auth?"
       << "client_id=" << curl_utils::urlEncode(d_config.google_client_id)
       << "&redirect_uri=" << curl_utils::urlEncode(d_config.google_redirect_uri)
       << "&response_type=code"
       << "&scope=" << curl_utils::urlEncode("https://www.googleapis.com/auth/calendar")
       << "&access_type=offline"
       << "&prompt=consent"
       << "&state=" << curl_utils::urlEncode(d_pendingState);
    return ss.str();
}

bool GoogleOAuth::validateState(const std::string &state) {
    std::lock_guard<std::mutex> lock(d_tokenMutex);
    if (d_pendingState.empty() || state != d_pendingState) {
        CROW_LOG_WARNING << "OAuth state validation failed: CSRF attack or stale request.";
        return false;
    }
    d_pendingState.clear();
    return true;
}

bool GoogleOAuth::exchangeCodeForTokens(const std::string &code) {
    std::stringstream ss;
    ss << "code=" << curl_utils::urlEncode(code)
       << "&client_id=" << curl_utils::urlEncode(d_config.google_client_id)
       << "&client_secret=" << curl_utils::urlEncode(d_config.google_client_secret)
       << "&redirect_uri=" << curl_utils::urlEncode(d_config.google_redirect_uri)
       << "&grant_type=authorization_code";

    auto response = makeTokenRequest(ss.str());
    if (response.success) {
        auto now = std::chrono::system_clock::now();
        auto expiry =
            std::chrono::system_clock::to_time_t(now + std::chrono::seconds(response.expires_in));

        std::string existingAccessToken, existingRefreshToken;
        int64_t existingExpiry;
        std::string newRefreshToken = response.refresh_token;

        if (newRefreshToken.empty() &&
            d_dbManager->getGoogleTokens(existingAccessToken, existingRefreshToken,
                                         existingExpiry)) {
            newRefreshToken = existingRefreshToken;
        }

        bool saved = d_dbManager->saveGoogleTokens(response.access_token, newRefreshToken,
                                                   static_cast<int64_t>(expiry));
        if (saved) {
            CROW_LOG_INFO << "Successfully exchanged code for tokens and saved to database.";
        } else {
            CROW_LOG_ERROR << "Exchanged code for tokens but failed to save to database.";
        }
        return saved;
    }
    CROW_LOG_ERROR << "Failed to exchange authorization code for tokens with Google API.";
    return false;
}

bool GoogleOAuth::refreshAccessToken() {
    std::string accessToken, refreshToken;
    int64_t expiryTime;
    if (!d_dbManager->getGoogleTokens(accessToken, refreshToken, expiryTime) ||
        refreshToken.empty()) {
        CROW_LOG_WARNING << "Cannot refresh token: missing refresh token in database.";
        return false;
    }

    std::stringstream ss;
    ss << "client_id=" << curl_utils::urlEncode(d_config.google_client_id)
       << "&client_secret=" << curl_utils::urlEncode(d_config.google_client_secret)
       << "&refresh_token=" << curl_utils::urlEncode(refreshToken) << "&grant_type=refresh_token";

    auto response = makeTokenRequest(ss.str());
    if (response.success) {
        auto now = std::chrono::system_clock::now();
        auto expiry =
            std::chrono::system_clock::to_time_t(now + std::chrono::seconds(response.expires_in));
        // Refresh tokens might not be returned in refresh requests
        std::string newRefreshToken =
            response.refresh_token.empty() ? refreshToken : response.refresh_token;
        bool saved = d_dbManager->saveGoogleTokens(response.access_token, newRefreshToken,
                                                   static_cast<int64_t>(expiry));
        if (saved) {
            CROW_LOG_INFO << "Successfully refreshed access token and saved to database.";
        } else {
            CROW_LOG_ERROR << "Refreshed access token but failed to save to database.";
        }
        return saved;
    }
    CROW_LOG_ERROR << "Failed to refresh access token with Google API.";
    return false;
}

std::string GoogleOAuth::getAccessToken() {
    std::lock_guard<std::mutex> lock(d_tokenMutex);

    std::string accessToken, refreshToken;
    int64_t expiryTime;
    if (!d_dbManager->getGoogleTokens(accessToken, refreshToken, expiryTime)) {
        CROW_LOG_WARNING << "Failed to retrieve Google tokens from database.";
        return "";
    }

    auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    if (now >= expiryTime - 60) {  // Refresh if within 60 seconds of expiry
        CROW_LOG_INFO << "Google access token expired or expiring soon, attempting refresh...";
        if (refreshAccessToken()) {
            d_dbManager->getGoogleTokens(accessToken, refreshToken, expiryTime);
        } else {
            CROW_LOG_ERROR << "Failed to refresh Google access token.";
            return "";
        }
    }

    return accessToken;
}

GoogleOAuth::TokenResponse GoogleOAuth::makeTokenRequest(const std::string &postData) {
    TokenResponse res;
    res.success = false;

    CURL *curl = curl_easy_init();
    if (curl) {
        std::string responseString;
        curl_easy_setopt(curl, CURLOPT_URL, "https://oauth2.googleapis.com/token");
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 0L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postData.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_utils::writeCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseString);

        CURLcode curl_res = curl_easy_perform(curl);
        if (curl_res == CURLE_OK) {
            auto json = crow::json::load(responseString);
            if (json && json.has("access_token")) {
                res.access_token = json["access_token"].s();
                if (json.has("refresh_token")) {
                    res.refresh_token = json["refresh_token"].s();
                }
                res.expires_in = json["expires_in"].i();
                res.success = true;
            } else {
                std::string errorMsg = "unknown error";
                if (json && json.has("error")) {
                    errorMsg = json["error"].s();
                    if (json.has("error_description")) {
                        errorMsg += ": " + std::string(json["error_description"].s());
                    }
                }
                CROW_LOG_ERROR << "Token request failed: " << errorMsg;
            }
        } else {
            CROW_LOG_ERROR << "curl_easy_perform() failed for token request: "
                           << curl_easy_strerror(curl_res);
        }
        curl_easy_cleanup(curl);
    }
    return res;
}
