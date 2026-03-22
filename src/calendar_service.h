#ifndef CALENDAR_SERVICE_H
#define CALENDAR_SERVICE_H

#include "google_oauth.h"
#include <string>

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
  bool createEvent(const std::string &summary, const std::string &description,
                   const std::string &startTime, const std::string &endTime);

  /**
   * @brief Lists upcoming events from the user's primary calendar.
   * @param maxResults Maximum number of results to return.
   * @return JSON string of events (or empty string on failure).
   */
  std::string listEvents(int maxResults = 10);

private:
  GoogleOAuth &d_oauth;

  std::string makeAuthorizedRequest(const std::string &url,
                                    const std::string &method = "GET",
                                    const std::string &postData = "");
};

#endif // CALENDAR_SERVICE_H
