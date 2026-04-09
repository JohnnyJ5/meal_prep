#!/usr/bin/env bash
# clang-format.sh — runs clang-format checks on project sources.
#
# Usage:
#   ./scripts/clang-format.sh [--fix]
#
# Options:
#   --fix   Apply clang-format fixes automatically (default: check only)
#
# Exit codes:
#   0  All checks passed
#   1  One or more checks failed

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

FIX_MODE=false
ERRORS=0

# Parse arguments
while [[ $# -gt 0 ]]; do
  case "$1" in
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

# Source files to format (project sources only, excluding third-party)
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
  for f in "${FORMAT_SOURCES[@]}"; do
    rel="${f#"${PROJECT_ROOT}/"}"
    if ! clang-format --dry-run --Werror "${f}" 2>/dev/null; then
      echo "  [FAIL] ${rel} — formatting issue (run with --fix to auto-correct)"
      ERRORS=$((ERRORS + 1))
    fi
  done
  if [[ "${ERRORS}" -eq 0 ]]; then
    echo "  All files correctly formatted."
  fi
fi

echo ""
echo "============================================"
if [[ "${ERRORS}" -eq 0 ]]; then
  echo " clang-format: all checks passed"
  echo "============================================"
  exit 0
else
  echo " clang-format: ${ERRORS} check(s) failed"
  echo "============================================"
  exit 1
fi
