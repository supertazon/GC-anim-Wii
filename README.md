# GC-anim-Wii
Attempt at a Gamecube intro animation recreation as a Wii native homebrew .dol using Gamecube IPL assets on-the-fly (audio and textures, the 3D models are ripped).

![GC- bootscreen](https://github.com/supertazon/GC-anim-Wii/assets/1402795/bdafffc4-9655-401e-831c-f9a3c3e54372)


## How to install
- You need to have a softmodded Wii capable of launching homebrew. Follow this guide to softmod your Wii: https://wii.guide/. You need the Homebrew Channel installed and Priiloader if you want use use the .dol as a boot up animation.
- Copy the GC-anim folder inside your "apps" folder on your SD card.
- You need a Gamecube IPL file for the animation to play, otherwise it will exit the .dol and go back to the main menu. Here is a tutorial on how to dump the IPL file from a Gamecube: https://www.rwiihacks.com/tutorials/nintendont/#optional-dump-gamecube-bios-for-use-with-nintendont. If you already have an IPL file set up for Nintendont then you're good to go. Otherwise place your IPL file at the root of your SD card and rename it "ipl.bin" ("iplpal.bin", "iplusa.bin" and "ipljap.bin" -Nintendont's naming convention- also work).
- Test the animation by launching it through the Homebrew Channel and making sure it works as intended. If you get a black screen or it exits straight away then you either don't have the IPL file at the root of the SD card, or it is a bad IPL dump (File size should be 2 MB).
- If you want to have it as a Wii start-up animation you can do so by changing these parameters on Priiloader:
  - Hold the RESET button while powering on the Wii to access Priiloader
  - Highlight "Load/Install File" and press A
  - Scroll through the menu until you find "GC-anim" and press A, wait for the install to finish
  - Press B to go back to the main menu
  - Scroll down to "Settings" and press A
  - Highlight "Autoboot" and scroll right until it shows "Installed Filed"
  - Scroll down to "Save settings" and press A
  - Press B to go back to the main menu
  - Scroll down to "System Menu Hacks" and press A
  - Highlight "Auto-Press A at Health Screen" scroll right to mark it as "enabled"
  - Scroll below to highlight "Replace Health Screen with Backmenu" scroll right to mark it as "enabled"
  - Scroll down to highlight "save settings" and press A
- Now everytime you boot up your Wii it should play the animation. If you remove the SD card it should skip the animation and boot straight to the Wii System Menu

## Why?
I have built a Wii portable, the [Famicom Gboy](https://bitbuilt.net/forums/index.php?threads/famicom-gboy.5514/), and it feels fantastic. The BitBuilt community has done an incredible job on the hardware and software side of things with a really integrated experience. However, I wasn't truly satisfied with RVLoader's splashscreen and wanted something that felt closer to the real experience.

## How?
I didn't want it to be a simple video that would be display at boot up but a real Wii homebrew. The advantages of coding the animation are the freedom to customize it in the future and let the [memes begin](https://www.youtube.com/watch?v=lHE91NaDxLQ). I had learnt a bit of C about 10 years ago and thought it was a reasonable challenge to code a 6-second long animation in C for the Wii. My approach has been really to replicate the animation frame-by-frame best as I could, for example here the tilting of the camera at the right angle when the small cube hits the big cube at the beginning of the animation:
![anim-ref](https://github.com/supertazon/GC-anim-Wii/assets/1402795/663a137a-8767-4ab7-9c88-92dab8d574be)
If you look at the source code you'll see that there are different frame and stage counters that track the models positions and update them on a frame-by-frame basis. I apologize for the quality of the source code, as I said this is my real first C project and everything is a bit of a mess with plenty of bad practices I'm sure. Here, have a couple of WIP GIFs to make up for it:

![1](https://github.com/supertazon/GC-anim-Wii/assets/1402795/20c9860a-9964-4da2-885c-958dceabc8b6)
![2](https://github.com/supertazon/GC-anim-Wii/assets/1402795/bab31285-2e87-42c9-818e-11f169b3d174)
![3](https://github.com/supertazon/GC-anim-Wii/assets/1402795/f32635d1-5f33-490a-a2c4-4e6fecb941e6)

The animation uses libogc, asndlib, and GRRLib as the backbone with other functions detailed below.

<a href="https://wiibrew.org/wiki/GRRLIB"><img src="https://grrlib.github.io/GRRLIB/grrlib_logo.png" width="20%" height="20%"></a>

## Assets
Textures and audio tracks are decoded on-the-fly from the Gamecube's IPL. This was made possible thanks to all the reverse engineering work previously made by the community, see further below for the list of resources used in this project.

The IPL is first descrambled, depending on its region it then contains either 14 Yay0 files (NTSC IPL) or 31 Yay0 files (PAL IPL). Some of these Yay0 files contain J3D files (magic J3D1bmd1) which are 3D models produced by Nintendo's [JSYSTEM toolkit](https://wiki.cloudmodding.com/tww/JSYSTEM). Unfortunately these types of early Gamecube models have not been completely reverse engineered. These J3D files contain [all the data to display the 3D models](https://wiki.cloudmodding.com/tww/BMD_and_BDL), including textures which are in [BTI format](https://wiki.cloudmodding.com/tww/BTI). Among the 14 Yay0 files, 9 of them are J3D files covering the different assets used by the IPL (both for the intro animation and the following Gamecube menu). Using [wimgt.exe](https://szs.wiimm.de/wimgt/), I was able to convert the associated BTI texture files to PNG files and identify the textures I needed for this project:
- The environment texture (which is found in 7 of the 9 J3D model files) for the small cube, bigger cube and NINTENDO letters
- The texture for the tiles
- The three textures for the final logo
- The GAMECUBE written logo

The J3D files containing these textures are always stored in that order no matter the region of the IPL. Knowing exactly the header of each Yay0 file, I could implement a function to correctly find and extract each texture from the descrambled IPL. [Aurelio](https://github.com/Aurelio92/) helped me by providing me with a mini BTI library that made working with BTI files a breeze.

Unfortunately the 3D model part of the J3D files (with magic J3D1bmd1) has not been reverse engineered and I had to settle with using ripped 3D models. I used [NinjaRipper](https://ninjaripper.com/) in combination with Dolphin to get the 3D models for both cubes, the NINTENDO letters, and the tiles.

For the audio part, I mainly referenced this very in-depth video made by SynaMax: [Extracting sounds from the GameCube Main Menu!](https://www.youtube.com/watch?v=FEbcyf8cis4). The audio for the intro animation consists of two tracks: the left and right channel. The audio files are stored as a combination of [WSYS and AW files](https://wiki.cloudmodding.com/tww/AAF#WSYS). The WSYS file contains all the needed information to decode the raw audio AW file (which is ADPCM). Luckily, as mentioned in SynaMax's video, a decoding algorithm was already available and I implemented it into this project: [wsyster](https://github.com/hcs64/vgm_ripping/blob/master/soundbank/wsyster/wsyster.c).

## Source files overview
- `afcdecoder.c`: contains a modified implementation of wsyster.c
- `bti.c`: contains Aurelio's mini BTI library
- `cubes.c`: contains all the functions to display and animate the small rolling cube and the big translucent cube
- `descrambler.c`: contains the code to descramble the IPL
- `globals.c`: contains some global variables and a function to initialize graphics
- `logos.c`: contains all the functions to display and animate the main logo, the GAMECUBE text logo and the NINTENDO letters logo
- `main.c`: main
- `models.c`: stores the 3D models in arrays that are just OBJ files exported with Blender, also stores the coordinates for the texture projection on the small rolling cube
- `tiles.c`: contains all the functions to display and animate the tiles
- `yay0dec.c`: contains a function that finds the offsets of the needed Yay0 archives and of the AW audio files, and a function to decode Yay0 archives

## Head-to-head comparison
[Gamecube intro animation comparison](https://www.youtube.com/watch?v=O4ZZEfldFb8): as you can see it's very close to the original but not 100% the same.

## Improvements that could be made but that I couldn't implement
Feel free to fork this project and update it as you see fit, it is very unlikely that I will keep on working on this.

### Improvements on the basic animation
- **Texture projection on the cubes**: I tried best as I could to replicate the environment texture projection on the small rolling cube and on the big translucent cube. However, comparing them with the original we see many differences. To achieve the texture projection, I followed [this example from the GRRLib github repo](https://github.com/GRRLIB/GRRLIB/tree/master/examples/NEED_GRRLIB_FUNCTION). You are welcome to try and tweak the values to better match the original. Beware as in GC-anim-Wii the texture projection coordinates are stored for each frame of the rolling cube, in arrays with 191 indices.![color-comparison](https://github.com/supertazon/GC-anim-Wii/assets/1402795/90dc53e3-42e1-40b8-8250-f7f79de36578)

- **Color of the cubes**: The environment texture provided by the IPL is grayscale and needs to be colored to get it to indigo. Looking at the picture above, you can see that the color is a bit different, it really shows on the smaller cube. I tried to tweak the TEV stages best as I could but could never manage to get the proper indigo color. I guess this improvement is related to the previous one.
- **Better antialiasing**: I am simply using GRRLib's `GRRLIB_Settings.antialias = true;` command but the models are a bit jagged around their edges. I am not sure how I can improve this.
- **Reverse engineering the J3Dbmd1 file format**: and load them on the fly, to not rely on external 3D models. I suspect that this RE work would solve the first two improvements.

### Further improvements for customising the animation
- **Color customization**: the environment texture is in grayscale thus easy to recolor, same goes for the tile texture. However the logo textures are already in indigo color. It would be nice to be able to recolor them to whatever we want to achieve full color customization, similar to [cubeboot](https://github.com/OffBroadway/flippyboot-ipl/tree/main/cubeboot).
- **GAMECUBE logo customization**: adding the ability to load an external texture to replace the "GAMECUBEtm" text. Also already implemented into cubeboot. It would be awesome if it could be generated on-the-fly as well.
- **Controlling the camera with the stick**: to slightly move the camera while the animation plays depending on the stick coordinates. A bit like Smash Bros' character selection screen.

## Acknowledgements
- [The BitBuilt community](https://bitbuilt.net/): for being awesome and pushing the boundaries of console portablizing
- [Owen](https://wiibrew.org/wiki/User:Owen): for helping me bootstrap the project and setting up devkitPro correctly, make sure you check out his awesome Wii homebrew games (still developing in 2023)!
- [Trevor](https://github.com/trevor403): for cubeboot that inspired this project and general guidance regarding the IPL
- [Aurelio](https://github.com/Aurelio92): for the immense help in Wii homebrew programming and IPL asset extraction
- [Extrems' Discord server](https://discordapp.com/invite/JZsBV3Bn): for general Wii homebrew development and in particular Luig who has helped me get the texture projection going by fixing the 3D models
- Nintendo for the excellent games and hardware I've been playing for all these years.

## Resources
- IPL descrambler: [taken from flippyboot-IPL](https://github.com/OffBroadway/ipl-decoder/blob/main/ext/descrambler.c)
- Yay0 file decompression: [adapted from ethteck's python script](https://github.com/ethteck/n64decompress/blob/main/Yay0/decompress.py)
- Audio AFC decoder: [adapted from hcs64's wsyster.c](https://github.com/hcs64/vgm_ripping/blob/master/soundbank/wsyster/wsyster.c)
- SynaMax's video on Gamecube IPL audio extraction: [Extracting sounds from the GameCube Main Menu](https://www.youtube.com/watch?v=FEbcyf8cis4)
- BTI file format: [Cloudmodding](https://wiki.cloudmodding.com/tww/BTI)
- [GRRLib](https://wiibrew.org/wiki/GRRLIB)
- Tutorial to start Wii homebrew programming from scratch: [Wii Homebrew App Creation Tutorial #1 (Setup and Hello World)](https://www.youtube.com/watch?v=noxGGdxHbDY)
- Spanish GRRLib tutorial with 11 lessons: [Tutoriales de programación de GRRLIB](https://wii.scenebeta.com/tutorial/tutoriales-de-programacion-de-grrlib)
- Spanish GX code examples with a nicely made PDF documentation: [Graficos 3D para Wii (PDF + Ejemplos)](https://www.elotrolado.net/hilo_wii-graficos-3d-para-wii-pdf-ejemplos_1049703)
- French 3D model viewer tutorial using GRRLib: [Intro à la 3D sur Wii](https://web.archive.org/web/20130704023828/http://www.dev-fr.org/index.php/topic,4870.0.html)
- Notes on setting up TEV stages: [from Amnoid's website](http://amnoid.de/gc/tev.html)
