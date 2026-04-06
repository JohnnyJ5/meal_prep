# Meal Prep REST API

The Meal Prep application provides a RESTful API to manage meals and generate grocery lists. The API is accessible when the server is running (e.g., via `make start`).

[← Back to README](../README.md)

All API responses are in JSON format.

## Endpoints

### 1. Get All Meals
- **URL**: `/api/meals`
- **Method**: `GET`
- **Description**: Returns a list of all available meals with their names and categories.
- **Example**:
  ```bash
  curl http://localhost:8080/api/meals
  ```
- **Response Example**:
  ```json
  [
    { "name": "Spaghetti Bolognese", "category": "Dinner" },
    { "name": "Chicken Salad", "category": "Lunch" }
  ]
  ```

### 2. Get a Specific Meal
- **URL**: `/api/meals/<meal_name>`
- **Method**: `GET`
- **Description**: Retrieves the details and ingredients of a specific meal.
- **Example**:
  ```bash
  curl http://localhost:8080/api/meals/Chicken%20Salad
  ```
- **Success Response**: `200 OK`
  ```json
  {
    "name": "Spaghetti Bolognese",
    "category": "Dinner",
    "ingredients": [
      {
        "name": "Ground Beef",
        "amount": 1.0,
        "unit": 1,
        "preparation": "Browned"
      }
    ]
  }
  ```
- **Error Response**: `404 Not Found` if the meal does not exist.

### 3. Add a New Meal
- **URL**: `/api/meals/add`
- **Method**: `POST`
- **Description**: Adds a new meal to the database.
- **Example**:
  ```bash
  curl -X POST http://localhost:8080/api/meals/add \
       -H "Content-Type: application/json" \
       -d '{"name": "New Meal", "category": "Dinner", "ingredients": []}'
  ```
- **Payload Example**:
  ```json
  {
    "name": "Spaghetti Bolognese",
    "category": "Dinner",
    "ingredients": [
      {
        "name": "Ground Beef",
        "amount": 1.0,
        "unit": 1,
        "preparation": "Browned"
      }
    ]
  }
  ```
- **Success Response**: `200 OK`
- **Error Response**: `400 Bad Request` or `500 Server Error`

### 4. Update an Existing Meal
- **URL**: `/api/meals/<meal_name>`
- **Method**: `PUT`
- **Description**: Updates the ingredients of an existing meal. The meal name is taken from the URL.
- **Example**:
  ```bash
  curl -X PUT http://localhost:8080/api/meals/Existing%20Meal \
       -H "Content-Type: application/json" \
       -d '{"category": "Lunch", "ingredients": []}'
  ```
- **Payload Example**: Same format as adding a new meal (without `name`; it is taken from the URL).
- **Success Response**: `200 OK`
- **Error Response**: `400 Bad Request` or `500 Server Error`

### 5. Delete a Meal
- **URL**: `/api/meals/<meal_name>`
- **Method**: `DELETE`
- **Description**: Removes a meal from the database.
- **Example**:
  ```bash
  curl -X DELETE http://localhost:8080/api/meals/Meal%20To%20Delete
  ```
- **Success Response**: `200 OK`
- **Error Response**: `404 Not Found`

### 6. Generate Weekly Plan & Consolidated Ingredients
- **URL**: `/api/plan`
- **Method**: `POST`
- **Description**: Submits a weekly meal schedule. This endpoint consolidates the required ingredients from all selected meals into a single grocery list returned in the response.
- **Example**:
  ```bash
  curl -X POST http://localhost:8080/api/plan \
       -H "Content-Type: application/json" \
       -d '{"Monday": ["Spaghetti Bolognese"], "Tuesday": ["Chicken Salad"]}'
  ```
- **Payload Example**:
  ```json
  {
    "Monday": ["Spaghetti Bolognese"],
    "Tuesday": ["Chicken Salad"],
    "Wednesday": [],
    "Thursday": [],
    "Friday": [],
    "Saturday": [],
    "Sunday": []
  }
  ```
- **Success Response**: `200 OK`
  ```json
  {
    "status": "success",
    "ingredients_text": "Whole Foods Order - Ingredients:\n- Ground Beef: 1 lb\n...",
    "failed_meals": ["Unknown Meal"]
  }
  ```
  > Note: `failed_meals` is only present if one or more meals could not be found.

---

## Ingredients Endpoints

### 7. Get Available Ingredients
- **URL**: `/api/ingredients`
- **Method**: `GET`
- **Description**: Returns a list of all available ingredients from the lookup table, used for autocomplete suggestions in the UI.
- **Example**:
  ```bash
  curl http://localhost:8080/api/ingredients
  ```
- **Response Example**:
  ```json
  [
    { "name": "Chicken Breast", "category": "Protein" },
    { "name": "Olive Oil", "category": "Pantry" }
  ]
  ```

### 8. Add Available Ingredient
- **URL**: `/api/ingredients/add`
- **Method**: `POST`
- **Description**: Adds a new ingredient to the available ingredients lookup table.
- **Example**:
  ```bash
  curl -X POST http://localhost:8080/api/ingredients/add \
       -H "Content-Type: application/json" \
       -d '{"name": "Quinoa", "category": "Grains"}'
  ```
- **Payload**:
  ```json
  { "name": "Quinoa", "category": "Grains" }
  ```
- **Success Response**: `200 OK`
- **Error Response**: `400 Bad Request` or `500 Server Error`

---

## Google Calendar Endpoints

These endpoints require Google OAuth to be authorized first (see OAuth Endpoints below).

### 9. Get Calendar Events
- **URL**: `/api/calendar/events`
- **Method**: `GET`
- **Description**: Retrieves Google Calendar events within a date/time range. Returns events grouped by calendar.
- **Query Parameters**: `timeMin` (RFC 3339 datetime), `timeMax` (RFC 3339 datetime)
- **Example**:
  ```bash
  curl "http://localhost:8080/api/calendar/events?timeMin=2026-03-24T00:00:00Z&timeMax=2026-03-30T23:59:59Z"
  ```
- **Success Response**: `200 OK` with array of calendar objects:
  ```json
  [
    {
      "summary": "My Calendar",
      "backgroundColor": "#4285f4",
      "foregroundColor": "#ffffff",
      "events": [ ... ]
    }
  ]
  ```
- **Error Response**: `403 Forbidden` if Google account is not linked:
  ```json
  { "linked": false, "message": "Google account not linked or error fetching events" }
  ```

### 10. Sync Meal Plan to Calendar
- **URL**: `/api/calendar/sync`
- **Method**: `POST`
- **Description**: Creates Google Calendar events for each meal in the weekly plan. Each meal is scheduled as a 1-hour dinner event at 18:00 UTC on the specified date.
- **Example**:
  ```bash
  curl -X POST http://localhost:8080/api/calendar/sync \
       -H "Content-Type: application/json" \
       -d '{"Monday": {"date": "2026-04-07", "meals": ["Spaghetti Bolognese"]}}'
  ```
- **Payload**:
  ```json
  {
    "Monday":    { "date": "2026-04-07", "meals": ["Spaghetti Bolognese"] },
    "Tuesday":   { "date": "2026-04-08", "meals": ["Chicken Salad"] },
    "Wednesday": { "date": "2026-04-09", "meals": [] },
    "Thursday":  { "date": "2026-04-10", "meals": [] },
    "Friday":    { "date": "2026-04-11", "meals": [] },
    "Saturday":  { "date": "2026-04-12", "meals": [] },
    "Sunday":    { "date": "2026-04-13", "meals": [] }
  }
  ```
- **Success Response**: `200 OK`
  ```json
  {
    "synced": 2,
    "failed": 0,
    "event_ids": ["eventId1", "eventId2"]
  }
  ```
- **Error Response**: `403 Forbidden` if Google account is not linked:
  ```json
  { "linked": false, "message": "Google account not linked or authorization failed" }
  ```

### 11. Create Grocery Order Event
- **URL**: `/api/calendar/order`
- **Method**: `POST`
- **Description**: Creates a Whole Foods grocery order reminder event on Google Calendar with the consolidated ingredients as the event description.
- **Payload**:
  ```json
  {
    "start": "2026-03-28T10:00:00Z",
    "end":   "2026-03-28T11:00:00Z",
    "ingredients": "Whole Foods Order - Ingredients:\n- Ground Beef: 1 lb\n..."
  }
  ```
- **Success Response**: `200 OK`
  ```json
  { "status": "success" }
  ```
- **Error Response**: `400 Bad Request` if fields are missing, `403 Forbidden` if Google account is not linked.

### 12. Delete Calendar Events
- **URL**: `/api/calendar/delete-events`
- **Method**: `POST`
- **Description**: Deletes specific Google Calendar events by their IDs. Used to undo a calendar sync.
- **Payload**:
  ```json
  { "event_ids": ["eventId1", "eventId2"] }
  ```
- **Success Response**: `200 OK`
  ```json
  { "deleted": 2 }
  ```
- **Error Response**: `400 Bad Request` if `event_ids` is missing.

---

## OAuth Endpoints

### 13. Initiate Google OAuth
- **URL**: `/auth/google`
- **Method**: `GET`
- **Description**: Redirects the browser to Google's OAuth consent screen to authorize Google Calendar access.
- **Example**: Navigate to `http://localhost:8080/auth/google` in a browser.

### 14. Google OAuth Callback
- **URL**: `/auth/google/callback`
- **Method**: `GET`
- **Description**: Handles the redirect from Google after the user grants permission. Exchanges the authorization code for access/refresh tokens and stores them (encrypted with AES-256-GCM) in the database, then redirects to `/`.
- **Query Parameters**: `code` (authorization code from Google), `state`
- **Note**: This URL must match `GOOGLE_REDIRECT_URI` configured in `docker-compose.yml` and registered in the GCP OAuth credentials.

---

## Utility Endpoints

### 15. Health Check
- **URL**: `/api/health`
- **Method**: `GET`
- **Description**: Returns a simple OK response for liveness checks.
- **Example**:
  ```bash
  curl http://localhost:8080/api/health
  ```
- **Response**: `200 OK` with plain text body `OK`
