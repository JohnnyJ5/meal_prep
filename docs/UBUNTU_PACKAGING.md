# Ubuntu Packaging & Installation Guide

This guide covers building and installing the `meal-prep` server locally on Ubuntu as a self-contained `.deb` package managed by `systemd`, with zero manual file setup.

---

## 1. Build the `.deb` Package

From the project root, run:

```bash
make package
```

Or build **and** install in a single step:

```bash
make install-package
```

### What Is Bundled Inside `build_pkg/meal-prep_1.0.0_amd64.deb`

| Installed Path | Source | Description |
| :--- | :--- | :--- |
| `/opt/meal_prep/bin/meal_prep` | Built from `src/` | Compiled C++ server binary (`Release` build) |
| `/opt/meal_prep/static/` | `static/` | Web UI assets (HTML, CSS, JS) |
| `/opt/meal_prep/meal_prep.conf.json` | `packaging/meal_prep.conf.json` | Pre-configured runtime config pointing to `/opt/meal_prep/calendar_credentials.json` on port `8080` (preserved across upgrades) |
| `/opt/meal_prep/meals.db.seed` | `meals.db` | Initial recipe/meal database (copied to `/opt/meal_prep/meals.db` on first install; never overwrites an existing DB on upgrade) |
| `/opt/meal_prep/calendar_credentials.json` | `~/.meal_prep/dev_calendar_credentials.json` | Bundled at build time (or copied automatically by `postinst` during `sudo apt install`) with `0600` permissions |
| `/etc/default/meal-prep` | `packaging/meal-prep.default` | Pre-configured `GOOGLE_REDIRECT_URI` + auto-generated 256-bit `MEAL_PREP_TOKEN_KEY` (`0600`, preserved across upgrades) |
| `/lib/systemd/system/meal-prep.service` | `packaging/meal-prep.service` | `systemd` service unit running under the `mealprep` system user |

---

## 2. Install the Package

If you ran `make package`, install the generated `.deb` with `apt`:

```bash
sudo apt install ./build_pkg/meal-prep_1.0.0_amd64.deb
```

### What `apt install` Does Automatically (`postinst`)

1. Installs required Ubuntu runtime libraries (`libssl3`, `libcurl4`, `libsqlite3-0`, `ca-certificates`, `openssl`).
2. Creates a dedicated unprivileged system user and group `mealprep` (home `/opt/meal_prep`).
3. Initializes `/opt/meal_prep/meals.db` from `meals.db.seed` if no database exists yet.
4. Ensures `/opt/meal_prep/calendar_credentials.json` is installed (either from the `.deb` payload or copied from `~/.meal_prep/dev_calendar_credentials.json`) and locked down to `0600`.
5. Generates a 256-bit `MEAL_PREP_TOKEN_KEY` in `/etc/default/meal-prep` (`0600`) for AES-256-GCM OAuth token encryption if one is not already set.
6. Sets ownership of `/opt/meal_prep` to `mealprep:mealprep`.
7. Reloads `systemd`, enables `meal-prep.service` on boot, and starts the server immediately on [http://localhost:8080](http://localhost:8080).

---

## 3. Managing the Service

```bash
# Check service status
sudo systemctl status meal-prep

# Follow live server logs
sudo journalctl -u meal-prep -f

# Restart / stop / start the service
sudo systemctl restart meal-prep
sudo systemctl stop meal-prep
sudo systemctl start meal-prep
```

---

## 4. Uninstalling

```bash
# Remove package (preserves /opt/meal_prep/meals.db and config files)
sudo apt remove meal-prep

# Purge package and configuration files
sudo apt purge meal-prep
```
