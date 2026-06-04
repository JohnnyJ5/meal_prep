#include <gtest/gtest.h>

#include <crow/json.h>

#include <string>

#include "../src/calendar_service.h"

namespace {

// Helper: count the number of events left in a filtered events.list response.
size_t countItems(const std::string &json) {
    auto parsed = crow::json::load(json);
    if (!parsed || !parsed.has("items")) return 0;
    return parsed["items"].size();
}

// Helper: does any remaining event have the given summary?
bool hasEventWithSummary(const std::string &json, const std::string &summary) {
    auto parsed = crow::json::load(json);
    if (!parsed || !parsed.has("items")) return false;
    for (const auto &item : parsed["items"]) {
        if (item.has("summary") && std::string(item["summary"].s()) == summary) {
            return true;
        }
    }
    return false;
}

}  // namespace

// Birthday events (eventType == "birthday") are removed.
TEST(CalendarServiceFilterTest, RemovesBirthdayEventType) {
    std::string input = R"({
        "items": [
            {"summary": "Dinner", "eventType": "default"},
            {"summary": "Alice's Birthday", "eventType": "birthday"},
            {"summary": "Lunch"}
        ]
    })";

    std::string out = CalendarService::filterBirthdayEvents(input);
    EXPECT_EQ(countItems(out), 2u);
    EXPECT_TRUE(hasEventWithSummary(out, "Dinner"));
    EXPECT_TRUE(hasEventWithSummary(out, "Lunch"));
    EXPECT_FALSE(hasEventWithSummary(out, "Alice's Birthday"));
}

// Events carrying birthdayProperties are removed even without eventType.
TEST(CalendarServiceFilterTest, RemovesEventsWithBirthdayProperties) {
    std::string input = R"({
        "items": [
            {"summary": "Keep me", "eventType": "default"},
            {"summary": "Bob's Birthday", "birthdayProperties": {"contact": "people/c123"}}
        ]
    })";

    std::string out = CalendarService::filterBirthdayEvents(input);
    EXPECT_EQ(countItems(out), 1u);
    EXPECT_TRUE(hasEventWithSummary(out, "Keep me"));
    EXPECT_FALSE(hasEventWithSummary(out, "Bob's Birthday"));
}

// Non-birthday events are all preserved.
TEST(CalendarServiceFilterTest, KeepsAllNonBirthdayEvents) {
    std::string input = R"({
        "items": [
            {"summary": "A", "eventType": "default"},
            {"summary": "B", "eventType": "outOfOffice"},
            {"summary": "C", "eventType": "focusTime"}
        ]
    })";

    std::string out = CalendarService::filterBirthdayEvents(input);
    EXPECT_EQ(countItems(out), 3u);
    EXPECT_TRUE(hasEventWithSummary(out, "A"));
    EXPECT_TRUE(hasEventWithSummary(out, "B"));
    EXPECT_TRUE(hasEventWithSummary(out, "C"));
}

// An empty items array stays empty and valid.
TEST(CalendarServiceFilterTest, HandlesEmptyItems) {
    std::string input = R"({"items": []})";
    std::string out = CalendarService::filterBirthdayEvents(input);
    EXPECT_EQ(countItems(out), 0u);
    auto parsed = crow::json::load(out);
    ASSERT_TRUE(parsed);
    EXPECT_TRUE(parsed.has("items"));
}

// Malformed JSON is returned unchanged rather than throwing.
TEST(CalendarServiceFilterTest, ReturnsInputUnchangedOnInvalidJson) {
    std::string input = "not valid json {{{";
    std::string out = CalendarService::filterBirthdayEvents(input);
    EXPECT_EQ(out, input);
}

// A response with no "items" key is returned unchanged.
TEST(CalendarServiceFilterTest, ReturnsInputUnchangedWhenNoItemsKey) {
    std::string input = R"({"kind": "calendar#events", "summary": "Family"})";
    std::string out = CalendarService::filterBirthdayEvents(input);
    EXPECT_EQ(out, input);
}

// Top-level metadata fields are preserved alongside the filtered items.
TEST(CalendarServiceFilterTest, PreservesTopLevelFields) {
    std::string input = R"({
        "kind": "calendar#events",
        "summary": "My Calendar",
        "items": [
            {"summary": "Birthday Party for Carol", "eventType": "birthday"},
            {"summary": "Real Meeting", "eventType": "default"}
        ]
    })";

    std::string out = CalendarService::filterBirthdayEvents(input);
    auto parsed = crow::json::load(out);
    ASSERT_TRUE(parsed);
    EXPECT_EQ(std::string(parsed["kind"].s()), "calendar#events");
    EXPECT_EQ(std::string(parsed["summary"].s()), "My Calendar");
    EXPECT_EQ(countItems(out), 1u);
    EXPECT_TRUE(hasEventWithSummary(out, "Real Meeting"));
}
