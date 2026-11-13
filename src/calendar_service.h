#pragma once

#include <memory>
#include <set>
#include <string>
#include <vector>

#include "google_oauth.h"

/**
 * @brief Handles interactions with the Google Calendar API.
 */
class CalendarService {
   public:
    explicit CalendarService(std::shared_ptr<GoogleOAuth> oauth);

    /**
     * @brief Creates a new event in the specified calendar (falls back to primary).
     * @param summary The title of the event.
     * @param description The description of the event.
     * @param startTime ISO 8601 formatted start time (e.g., "2026-03-23T09:00:00Z").
     * @param endTime ISO 8601 formatted end time (e.g., "2026-03-23T10:00:00Z").
     * @param withReminders Whether to add custom popup reminder overrides.
     * @param calendarName The display name (summary) of the target calendar.
     * @return The created event ID, or empty string on failure.
     */
    std::string createEvent(const std::string &summary, const std::string &description,
                            const std::string &startTime, const std::string &endTime,
                            bool withReminders = false,
                            const std::string &calendarName = "Michael Coffey");

    bool deleteEvent(const std::string &eventId,
                     const std::string &calendarName = "Michael Coffey");

    struct CalendarEvents {
        std::string summary;
        std::string backgroundColor;
        std::string foregroundColor;
        std::string eventsJson;
    };

    struct RecipeHistoryEntry {
        std::string name;
        std::string date;
    };

    /**
     * @brief Lists upcoming events from the user's calendars.
     * @param timeMin Optional ISO 8601 string for start date constraint
     * @param timeMax Optional ISO 8601 string for end date constraint
     * @param maxResults Maximum number of results to return.
     * @return Vector of CalendarEvents structs (or empty on failure).
     */
    std::vector<CalendarEvents> listEvents(const std::string &timeMin = "",
                                           const std::string &timeMax = "", int maxResults = 100);

    /**
     * @brief Removes birthday/anniversary events from a Calendar API events list response.
     *
     * Google tags these with eventType == "birthday" (an all-day, annually recurring
     * event sourced from Google Contacts). Such events can appear in any calendar feed,
     * including the user's primary calendar, so filtering by calendar name/ID alone is
     * not sufficient. Exposed as static for unit testing.
     *
     * @param eventsJson Raw JSON body of a calendar events.list response.
     * @return The same JSON with birthday events removed from "items". If the input is
     *         not valid JSON or has no "items" array, it is returned unchanged.
     */
    static std::string filterBirthdayEvents(const std::string &eventsJson);

    /**
     * @brief Finds the calendar ID matching the given summary name (case-insensitive)
     *        from a Google Calendar calendarList JSON response, or "primary" if not found.
     *
     * Exposed as static for unit testing.
     */
    static std::string findCalendarId(const std::string &calendarListJson,
                                      const std::string &calendarName);

    /**
     * @brief Extracts the most recent recipes made from a list of calendar event feeds.
     *
     * Matches events tagged with extendedProperties.private.mealPrepApp == "true"
     * (excluding "Whole Foods Order"), events with description == "Meal planned via app",
     * or events whose normalized summary matches a known meal name in `knownMeals`.
     * Filters to events on or before `maxDate` (when non-empty), sorts by date descending,
     * and returns up to `limit` entries.
     *
     * Exposed as static for unit testing.
     */
    static std::vector<RecipeHistoryEntry> extractRecipeHistory(
        const std::vector<CalendarEvents> &calendars, const std::set<std::string> &knownMeals,
        const std::string &maxDate = "", size_t limit = 10);

   private:
    std::shared_ptr<GoogleOAuth> d_oauth;

    std::string makeAuthorizedRequest(const std::string &url, const std::string &method = "GET",
                                      const std::string &postData = "");

    /**
     * @brief Returns the ID of the calendar matching calendarName, or "primary" if not found.
     */
    std::string getCalendarId(const std::string &calendarName);
};
