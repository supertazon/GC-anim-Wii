# GC-anim-Wii
Attempt at a Gamecube intro animation recreation as a Wii native homebrew .dol using Gamecube IPL assets on-the-fly (audio and textures, the 3D models are ripped).

![image](https://github.com/supertazon/GC-anim-Wii/assets/1402795/73363793-04cc-4457-a1fd-abe6f1c36d65)

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
If you look at the source code you'll see that there are different frame and stage counters that track the models positions and update them on a frame-by-frame basis. I apologize for the quality of the source code, as I said this is my real first C project and everything is a bit of a mess with plenty of bad practices I'm sure. Here a couple of WIP GIFs to make up for it:

![1](https://github.com/supertazon/GC-anim-Wii/assets/1402795/9618235d-009b-4ab6-99a3-3d474d0c4314)
![2](https://github.com/supertazon/GC-anim-Wii/assets/1402795/4bd25d5a-0337-4dee-ae36-de955c6e53af)
![3](https://github.com/supertazon/GC-anim-Wii/assets/1402795/51d8a90f-e1e5-4e04-879c-c1082d3c458e)

The animation uses libogc, asndlib, and GRRLib as the backbone with other functions detailed below.![image](https://github.com/supertazon/GC-anim-Wii/assets/1402795/410b723e-32e8-45ac-a483-ffbe533a8be4)
