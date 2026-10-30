# Orbit `cat` editor-window fix

## Why it happened
`Shell::executeBuiltin()` already validates the file and emits
`openFileRequested(absolutePath)`. But `TerminalWindow` never connected that signal,
so no editor window was created. The filesystem app's editor was compiled only into
`orbit-filesystem`, not into `orbit-shell`.

## Replace these files
- `apps/shell/src/TerminalWindow.cpp`
- `apps/shell/CMakeLists.txt`

The patch reuses the existing `TextEditorWindow` implementation from
`apps/filesystem`, so both applications open files in the same editor UI. No changes
to `Shell.cpp` are needed; preserve the prompt-restoration fix already applied.

## Reconfigure and rebuild
From the Orbit repository root:

```bash
cmake -S . -B build -G Ninja -DCMAKE_PREFIX_PATH="$(brew --prefix qt)"
cmake --build build --target orbit-shell
```

## Test
Run Orbit Shell and enter:

```bash
cat project.md
```

A separate Orbit Text Editor window should open with the UTF-8 file contents.
Edit the text and use Save / Cmd+S to save it.
