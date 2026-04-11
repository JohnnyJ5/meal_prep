#!/usr/bin/env bash

CONTAINER_NAME="claude-workspace"
DOTFILES_REPO="git@github.com:JohnnyJ5/dotfiles.git"
DOTFILES_DIR="/home/claude/dotfiles"
CLAUDE_CONFIG_DIR="$HOME/.config"

DOCKER_COMMON=(
    # -e HOME=/app/.claude_workspace_env
    -e GIT_SSH_COMMAND="ssh -i /home/claude/.ssh/claude_github -o StrictHostKeyChecking=no -o IdentitiesOnly=yes"
    -e HOST_UID="$(id -u)"
    -e HOST_GID="$(id -g)"
    -v "$(pwd)":/app
    -v "$HOME/.ssh/claude_github:/home/claude/.ssh/claude_github:ro"
)

setup_gh_token() {
if [ -f "$CLAUDE_CONFIG_DIR/gh/claude_gh_token" ]; then
    GH_TOKEN_VALUE=$(cat "$CLAUDE_CONFIG_DIR/gh/claude_gh_token")
elif [ -n "$GH_TOKEN" ]; then
    GH_TOKEN_VALUE="$GH_TOKEN"
else
    echo "WARNING: No GH_TOKEN found. gh commands will not be authenticated."
    echo "  Fix: echo 'ghp_yourtoken' > ~/.config/gh/claude_gh_token && chmod 600 ~/.config/gh/claude_gh_token"
    GH_TOKEN_VALUE=""
fi

DOCKER_COMMON+=(
    -e GH_TOKEN="${GH_TOKEN_VALUE}"
)

}

setup_ssh_config() {

if [ ! -f "$HOME/.ssh/claude_github" ]; then
    echo "ERROR: SSH key not found at ~/.ssh/claude_github"
    exit 1
fi

SSH_CONFIG_FILE="$HOME/.ssh/claude_ssh_config"
cat > "${SSH_CONFIG_FILE}" <<'EOF'
Host github.com
    HostName github.com
    User git
    IdentityFile ~/.ssh/claude_github
    IdentitiesOnly yes
    StrictHostKeyChecking no
EOF
chmod 600 "${SSH_CONFIG_FILE}"

DOCKER_COMMON+=(
    -v "${SSH_CONFIG_FILE}:/home/claude/.ssh/config:ro"
)

}

# Main execution
if [ "$(docker ps -q -f name=^${CONTAINER_NAME}$)" ]; then
    echo "Container '${CONTAINER_NAME}' is already running"
    echo "Logging you into bash..."
    docker exec -it -u claude ${CONTAINER_NAME} bash
else
    echo "Starting container '${CONTAINER_NAME}'"
    setup_gh_token
    setup_ssh_config

    # docker build --no-cache --pull -t claude-cli-env -f Dockerfile.claude .
    docker build -t claude-cli-env -f Dockerfile.claude .

    docker run --rm -it --name "${CONTAINER_NAME}" "${DOCKER_COMMON[@]}" claude-cli-env bash  -c "
            if [ ! -d /home/claude/dotfiles ]; then
                echo 'Cloning dotfiles...'
                git clone ${DOTFILES_REPO} /home/claude/dotfiles
            fi
            cd /home/claude/dotfiles &&
            git pull origin main &&
            ./install.sh && cd /app && exec bash
            "   

fi
