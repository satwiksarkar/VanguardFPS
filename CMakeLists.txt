cmake_minimum_required(VERSION 3.16)
project(VanguardFPS VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Network Multimedia)

# Compiler optimization flags for high-performance software rasterization
if(MSVC)
    add_compile_options(/O2 /Oi /Ot /W4 /permissive-)
else()
    add_compile_options(-O3 -Wall -Wextra)
endif()

set(SOURCES
    src/main.cpp
    src/world/Map.h
    src/world/Map.cpp
    src/world/WorldItem.h
    src/world/WorldItem.cpp
    src/world/TaskManager.h
    src/world/TaskManager.cpp
    src/world/MapLoader.h
    src/world/MapLoader.cpp
    src/render/TextureManager.h
    src/render/TextureManager.cpp
    src/render/RasterRenderer.h
    src/render/RasterRenderer.cpp
    src/render/HudRenderer.h
    src/render/HudRenderer.cpp
    src/game/Item.h
    src/game/Item.cpp
    src/game/Inventory.h
    src/game/Inventory.cpp
    src/game/Player.h
    src/game/Player.cpp
    src/game/Enemy.h
    src/game/Enemy.cpp
    src/game/EnemyManager.h
    src/game/EnemyManager.cpp
    src/game/GameWidget.h
    src/game/GameWidget.cpp
    src/ui/WiringTaskDialog.h
    src/ui/WiringTaskDialog.cpp
    src/ui/RiddleTerminalDialog.h
    src/ui/RiddleTerminalDialog.cpp
    src/ui/LobbyWidget.h
    src/ui/LobbyWidget.cpp
    src/ui/MissionBriefingWidget.h
    src/ui/MissionBriefingWidget.cpp
    src/ui/StoryboardWidget.h
    src/ui/StoryboardWidget.cpp
    src/net/NetProtocol.h
    src/net/NetProtocol.cpp
    src/net/NetworkManager.h
    src/net/NetworkManager.cpp
    src/audio/AudioManager.h
    src/audio/AudioManager.cpp
    assets.qrc
)

add_executable(VanguardFPS WIN32 ${SOURCES})

target_include_directories(VanguardFPS PRIVATE src)

target_link_libraries(VanguardFPS PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
    Qt6::Multimedia
)
