#!/usr/bin/env bash
set -e

# If HOST_UID/HOST_GID are provided, remap the 'claude' user to match the
# host user that owns the mounted volume. This prevents permission mismatches
# when run_claude.sh creates files on the host and places them in the mount.
if [ -n "${HOST_UID}" ] && [ "${HOST_UID}" != "$(id -u claude)" ]; then
    usermod -u "${HOST_UID}" claude
fi

if [ -n "${HOST_GID}" ] && [ "${HOST_GID}" != "$(id -g claude)" ]; then
    groupmod -g "${HOST_GID}" claude
fi

exec gosu claude "$@"
