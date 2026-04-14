#include "calendar_service.h"

#include <crow.h>
#include <curl/curl.h>

#include <algorithm>
#include <iostream>
#include <vector>

#include "curl_utils.h"

CalendarService::CalendarService(std::shared_ptr<GoogleOAuth> oauth) : d_oauth(std::move(oauth)) {}

std::string CalendarService::createEvent(const std::string &summary, const std::string &description,
                                         const std::string &startTime, const std::string &endTime,
                                         bool withReminders) {
    crow::json::wvalue body;
    body["summary"] = summary;
    body["description"] = description;
    body["start"]["dateTime"] = startTime;
    body["end"]["dateTime"] = endTime;
    body["extendedProperties"]["private"]["mealPrepApp"] = "true";

    if (withReminders) {
        body["reminders"]["useDefault"] = false;
        body["reminders"]["overrides"][0]["method"] = "popup";
        body["reminders"]["overrides"][0]["minutes"] = 2880;  // 2 days prior
        body["reminders"]["overrides"][1]["method"] = "popup";
        body["reminders"]["overrides"][1]["minutes"] = 1440;  // 1 day prior
    }

    std::string calId = getFamilyCalendarId();
    std::string response = makeAuthorizedRequest(
        "https://www.googleapis.com/calendar/v3/calendars/" + curl_utils::urlEncode(calId) +
            "/events",
        "POST", body.dump());

    if (response.empty()) {
        std::cerr << "Calendar API returned empty response for createEvent" << std::endl;
        return "";
    }

    auto json = crow::json::load(response);
    if (json && json.has("id")) {
        return std::string(json["id"].s());
    }

    std::cerr << "Calendar API error: " << response << std::endl;
    return "";
}

bool CalendarService::deleteEvent(const std::string &eventId) {
    std::string token = d_oauth->getAccessToken();
    if (token.empty()) return false;

    std::string calId = getFamilyCalendarId();
    std::string url = "https://www.googleapis.com/calendar/v3/calendars/" +
                      curl_utils::urlEncode(calId) + "/events/" + eventId;

    CURL *curl = curl_easy_init();
    if (!curl) return false;

    struct curl_slist *headers = nullptr;
    headers = curl_slist_append(headers, ("Authorization: Bearer " + token).c_str());

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
    curl_easy_setopt(
        curl, CURLOPT_WRITEFUNCTION,
        +[](char *, size_t size, size_t nmemb, void *) -> size_t { return size * nmemb; });

    CURLcode res = curl_easy_perform(curl);
    long httpCode = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    return res == CURLE_OK && httpCode == 204;
}

std::vector<CalendarService::CalendarEvents> CalendarService::listEvents(const std::string &timeMin,
                                                                         const std::string &timeMax,
                                                                         int maxResults) {
    std::vector<CalendarEvents> results;

    std::string listUrl = "https://www.googleapis.com/calendar/v3/users/me/calendarList";
    std::string listResponse = makeAuthorizedRequest(listUrl);
    if (listResponse.empty()) {
        return results;
    }

    auto listJson = crow::json::load(listResponse);
    if (!listJson || !listJson.has("items")) {
        return results;
    }

    for (const auto &cal : listJson["items"]) {
        if (!cal.has("id")) continue;
        std::string calId = cal["id"].s();

        CalendarEvents calData;
        calData.summary = cal.has("summary") ? std::string(cal["summary"].s()) : "Unnamed Calendar";
        calData.backgroundColor =
            cal.has("backgroundColor") ? std::string(cal["backgroundColor"].s()) : "#58a6ff";
        calData.foregroundColor =
            cal.has("foregroundColor") ? std::string(cal["foregroundColor"].s()) : "#ffffff";

        // Filter out the birthday/contacts calendar
        if (calId == "addressbook#contacts@group.v.calendar.google.com" ||
            calData.summary == "Birthdays" || calData.summary == "Contacts") {
            continue;
        }

        std::string url =
            "https://www.googleapis.com/calendar/v3/calendars/" + curl_utils::urlEncode(calId) +
            "/events?singleEvents=true&orderBy=startTime&maxResults=" + std::to_string(maxResults);

        if (!timeMin.empty()) {
            url += "&timeMin=" + curl_utils::urlEncode(timeMin);
        }
        if (!timeMax.empty()) {
            url += "&timeMax=" + curl_utils::urlEncode(timeMax);
        }

        std::string evResponse = makeAuthorizedRequest(url);
        if (!evResponse.empty()) {
            calData.eventsJson = evResponse;
            results.push_back(calData);
        }
    }

    return results;
}

std::string CalendarService::getFamilyCalendarId() {
    std::string listResponse =
        makeAuthorizedRequest("https://www.googleapis.com/calendar/v3/users/me/calendarList");
    if (listResponse.empty()) return "primary";

    auto listJson = crow::json::load(listResponse);
    if (!listJson || !listJson.has("items")) return "primary";

    for (const auto &cal : listJson["items"]) {
        if (!cal.has("summary") || !cal.has("id")) continue;
        std::string summary = cal["summary"].s();
        // Case-insensitive match for "Family"
        std::string summaryLower = summary;
        std::transform(summaryLower.begin(), summaryLower.end(), summaryLower.begin(), ::tolower);
        if (summaryLower == "family calendar") {
            return std::string(cal["id"].s());
        }
    }

    return "primary";
}

std::string CalendarService::makeAuthorizedRequest(const std::string &url,
                                                   const std::string &method,
                                                   const std::string &postData) {
    std::string token = d_oauth->getAccessToken();
    if (token.empty()) {
        std::cerr << "Failed to get access token for Calendar API" << std::endl;
        return "";
    }

    CURL *curl = curl_easy_init();
    std::string responseString;
    if (curl) {
        struct curl_slist *headers = nullptr;
        headers = curl_slist_append(headers, ("Authorization: Bearer " + token).c_str());
        headers = curl_slist_append(headers, "Content-Type: application/json");

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        if (method == "POST") {
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postData.c_str());
        } else if (method == "PUT") {
            curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postData.c_str());
        } else if (method == "DELETE") {
            curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
        }

        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_utils::writeCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseString);

        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            std::cerr << "Calendar API request failed: " << curl_easy_strerror(res) << std::endl;
            responseString = "";
        }

        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }
    return responseString;
}
