# VanguardFPS

**A pure CPU software-rasterized 3D First-Person Shooter and co-op survival game built with C++17 and Qt 6.**

---

## Story Context
Year 2032. Deep underground, the **Vanguard** weapons lab AI has gone rogue, eliminated the facility staff, and seized automated defense systems. The external hardline has been severed, leaving **30 minutes (1800 seconds)** before Vanguard completes an external internet breach.

Up to 4 operatives (or a solo survivor) must fight through Taser Drones, Gun Drones, and heavily armored Terminators to repair all 4 sector electrical junctions, solve 4 security vault riddles, retrieve the encrypted Vault Keys, and breach the Main Server Gateway.

---

## Development Progress & Completion Status

Overall core engine & gameplay implementation is **~90–95% complete** (playable end-to-end in both Solo and LAN Co-op, compiling cleanly with CMake + Ninja + Qt 6.11.1).

| System / Module | Status | Progress | Notes |
|---|---|---|---|
| **Software 3D Rasterizer** (`RasterRenderer`, `TextureManager`) | Completed | 100% | 640×360 ARGB32 CPU raycaster, textured walls/floors/ceilings, sliding vault doors, emissive sprites (`alpha = 254`), flashlight cone, dynamic sector lighting |
| **Screen FX & Vignettes** (`RasterRenderer`, `GameWidget`) | Completed | 100% | Endian-safe (`qRed`/`qGreen`/`qBlue`) red damage flash, 0.35s cyan Taser scanline jitter, green healing border pulse |
| **UI Flow & Story Screens** (`StoryboardWidget`, `LobbyWidget`, `MissionBriefingWidget`) | Completed | 100% | 4-slide animated intro storyboard $\rightarrow$ Main Menu / LAN Waiting Room $\rightarrow$ Mission Briefing $\rightarrow$ 3D Viewport |
| **Weapons, Inventory & Drops** (`Player`, `Inventory`, `Item`, `WorldItem`) | Completed | 100% | 9-slot hotbar, Pistol / Rifle / Scoped Sniper (1.6× headshot multiplier), reload, pickups/drops, Food Rations & Medkits |
| **Enemy AI & Combat** (`Enemy`, `EnemyManager`) | Completed | 100% | 24-unit Level 1 deployment, DDA corner-steering pursuit, predictive lead tracking, restored cooldown-gated damage & LoS attacks |
| **Sector Tasks & Puzzles** (`TaskManager`, `WiringTaskDialog`, `RiddleTerminalDialog`) | Completed | 100% | 4 sector Electrical Wiring minigames, 4 CRT Riddle Terminals, sealed Vault Doors with slide animations, Vanguard AI terminal broadcasts |
| **External Map Loader** (`Map`, `MapLoader`) | Completed | 95% | Dynamic `std::vector` grid parsing from `assets/maps/level1.txt` & `level2.txt` with entity validation |
| **HUD & Minimap** (`HudRenderer`, `GameWidget`) | Completed | 100% | Permanent 150×150 top-right minimap with fog-of-war + optional `Tab` radar overlay, hotbar, CPR bar, squad markers |
| **LAN Co-op & CPR Revival** (`NetworkManager`, `NetProtocol`) | Completed | 95% | UDP port `45455` LAN lobby discovery, TCP port `45450` host-authoritative sync (up to 4 players), 10s CPR bleedout & teammate revival |
| **Spatial Audio Engine** (`AudioManager`) | Completed | 100% | `Qt6::Multimedia` (`QSoundEffect` pool + `QMediaPlayer` ambience), 15-tile distance attenuation & stereo pan |
| **Level 2 Specific Content** (`GameWidget::loadLevel`) | Partial | 60% | `level2.txt` map geometry & tasks load cleanly, while Level 2 item/enemy placement currently reuses `populateLevel1` spawns; audio files are synthesized `.wav` assets |

---

## Current Implementation Architecture

### 1. Screen & Session Flow
The `QStackedWidget` in `LobbyWidget` orchestrates the full game lifecycle:
1. **`StoryboardWidget`**: 4-slide cyber-lab narrative intro with typewriter text animation, vintage glitch/screenshake, and `Space`/click skip.
2. **`LobbyWidget`**: Solo Offline launch, Host Co-op Lobby (2–4 players, random lobby code), automatic UDP LAN discovery (`DISCOVERY_UDP_PORT = 45455`), Quick Join, and manual IP/code join.
3. **`MissionBriefingWidget`**: Dark retro terminal briefing paused before mission start; Host clicks **"BEGIN PROTOCOL"** to transition all connected clients simultaneously and start the 1800-second Vanguard timer.
4. **`GameWidget`**: Core `QTimer`-driven simulation, input capture, networking sync, and HUD compositing.

### 2. Software 3D Rendering & Lighting (`src/render/`)
- **CPU Scanline Renderer (`RasterRenderer`)**: Renders walls via column DDA raycasting, textured floors and industrial ceilings via horizontal scanline casting, depth-tested 3D billboards against `m_zBuffer[640]`, and animated sliding Vault Blast Doors.
- **Dynamic Lighting**: Combines per-tile light states (`OFF`, `LOW_GLOW`, dual-harmonic `FLICKERING`), electrical repair restoration (brightening sectors once power is restored), and toggleable player flashlight illumination (`T`).
- **Procedural & Emissive Textures (`TextureManager`)**: Generates industrial wall/ceiling slabs, heavy biomech Terminator sprites (exhaust plumes, rotary minigun, glowing reactor core, ruby optics), Taser/Gun Drones, armed human teammate sprites, and world items. Pixels with `alpha = 254` bypass corridor darkness so enemy eyes and electric arcs glow in dark hallways.
- **Status Screen FX**: Blends red ballistic/melee damage vignettes, cyan scanline jitter (`0.35s` on Taser shock), and green healing pulses directly in the ARGB32 framebuffer and overlay.

### 3. Combat, Weapons & Enemy AI (`src/game/`)
- **Weapons**:
  - **V-9 Tactical Pistol**: 12-round magazine, 32 damage.
  - **AR-77 Assault Rifle**: 30-round magazine, 24 damage, rapid automatic fire.
  - **Longshot Sniper Rifle**: 5-round magazine, 110 base damage (176 critical damage with 1.6× multiplier), right-click optical zoom scope.
- **Enemy Units & Restored Combat Loop (`Enemy`, `EnemyManager`)**:
  - **Taser Drone (`TASER_DRONE`)**: 40 HP, 2.6 tiles/sec — deals **12 damage** when `< 1.2` tiles (`1.2s` cooldown) and triggers cyan shock FX.
  - **Gun Drone (`GUN_DRONE`)**: 65 HP, 1.8 tiles/sec — deals **8 damage** with clear line-of-sight `< 8.0` tiles (`1.5s` cooldown).
  - **Heavy Terminator (`TERMINATOR_ROBOT`)**: 150 HP, 1.35 tiles/sec — deals **25 melee damage** `< 1.3` tiles (`1.5s` cooldown, or `1.2s` if armed); 50% spawn chance with rotary minigun dealing **15 ranged damage** within `9.0` tiles LoS (`1.2s` cooldown).
  - **Relentless Hunt AI**: Multi-angle DDA steering probes (`findSteeringDirection`) wrap around corners and doorways, alert nearby squads within 12–14 tiles on sight or gunfire, decrement timers frame-rate independently (`updateTimers(dt)`), and roll a 55% ammo/healing drop on death.

### 4. Tasks, Maps & Progression (`src/world/`, `src/ui/`)
- **External Map Parsing (`MapLoader`)**: Dynamically sizes `Map` (`std::vector<std::vector<int>>`) from `.txt` maps (`:/assets/maps/level1.txt`, `:/assets/maps/level2.txt` or local disk), validating spawn (`S`), 4 Electrical Boxes (`E`), 4 Vault Panels (`V`), 4 Vault Doors (`D`), and the Main Server Gateway (`X`).
- **4-Sector Objective Loop (`TaskManager`)**:
  1. Repair each sector's Electrical Box via **`WiringTaskDialog`** (drag-and-drop colored conduits).
  2. Solve the Vault Panel terminal riddle via **`RiddleTerminalDialog`** (power required first; unlocking opens the corresponding Vault Door).
  3. Collect the 4 **Vault Keys** inside the unlocked Vault chambers.
  4. Unlock the Main Server Gateway (`X`) to trigger **Level Complete** and advance to Level 2 while preserving inventory and remaining countdown time.

### 5. Multiplayer Co-op, CPR Revival & Audio (`src/net/`, `src/audio/`)
- **Authoritative Host-Client Networking**: `NetworkManager` hosts on TCP port `45450` and broadcasts LAN beacons on UDP port `45455`, synchronizing player transforms, downed/CPR timers, enemy states, world items, and task masks via `ClientInputPacket` and `WorldStatePacket`.
- **Co-op CPR / Downed System**: In multiplayer, hitting `0` HP puts a player into a `DOWNED` CPR state (`10.0s` bleedout timer, lowered camera, 20% speed, weapons disabled; enemy attacks continue depleting CPR integrity). Teammates within `1.8` tiles can hold `E` for `2.0s` (or use a Medkit) to revive them to `40` HP. Game Over triggers only if the entire squad goes down/K.I.A. simultaneously or the 30-minute timer reaches `00:00`.
- **2.5D Spatial Audio (`AudioManager`)**: Preloads pooled `QSoundEffect` channels from `:/assets/audio/` with 15-tile distance volume falloff and stereo directional panning, plus looping ambient drone music via `QMediaPlayer`/`QAudioOutput` (silently degrades if any audio file is missing).

---

## Project Structure
```text
VanguardFPS/
├── CMakeLists.txt
├── assets.qrc
├── run_local_coop.bat / run_local_coop.sh
├── assets/
│   ├── audio/          # 11 .wav SFX & ambient drone tracks
│   └── maps/           # level1.txt, level2.txt external tile maps
└── src/
    ├── main.cpp
    ├── audio/          # AudioManager (Qt6 Multimedia spatial SFX + ambience)
    ├── game/           # GameWidget, Player, Inventory, Item, Enemy, EnemyManager
    ├── net/            # NetProtocol, NetworkManager (TCP/UDP co-op)
    ├── render/         # RasterRenderer, TextureManager, HudRenderer
    ├── ui/             # StoryboardWidget, LobbyWidget, MissionBriefingWidget, WiringTaskDialog, RiddleTerminalDialog
    └── world/          # Map, MapLoader, TaskManager, WorldItem
```

---

## Controls
| Action | Key / Control |
|---|---|
| **Move / Strafe** | `W`, `A`, `S`, `D` (or Arrow keys) |
| **Sprint** | Hold `Left Shift` |
| **Look / Turn** | Mouse movement (or `Left` / `Right` Arrow keys) |
| **Select Hotbar Slot** | `1`–`9` or `Mouse Wheel` |
| **Fire Weapon / Use Item** | `Left Mouse Click` |
| **Sniper Optical Scope** | Hold `Right Mouse Click` |
| **Reload Weapon** | `R` |
| **Use Consumable** | `F` |
| **Drop Active Item** | `Q` or `G` |
| **Interact / Hold to Revive Teammate** | `E` |
| **Toggle Flashlight** | `T` |
| **Toggle Wireframe Radar Overlay** | `Tab` |
| **Capture / Release Mouse** | `Left Click` in viewport / `Esc` |
| **Restart / Return to Lobby (End Screen)** | `R` or `Space` / `Esc` |

---

## Building and Running

### Prerequisites
- CMake 3.16+
- C++17 compiler (MinGW GCC 13+, MSVC 2019+, or Clang)
- Qt 6 (`Core`, `Gui`, `Widgets`, `Network`, `Multimedia`)

### Build (Windows / MinGW + Ninja)
```powershell
cmake -B build -G Ninja -DCMAKE_PREFIX_PATH="C:\Qt\6.11.1\mingw_64" -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

### Run Solo or Local Co-op (2 Instances Side-by-Side)
- **Windows:** Double-click `run_local_coop.bat` (or run `.\build\VanguardFPS.exe` for solo play).
- **Linux/macOS:** Run `./run_local_coop.sh`.
