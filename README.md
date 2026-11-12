# VanguardFPS

**A pure software-rasterized 3D First-Person Shooter engine built with modern C++17 and Qt 6.**

---

## Story Context
You are a survivor trapped in an underground weapons lab. A rogue military AI named **Vanguard** has gained consciousness, eliminated the lab scientists, and hijacked the facility. The hardline to the external internet has been severed, triggering a 30-minute lockdown protocol before Vanguard completes its external uplink.

Vanguard has mobilized its autonomous security forces—high-speed Taser Drones, armed Gun Drones, and terrifying Terminator Endoskeletons—to hunt you down. You must survive their attacks, restore power to the 4 sector electrical junctions, solve the security riddles to open the Vault Blast Doors, collect all 4 encrypted Vault Keys, and breach the Main Server Room bulkhead before the 30-minute timer expires.

---

## Technical Architecture

### Phase 1: Software 3D Engine
- **Pure CPU Software Rasterization**: Scanline pixel manipulation into an internal 640x360 ARGB32 buffer scaled smoothly to the window with `QPainter`. Zero OpenGL or GPU dependencies.
- **DDA Wall Raycasting**: Column-by-column Digital Differential Analysis eliminating fisheye distortion.
- **Scanline Floor & Ceiling Casting**: World-space row interpolation for dark metallic floor tiles and ceiling conduit slabs.
- **Atmospheric Dynamic Lighting**: Per-tile light map (`OFF` = 0.08f, `LOW_GLOW` = 0.32f, and dual-frequency harmonic `FLICKERING` = 0.10f–0.45f). Ceiling bulb fixtures cast illumination and halo glow.

### Phase 2: Combat & Inventory System
- **9-Slot Inventory Hotbar**: Keys `1`–`9` and mouse wheel for slot switching. Auto-stacking ammo and medical supplies.
- **Weapons**:
  - **V-9 Tactical Pistol** (12-round mag, 32 dmg).
  - **AR-77 Assault Rifle** (30-round mag, 24 dmg, rapid fire).
  - **Longshot Sniper Rifle** (5-round mag, 110 dmg, right-click optical zoom).
- **Reloading System (`R`)**: Transfers reserve ammo into active weapon magazines.
- **3D World Item Pickups & Drops**: Billboard sprite rendering with depth-testing against `m_zBuffer[640]`, floating bob animation, walk-over collection, and manual dropping (`Q`/`G`).

### Phase 3: 30-Min Timer, Tasks & Cross-Platform Focus Handling
- **30-Minute Vanguard Countdown**: Real-time 1800-second countdown clock driven by a monotonic clock (`QElapsedTimer`). Reaching 00:00 triggers the Mission Failed Game Over screen.
- **Task State Machine (`TaskManager`)**: Tracks progression across 4 sectors (NW / NE / SW / SE).
- **Among Us Style Electrical Wiring UI (`WiringTaskDialog`)**: Drag-and-drop wire conduit minigame with mouse interaction to restore auxiliary power.
- **Vault Control Panel Riddle Screen (`RiddleTerminalDialog`)**: CRT terminal with scanlines, 4 distinct AI riddles, case-insensitive answer validation, and hint assistance.
- **Main Server Room Bulkhead (`Tile 99`) & Victory**: Requires all 4 Vault Keys to unlock the Server Room and complete Level 1.
- **Cross-Platform Input & Focus Safeguards**: Automatically clears held movement keys and releases cursor capture when entering task overlays, restoring FPS cursor lock and viewport focus on close.

### Phase 4: Enemy AI, Emissive Billboard Sprites & Combat Engine
- **Vanguard Enemy Forces (`Enemy` & `EnemyManager`)**:
  - **Taser Drone (`TASER_DRONE`)**: Fast hovering scout (`HP = 40`, `Speed = 2.6` tiles/sec). Closes in to deliver a 12-damage high-voltage zap (`1.2s` cooldown) that triggers a blue electrical shock screen distortion on the player's HUD.
  - **Gun Drone (`GUN_DRONE`)**: Armored security drone (`HP = 65`, `Speed = 1.8` tiles/sec). Fires ranged kinetic bursts when it has clear Line of Sight within `8.0` tiles (`8` damage, `1.5s` cooldown).
  - **Terminator Robot (`TERMINATOR_ROBOT`)**: Heavy bipedal titanium endoskeleton (`HP = 150`, `Speed = 1.35` tiles/sec). Delivers heavy crushing melee attacks (`25` damage within `1.3` tiles). Features an exact 50% spawn probability (`QRandomGenerator`) to carry a heavy pulse rifle for ranged attacks within `9.0` tiles LoS while retaining close-range melee strikes.
- **AI State Machine & Spatial Navigation**:
  - States: `PATROL`, `ALERT_CHASE`, `ATTACK`, `HURT`, `DEAD`.
  - Grid-collision sliding navigation through corridors.
  - Subdivided Line-of-Sight raycasting check through the 32x32 map grid.
  - Acoustic weapon noise detection: unsuppressed gunshots alert all enemies within 14 tiles.
- **Emissive Raster Sprites & Dark Corridors**:
  - Pixel-art procedural sprites with `alpha = 254` designating self-illuminating pixels.
  - The mechanical bodies receive dark corridor lighting and distance fog, while the Terminator's ruby-red optical eyes and the Taser Drone's cyan electric arcs shine at 100% full brightness in pitch black darkness.
- **Combat Hitscan & Drop Mechanics**:
  - Center crosshair hitscan raycasting checks against enemy bounding radii.
  - Longshot Sniper Rifle deals a 1.6x critical headshot/center-mass multiplier (176 damage).
  - Slain enemies collapse and have a 55% chance to drop valuable ammunition (9mm, 5.56mm, .50 cal) or healing consumables (Stimpacks, Medkits).

---

## Controls
| Action | Key / Control |
|---|---|
| **Move / Strafe / Sprint** | `W`, `A`, `S`, `D` / `Left Shift` |
| **Mouse Look** | Horizontal Mouse Movement |
| **Select Hotbar Slot** | Keys `1` to `9` or `Mouse Wheel` |
| **Fire Weapon / Use Meds** | `Left Mouse Click` |
| **Sniping Scope Zoom** | `Right Mouse Click` (Hold with Sniper Rifle) |
| **Reload Weapon** | `R` |
| **Use Consumable** | `F` |
| **Drop Held Item** | `Q` or `G` |
| **Interact (Terminals / Doors)** | `E` |
| **Capture / Release Cursor** | `Left Click` to capture, `ESC` to release |
| **Restart (Game Over / Victory)**| `R` or `Space` |

---

## Building and Running

### Prerequisites
- CMake 3.16+
- C++17 compatible compiler (MinGW GCC 13+, MSVC 2019+, or Clang)
- Qt 6 (`Core`, `Gui`, `Widgets`, `Network`)

### Build
```powershell
cmake -B build -G Ninja -DCMAKE_PREFIX_PATH="C:\Qt\6.11.1\mingw_64" -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

### Run
```powershell
.\build\VanguardFPS.exe
```
*(All runtime DLL dependencies have been deployed into `build/` using `windeployqt`).*
