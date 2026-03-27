# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Commands

All development runs inside Docker via the Makefile:

```bash
make build    # Build Docker image and compile C++ code
make start    # Start API server on port 8080
make stop     # Bring down Docker containers
make test     # Run test suite with ctest inside container
make clean    # Remove build_docker/ and build/ directories
```

The local directory is volume-mounted into the container at `/home/devuser/meal_prep`, so code changes are reflected without rebuilding.

To run a single test target, exec into the container and use ctest:
```bash
docker compose exec meal_prep_dev bash
cd build && ctest -R test_meal_planner --output-on-failure
```

## Architecture

C++ backend (Crow web framework) + single-page HTML/JS frontend + SQLite database.

**Request flow:** Browser → Crow HTTP server (`src/api_routes.cpp`) → Service layer → `DBManager` (SQLite)

**Key service classes:**
- `DBManager` — all SQLite operations; tables: `meals`, `ingredients`, `available_ingredients`, `oauth_tokens`
- `MealFactory` — constructs `Meal` objects from DB rows
- `MealPlanner` — consolidates ingredients across a meal plan, formats output for email
- `EmailService` — sends grocery lists via SMTP using libcurl
- `GoogleOAuth` — Authorization Code Flow, token exchange and refresh, stores tokens in DB
- `CalendarService` — creates/lists Google Calendar events via REST API
- `RequestTimerMiddleware` (`src/middleware.h`) — logs request duration, applied to all Crow routes

**Domain models:** `Meal` → has many `Ingredient`s, each with a `Measurement`. `Measurement` handles unit conversions (cups, tbsp, grams, etc.).

**Frontend** (`static/`) is a single-page app served as static files by Crow. Drag-and-drop scheduling, meal CRUD via fetch calls, Google Calendar link button.

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

Credential files are stored in `~/.meal_prep/` on the host and mounted read-only into the container. `GOOGLE_REDIRECT_URI` is set via docker-compose env.

Database path: `meals.db` locally; `/mnt/db/meals.db` on GCP (GCS bucket mount).

## Build System

CMake (C++17) with FetchContent for: Crow, Asio, GoogleTest. System deps: Boost, OpenSSL, CURL, SQLite3. Builds `meal_prep_lib` (static) + `meal_prep` executable. Tests are a separate `meal_prep_tests` target.

## Cloud Deployment

`cloudbuild.yaml` defines a 3-step pipeline: build image → push to Artifact Registry → deploy to Cloud Run. The GCS bucket `meal-prep-db-bucket` is mounted for persistent SQLite storage. Secrets for email/calendar credentials are injected via Cloud Secrets Manager. See `docs/GCP_COMMANDS.md` for deployment and database management commands.
