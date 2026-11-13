#include <gtest/gtest.h>

#include <crow/json.h>

#include <algorithm>
#include <cstdlib>
#include <memory>
#include <string>

#include "../src/calendar_service.h"
#include "../src/curl_utils.h"
#include "../src/db_manager.h"
#include "../src/google_oauth.h"

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
    const auto &items = parsed["items"];
    return std::any_of(items.begin(), items.end(), [&summary](const auto &item) {
        return item.has("summary") && std::string(item["summary"].s()) == summary;
    });
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

TEST(CalendarServiceFindCalendarIdTest, FindsMatchingCalendarCaseInsensitive) {
    std::string listJson = R"({
        "items": [
            {"id": "incomplete_no_summary"},
            {"summary": "Incomplete No Id"},
            {"id": "family_id@group.calendar.google.com", "summary": "Family Calendar"},
            {"id": "michaelcoffey5@gmail.com", "summary": "Michael Coffey"}
        ]
    })";

    EXPECT_EQ(CalendarService::findCalendarId(listJson, "Michael Coffey"),
              "michaelcoffey5@gmail.com");
    EXPECT_EQ(CalendarService::findCalendarId(listJson, "michael coffey"),
              "michaelcoffey5@gmail.com");
    EXPECT_EQ(CalendarService::findCalendarId(listJson, "Family Calendar"),
              "family_id@group.calendar.google.com");
    EXPECT_EQ(CalendarService::findCalendarId(listJson, "Nonexistent Calendar"), "primary");
    EXPECT_EQ(CalendarService::findCalendarId(listJson, ""), "primary");
    EXPECT_EQ(CalendarService::findCalendarId("", "Michael Coffey"), "primary");
    EXPECT_EQ(CalendarService::findCalendarId("not json{{{", "Michael Coffey"), "primary");
    EXPECT_EQ(CalendarService::findCalendarId(R"({"kind": "calendar#calendarList"})",
                                              "Michael Coffey"),
              "primary");
}

TEST(CalendarServiceRecipeHistoryTest, ExtractsSortsAndLimitsLastTenRecipes) {
    CalendarService::CalendarEvents badCal{"Bad", "#000", "#fff", "not json{{{"};
    CalendarService::CalendarEvents noItemsCal{"NoItems", "#000", "#fff", R"({"kind":"events"})"};
    CalendarService::CalendarEvents mainCal{
        "Michael Coffey", "#000", "#fff", R"({
            "items": [
                {"id": "no_summary", "start": {"date": "2026-11-01"}},
                {"summary": "   ", "start": {"date": "2026-11-01"}},
                {"summary": "Whole Foods Order", "extendedProperties": {"private": {"mealPrepApp": "true"}}, "start": {"dateTime": "2026-11-12T09:00:00Z"}},
                {"summary": "Random Meeting", "start": {"dateTime": "2026-11-11T10:00:00Z"}},
                {"summary": "turkey-burgers", "extendedProperties": {"private": {"mealPrepApp": "true"}}},
                {"summary": "turkey-burgers", "extendedProperties": {"private": {"mealPrepApp": "true"}}, "start": {}},
                {"summary": "turkey-burgers", "extendedProperties": {"private": {"mealPrepApp": "true"}}, "start": {"date": "bad"}},
                {"summary": "future-meal", "extendedProperties": {"private": {"mealPrepApp": "true"}}, "start": {"dateTime": "2026-11-20T18:00:00Z"}},
                {"summary": "meal-01", "extendedProperties": {"private": {"mealPrepApp": "true"}}, "start": {"dateTime": "2026-11-01T18:00:00Z"}},
                {"summary": "meal-02", "description": "Meal planned via app", "start": {"date": "2026-11-02"}},
                {"summary": "Turkey Meatballs", "start": {"dateTime": "2026-11-03T18:00:00Z"}},
                {"summary": "meal-04", "extendedProperties": {"private": {"mealPrepApp": "true"}}, "start": {"dateTime": "2026-11-04T18:00:00Z"}},
                {"summary": "meal-05", "extendedProperties": {"private": {"mealPrepApp": "true"}}, "start": {"dateTime": "2026-11-05T18:00:00Z"}},
                {"summary": "meal-06", "extendedProperties": {"private": {"mealPrepApp": "true"}}, "start": {"dateTime": "2026-11-06T18:00:00Z"}},
                {"summary": "meal-07", "extendedProperties": {"private": {"mealPrepApp": "true"}}, "start": {"dateTime": "2026-11-07T18:00:00Z"}},
                {"summary": "meal-08", "extendedProperties": {"private": {"mealPrepApp": "true"}}, "start": {"dateTime": "2026-11-08T18:00:00Z"}},
                {"summary": "meal-09", "extendedProperties": {"private": {"mealPrepApp": "true"}}, "start": {"dateTime": "2026-11-09T18:00:00Z"}},
                {"summary": "meal-10", "extendedProperties": {"private": {"mealPrepApp": "true"}}, "start": {"dateTime": "2026-11-10T18:00:00Z"}},
                {"summary": "meal-11 -- ", "extendedProperties": {"private": {"mealPrepApp": "true"}}, "start": {"dateTime": "2026-11-11T18:00:00Z"}}
            ]
        })"};

    std::set<std::string> knownMeals = {"turkey-meatballs", "  "};
    auto history = CalendarService::extractRecipeHistory(
        {badCal, noItemsCal, mainCal}, knownMeals, "2026-11-13", 10);

    ASSERT_EQ(history.size(), 10u);
    EXPECT_EQ(history[0].name, "meal-11 -- ");
    EXPECT_EQ(history[0].date, "2026-11-11");
    EXPECT_EQ(history[1].name, "meal-10");
    EXPECT_EQ(history[1].date, "2026-11-10");
    EXPECT_EQ(history[8].name, "Turkey Meatballs");
    EXPECT_EQ(history[8].date, "2026-11-03");
    EXPECT_EQ(history[9].name, "meal-02");
    EXPECT_EQ(history[9].date, "2026-11-02");
}

TEST(CurlUtilsTest, UrlEncodeAndWriteCallback) {
    EXPECT_EQ(curl_utils::urlEncode("hello world@2026"), "hello%20world%402026");

    std::string buf;
    const char *chunk = "abc123";
    size_t written = curl_utils::writeCallback(const_cast<char *>(chunk), 1, 6, &buf);
    EXPECT_EQ(written, 6u);
    EXPECT_EQ(buf, "abc123");
}

class GoogleOAuthAndCalendarTest : public ::testing::Test {
   protected:
    void SetUp() override {
        setenv("HTTPS_PROXY", "http://127.0.0.1:1", 1);
        db = std::make_shared<DBManager>(":memory:");
        ASSERT_TRUE(db->initializeSchema());
        cfg.google_client_id = "test_client_id";
        cfg.google_client_secret = "test_secret";
        cfg.google_redirect_uri = "http://localhost:8080/callback";
        oauth = std::make_shared<GoogleOAuth>(cfg, db);
        cal = std::make_unique<CalendarService>(oauth);
    }

    void TearDown() override { unsetenv("HTTPS_PROXY"); }

    std::shared_ptr<DBManager> db;
    Config cfg;
    std::shared_ptr<GoogleOAuth> oauth;
    std::unique_ptr<CalendarService> cal;
};

TEST_F(GoogleOAuthAndCalendarTest, GetAuthUrlAndValidateState) {
    EXPECT_FALSE(oauth->validateState("any"));

    std::string url = oauth->getAuthUrl();
    EXPECT_NE(url.find("client_id=test_client_id"), std::string::npos);
    EXPECT_FALSE(oauth->validateState("wrong_state"));

    // Generate again and validate with matching state
    url = oauth->getAuthUrl();
    auto pos = url.find("&state=");
    ASSERT_NE(pos, std::string::npos);
    std::string state = url.substr(pos + 7);
    EXPECT_TRUE(oauth->validateState(state));
    // State is cleared after one successful validation
    EXPECT_FALSE(oauth->validateState(state));
}

TEST_F(GoogleOAuthAndCalendarTest, GetAccessTokenStates) {
    // 1. No tokens in DB
    EXPECT_EQ(oauth->getAccessToken(), "");
    EXPECT_FALSE(oauth->refreshAccessToken());

    // 2. Valid non-expired token in DB
    ASSERT_TRUE(db->saveGoogleTokens("valid_tok", "ref_tok", 4102444800LL));
    EXPECT_EQ(oauth->getAccessToken(), "valid_tok");

    // 3. Expired token with empty refresh token
    ASSERT_TRUE(db->saveGoogleTokens("expired_tok", "", 1000LL));
    EXPECT_EQ(oauth->getAccessToken(), "");

    // 4. Expired token with refresh token (fails fast via proxy)
    ASSERT_TRUE(db->saveGoogleTokens("expired_tok", "ref_tok", 1000LL));
    EXPECT_EQ(oauth->getAccessToken(), "");

    // 5. Exchange code fails fast via proxy
    EXPECT_FALSE(oauth->exchangeCodeForTokens("dummy_code"));
}

TEST_F(GoogleOAuthAndCalendarTest, CalendarServiceWithoutAndWithToken) {
    // Without token in DB
    EXPECT_EQ(
        cal->createEvent("Test", "Desc", "2026-03-23T09:00:00Z", "2026-03-23T10:00:00Z", true), "");
    EXPECT_FALSE(cal->deleteEvent("ev123"));
    EXPECT_TRUE(cal->listEvents("2026-03-23T00:00:00Z", "2026-03-30T00:00:00Z").empty());

    // With valid token in DB (exercises curl request setup and failure path via proxy)
    ASSERT_TRUE(db->saveGoogleTokens("valid_tok", "ref_tok", 4102444800LL));
    EXPECT_EQ(
        cal->createEvent("Test", "Desc", "2026-03-23T09:00:00Z", "2026-03-23T10:00:00Z", true), "");
    EXPECT_EQ(cal->createEvent("Order", "Desc", "2026-03-23T09:00:00Z", "2026-03-23T10:00:00Z",
                               true, "Family Calendar"),
              "");
    EXPECT_FALSE(cal->deleteEvent("ev123"));
    EXPECT_FALSE(cal->deleteEvent("ev123", "Family Calendar"));
    EXPECT_TRUE(cal->listEvents("2026-03-23T00:00:00Z", "2026-03-30T00:00:00Z").empty());
}
