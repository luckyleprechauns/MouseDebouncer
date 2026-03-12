cmake_minimum_required(VERSION 3.25)
project(MouseDebouncer VERSION 2.0 LANGUAGES CXX RC)

set(CMAKE_CXX_STANDARD 17)
add_compile_definitions(UNICODE _UNICODE)

# Output the executable to bin/ instead of deep in the build tree
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/bin")

add_executable(MouseDebouncer WIN32
    src/main.cpp
    resources/MouseDebouncer.rc
)

# Include paths: resources/ for resource.h (used by both main.cpp and .rc)
target_include_directories(MouseDebouncer PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/resources"
)

target_link_libraries(MouseDebouncer PRIVATE shlwapi.lib Comctl32.lib)

# --- Install targets ---
# Default install prefix on Windows: C:/Program Files/MouseDebouncer
# Override with: cmake --install build --prefix "C:/your/path"
install(TARGETS MouseDebouncer
    RUNTIME DESTINATION .
)

# Install a helper script to add/remove startup entry and Start Menu shortcut
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/scripts/install-shortcuts.bat"
    DESTINATION .
    OPTIONAL
)
