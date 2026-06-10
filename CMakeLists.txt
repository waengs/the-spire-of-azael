cmake_minimum_required(VERSION 3.16)

if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()

project(SpireOfAzael VERSION 1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# --- Dear ImGui ---
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.91.6
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(imgui)

set(IMGUI_DIR ${imgui_SOURCE_DIR})
set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_sdl2.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

# --- SDL2 ---
set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)
set(SDL_TEST OFF CACHE BOOL "" FORCE)
set(SDL2_DISABLE_INSTALL ON CACHE BOOL "" FORCE)
FetchContent_Declare(
    sdl2
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG release-2.30.9
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(sdl2)

# --- SQLite ---
option(USE_SYSTEM_SQLITE "Use system SQLite3" OFF)

set(CORE_GAME_SOURCES
    Character.cpp
    Game.cpp
    GameIo.cpp
    GameSaveData.cpp
    Item.cpp
    Player.cpp
    Room.cpp
)

# Text-mode build for quick playtesting (stdin/stdout, no SDL/ImGui).
add_executable(SpireOfAzaelConsole
    main_console.cpp
    ${CORE_GAME_SOURCES}
)

target_include_directories(SpireOfAzaelConsole PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

if(MSVC)
    target_compile_options(SpireOfAzaelConsole PRIVATE /W4)
else()
    target_compile_options(SpireOfAzaelConsole PRIVATE -Wall -Wextra)
endif()

add_executable(SpireOfAzael
    main.cpp
    GameApplication.cpp
    UiAssets.cpp
    UiFonts.cpp
    AnsiText.cpp
    SaveDatabase.cpp
    ${CORE_GAME_SOURCES}
    ${IMGUI_SOURCES}
)

if(WIN32)
    target_sources(SpireOfAzael PRIVATE windows/app.rc)

    if(EXISTS "${CMAKE_SOURCE_DIR}/assets/logo.png")
        add_custom_command(
            TARGET SpireOfAzael PRE_BUILD
            COMMAND powershell -NoProfile -ExecutionPolicy Bypass
                -File "${CMAKE_SOURCE_DIR}/scripts/png_to_ico.ps1"
                -PngPath "${CMAKE_SOURCE_DIR}/assets/logo.png"
                -IcoPath "${CMAKE_SOURCE_DIR}/assets/app.ico"
            COMMENT "Regenerate app.ico from assets/logo.png"
            VERBATIM
        )
    endif()
endif()

target_include_directories(SpireOfAzael PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
)

add_custom_command(TARGET SpireOfAzael POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/assets"
        "$<TARGET_FILE_DIR:SpireOfAzael>/assets"
    COMMENT "Copy assets folder beside SpireOfAzael executable"
)

target_link_libraries(SpireOfAzael PRIVATE
    SDL2::SDL2-static
    SDL2::SDL2main
)

if(WIN32)
    target_link_libraries(SpireOfAzael PRIVATE opengl32 imm32 version setupapi winmm)
else()
    find_package(OpenGL REQUIRED)
    target_link_libraries(SpireOfAzael PRIVATE OpenGL::GL ${CMAKE_DL_LIBS})
endif()

if(USE_SYSTEM_SQLITE)
    find_package(SQLite3 REQUIRED)
    target_link_libraries(SpireOfAzael PRIVATE SQLite::SQLite3)
else()
    FetchContent_Declare(
        sqlite_amalgamation
        URL https://www.sqlite.org/2024/sqlite-amalgamation-3460100.zip
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    FetchContent_MakeAvailable(sqlite_amalgamation)
    add_library(sqlite3_amalgamation STATIC ${sqlite_amalgamation_SOURCE_DIR}/sqlite3.c)
    target_include_directories(sqlite3_amalgamation PUBLIC ${sqlite_amalgamation_SOURCE_DIR})
    if(MSVC)
        target_compile_options(sqlite3_amalgamation PRIVATE /W0)
    endif()
    target_link_libraries(SpireOfAzael PRIVATE sqlite3_amalgamation)
endif()

if(MSVC)
    target_compile_options(SpireOfAzael PRIVATE /W4)
else()
    target_compile_options(SpireOfAzael PRIVATE -Wall -Wextra)
endif()
