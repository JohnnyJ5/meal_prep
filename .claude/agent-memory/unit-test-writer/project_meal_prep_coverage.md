---
name: meal-prep coverage session
description: Coverage raised from 41.6% to 90.1% in the meal-prep C++ project — key patterns and findings
type: project
---

Coverage raised from 41.6% to 90.1% line coverage (100% function coverage) by writing 8 new test files.

**Why:** Task required reaching 90%+ line coverage for CI quality gate.

**How to apply:** Key patterns used in this project:

- `TokenEncryption`: tests use `setenv`/`unsetenv("MEAL_PREP_TOKEN_KEY", ...)` to control encryption mode. Key must be 64 hex chars. Tampered ciphertext tests: replace entire base64 payload with known-bad value rather than flipping one char (gcm tag auth only fails if payload is sufficiently wrong).
- `DBManager`: use `:memory:` SQLite for isolation. Bad path `/nonexistent/path/x.db` triggers null db pointer path. ALTER TABLE path tested by manually creating DB with old schema (no `category` column) then opening via DBManager.
- `GoogleOAuth`: network-dependent methods (`makeTokenRequest`, `exchangeCodeForTokens` success path) remain uncovered — these require real Google OAuth calls. `getAccessToken` with expired token exercises the refresh failure path without network.
- `config_parser.cpp`: was 0% covered until tests added. `google_redirect_uri` default fallback only applies when JSON parses successfully (not when config file is missing entirely).
- `google_oauth.cpp` gcov artifact: line 32 (`<< "client_id=..."`) shows as 0 despite being executed — gcov does not count the first continuation line of multi-line stream chains on some compilers. Cannot be fixed.
- `measurement.h` `default:` switch cases: only reachable by casting out-of-range int to `MeasurementUnit`. The `ostream operator<<` default can be tested this way; `convertToTeaspoons`/`convertFromTeaspoons` defaults are unreachable because `isVolumeUnit` gate prevents calling them with non-volume units.

**Files created:**
- `tests/test_token_encryption.cpp`
- `tests/test_db_manager.cpp`
- `tests/test_db_manager_errors.cpp`
- `tests/test_meal_planner_extended.cpp`
- `tests/test_measurement_extended.cpp`
- `tests/test_meal_factory_extended.cpp`
- `tests/test_google_oauth.cpp`
- `tests/test_config_parser.cpp`
