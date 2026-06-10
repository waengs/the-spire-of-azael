# The Spire of Azael

A dark fantasy **text adventure** written in C++. Climb the Spire, recruit companions, fight demons, and uncover the truth behind Demon Lord Azael.

Two builds are included:

| Build | Executable | Description |
|-------|------------|-------------|
| **Graphical** | `SpireOfAzael.exe` | Pixel-art UI (SDL2 + OpenGL + Dear ImGui) — **recommended** |
| **Console** | `SpireOfAzaelConsole.exe` | Classic terminal text adventure |

---

## Quick start (Windows)

**Requirements:** [CMake](https://cmake.org/download/) 3.16+ and a C++20 compiler (Visual Studio 2022 with “Desktop development with C++” works well).

```powershell
.\compile.bat
.\build\Release\SpireOfAzael.exe
```

The first build downloads SDL2, Dear ImGui, and SQLite into `build/_deps/` automatically.

---

## How to play

1. Launch **SpireOfAzael.exe**
2. Click **New Game** or load a save slot
3. Read the story on the left; type commands in **“What will you do?”** at the bottom
4. Press **Enter** or click **Send**

### Common commands

| Command | Action |
|---------|--------|
| `look` / `l` | Examine the room |
| `north` / `n`, `south` / `s`, `east` / `e`, `west` / `w` | Move |
| `take <item>` | Pick up an item |
| `inventory` / `inv` / `i` | View your pack |
| `talk` / `talk <name>` | Speak to an NPC |
| `attack` | Fight an enemy in the room |
| `help` / `h` | List commands |
| `quit` / `exit` / `x` | Leave the game |

Type `help` in-game for the full command list, including story-specific actions.

### Graphical UI

- **Left panel** — story log with colored text (exits, commands, combat)
- **Right sidebar** — hero stats, party, inventory, map compass, last NPC spoken to, save/load
- **Map** — white lines show available exits; colored N/E/S/W labels; red dot = you
- **Saves** — 3 slots via the sidebar or main menu; stored in `spire_save.db` next to the exe

---

## Display settings

Open **Display** on the main menu or in the right sidebar while playing.

| Setting | How |
|---------|-----|
| **Text size** | Slider (0.75×–2.5×) or **Ctrl + mouse wheel** |
| **Line-by-line text reveal** | Checkbox — typewriter effect on/off |

Settings are saved in `spire_ui.ini` beside the executable.

---

## Project layout

```
TheSpireofAzael/
  main.cpp              Graphical app entry
  main_console.cpp      Console app entry
  Game.cpp / Game.h     Core adventure logic
  GameApplication.*     SDL / ImGui UI loop
  Room.cpp, Player.cpp  World and character systems
  SaveDatabase.*        SQLite save slots
  CMakeLists.txt        Build configuration
  compile.bat           One-click Windows build
  assets/               Fonts, logo, portraits, UI art
  windows/app.rc        Windows exe icon resource
  scripts/png_to_ico.ps1  Converts logo.png → app.ico
```

See [`assets/README.md`](assets/README.md) for image filenames and sizes.

**Safe to delete anytime** (recreated on build): `build/`, `out/`, `.vs/`, `imgui.ini`

---

## Sharing with friends

Zip the **Release folder contents** — do not send only the `.exe`:

```
SpireOfAzael/
  SpireOfAzael.exe
  assets/
    logo.png
    fonts/
      PressStart2P-Regular.ttf    ← required for pixel font
    ui/                           ← optional backgrounds
    portraits/                    ← optional character art
    icons/                        ← optional HUD icons
```

Your friend unzips and runs `SpireOfAzael.exe` from inside that folder.

- **Required for best look:** `assets/fonts/PressStart2P-Regular.ttf`
- **Optional:** PNG images — missing files use colored placeholders; the game still runs
- **Not needed:** source code, `build/`, or CMake

---

## Manual build targets

```powershell
cmake -B build
cmake --build build --config Release --target SpireOfAzael          # graphical
cmake --build build --config Release --target SpireOfAzaelConsole   # terminal only
```

---

## Tech stack

- **C++20** — game logic
- **SDL2** — window, input, OpenGL context
- **Dear ImGui** — UI rendering
- **SQLite** — save slots
- **stb_image** — PNG loading
- **Press Start 2P** — pixel font ([SIL Open Font License](https://openfontlicense.org/))

---

## License

Game code is provided as-is for personal use and sharing. Third-party libraries (SDL2, ImGui, SQLite, stb, Press Start 2P) have their own licenses.
