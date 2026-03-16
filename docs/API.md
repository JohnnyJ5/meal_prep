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
