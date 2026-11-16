#!/usr/bin/env bash
# build_package.sh — builds a self-contained Ubuntu .deb package for meal-prep
# locally, bundling the binary, static assets, systemd unit, pre-configured
# meal_prep.conf.json, seed meals.db, and calendar_credentials.json.
#
# Usage:
#   ./scripts/build_package.sh [--install]
#
# Options:
#   --install    Automatically run 'sudo apt install' on the generated .deb after building
#   -h, --help   Show this help message

set -euo pipefail
umask 022

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
BUILD_DIR="${PROJECT_ROOT}/build_pkg"

AUTO_INSTALL=false

usage() {
  sed -n '2,12p' "${BASH_SOURCE[0]}" | sed 's/^# \{0,1\}//'
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --install)
      AUTO_INSTALL=true
      shift
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      echo "Unknown option: $1" >&2
      usage >&2
      exit 1
      ;;
  esac
done

VERSION="$(grep -E 'project\(MealPrep VERSION [0-9]+\.[0-9]+\.[0-9]+' "${PROJECT_ROOT}/CMakeLists.txt" \
  | sed -E 's/.*VERSION ([0-9]+\.[0-9]+\.[0-9]+).*/\1/')"
if [[ -z "${VERSION}" ]]; then
  VERSION="1.0.0"
fi

echo "============================================"
echo " Building meal-prep ${VERSION} (.deb)"
echo "============================================"

mkdir -p "${BUILD_DIR}"
CMAKE_EXTRA_ARGS=()
if [[ -d "${PROJECT_ROOT}/build/_deps/googletest-src" ]]; then
  CMAKE_EXTRA_ARGS+=("-DFETCHCONTENT_SOURCE_DIR_GOOGLETEST=${PROJECT_ROOT}/build/_deps/googletest-src")
fi
if [[ -d "${PROJECT_ROOT}/build/_deps/asio-src" ]]; then
  CMAKE_EXTRA_ARGS+=("-DFETCHCONTENT_SOURCE_DIR_ASIO=${PROJECT_ROOT}/build/_deps/asio-src")
fi
if [[ -d "${PROJECT_ROOT}/build/_deps/crow-src" ]]; then
  CMAKE_EXTRA_ARGS+=("-DFETCHCONTENT_SOURCE_DIR_CROW=${PROJECT_ROOT}/build/_deps/crow-src")
fi

(
  cd "${BUILD_DIR}"
  cmake -DCMAKE_BUILD_TYPE=Release "${CMAKE_EXTRA_ARGS[@]}" "${PROJECT_ROOT}"
  make -j"$(nproc)" meal_prep
  cpack -G DEB
)

DEB_FILE="$(find "${BUILD_DIR}" -maxdepth 1 -name "meal-prep_${VERSION}_*.deb" | head -n 1)"
if [[ -z "${DEB_FILE}" || ! -f "${DEB_FILE}" ]]; then
  echo "Error: Failed to locate generated .deb file in ${BUILD_DIR}" >&2
  exit 1
fi

DEB_BASENAME="$(basename "${DEB_FILE}")"

echo ""
echo "============================================"
echo " Verifying Package: ${DEB_BASENAME}"
echo "============================================"
if command -v dpkg-deb >/dev/null 2>&1; then
  dpkg-deb --info "${DEB_FILE}"
  echo "--- Package Contents ---"
  dpkg-deb --contents "${DEB_FILE}"
fi

echo ""
echo "Package ready: ${DEB_FILE}"
echo "To install   : sudo apt install ${DEB_FILE}"

if "${AUTO_INSTALL}"; then
  echo ""
  echo "Installing ${DEB_FILE}..."
  sudo apt install -y "${DEB_FILE}"
  sudo systemctl status meal-prep --no-pager
fi
