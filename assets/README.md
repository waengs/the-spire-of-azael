# The Spire of Azael — UI image assets

Place your PNG images here (recommended size notes below). The game looks in the `assets/` folder next to the executable and in the project root.

## Folder layout

```
assets/
  logo.png      Game logo (Windows .exe icon + window title-bar icon)
  app.ico       Auto-generated from logo.png at build time (Windows)
  fonts/        Pixel UI font (bundled)
  ui/           Backgrounds and large panels
  portraits/    Character speaking / hero portraits
  icons/        Small HUD icons
```

### Logo (`assets/logo.png`)

| File | Purpose |
|------|---------|
| `logo.png` | Square PNG (256×256 or larger recommended). Used for the Windows executable icon and the in-game window icon. |

On Windows, `app.ico` is regenerated from `logo.png` when you build. Replace `logo.png` and rebuild to update the icon.

## Pixel font (`assets/fonts/`)

| File | Purpose |
|------|---------|
| `PressStart2P-Regular.ttf` | All UI text (menu + gameplay). Menu title uses 2× size. |

Licensed under the [SIL Open Font License](https://openfontlicense.org/). If this file is missing, the game falls back to ImGui’s built-in ProggyClean pixel font.

## File names (use exactly these)

### Story panel background (`assets/ui/`)

| File | Purpose |
|------|---------|
| `bg_story_panel.png` | Main story log background (left panel, ~900×700 or larger) |
| `bg_main_menu.png` | Optional full-screen menu backdrop (1280×800) |

If `bg_story_panel.png` is missing, a solid purple placeholder is used.

### Character portraits (`assets/portraits/`)

| File | Character |
|------|-----------|
| `portrait_hero.png` | Player (Adventurer's Record) |
| `portrait_none.png` | Empty / unknown speaker (??? placeholder) |
| `portrait_elder_cedric.png` | Elder Cedric |
| `portrait_pip.png` | Pip |
| `portrait_lyra.png` | Lyra |
| `portrait_bram.png` | Bram |
| `portrait_valerie.png` | Valerie |
| `portrait_garrison.png` | Sir Garrison / Armored Knight |
| `portrait_eleanor.png` | Lady Eleanor |
| `portrait_garrick.png` | Garrick the Merchant |
| `portrait_malakor.png` | Demon General Malakor (optional) |
| `portrait_azael.png` | Demon Lord Azael (optional) |

Suggested portrait size: **128×128** or **256×256** PNG with transparency.

The **Character Speaking** panel uses the portrait for whoever you last talked to (`talk` command). The **Adventurer's Record** always uses `portrait_hero.png`.

### HUD icons (`assets/icons/`)

| File | Purpose |
|------|---------|
| `icon_gold.png` | Gold coin (optional, ~24×24) |
| `icon_atk.png` | Attack swords (optional) |
| `icon_def.png` | Defense shield (optional) |
| `icon_placeholder.png` | Empty party/inventory slot diamond |

Icons are optional; missing files fall back to text labels.

## Tips

- Use **PNG** with alpha for portraits and icons.
- Keep filenames **lowercase with underscores** as shown.
- After adding images, rebuild and run **`SpireOfAzael`** (the graphical build, not the console exe).
- Copy the `assets` folder next to `SpireOfAzael.exe` if you run from Visual Studio output directories.
