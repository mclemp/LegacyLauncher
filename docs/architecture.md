# Launcher Architecture

## Design goals

- Keep UI logic focused on orchestration and presentation.
- Isolate persistence, installation, and release networking concerns.
- Make feature behavior easy to trace for code review and maintenance.

## Component boundaries

### `MainWindow`

Owns Qt widgets and user interaction flow. It coordinates the services but does not implement their low-level behavior.

Responsibilities:
- profile switching and view updates
- startup prompt flow (browse/download)
- launch process lifecycle
- status/log presentation

### `ProfileStore`

Persistence layer over `QSettings`.

Responsibilities:
- active profile and theme preference
- profile data read/write (`installDir`, `executablePath`, `arguments`, `username`)
- runtime metrics (`playtimeSeconds`, `lastPlayed`)
- installed release metadata

### `InstallService`

Filesystem and install helper utilities.

Responsibilities:
- executable detection in directory trees
- save directory discovery
- recursive copy for backup/restore
- archive extraction (Windows via PowerShell `Expand-Archive`)
- install root and detection root strategy

### `ReleaseClient`

Network client for release metadata and asset download.

Responsibilities:
- fetch nightly release metadata from GitHub Releases API
- parse metadata into `ReleaseInfo`
- stream download assets with progress callbacks

## Startup flow

1. `MainWindow` loads profile and attempts auto-detection through `InstallService`.
2. If executable is unresolved, user gets an explicit prompt:
   - Browse existing location
   - Download and auto-extract
3. On download, `ReleaseClient` fetches nightly metadata and downloads the selected archive.
4. `InstallService` extracts archive and resolves `Minecraft.Client.exe`.
5. `ProfileStore` is updated with install and release metadata.

## Notes

- Release metadata source: `https://api.github.com/repos/MLE-MP/MinecraftConsoles/releases/tags/nightly`
- Runtime deployment for Qt DLLs is handled by post-build `windeployqt` in `CMakeLists.txt`.
