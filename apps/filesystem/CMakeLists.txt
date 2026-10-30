add_executable(orbit-filesystem
    src/main.cpp
    src/FileManagerWindow.cpp
    src/FileManagerWidget.cpp
    src/TextEditorWindow.cpp

    include/FileManagerWindow.h
    include/FileManagerWidget.h
    include/TextEditorWindow.h
)

target_include_directories(orbit-filesystem PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(orbit-filesystem PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
)

target_compile_options(orbit-filesystem PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)
