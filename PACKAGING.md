# Packaging and Publishing meal_prep

This guide covers how to build and publish Debian packages for the meal_prep application.

## Initial Setup (First Time Only)

1. **Configure CMake with CPack support**
   - The `CMakeLists.txt` is already configured with CPack settings
   - Key variables:
     - `CPACK_PACKAGE_NAME`: meal-prep
     - `CPACK_PACKAGE_VERSION`: 1.0.0
     - `CPACK_PACKAGE_CONTACT`: michaelcoffey5@gmail.com
     - `CPACK_DEBIAN_PACKAGE_DEPENDS`: libssl3

2. **Build the project**
   ```bash
   mkdir -p build
   cd build
   cmake ..
   make
   ```

## Building and Releasing Packages

### For Initial Release (1.0.0)

```bash
cd build
cmake ..
make
cpack -G DEB
```

This generates: `meal-prep-1.0.0-Linux.deb`

### For Subsequent Releases

1. **Update the version in CMakeLists.txt**
   ```cmake
   project(MealPrep VERSION 1.0.1 DESCRIPTION "...")
   set(CPACK_PACKAGE_VERSION "1.0.1")
   ```

2. **Rebuild and generate package**
   ```bash
   cd build
   cmake ..
   make
   cpack -G DEB
   ```

3. This generates: `meal-prep-1.0.1-Linux.deb`

## Installing the Package

### Local Installation
```bash
sudo apt install ./build/meal-prep-1.0.0-Linux.deb
```

### System-wide Access
Once installed, run the application:
```bash
meal_prep
```

The executable is installed to: `/usr/local/bin/meal_prep`

## Uninstalling

```bash
sudo apt remove meal-prep
```

## Package Contents

- **Binary**: `/usr/local/bin/meal_prep`
- **Dependencies**: libssl3 (automatically installed if needed)

## Publishing to Repository (Advanced)

For distributing packages publicly:

1. **Create a PPA** (Personal Package Archive) on Launchpad
2. **Sign the package** with GPP key
3. **Upload** using `dput` tool

Example:
```bash
# Generate source package
cpack -G TGZ  # Create source tarball
# Configure debsign credentials
debsign -k YOUR_KEY_ID meal-prep_1.0.0-1_source.changes
# Upload to PPA
dput ppa:yourname/meal-prep meal-prep_1.0.0-1_source.changes
```

## Troubleshooting

**Error: "CPack project name not specified"**
- Run `cmake ..` to reconfigure before `cpack`

**Error: "Unmet dependencies"**
- Ensure required packages are listed in `CPACK_DEBIAN_PACKAGE_DEPENDS`
- Users must install dependencies before or alongside the package

**Package not found in PATH**
- Verify installation: `which meal_prep` or `ls /usr/local/bin/meal_prep`
- May need to reload shell: `hash -r` or open new terminal
