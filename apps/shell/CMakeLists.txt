add_executable(orbit-shell
    src/main.cpp
    src/CommandParser.cpp
    src/ProcessManager.cpp
    src/Shell.cpp
    src/TerminalWindow.cpp

    # Reuse the same editor UI in both Orbit Files and Orbit Shell's `cat`.
    ../filesystem/src/TextEditorWindow.cpp

    include/Command.h
    include/CommandParser.h
    include/ProcessManager.h
    include/Shell.h
    include/TerminalWindow.h
    ../filesystem/include/TextEditorWindow.h
)

target_include_directories(orbit-shell PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../filesystem/include
)

target_link_libraries(orbit-shell PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
)

target_compile_options(orbit-shell PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)