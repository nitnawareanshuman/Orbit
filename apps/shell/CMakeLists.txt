add_executable(orbit-shell
    src/main.cpp
    src/Shell.cpp
    src/TerminalWindow.cpp
)

target_link_libraries(orbit-shell PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
)