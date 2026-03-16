---
name: Documentation Management
description: Instructions for maintaining and updating project documentation, including README, architecture diagrams, and command guides.
---

# Documentation Management Skill

This skill provides guidelines and procedures for keeping the project documentation up-to-date and consistent with the codebase and deployment workflows.

## core Principles

1.  **Architecture First**: Always update the architecture section in `README.md` when structural changes are made to the backend or frontend.
2.  **Command Accuracy**: Ensure that any commands listed in `docs/` (GCP, Docker, API) are tested and match the actual `Makefile` or deployment scripts.
3.  **Cross-Linking**: Maintain functional relative links between the `README.md` and the documents in `docs/`.
4.  **No Redundancy**: If a command is better documented in a targeted file (e.g., `GCP_COMMANDS.md`), refer to it from the `README.md` rather than duplicating the content.

## Documentation Structure

- `README.md`: Entry point, features, quickstart, architecture, and workflow.
- `docs/API.md`: Comprehensive REST API reference with examples.
- `docs/GCP_COMMANDS.md`: Guide for Google Cloud deployment, monitoring, and storage management.
- `docs/DOCKER_COMMANDS.md`: Essential Docker and Docker Compose commands for local development.

## Procedures

### Updating Architecture Diagrams
When the project structure changes, update the Mermaid diagram in `README.md`.
1.  Identify new components or altered interactions.
2.  Modify the Mermaid code block in `README.md`.
3.  Verify the rendering using a Markdown viewer.

### documenting New API Endpoints
1.  Add the endpoint signature to `docs/API.md`.
2.  Include a brief description, method, and URL.
3.  Provide a JSON request/response example.
4.  Add a `curl` example for easy testing.

### Syncing Deployment Commands
When `cloudbuild.yaml` or Docker configurations change:
1.  Update `docs/GCP_COMMANDS.md` or `docs/DOCKER_COMMANDS.md`.
2.  Ensure standard variables (like `[YOUR_PROJECT_ID]`) are clearly marked for the user.
