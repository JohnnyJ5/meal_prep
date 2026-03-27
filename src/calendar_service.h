#ifndef CALENDAR_SERVICE_H
#define CALENDAR_SERVICE_H

#include "google_oauth.h"
#include <string>

#include <vector>

/**
 * @brief Handles interactions with the Google Calendar API.
 */
class CalendarService {
public:
  CalendarService(GoogleOAuth &oauth);

  /**
   * @brief Creates a new event in the user's primary calendar.
   * @param summary The title of the event.
   * @param description The description of the event.
   * @param startTime ISO 8601 formatted start time (e.g., "2026-03-23T09:00:00Z").
   * @param endTime ISO 8601 formatted end time (e.g., "2026-03-23T10:00:00Z").
   * @return true if successful, false otherwise.
   */
  std::string createEvent(const std::string &summary, const std::string &description,
                          const std::string &startTime, const std::string &endTime);

  bool deleteEvent(const std::string &eventId);

  struct CalendarEvents {
    std::string summary;
    std::string backgroundColor;
    std::string foregroundColor;
    std::string eventsJson;
  };

  /**
   * @brief Lists upcoming events from the user's calendars.
   * @param timeMin Optional ISO 8601 string for start date constraint
   * @param timeMax Optional ISO 8601 string for end date constraint
   * @param maxResults Maximum number of results to return.
   * @return Vector of CalendarEvents structs (or empty on failure).
   */
  std::vector<CalendarEvents> listEvents(const std::string &timeMin = "", const std::string &timeMax = "", int maxResults = 100);

private:
  GoogleOAuth &d_oauth;

  std::string makeAuthorizedRequest(const std::string &url,
                                    const std::string &method = "GET",
                                    const std::string &postData = "");
};

#endif // CALENDAR_SERVICE_H
