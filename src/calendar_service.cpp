#include "calendar_service.h"
#include <crow.h>
#include <curl/curl.h>
#include <iostream>

namespace {
size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp) {
  ((std::string *)userp)->append((char *)contents, size * nmemb);
  return size * nmemb;
}
} // namespace

CalendarService::CalendarService(GoogleOAuth &oauth) : d_oauth(oauth) {}

bool CalendarService::createEvent(const std::string &summary,
                                  const std::string &description,
                                  const std::string &startTime,
                                  const std::string &endTime) {
  crow::json::wvalue body;
  body["summary"] = summary;
  body["description"] = description;
  body["start"]["dateTime"] = startTime;
  body["end"]["dateTime"] = endTime;

  std::string response = makeAuthorizedRequest(
      "https://www.googleapis.com/calendar/v3/calendars/primary/events", "POST",
      body.dump());

  if (response.empty()) {
    std::cerr << "Calendar API returned empty response for createEvent" << std::endl;
    return false;
  }

  auto json = crow::json::load(response);
  if (json && json.has("id")) {
    return true;
  }

  std::cerr << "Calendar API error: " << response << std::endl;
  return false;
}

std::string CalendarService::listEvents(int maxResults) {
  std::string url =
      "https://www.googleapis.com/calendar/v3/calendars/primary/"
      "events?maxResults=" +
      std::to_string(maxResults);
  return makeAuthorizedRequest(url);
}

std::string CalendarService::makeAuthorizedRequest(const std::string &url,
                                                  const std::string &method,
                                                  const std::string &postData) {
  std::string token = d_oauth.getAccessToken();
  if (token.empty()) {
    std::cerr << "Failed to get access token for Calendar API" << std::endl;
    return "";
  }

  CURL *curl = curl_easy_init();
  std::string responseString;
  if (curl) {
    struct curl_slist *headers = nullptr;
    headers =
        curl_slist_append(headers, ("Authorization: Bearer " + token).c_str());
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

    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseString);

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
      std::cerr << "Calendar API request failed: " << curl_easy_strerror(res)
                << std::endl;
      responseString = "";
    }

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
  }
  return responseString;
}
