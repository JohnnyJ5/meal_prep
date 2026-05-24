# Meal Prep Application

A C++ based meal preparation and planning application. It allows you to manage recipes, schedule meals for the week, and automatically generate consolidated grocery lists which can be emailed to you.

## Features

- **Store Meals:** Create, read, update, and delete meals and their ingredients in a local SQLite database.
- **Weekly Schedule:** Plan your meals for each day of the week with drag-and-drop scheduling.
- **Grocery List Generation:** Automatically consolidate ingredients from selected meals into a single, unified grocery list, returned via the API and optionally saved as a Google Calendar order reminder.
- **Google Calendar Integration:** Sync your weekly meal plan to Google Calendar and create grocery order reminders.
- **Web Interface:** A responsive multi-page application with separate pages for meal planning (`/planner`) and workouts (`/workouts`).
- **Workout Tracking:** Log circuit and straight-set workouts; save and reuse workout templates.
- **REST API:** A robust API backing the web interface for meal management, planning, and calendar sync.

## Prerequisites

This project uses a fully Dockerized development environment to ensure consistency. You will need:
- Docker
- Docker Compose
- `make`

## Quickstart

All development commands are wrapped in the `Makefile` and are executed inside the Docker container automatically.

1. **Set up Google Calendar credentials** (required for Calendar integration)

   Download an OAuth 2.0 client credentials file from [Google Cloud Console](https://console.cloud.google.com/):
   - Go to **APIs & Services > Credentials**
   - Click **Create Credentials > OAuth client ID**
   - Choose **Web application**, set the authorised redirect URI to `http://localhost:8080/auth/google/callback`
   - Download the JSON file and save it to `~/.meal_prep/dev_calendar_credentials.json`

   Then create your local config:
   ```bash
   cp meal_prep.conf.json.example meal_prep.conf.json
   ```
   Edit `meal_prep.conf.json` and update the path to your credentials file if needed. The server will start without this file, but Google Calendar features will not work.

2. **Build the Environment**
   ```bash
   make build
   ```
   This command starts the background containers and compiles the C++ codebase inside the container.

3. **Start the API Server**
   ```bash
   make start
   ```
   This will start the Meal Prep API server on port 8080. You can then access the web interface at [http://localhost:8080](http://localhost:8080).

4. **Stop the Environment**
   ```bash
   make stop
   ```
   Brings down the Docker containers and cleans up the active environment.

5. **Clean Build Files**
   ```bash
   make clean
   ```
   Removes the generated build directories both natively and within the container.

## Testing

To run the automated test suite:
```bash
make test
```
This command compiles the tests and runs them using `ctest` inside the Docker environment.

Additional quality targets:

| Command | Description |
| :--- | :--- |
| `make lint` | Run clang-tidy and clang-format checks via `lintenator.sh`. |
| `make lint-fix` | Same as `lint` but auto-applies fixes. |
| `make asan` | Build and test with AddressSanitizer + UBSan. |
| `make tsan` | Build and test with ThreadSanitizer. |
| `make coverage` | Build with coverage instrumentation and print a summary. |
| `make cppcheck` | Run static analysis with cppcheck. |

## Architecture

The Meal Prep application follows a modular architecture consisting of a C++ backend, a web-based frontend, and a cloud-native deployment strategy.

```mermaid
graph TD
    subgraph "Local environment"
        User(["User (Browser)"]) --> Planner["/planner page"]
        User --> Workouts["/workouts page"]
        Planner --> MW["RequestTimerMiddleware"]
        Workouts --> MW
        MW --> API["C++ Crow API Server"]
        API --> MealsRepo["MealsRepository"]
        API --> WorkoutsRepo["WorkoutsRepository"]
        API --> TokensRepo["GoogleTokensRepository"]
        MealsRepo --> Conn["DbConnection (sqlite3*)"]
        WorkoutsRepo --> Conn
        TokensRepo --> Conn
        Conn --> DB[("SQLite Database")]
        API --> MF["MealFactory / MealPlanner"]
        MF --> MealsRepo
        API --> OAuth["GoogleOAuth"]
        OAuth --> TokensRepo
        TokensRepo --> TE["TokenEncryption (AES-256-GCM)"]
        API --> Cal["CalendarService (REST)"]
    end

    subgraph "Google APIs"
        OAuth --> GAuth["Google OAuth 2.0"]
        Cal --> GCal["Google Calendar API"]
    end

    subgraph "Google Cloud Platform"
        CB["Cloud Build"] --> GCR["Container Registry"]
        GCR --> CR["Cloud Run (Serverless)"]
        CR --> GCS[("Cloud Storage (Persistent DB)")]
        CR --> SM["Secrets Manager"]
    end

    API -.->|Deployment| CB
```

### Components
- **Backend (C++):** Built on Crow, organized as feature folders (`src/features/meals`, `src/features/workouts`) over a shared core (`src/core/{db,http,config,util}`). Each feature owns its own routes, repository, and tests. `RequestTimerMiddleware` logs request durations on all routes.
- **Frontend:** Multi-page. Each page (`/planner`, `/workouts`) is an independent HTML document under `static/pages/<name>/`, sharing chrome (sidebar + topbar) via `static/shared/chrome.js`.
- **Database:** SQLite stores recipes, schedules, workouts, templates, and encrypted OAuth tokens. A single `DbConnection` is shared across per-feature repositories (`MealsRepository`, `WorkoutsRepository`, `GoogleTokensRepository`). In production, the database file is persisted on Google Cloud Storage via FUSE mount.
- **Google Integration:** OAuth 2.0 Authorization Code Flow for Google Calendar access. Tokens are stored encrypted with AES-256-GCM via `TokenEncryption` (requires `MEAL_PREP_TOKEN_KEY` env var).
- **MealFactory / MealPlanner:** `MealFactory` constructs `Meal` objects from `MealsRepository`; `MealPlanner` consolidates ingredients across a plan.
- **Infrastructure:** Dockerized for local development and deployed to Google Cloud Run for scalability.

## Workflow

The project follows a streamlined development-to-deployment workflow:

1.  **Local Development:**
    - Use `make build` to set up the Docker environment and compile the code.
    - Run `make start` to launch the API and Web UI locally on [http://localhost:8080](http://localhost:8080).
2.  **Testing:**
    - Execute `make test` to run the suite of automated C++ unit tests.
3.  **Deployment:**
    - Changes are pushed to production via `gcloud builds submit`, which triggers the configuration in `cloudbuild.yaml`.
    - The application is containerized and deployed to Google Cloud Run.
    - Persistence is maintained by syncing the SQLite `meals.db` with GCS before and after service execution.
    - To verify the deployment status and get the live URL, run: `gcloud run services describe meal-prep --region us-central1`

## Project Structure

```
src/
  core/
    config/    config_parser
    db/        db_connection, schema
    http/      api_routes (aggregator), static_routes, middleware
    util/      curl_utils
  features/
    meals/     meal, ingredient, measurement, meal_factory, meal_planner,
               meals_repository, meals_routes
    workouts/  workout, workouts_repository, workouts_routes
  integrations/
    google/    google_oauth, calendar_service, token_encryption,
               google_tokens_repository, google_routes
  main.cpp
tests/         Mirrors src/ (tests/features/meals/, tests/integrations/google/, …)
static/
  shared/      base.css, chrome.js, favicon.ico
  pages/
    planner/   index.html, planner.js   (served at /planner)
    workouts/  index.html, workouts.js  (served at /workouts)
```

Adding a new page is a localised change: create `src/features/<name>/<name>_repository.{h,cpp}` and `<name>_routes.{h,cpp}`, add one `register<Name>Routes()` line to `src/core/http/api_routes.cpp`, and create `static/pages/<name>/index.html` + a one-line entry in `static/shared/chrome.js`.

Other directories:

- `docs/` — additional project documentation:
    - [API Reference](docs/API.md)
    - [GCP Commands](docs/GCP_COMMANDS.md)
    - [Docker Commands](docs/DOCKER_COMMANDS.md)
    - [DB Dump Job](docs/DB_DUMP_JOB.md)
- `Dockerfile` & `docker-compose.yml` — definitions for the Docker development environment.
- `Makefile` — shortcuts for building, starting, and testing the project.
