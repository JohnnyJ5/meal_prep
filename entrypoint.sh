#!/usr/bin/env bash
set -e

# If HOST_UID/HOST_GID are provided, remap the 'claude' user to match the
# host user that owns the mounted volume. This prevents permission mismatches
# when run_claude.sh creates files on the host and places them in the mount.
if [ -n "${HOST_UID}" ] && [ "${HOST_UID}" != "$(id -u claude)" ]; then
    # If another user already owns the target UID (e.g. the 'ubuntu' user
    # pre-installed in ubuntu:24.04), bump it out of the way first.
    conflicting_user=$(getent passwd "${HOST_UID}" | cut -d: -f1 || true)
    if [ -n "$conflicting_user" ] && [ "$conflicting_user" != "claude" ]; then
        usermod -u 65534 "$conflicting_user"
    fi
    usermod -u "${HOST_UID}" claude
fi

if [ -n "${HOST_GID}" ] && [ "${HOST_GID}" != "$(id -g claude)" ]; then
    # Same treatment for GID conflicts.
    conflicting_group=$(getent group "${HOST_GID}" | cut -d: -f1 || true)
    if [ -n "$conflicting_group" ] && [ "$conflicting_group" != "claude" ]; then
        groupmod -g 65534 "$conflicting_group"
    fi
    groupmod -g "${HOST_GID}" claude
fi

exec gosu claude "$@"
