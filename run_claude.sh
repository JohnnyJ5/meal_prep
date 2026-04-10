#!/usr/bin/env bash

CONTAINER_NAME="claude-workspace"
DOTFILES_REPO="git@github.com:JohnnyJ5/dotfiles.git"
DOTFILES_DIR=".claude_workspace_env/dotfiles"
CLAUDE_CONFIG_DIR="$HOME/.config"

if [ -f "$CLAUDE_CONFIG_DIR/gh/claude_gh_token" ]; then
    GH_TOKEN_VALUE=$(cat "$CLAUDE_CONFIG_DIR/gh/claude_gh_token")
elif [ -n "$GH_TOKEN" ]; then
    GH_TOKEN_VALUE="$GH_TOKEN"
else
    echo "WARNING: No GH_TOKEN found. gh commands will not be authenticated."
    echo "  Fix: echo 'ghp_yourtoken' > ~/.config/gh/claude_gh_token && chmod 600 ~/.config/gh/claude_gh_token"
    GH_TOKEN_VALUE=""
fi

DOCKER_COMMON=(
    -e HOME=/app/.claude_workspace_env
    -e GIT_SSH_COMMAND="ssh -i /app/.claude_workspace_env/.ssh/claude_github -o StrictHostKeyChecking=no -o IdentitiesOnly=yes"
    -e GH_TOKEN="${GH_TOKEN_VALUE}"
    -e HOST_UID="$(id -u)"
    -e HOST_GID="$(id -g)"
    -v "$(pwd)":/app
    -v "$CLAUDE_CONFIG_DIR/ssh/claude_github:/app/.claude_workspace_env/.ssh/claude_github:ro"
)

dotfiles() {
    # Clone and install dotfiles on first run so ~/.claude agents/settings are available

    if [ ! -d "$DOTFILES_DIR" ]; then
        echo "Installing dotfiles..."
        docker run --rm "${DOCKER_COMMON[@]}" claude-cli-env bash -c "
            mkdir -p /app/.claude_workspace_env/.claude &&
            git clone ${DOTFILES_REPO} /app/.claude_workspace_env/dotfiles &&
            git config --global --add safe.directory /app/.claude_workspace_env/dotfiles &&
            cd /app/.claude_workspace_env/dotfiles &&
            ./install.sh"
    else
        echo "Dotfiles already installed, updating..."
        docker run --rm "${DOCKER_COMMON[@]}" claude-cli-env bash -c "
            git config --global --add safe.directory /app/.claude_workspace_env/dotfiles &&
            cd /app/.claude_workspace_env/dotfiles &&
            git pull origin main &&
            ./install.sh"
    fi
}


# if [ "$(docker ps -q -f name=^${CONTAINER_NAME}$)" ]; then
#     echo "Container '${CONTAINER_NAME}' is already running"
    
#     dotfiles

#     echo "Logging you into bash..."
#     docker exec -it ${CONTAINER_NAME} bash
# else
    echo "Starting a new '${CONTAINER_NAME}' container..."

    if [ ! -f "$CLAUDE_CONFIG_DIR/ssh/claude_github" ]; then
        echo "ERROR: SSH key not found at ~/.claude_config/ssh/claude_github"
        exit 1
    fi

    mkdir -p .claude_workspace_env/.ssh

    cat > .claude_workspace_env/.ssh/config <<'EOF'
Host github.com
    HostName github.com
    User git
    IdentityFile ~/.ssh/claude_github
    IdentitiesOnly yes
    StrictHostKeyChecking no
EOF
        chmod 600 .claude_workspace_env/.ssh/config

    docker build --no-cache --pull -t claude-cli-env -f Dockerfile.claude .

    # Clone and install dotfiles on first run so ~/.claude agents/settings are available
    dotfiles

    docker run --rm -it --name ${CONTAINER_NAME} "${DOCKER_COMMON[@]}" claude-cli-env bash
# fi
