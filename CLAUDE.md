# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Commands

All development runs inside Docker via the Makefile:

```bash
make build      # Build Docker image and compile C++ code
make start      # Start API server on port 8080
make stop       # Bring down Docker containers
make test       # Run test suite with ctest inside container
make clean      # Remove build_docker/ and build/ directories
make lint       # Run clang-tidy + clang-format checks (lintenator.sh)
make lint-fix   # Same as lint but auto-applies fixes
make asan       # Build and test with AddressSanitizer + UBSan
make tsan       # Build and test with ThreadSanitizer
make coverage   # Build with coverage instrumentation; print summary
make cppcheck   # Run cppcheck static analysis
```

The local directory is volume-mounted into the container at `/home/devuser/meal_prep`, so code changes are reflected without rebuilding.

To run a single test target, exec into the container and use ctest:
```bash
docker compose exec meal_prep_dev bash
cd build && ctest -R test_meal_planner --output-on-failure
```

## Architecture

C++ backend (Crow web framework) + multi-page HTML/JS frontend + SQLite database.

### Source layout

`src/` is organized as feature folders on top of a shared core. Adding a new page = add a new `src/features/<name>/` folder and one `register<Name>Routes()` call in `src/core/http/api_routes.cpp`.

```
src/
  core/
    config/       config_parser  — meal_prep.conf.json loader
    db/           db_connection  — RAII sqlite3* wrapper + shared mutex
                  schema         — initializeSchema() runs CREATE TABLE + migrations
    http/         api_routes     — aggregator: calls register*Routes() per feature
                  static_routes  — /planner, /workouts, /shared/<>, /pages/<>
                  middleware     — RequestTimerMiddleware
    util/         curl_utils     — shared HTTP helpers
  features/
    meals/        meal, ingredient, measurement, meal_factory, meal_planner
                  meals_repository  — meals/ingredients CRUD + default seed
                  meals_routes      — /api/meals, /api/ingredients, /api/plan
    workouts/     workout
                  workouts_repository — workouts + workout_templates CRUD
                  workouts_routes     — /api/workouts, /api/workout-templates
  integrations/
    google/       google_oauth, calendar_service, token_encryption
                  google_tokens_repository — encrypted OAuth token storage
                  google_routes            — /auth/google/*, /api/calendar/*, /
  main.cpp        Constructs DbConnection + repos + factory + services, wires routes
```

Include paths are rooted at `src/`, so includes are path-qualified: `#include "features/meals/meal.h"`, `#include "core/db/db_connection.h"`.

### Request flow

Browser → Crow HTTP server → per-feature `register*Routes()` (in `src/features/<name>/<name>_routes.cpp`) → feature repository (in `src/features/<name>/<name>_repository.cpp`) → `DbConnection` (SQLite).

### Key classes

- `DbConnection` (`core/db/db_connection.h`) — owns the single `sqlite3*` handle. Shared via `std::shared_ptr` to every repository; the per-connection `std::recursive_mutex` serializes multi-statement transactions across repositories.
- `MealsRepository`, `WorkoutsRepository`, `GoogleTokensRepository` — per-feature CRUD. Each takes a `shared_ptr<DbConnection>`. Tables they own:
  - `MealsRepository` → `meals`, `ingredients`, `available_ingredients`
  - `WorkoutsRepository` → `workouts`, `workout_blocks`, `workout_exercises`, `workout_templates`, `template_blocks`, `template_exercises`
  - `GoogleTokensRepository` → `google_tokens` (encrypted at rest)
- `MealFactory` — constructs `Meal` objects from `MealsRepository`, filtering optional ingredients.
- `MealPlanner` (`features/meals/meal_planner.h`) — consolidates ingredients across a plan, formats output for email.
- `GoogleOAuth` — Authorization Code Flow; takes a `GoogleTokensRepository` for persistence.
- `CalendarService` — creates/lists/deletes Google Calendar events.
- `TokenEncryption` (`integrations/google/token_encryption.h`) — AES-256-GCM for OAuth tokens; reads `MEAL_PREP_TOKEN_KEY` (64 hex chars). Falls back to plaintext with a warning if unset.
- `RequestTimerMiddleware` (`core/http/middleware.h`) — logs request duration, applied to all Crow routes.

### Domain models

`Meal` → has many `Ingredient`s, each with a `Measurement`. `Measurement` handles unit conversions (cups, tbsp, grams, etc.). Workouts have a `Workout` → many `WorkoutBlock` → many `WorkoutExercise` shape; `WorkoutTemplate` shares the same nested structure minus the date/duration fields.

### Frontend

Multi-page, served as static files by Crow. Each page is its own HTML document under `static/pages/<name>/index.html`:

```
static/
  shared/
    base.css        Design tokens, sidebar, topbar, buttons, modal
    chrome.js       mountChrome({activePage, crumbLabel, weekLabel})
                    injects the sidebar + topbar at the top of <body>
    favicon.ico
  pages/
    planner/
      index.html    Served at /planner
      planner.js    Drag-and-drop scheduling, meal CRUD, Google Calendar
    workouts/
      index.html    Served at /workouts
      workouts.js   Log workouts, manage templates
```

Adding a new page: create `static/pages/<name>/index.html` (link `/shared/base.css`, include `/shared/chrome.js`, call `mountChrome({activePage:'<name>', ...})`), add the page descriptor to the `PAGES` array in `chrome.js`, and add a `CROW_ROUTE(app, "/<name>")` in `core/http/static_routes.cpp`.

## Configuration

Runtime config is loaded from `meal_prep.conf.json` (project root):
```json
{
    "port": 8080,
    "email_recipients": [...],
    "email_credentials_file": "/home/devuser/.meal_prep/email_credentials.json",
    "gmail_calendar_credentials_file": "/home/devuser/.meal_prep/dev_calendar_credentials.json"
}
```

Credential files are stored in `~/.meal_prep/` on the host and mounted read-only into the container.

Environment variables (set in `docker-compose.yml` or Cloud Run):
- `GOOGLE_REDIRECT_URI` — OAuth callback URL (e.g. `http://localhost:8080/auth/google/callback`)
- `MEAL_PREP_TOKEN_KEY` — 64 hex char (256-bit) key for AES-256-GCM token encryption. Generate with: `openssl rand -hex 32`

Database path: `meals.db` locally; `/mnt/db/meals.db` on GCP (GCS bucket mount).

## Build System

CMake (C++17) with FetchContent for: Crow, Asio, GoogleTest. System deps: Boost, OpenSSL, CURL, SQLite3. Builds `meal_prep_lib` (static) + `meal_prep` executable. Tests are a separate `meal_prep_tests` target.

## Cloud Deployment

`cloudbuild.yaml` defines a 3-step pipeline: build image → push to Artifact Registry → deploy to Cloud Run. The GCS bucket `meal-prep-db-bucket` is mounted for persistent SQLite storage. Secrets for email/calendar credentials are injected via Cloud Secrets Manager. See `docs/GCP_COMMANDS.md` for deployment and database management commands.

## Working with the UI

When making any UI/frontend changes (HTML, CSS, JS in `static/`), always produce screenshot images of the result and attach them via `SendUserFile` so the user can see before/after visuals. This applies to every UI change, not just when explicitly requested.

The pages are independent documents — switching pages reloads the browser. There is no SPA router; navigation uses regular `<a href="/planner">`/`<a href="/workouts">` links rendered by `chrome.js`. If you need a value to persist across pages, save it server-side or in `localStorage`.
