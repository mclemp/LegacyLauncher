# Minecraft Legacy Launcher

A Qt-based desktop launcher for Minecraft Legacy Console Edition community builds.

## Highlights

- Profile-based launch configuration
- Automatic install detection
- Startup flow for missing game files (Browse or Download)
- GitHub nightly update check (`MLE-MP/MinecraftConsoles`)
- Repository archive download and extraction
- Save backup / restore utilities
- Session playtime tracking and activity logs
- Automatic Qt runtime deployment during Windows builds

## Project Structure

- `src/main.cpp` - application entry point
- `src/MainWindow.h` - main window orchestration layer
- `src/MainWindow.cpp` - UI flow and service coordination
- `src/services/ProfileStore.*` - profile persistence over `QSettings`
- `src/services/InstallService.*` - install detection, extraction, backup/restore helpers
- `src/services/ReleaseClient.*` - GitHub release metadata and download client
- `resources/resources.qrc` - embedded icon assets
- `CMakeLists.txt` - build configuration and post-build runtime deployment
- `build_and_launch.bat` - one-command local build + launch helper
- `docs/architecture.md` - component boundaries and startup/install flow

## Build (Windows, MSVC + Qt)

Expected Qt kit format: `C:\Qt\<version>\msvc2022_64`

```bat
build_and_launch.bat "C:\Qt\6.10.2\msvc2022_64"
```

If no argument is provided, the script defaults to `C:\Qt\6.10.2\msvc2022_64`.

## Notes

- The launcher uses the GitHub Releases API and expects a nightly release for `MLE-MP/MinecraftConsoles`.
- On first launch (or when files are missing), the launcher prompts to browse for existing files or download the repository archive automatically.
