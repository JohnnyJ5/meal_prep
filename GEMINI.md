# GEMINI.md

This file provides guidance to Gemini / Antigravity when working with code in this repository.

## Commands

All development runs inside Docker via the Makefile (or directly via CMake/CTest when building in the local environment):

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

To run a single test target, exec into the container and use `ctest`:
```bash
docker compose exec meal_prep_dev bash
cd build && ctest -R test_meal_planner --output-on-failure
```

## Architecture

C++ backend (Crow web framework) + single-page HTML/JS frontend + SQLite database.

**Request flow:** Browser → Crow HTTP server (`src/api_routes.cpp`) → Service layer → `DBManager` (SQLite)

**Key service classes:**
- `DBManager` — all SQLite operations; tables: `meals`, `ingredients`, `available_ingredients`, `google_tokens`
- `MealFactory` — constructs `Meal` objects from DB rows
- `MealPlanner` — consolidates ingredients across a meal plan, formats output for email
- `EmailService` — sends grocery lists via SMTP using libcurl
- `GoogleOAuth` — Authorization Code Flow, token exchange and refresh, stores tokens in DB
- `CalendarService` — creates/lists/deletes Google Calendar events via REST API
- `TokenEncryption` (`src/token_encryption.h`) — AES-256-GCM encryption for OAuth tokens stored in SQLite; reads key from `MEAL_PREP_TOKEN_KEY` env var (64 hex chars). Falls back to plaintext with a warning if unset.
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

Credential files are stored in `~/.meal_prep/` on the host and mounted read-only into the container.

Environment variables (set in `docker-compose.yml` or Cloud Run):
- `GOOGLE_REDIRECT_URI` — OAuth callback URL (e.g. `http://localhost:8080/auth/google/callback`)
- `MEAL_PREP_TOKEN_KEY` — 64 hex char (256-bit) key for AES-256-GCM token encryption. Generate with: `openssl rand -hex 32`

Database path: `meals.db` locally; `/mnt/db/meals.db` on GCP (GCS bucket mount).

## Build System

CMake (C++17) with FetchContent for: Crow, Asio, GoogleTest. System deps: Boost, OpenSSL, CURL, SQLite3. Builds `meal_prep_lib` (static) + `meal_prep` executable. Tests are a separate `meal_prep_tests` target.

## Unit Testing & Coverage Guidelines

The project maintains a 90%+ line coverage quality gate (currently ~90.1% line coverage, 100% function coverage). Keep these patterns in mind when writing or updating tests in `tests/`:

- **`TokenEncryption`**: Tests use `setenv`/`unsetenv("MEAL_PREP_TOKEN_KEY", ...)` to control encryption mode. Key must be 64 hex chars. For tampered ciphertext tests, replace the entire base64 payload with a known-bad value rather than flipping one character (GCM tag auth only fails reliably if the payload is sufficiently wrong).
- **`DBManager`**: Use `:memory:` SQLite for isolation. A bad path such as `/nonexistent/path/x.db` triggers the null DB pointer path. The `ALTER TABLE` migration path is tested by manually creating a DB with the old schema (no `category` column) and then opening it via `DBManager`.
- **`GoogleOAuth`**: Network-dependent methods (`makeTokenRequest`, `exchangeCodeForTokens` success path) require real Google OAuth calls. Calling `getAccessToken` with an expired token exercises the refresh failure path without network access.
- **`config_parser.cpp`**: The `google_redirect_uri` default fallback only applies when JSON parses successfully (not when the config file is missing entirely).
- **`google_oauth.cpp` gcov artifact**: Line 32 (`<< "client_id=..."`) may show as `0` in `gcov` despite being executed because `gcov` does not count the first continuation line of multi-line stream chains on some compilers.
- **`measurement.h` `default:` switch cases**: Only reachable by casting an out-of-range `int` to `MeasurementUnit`. The `ostream operator<<` default can be tested this way; `convertToTeaspoons`/`convertFromTeaspoons` defaults are unreachable because the `isVolumeUnit` gate prevents calling them with non-volume units.

## Cloud Deployment

`cloudbuild.yaml` defines a 3-step pipeline: build image → push to Artifact Registry → deploy to Cloud Run. The GCS bucket `meal-prep-db-bucket` is mounted for persistent SQLite storage. Secrets for email/calendar credentials are injected via Cloud Secrets Manager. See `docs/GCP_COMMANDS.md` for deployment and database management commands.

## Project Skills (`.agents/skills/`)

- **`heic-recipe`** (`.agents/skills/heic-recipe/SKILL.md`) — Parse a HEIC/JPG/PNG photo of a recipe and upload it to the production meal-prep API.
- **`pdf-recipe-parser`** (`.agents/skills/pdf-recipe-parser/SKILL.md`) — Parse a recipe PDF into a JSON payload and upload it to the meal-prep database after user review.
- **`documentation-management`** (`.agents/skills/documentation-management/SKILL.md`) — Guidelines and procedures for keeping `README.md` and `docs/` synchronized with code and deployment changes.

## Working with the UI

When making any UI/frontend changes (HTML, CSS, JS in `static/`), always capture before/after screenshot images or artifacts and embed them in the artifact directory so the user can visually verify the result. This applies to every UI change, not just when explicitly requested.
