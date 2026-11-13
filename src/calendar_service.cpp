#include "calendar_service.h"

#include <crow.h>
#include <curl/curl.h>

#include <algorithm>
#include <cctype>
#include <iostream>
#include <set>
#include <vector>

#include "curl_utils.h"

namespace {

std::string normalizeMealSlug(const std::string &str) {
    std::string out;
    bool prevHyphen = true;
    for (unsigned char ch : str) {
        if (std::isspace(ch) || ch == '-') {
            if (!prevHyphen) {
                out.push_back('-');
                prevHyphen = true;
            }
        } else {
            out.push_back(static_cast<char>(std::tolower(ch)));
            prevHyphen = false;
        }
    }
    if (!out.empty() && out.back() == '-') {
        out.pop_back();
    }
    return out;
}

}  // namespace

CalendarService::CalendarService(std::shared_ptr<GoogleOAuth> oauth) : d_oauth(std::move(oauth)) {}

std::vector<CalendarService::RecipeHistoryEntry> CalendarService::extractRecipeHistory(
    const std::vector<CalendarEvents> &calendars, const std::set<std::string> &knownMeals,
    const std::string &maxDate, size_t limit) {
    std::set<std::string> normalizedKnown;
    for (const auto &meal : knownMeals) {
        std::string slug = normalizeMealSlug(meal);
        if (!slug.empty()) {
            normalizedKnown.insert(slug);
        }
    }

    std::vector<RecipeHistoryEntry> entries;
    for (const auto &cal : calendars) {
        auto parsed = crow::json::load(cal.eventsJson);
        if (!parsed || !parsed.has("items")) {
            continue;
        }
        for (const auto &item : parsed["items"]) {
            if (!item.has("summary")) continue;
            std::string summary = item["summary"].s();
            std::string normSummary = normalizeMealSlug(summary);
            if (normSummary.empty() || normSummary == "whole-foods-order") {
                continue;
            }

            bool isAppEvent =
                item.has("extendedProperties") && item["extendedProperties"].has("private") &&
                item["extendedProperties"]["private"].has("mealPrepApp") &&
                std::string(item["extendedProperties"]["private"]["mealPrepApp"].s()) == "true";
            bool isPlannedDesc = item.has("description") &&
                                 std::string(item["description"].s()) == "Meal planned via app";
            bool isKnownMeal = normalizedKnown.count(normSummary) > 0;
            if (!isAppEvent && !isPlannedDesc && !isKnownMeal) {
                continue;
            }

            if (!item.has("start")) continue;
            std::string startStr;
            if (item["start"].has("dateTime")) {
                startStr = item["start"]["dateTime"].s();
            } else if (item["start"].has("date")) {
                startStr = item["start"]["date"].s();
            }
            if (startStr.size() < 10) continue;

            std::string date = startStr.substr(0, 10);
            if (!maxDate.empty() && date > maxDate) {
                continue;
            }

            entries.push_back({summary, date});
        }
    }

    std::stable_sort(
        entries.begin(), entries.end(),
        [](const RecipeHistoryEntry &a, const RecipeHistoryEntry &b) { return a.date > b.date; });

    if (entries.size() > limit) {
        entries.resize(limit);
    }
    return entries;
}

std::string CalendarService::filterBirthdayEvents(const std::string &eventsJson) {
    auto parsed = crow::json::load(eventsJson);
    if (!parsed || !parsed.has("items")) {
        return eventsJson;
    }

    crow::json::wvalue out(parsed);
    std::vector<crow::json::wvalue> kept;
    for (const auto &item : parsed["items"]) {
        bool isBirthday = item.has("eventType") && std::string(item["eventType"].s()) == "birthday";
        // Belt-and-suspenders: also drop anything carrying birthdayProperties.
        bool hasBirthdayProps = item.has("birthdayProperties");
        if (isBirthday || hasBirthdayProps) {
            continue;
        }
        kept.emplace_back(item);
    }

    out["items"] = std::move(kept);
    return out.dump();
}

std::string CalendarService::createEvent(const std::string &summary, const std::string &description,
                                         const std::string &startTime, const std::string &endTime,
                                         bool withReminders, const std::string &calendarName) {
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

    std::string calId = getCalendarId(calendarName);
    std::string response =
        makeAuthorizedRequest("https://www.googleapis.com/calendar/v3/calendars/" +
                                  curl_utils::urlEncode(calId) + "/events",
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

bool CalendarService::deleteEvent(const std::string &eventId, const std::string &calendarName) {
    std::string token = d_oauth->getAccessToken();
    if (token.empty()) return false;

    std::string calId = getCalendarId(calendarName);
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

        // Filter out the birthday/contacts calendar.
        // The calendar ID for this auto-generated calendar contains
        // "#contacts@group.v.calendar.google.com". The displayed summary varies
        // ("Birthdays", "Birthday", "Contacts", localized variants), so match it
        // case-insensitively as a fallback.
        std::string summaryLower = calData.summary;
        std::transform(summaryLower.begin(), summaryLower.end(), summaryLower.begin(), ::tolower);
        bool isBirthdaysCalendar =
            calId.find("#contacts@group.v.calendar.google.com") != std::string::npos ||
            summaryLower.find("birthday") != std::string::npos || summaryLower == "contacts";
        if (isBirthdaysCalendar) {
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
            calData.eventsJson = filterBirthdayEvents(evResponse);
            results.push_back(calData);
        }
    }

    for (const auto &cal : results) {
        std::cout << "Calendar loaded: " << cal.summary << std::endl;
    }

    return results;
}

std::string CalendarService::findCalendarId(const std::string &calendarListJson,
                                            const std::string &calendarName) {
    if (calendarListJson.empty() || calendarName.empty()) return "primary";

    auto listJson = crow::json::load(calendarListJson);
    if (!listJson || !listJson.has("items")) return "primary";

    std::string targetLower = calendarName;
    std::transform(targetLower.begin(), targetLower.end(), targetLower.begin(), ::tolower);

    for (const auto &cal : listJson["items"]) {
        if (!cal.has("summary") || !cal.has("id")) continue;
        std::string summary = cal["summary"].s();
        std::string summaryLower = summary;
        std::transform(summaryLower.begin(), summaryLower.end(), summaryLower.begin(), ::tolower);
        if (summaryLower == targetLower) {
            return std::string(cal["id"].s());
        }
    }

    return "primary";
}

std::string CalendarService::getCalendarId(const std::string &calendarName) {
    std::string listResponse =
        makeAuthorizedRequest("https://www.googleapis.com/calendar/v3/users/me/calendarList");
    return findCalendarId(listResponse, calendarName);
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
