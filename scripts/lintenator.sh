#!/usr/bin/env bash
# lintenator.sh — runs clang-tidy and clang-format checks on project sources.
#
# Usage:
#   ./scripts/lintenator.sh [--build-dir <dir>] [--fix]
#
# Options:
#   --build-dir <dir>   Directory containing compile_commands.json (default: build)
#   --fix               Apply clang-tidy fixes automatically (default: check only)
#
# Exit codes:
#   0  All checks passed
#   1  One or more checks failed

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

BUILD_DIR="${PROJECT_ROOT}/build"
FIX_MODE=false
ERRORS=0

# Parse arguments
while [[ $# -gt 0 ]]; do
  case "$1" in
    --build-dir)
      BUILD_DIR="$2"
      shift 2
      ;;
    --fix)
      FIX_MODE=true
      shift
      ;;
    *)
      echo "Unknown argument: $1" >&2
      exit 1
      ;;
  esac
done

COMPILE_COMMANDS="${BUILD_DIR}/compile_commands.json"

if [[ ! -f "${COMPILE_COMMANDS}" ]]; then
  echo "Error: compile_commands.json not found at ${COMPILE_COMMANDS}" >&2
  echo "Run cmake with -DCMAKE_EXPORT_COMPILE_COMMANDS=ON first." >&2
  exit 1
fi

# Source files to lint (project sources only, excluding third-party)
PROJECT_SOURCES=(
  src/db_manager.cpp
  src/meal_factory.cpp
  src/email_service.cpp
  src/meal_planner.cpp
  src/api_routes.cpp
  src/config_parser.cpp
  src/google_oauth.cpp
  src/calendar_service.cpp
  src/token_encryption.cpp
  src/main.cpp
  tests/test_ingredient.cpp
  tests/test_meal.cpp
  tests/test_meal_factory.cpp
  tests/test_measurement.cpp
  tests/test_meal_planner.cpp
)

# ── clang-tidy ────────────────────────────────────────────────────────────────
echo "============================================"
echo " Running clang-tidy"
echo "============================================"

TIDY_ARGS=(-p "${BUILD_DIR}" --extra-arg=-std=c++20)
if $FIX_MODE; then
  TIDY_ARGS+=(--fix --fix-errors)
fi

for src in "${PROJECT_SOURCES[@]}"; do
  full_path="${PROJECT_ROOT}/${src}"
  if [[ ! -f "${full_path}" ]]; then
    echo "  [SKIP] ${src} (not found)"
    continue
  fi
  echo "  Checking ${src}..."
  tidy_out=$(clang-tidy "${TIDY_ARGS[@]}" "${full_path}" 2>&1 || true)
  # Only fail on diagnostics in project source files, not third-party deps
  if echo "${tidy_out}" | grep -qE "^${PROJECT_ROOT}/(src|tests)/[^/]+\.(cpp|h):[0-9]+:[0-9]+: (warning|error):"; then
    echo "${tidy_out}"
    ERRORS=$((ERRORS + 1))
  fi
done

# ── clang-format ──────────────────────────────────────────────────────────────
echo ""
echo "============================================"
echo " Running clang-format"
echo "============================================"

FORMAT_SOURCES=()
for src in "${PROJECT_SOURCES[@]}"; do
  full_path="${PROJECT_ROOT}/${src}"
  [[ -f "${full_path}" ]] && FORMAT_SOURCES+=("${full_path}")
done

# Also include header files
while IFS= read -r -d '' hdr; do
  FORMAT_SOURCES+=("${hdr}")
done < <(find "${PROJECT_ROOT}/src" -maxdepth 1 -name '*.h' -print0)

if $FIX_MODE; then
  echo "  Applying clang-format..."
  clang-format -i "${FORMAT_SOURCES[@]}"
  echo "  Done."
else
  echo "  Checking formatting..."
  FORMAT_ERRORS=0
  for f in "${FORMAT_SOURCES[@]}"; do
    rel="${f#"${PROJECT_ROOT}/"}"
    if ! clang-format --dry-run --Werror "${f}" 2>/dev/null; then
      echo "  [FAIL] ${rel} — formatting issue (run with --fix to auto-correct)"
      FORMAT_ERRORS=$((FORMAT_ERRORS + 1))
    fi
  done
  if [[ "${FORMAT_ERRORS}" -eq 0 ]]; then
    echo "  All files correctly formatted."
  else
    ERRORS=$((ERRORS + FORMAT_ERRORS))
  fi
fi

# ── Summary ───────────────────────────────────────────────────────────────────
echo ""
echo "============================================"
if [[ "${ERRORS}" -eq 0 ]]; then
  echo " lintenator: all checks passed"
  echo "============================================"
  exit 0
else
  echo " lintenator: ${ERRORS} check(s) failed"
  echo "============================================"
  exit 1
fi
