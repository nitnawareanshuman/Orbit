# Separate the filesystem app from the shell

The previous integration placed the browser/editor sources in `apps/shell` and modified shell command handling. The desired architecture is two independent CMake targets:

- `orbit-shell` — terminal only; no file-manager widget, editor, or special `mkdir`/`cat` GUI behavior.
- `orbit-filesystem` — standalone Qt6 desktop file manager; it has its own `main.cpp` and window.

## Revert only the old shell integration

If the previous patch is still uncommitted and no other local shell work should be discarded, inspect first:

```bash
git status --short
git diff -- apps/shell
```

Remove these old integration files after copying the standalone versions from this package into `apps/filesystem`:

```bash
rm -f apps/shell/include/FileManagerWidget.h \
      apps/shell/include/TextEditorWindow.h \
      apps/shell/src/FileManagerWidget.cpp \
      apps/shell/src/TextEditorWindow.cpp
```

Restore the shell files to the last shell-only commit **only if those files contain no other work you want to keep**:

```bash
git restore apps/shell/CMakeLists.txt \
            apps/shell/include/Shell.h \
            apps/shell/include/TerminalWindow.h \
            apps/shell/src/Shell.cpp \
            apps/shell/src/TerminalWindow.cpp
```

Then apply the single root CMake addition shown in README.md. If you have unrelated changes in any of those shell files, do not run `git restore`; manually remove only the file-manager integration hunks.
