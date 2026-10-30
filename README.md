# Orbit Files — standalone Qt6 file manager

This is a separate desktop application target. It does not link to, instantiate, or modify Orbit Shell.

## Features
- Browse the local filesystem using Qt 6 `QFileSystemModel` and `QTreeView`.
- Navigate to a directory by entering its path.
- Double-click folders to navigate and files to open them in a separate text editor window.
- Uses Qt's platform file icon provider for folder and file-type icons, including extension-associated icons on macOS.
- Edit UTF-8 text files and save atomically with Save / Cmd+S.

## Integrate into the Orbit repository

Add this line to the root `CMakeLists.txt`, after `add_subdirectory(apps/shell)`:

```cmake
add_subdirectory(apps/filesystem)
```

Keep `apps/shell/CMakeLists.txt` unchanged from the shell-only version. Do not add any filesystem sources to the `orbit-shell` target.

## Build and run on macOS

```bash
cmake -S . -B build -G Ninja -DCMAKE_PREFIX_PATH="$(brew --prefix qt)"
cmake --build build --target orbit-filesystem
./build/apps/filesystem/orbit-filesystem
```

Build the shell independently with:

```bash
cmake --build build --target orbit-shell
```
