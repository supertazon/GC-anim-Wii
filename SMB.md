# Super Mario Bros. easter egg

<p align="center">
  <img src="assets/smb-easter-egg-cut.gif" alt="SMB in GameCube IPL" width="444">
</p>

Press B (on the GC pad, or 1 on the Wiimote sideways) early in the animation and the GameCube boot animation turns into a playable Super Mario Bros., drawn onto the big cube's face: World 1-1 only, or the whole game, with a single life. Only works when setting the intro animation to IPL, does not work with the promo intro. When the run is over (the 1-1 flagpole, a lost life or the end of the game), the animation picks up again and ends as usual.

Nothing from the game is distributed. The program runs **your own copy of the game** (`smb.nes`) in a small NES emulator written for this project. The game's music is played on the GameCube IPL's own instruments, taken from your IPL dump.

## What you need

- Everything from the main [README](README.md): a working GC-anim-Wii with your IPL dump.
- `smb.nes`: **Super Mario Bros.** for the NES, the original World/Japan release (one ROM for both regions), in iNES format (`.nes` with its 16-byte header). Put it next to `boot.dol` in `apps/gc-anim-wii/`. Only that exact ROM is used (its CRC-32 is checked): with any other file, B and 1 do nothing and the animation plays as usual. The SMB mode is part of the IPL intro only (`intro = ipl`).

The ROM is checked when the program starts (CRC-32 of the game data, header excluded: `D445F698`). Any other file — another game, a hack, the PAL release (*Super Mario Bros. (Europe)*), a bad dump — is ignored, and the animation plays as usual.

## How to play

Press the button **in the first seconds of the animation**, before the small cube finishes its rolls on the side of the big cube (holding it from power-on works too). Too late and the animation simply goes on.

| | GameCube controller | Wii Remote (held sideways) |
|---|---|---|
| Launch (during the animation) | **B** | **1** |
| Launch in the game's original colors* | **Y** | **2** |
| Move | Control Stick or D-Pad | D-Pad |
| Run / fireballs | **B** | **1** |
| Jump | **A** | **2** |
| Pause | **Start** | **+** |
| Select | **Z** | **−** |
| Quit (hold 1 s) | **Start + Z** | **HOME** |

*once the whole game has been finished, once is enough and kept in memory for future boots

Both controllers can be used at the same time. The Wii Remote is only connected when an `smb.nes` is present. It needs a second or two after the program starts, so on a Wii Remote, press 1 a little later than you would press B, or use the `delay` setting (below).

## What happens

1. **The launch.** As the small cube lays its last tile, the camera glides from the logo onto the big cube's face, and World 1-1 appears on it. The two cubes roll on ahead and lay the ground as they go, Mario jumps out of the top one with a spin.
2. **Playing.** The level is the game itself, running in real time: the background is drawn flat on the face, the ground is made of 3D blocks like the IPL's cube, and sprites are drawn as in the game. Mario waits until the cubes have rolled out of the picture. The camera follows the height he stands at (stairs, blocks) like a platformer camera.
   - **16:9:** the game's picture keeps its right edge at the screen's edge (things come in from off-screen as on the NES). The extra space on the left shows the part of the level already passed, fading out, with the enemies that left the NES picture.
   - **C-stick:** if you turned the view with the C-stick (`camera = 1`) before launching, the game is played in that view.
   - **Pipes:** the underground rooms work like in the game.
   - **The whole game (`smb = full`):** after each flagpole the game goes on as on the NES, with its castle, the time counted into the score and its fireworks, then the next world screen (shown face on, with Mario as he is: small, big or fire). Toad's and Peach's messages and the warp zone's text are shown face on too.
3. **Losing a life.** The game's own death animation plays with the camera still. As Mario falls out of the picture the camera follows him, the level leaves the top of the screen, and after a moment of black the GameCube logo comes up from below with the cube at the top of its jump. Mario turns to face you, lands on the cube as the death tune ends, and the end of the IPL jingle plays from that point. The cube drops into the logo and the animation ends as usual.
4. **The 1-1 flagpole (`smb = 1-1`).** Mario slides down the pole, turns round and leaps off with a backflip, to the level-clear fanfare (played faster, so that it ends exactly as he lands on the cube). The game goes on unseen: the castle (not drawn), the time counted into the score, and the fireworks. The score is shown under the logo as it counts, and three fireworks or more go off around the logo, face on. The animation holds the logo until it is all over and the result has been shown, then fades out.
5. **The end of the game (`smb = full`).** Once Peach's message is shown, Mario leaves the level as from the 1-1 flagpole, from where he stands by Peach, and lands on the cube, while the game's ending music plays on. Six fireworks go off around the logo (for show, no points), and the animation holds the logo until they are over and the result has been shown, then fades out.
6. If you turned the view with the C-stick, it eases back to the IPL's own view once Mario is on the cube.

## Sound

- **Sound effects:** the NES's own sounds, synthesized by an emulation of its sound chip (APU), checked against Mesen.
- **Music:** the game's music, played on the IPL's instruments. The melody and harmony use one of the IPL's soft instruments, the bass another, the drums are a hit cut from the jingle. The music is in stereo with a light reverb, the sound effects stay dry and centered, as on the NES.
- **The transition:** from the IPL jingle into the 1-1 theme. The jingle fades out while the theme comes in on the jingle's beat, at the jingle's tempo, and slows down to its own over two seconds.
- **Pause:** the pause sound plays and the music waits.

## Settings

The usual `gc-anim.ini` settings apply. These are the ones that matter for the game:

- `smb = 1-1` or `full`: World 1-1 only, or the whole game. Either way, with a single life.
- `delay = N`: N ms of black screen before the animation starts. Meant for slow HDMI screens and mods, it also gives the game (and a Wii Remote) more time to get ready before you press the button.
- `camera = 1`: the C-stick view, kept while playing (see above).
- `volume`: sets the jingle's level, the game's sound follows it.
- The theme colors tint the level's flat background, the cubes and the score.
- While playing, the score and the time (after a clock) are shown in the top-left corner of the screen (the game's own status bar is not shown), each world screen shows Mario as he is (small, big or fire).
- Under the logo at the end: the run (score, time or where it ended, C-stick angle in degrees, NEW BEST), then one BEST line: the best score before it, the best time (or, before the game has been finished, where the best score's run ended) and an angle.

## Original colors

Once the whole game has been finished (`smb = full`, Peach's message reached, `smb.sav` then holds an `unlock` code made from that run: it cannot simply be typed in), the game can be launched with **Y** (GameCube controller) or **2** (Wii Remote) instead of B / 1: it is then played in its own NES colors, the floors' IPL tiles taking the color of the ground they stand for. B / 1 still gives the theme colors. Finishing World 1-1 alone does not unlock it.

The animation around the game keeps its theme colors, with one exception: when a run in the original colors ends (death, the 1-1 flagpole or the end of the game), the rest of the animation is in Mario's colors: red cube, tiles and logo, with Mario's two gold buttons on the bottom row of the logo, and a gold glass cube.

The records are the same whichever the colors, `smb_runs.txt` notes them for each run.

## Limits

- A single life: after a lost life, the 1-1 flagpole (`smb = 1-1`) or the end of the game (`smb = full`), the animation ends.
- 50 Hz PAL video: the game runs at 50 frames per second, slower, as on a PAL console. This mode has not been tested yet.

## How it works

On the Wii, the game boots in a background thread while the animation plays, from power-on to the first frame of 1-1 (about 200 NES frames). The thread then records the level's music and the level-clear fanfare from a copy of the game. When you press the button, the program waits for the animation to reach the right frame, then takes over from it.

The game runs as it would on a NES, one NES frame per picture. What is shown is rebuilt every frame from the game's video memory and its RAM:
- the background layer from the nametables (the 3D ground replaces the ground tiles),
- the sprites from the sprite memory,
- the scroll and the level position from the game's RAM.

The memory addresses come from doppelganger's *Comprehensive Super Mario Bros. Disassembly* (`Enemy_ID`, `ScreenLeft_PageLoc`, `GameEngineSubroutine`, …). The endings and the camera use the game's state to follow what happens: on the pole, dead, paused, in another area.

| file | role |
|---|---|
| `source/nes/nes.c` | the NES: 6502 CPU, PPU (registers, scroll, sprite-0 hit and timing, no pixel output), controller, mapper 0 |
| `source/nes/apu.c` | the NES sound chip: pulse, triangle, noise and DMC level, frame counter and the NES's output filters |
| `source/smb.c` | the mode itself: boot thread, launch, play loop, camera, the endings, the transitions, sound timing |
| `source/smb_flow.c` | follows the run: 1-1 or the whole game, death, flagpole, end of the game |
| `source/smb_layout.c` | turns the game's video memory and RAM into what is drawn: background layer, 3D ground, sprites, the player, the enemies at the left edge, score, fireworks |
| `source/smb_render.c` | draws it on the big cube's face with GX (CI8 textures with the NES palette, the 3D blocks, the player turned to face the viewer) |
| `source/smb_sound.c` | sound effects from the APU, music turned into notes for the IPL instruments, stereo, reverb |
| `source/smb_music.c` | records the game's music (its sound routine run on its own) and replays it at any tempo |
| `source/smb_audio.c` | the audio output: one stereo voice fed from a ring buffer |
| `source/smb_records.c` | the records (`smb.sav`) and the run log (`smb_runs.txt`) |
| `source/ipl_bank.c` | reads the IPL's instrument bank (WSYS/IBNK) and decodes its samples |

## Credits

- Super Mario Bros. © Nintendo. The game is not included, the program runs your own copy.
- **doppelganger**, for *A Comprehensive Super Mario Bros. Disassembly* (`smbdis.asm`): the RAM and routine names used throughout (annotated HTML version: <https://6502disassembly.com/nes-smb/>).
- [Mesen](https://www.mesen.ca/) as the reference for the sound chip.
