# GC-anim-Wii
Attempt at a Gamecube intro animation recreation as a Wii native homebrew .dol using Gamecube IPL assets on-the-fly.

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
I have built a Wii portable, the Famicom Gboy, and it feels fantastic. The BitBuilt community has done an incredible job on the hardware and software side of things with a really integrated experience. However, I wasn't truly satisfied with RVLoader's splashscreen and wanted something that felt closer to the real experience.
