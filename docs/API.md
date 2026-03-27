# Meal Prep REST API

The Meal Prep application provides a RESTful API to manage meals and generate grocery lists. The API is accessible when the server is running (e.g., via `make start`).

[← Back to README](../README.md)

All API responses are in JSON format.

## Endpoints

### 1. Get All Meals
- **URL**: `/api/meals`
- **Method**: `GET`
- **Description**: Returns a list of all available meal names in the database.
- **Example**:
  ```bash
  curl http://localhost:8080/api/meals
  ```
- **Response Example**:
  ```json
  [
    "Spaghetti Bolognese",
    "Chicken Salad"
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
       -d '{"name": "New Meal", "ingredients": []}'
  ```
- **Payload Example**:
  ```json
  {
    "name": "Spaghetti Bolognese",
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
- **Description**: Updates the ingredients of an existing meal.
- **Example**:
  ```bash
  curl -X PUT http://localhost:8080/api/meals/Existing%20Meal \
       -H "Content-Type: application/json" \
       -d '{"name": "Updated Name", "ingredients": []}'
  ```
- **Payload Example**: Same format as adding a new meal.
- **Success Response**: `200 OK`
- **Error Response**: `500 Server Error`

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

### 6. Generate Weekly Plan & Email
- **URL**: `/api/plan`
- **Method**: `POST`
- **Description**: Submits a weekly meal schedule. This endpoint consolidates the required ingredients into a single grocery list and triggers an email to the configured address.
- **Example**:
  ```bash
  curl -X POST http://localhost:8080/api/plan \
       -H "Content-Type: application/json" \
       -d '{"Monday": ["Spaghetti Bolognese"], ...}'
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
    "schedule": "Consolidated text schedule...",
    "failed_meals": ["Unknown Meal"]
  }
  ```

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
- **Description**: Retrieves Google Calendar events within a date range.
- **Query Parameters**: `start` (ISO 8601 date), `end` (ISO 8601 date)
- **Example**:
  ```bash
  curl "http://localhost:8080/api/calendar/events?start=2026-03-24&end=2026-03-30"
  ```
- **Success Response**: `200 OK` with array of calendar event objects.
- **Error Response**: `401 Unauthorized` if not authenticated, `500 Server Error`

### 10. Sync Meal Plan to Calendar
- **URL**: `/api/calendar/sync`
- **Method**: `POST`
- **Description**: Creates Google Calendar events for each meal in the weekly plan.
- **Example**:
  ```bash
  curl -X POST http://localhost:8080/api/calendar/sync \
       -H "Content-Type: application/json" \
       -d '{"Monday": ["Spaghetti Bolognese"], "Tuesday": ["Chicken Salad"], ...}'
  ```
- **Payload**: Same format as `/api/plan`.
- **Success Response**: `200 OK`
- **Error Response**: `401 Unauthorized` if not authenticated, `500 Server Error`

### 11. Create Grocery Order Event
- **URL**: `/api/calendar/order`
- **Method**: `POST`
- **Description**: Creates a Whole Foods grocery order reminder event on Google Calendar.
- **Payload**:
  ```json
  { "date": "2026-03-28", "time": "10:00" }
  ```
- **Success Response**: `200 OK`
- **Error Response**: `401 Unauthorized`, `500 Server Error`

### 12. Delete Calendar Events
- **URL**: `/api/calendar/delete-events`
- **Method**: `POST`
- **Description**: Deletes specific Google Calendar events by their IDs.
- **Payload**:
  ```json
  { "event_ids": ["eventId1", "eventId2"] }
  ```
- **Success Response**: `200 OK`
- **Error Response**: `401 Unauthorized`, `500 Server Error`

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
- **Description**: Handles the redirect from Google after the user grants permission. Exchanges the authorization code for access/refresh tokens and stores them (encrypted) in the database.
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
- **Response**: `200 OK` with `{"status": "ok"}`
