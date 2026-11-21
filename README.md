# GC-anim-Wii

The GameCube boot animation, running natively on the Wii as a homebrew `.dol`, reproduced frame for frame from the GameCube IPL's own code and data.

It also has a second intro: the alternative GameCube intro seen in Nintendo's promotional videos, which never shipped on the console (see [The promo intro](#the-promo-intro)). `gc-anim.ini` picks which one plays.

Nothing from Nintendo is distributed. The models, textures, animations and the jingle are read at run time from **your own GameCube IPL dump** on SD or USB.

## What's new (v2)

- **No GRRLIB:** plain libogc/GX.
- **No ripped models:** the original J3D models are loaded straight from the IPL and drawn with the same GX state as the IPL's own renderer. That fixes the colour and env-mapping problems of v1 for both cubes.
- **Exact animation:** the animation logic was reverse-engineered from the IPL (BS2) and ported to C. It matches a Dolphin FIFO capture of the real boot sequence on every one of its 331 frames: cube path, camera shakes, tiles, logo, NINTENDO letters, GAMECUBE text and fade-out.
- **Anti-aliasing like the original:** 3-sample AA with the two-pass top/bottom rendering the IPL uses.
- **All IPL versions:** NTSC 1.0/1.1/1.2 and PAL 1.0/1.1/1.2 (the assets are located through the IPL's resource directory and identified by name).
- **Video:**
  - 60 Hz (NTSC, PAL60, MPAL; 480i and 480p): the 60 Hz timings.
  - 50 Hz PAL: the PAL IPL's own 50 Hz timings (it has its own set) and its 520-line picture.
  - 16:9: the scene is widened, not stretched.
- **The alternate jingles:** hold **Z** on controller 1, or on all four controllers, while the console starts.
- **The promo intro:** `intro = promo` plays the alternative intro of Nintendo's promo videos instead, rebuilt in real time from the IPL's own logo and letters.
- **Random colours:** `color = random` draws a new colour scheme at each boot.
- **Always back to the System Menu:** made to be installed in Priiloader as the Wii's boot animation.
- **Super Mario Bros. 1-1 easter egg:** with your own `smb.nes` next to `boot.dol`, press **B** (or **1** on a Wii Remote held sideways) early in the animation and play World 1-1 on the big cube's face, then land back in the animation. See **[SMB.md](SMB.md)**.

## Installation

1. You need a softmodded Wii with the Homebrew Channel (<https://wii.guide/>), and Priiloader if you want to use the animation as a boot animation.
2. Download `GC-anim-Wii-v2.0.zip` from the [Releases](https://github.com/supertazon/GC-anim-Wii/releases) page and extract it to the root of the SD card (or USB drive). This creates `apps/gc-anim-wii/` with `boot.dol`, `meta.xml`, `icon.png` and a `gc-anim.ini` to edit. Everything goes in that folder: the program, your IPL dump, and the optional `gc-anim.ini`, logo image, `smb.nes` and `promo.wav`. Files anywhere else are not read, except the IPL dump, which may also be at the root of the card or drive. SD and USB work the same way: when both have a file, the SD card's copy is used. Keep all the files on the same device.
3. Put your GameCube IPL dump there as `ipl.bin`. The names `iplusa.bin`, `iplpal.bin` and `ipljap.bin` (Nintendont's naming) also work. If it isn't in `apps/gc-anim-wii/`, it is also looked for at the root of the SD card, then of the USB drive. A USB drive that is still starting up at power-on is given up to 3 seconds when the IPL dump isn't found on the SD card. Scrambled (raw) and descrambled dumps are both accepted. How to dump the IPL: <https://www.rwiihacks.com/tutorials/nintendont/#optional-dump-gamecube-bios-for-use-with-nintendont>.
4. Test it from the Homebrew Channel.
5. Optional, to use it as the boot animation: in Priiloader, choose *Load/Install File* → gc-anim-wii, then *Settings* → *Autoboot: Installed File*. Also enable *Replace Health Screen with Backmenu* and save.

Without an IPL dump the program exits immediately to the System Menu. Whatever happens (end of the animation, skip, reset), it always ends in the System Menu, even when launched from the Homebrew Channel.

## Customisation (optional)

Edit `gc-anim.ini` in `apps/gc-anim-wii/`, next to `boot.dol` (the release comes with one; without it, every setting keeps its default). The same folder is used when the program is installed in Priiloader.

```ini
[general]
intro = ipl          ; ipl (the IPL's boot animation) or promo (the promo videos' intro)
skip = 0             ; 1 = A or Start skips the animation
camera = 1           ; C-stick (GameCube controller 1) orbits the camera
force_4_3 = 0        ; ignore the console's 16:9 setting
volume = 199         ; jingle volume 0-255 (199 = the IPL's own level)
delay = 0            ; ms of black screen before the animation (0-10000), for slow HDMI screens/mods

[colors]
color = 255,60,60    ; theme colour: maps the cube's indigo onto this colour

[text]
letters = WIIBREW              ; replaces the 3D "NINTENDO" letters
letters_spacing = natural      ; natural or fill
image = logo.png               ; replaces the "GAMECUBE" logotype (file in apps/gc-anim-wii/)
```

- **Letters:** up to 16 capital letters A–Z and spaces; lower case is converted.
  - N, I, T, E, D and O are the IPL's own letters. The other 20 are an original block alphabet drawn in the same style (`tools/letters_design.py`).
  - Each letter flips up with the motion of the original letter nearest its position.
  - `natural` keeps NINTENDO's letter pitch for words of up to 8 letters, centred; longer words are squeezed to NINTENDO's width.
  - `fill` spreads every word over NINTENDO's width.

- **Logotype:** PNG, BMP, JPG or TGA, drawn white on a transparent or black background. A 352×40 image (e.g. from <https://trevor.cafe/logo/>) is used pixel for pixel. Any other size has its empty borders cropped, then is fitted and centred, keeping its aspect ratio. Give just the file name and put the image in `apps/gc-anim-wii/`; a full path such as `usb:/logos/mine.png` also works. A `logo.png` in that folder is used even without the setting.

`camera`, the alternate jingles (Z) and the SMB easter egg are for `intro = ipl` only. The `delay` is counted in frames at the console's real refresh rate (50 Hz, or 59.94 Hz for NTSC, PAL60 and 480p), the same in both intros.

### Colours

Every key is optional. Colours are `R,G,B` or `#RRGGBB`. Each one is applied with the model's own shading, so it keeps the original lighting and shine.

| key | what it colours | default |
|---|---|---|
| `color` | everything below, unless set separately; `random`: see below | the original indigo |
| `cube` | the small rolling cube; `tiles` = takes the colour of the tile appearing under it | `color` |
| `tiles` | the tiles' final colours: one colour, a list of up to 16, or `rainbow` | `color` |
| `tiles_mode` | with a list: `cycle` (A B C A B C…) or `spread` (bands along the path) | `cycle` |
| `trail` | new tiles start in the cube's colour and reach their own over this many tiles (0 = at once) | 0 |
| `logo` | the final 3D logo | the colour that follows the last tile |
| `drop` | colour the cube turns into on its final drop | `logo` |
| `drop_blend` | when: `fall` (the drop only), `jump` (rise and drop) or `off` | `fall` |
| `glass` | the glass cube flashing on the impacts | `logo` |
| `glass_opacity` | strength of those flashes, in % (0 = hidden) | 100 |
| `background` | the background, and the colour of the final fade | black |
| `text` | "NINTENDO" and "GAMECUBE" | white, or black on a light background |
| `hue`, `saturation`, `brightness` | global adjustment on top of all the above (degrees, %, %) | 0, 100, 100 |

The cube lays 16 tiles. With a shorter list, `cycle` repeats it and `spread` stretches it along the path. By the time the cube lands, all tiles have reached their final colour.

**`color = random`:** a new colour scheme at each boot. It replaces `color`, `cube`, `tiles`, `tiles_mode` and `trail`; the other keys (`logo`, `glass`, `background`, `text`, `hue`…) still apply if set. The schemes, with how often they come up:
- one colour (25%), sometimes with a small cube in a contrasting colour;
- a gradient between two colours (30%), in 4 to 16 bands along the path;
- part or all of the colour wheel from a random starting hue (15%);
- a repeating motif of 2 to 6 tiles in 2 or 3 matching colours (30%).

With tiles, the small cube either keeps the theme colour or takes each tile's colour, and a trail of 1 to 4 tiles is sometimes added. Colours are always vivid: never grey and never dark. In the promo intro, `random` gives a random theme colour.

Examples:

```ini
; Famicom: gold cube, red trail, red logo
[colors]
cube  = 212,175,55
tiles = 113,21,33
trail = 4

; rainbow
[colors]
tiles = rainbow
cube  = tiles

; French flag
[colors]
tiles = 0,85,164  255,255,255  239,65,53
cube  = tiles

; black console
[colors]
color      = 40,40,45
background = 255,255,255
```

A very dark grey such as `40,40,45` keeps the 3D look; pure black flattens the models into silhouettes.

## The promo intro

`intro = promo` plays the alternative GameCube intro seen in Nintendo's promotional videos: light streaks draw the cube, a glass cube "slams" into the solid logo, then the picture fades.
- **Logo, NINTENDO letters and GAMECUBE logotype:** read at run time from your IPL dump (`boot_demo_logo_mark` and `boot_demo_mat28` models), drawn as an isometric view in the promo's colours.
- **Sound:** `promo.wav` in `apps/gc-anim-wii/`, the audio of the promo clip (PCM 16-bit, mono or stereo, any rate). It isn't in the IPL. Without it the intro plays silently.
- **Timings, colours, streak paths and slam layers:** measured on the reference clip and stored in `source/anim_data.h` (generated by `tools/anim.py`). Nothing from Nintendo is in the source or the `.dol`.
- **Size and layout:** drawn 1.28× the clip's size, with its final layout matching the IPL's last frame (logo, NINTENDO and GAMECUBE each mapped onto their box in that frame).

Settings used by the promo intro:

| key | effect |
|---|---|
| `[general]` `skip`, `force_4_3`, `volume`, `delay` | as for the IPL intro (`volume = 199` is the promo's full level) |
| `color` | the logo and the light (streaks, glow, glass cube, slam) |
| `logo` | the final logo: the IPL's face textures recoloured exactly as in the IPL intro. Without `logo` or `color`, the promo's pale lavender |
| `glass` | the light only: it takes this colour's hue and saturation and keeps its own brightness |
| `hue`, `saturation`, `brightness`, `glass_opacity` | as for the IPL intro |
| `background` | background and final fade (it lightens the picture: "screen" blend, so the text can't be darker than it) |
| `text` | final NINTENDO / GAMECUBE colour (NINTENDO a bit darker, as in the promo) |
| `[text]` `letters`, `letters_spacing` | the custom letters replace NINTENDO, with the same spacing rules and alphabet as the IPL intro, drawn flat in the promo's style |
| `[text]` `image` (or `logo.png`) | the custom logotype replaces GAMECUBE, placed as in the IPL intro |

`cube`, `tiles`, `tiles_mode`, `trail`, `drop`, `drop_blend`, `camera`, the Z jingles and the SMB easter egg do nothing in the promo intro.

## Building

Requires devkitPro (devkitPPC, libogc, libfat).

```sh
make
```

This builds `GC-anim-Wii.dol` (named after the project folder); copy it to `apps/gc-anim-wii/` as `boot.dol`.

## How it works

| file | role |
|---|---|
| `source/ipl.c` | loads and descrambles the dump, finds the resource directory, extracts the assets by name |
| `source/j3d.c` | the IPL's model renderer: INF1 traversal, MAT1 materials → GX, shapes drawn from their display lists, env-map texture matrices, the "model alpha" TEV stage |
| `source/anim.c` | joint animation (`J3D1bck1`) and material colour animation (`IPK1`) of the NINTENDO/GAMECUBE model |
| `source/intro.c` | the animation logic: drop, roll path, final jump, impact shakes, tiles, alphas, fade. Each function names the IPL routine it reproduces |
| `source/gcmath.c` | the IPL's 4096-entry sine table, Euler matrices and hermite interpolation, in the same float operation order |
| `source/scene.c` | the draw order, tiles, light and fade quad |
| `source/video.c` | render mode (592×448 or 592×520, VI-scaled to 640), AA two-pass copy, 16:9 |
| `source/ipl_audio.c` | locates and decodes the AFC jingle (WSYS/IBNK/sequence) |
| `source/recolor.c`, `source/textimg.c`, `source/config.c` | customisation |
| `source/letters_data.c` | the custom alphabet's meshes (generated) |
| `source/promo.c`, `source/anim_data.h` | the promo intro: logo layers, light streaks, slam, glow; its timeline (generated by `tools/anim.py`) |
| `source/smb*.c`, `source/nes/`, `source/ipl_bank.c` | the Super Mario Bros. 1-1 easter egg (details in [SMB.md](SMB.md)) |

`tools/` holds the generators of the two generated source files (Python 3):
- `letters_design.py` defines the custom alphabet; `make_letters.py` (needs shapely ≥ 2.1) extrudes it into `source/letters_data.c`;
- `anim.py` is the promo intro's timeline (timings, colours, streak paths, slam layers); `python3 tools/anim.py source/anim_data.h` regenerates the header.

## Acknowledgements

- [The BitBuilt community](https://bitbuilt.net/)
- [Aurelio](https://github.com/Aurelio92/) for the help with Wii homebrew and IPL asset extraction (v1)
- [Trevor](https://github.com/trevor403) for cubeboot, which inspired this project
- [DacoTaco](https://github.com/DacoTaco) for the return-to-menu fix
- Owen, Luig and Extrems' Discord server (v1)
- IPL descrambler by Segher Boessenkool (via flippyboot's ipl-decoder)
- [stb_image](https://github.com/nothings/stb) (public domain) for the custom logotype
- Nintendo, for the hardware and games
