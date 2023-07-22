# GC-anim-Wii
Attempt at a GC intro animation recreation as a Wii native homebrew .dol using GC IPL assets on-the-fly.

![image](https://github.com/supertazon/GC-anim-Wii/assets/1402795/73363793-04cc-4457-a1fd-abe6f1c36d65)

## How to install
- You need to have a softmodded Wii capable of launching homebrew. Follow this guide to softmod your Wii: https://wii.guide/. You need the Homebrew Channel installed and Priiloader if you want use use the .dol as a boot up animation.
- Copy the GC-anim folder inside your "apps" folder on your SD card.
- You need a Gamecube IPL file for the animation to play, otherwise it will exit the .dol and go back to the main menu. Here is a tutorial on how to dump the IPL file from a Gamecube: https://www.rwiihacks.com/tutorials/nintendont/#optional-dump-gamecube-bios-for-use-with-nintendont. If you already have an IPL file set up for Nintendont then you're good to go. Otherwise place your IPL file at the root of your SD card and rename it "ipl.bin" ("iplpal.bin", "iplusa.bin" and "ipljap.bin" -Nintendont's naming convention- also work).
- Test the animation by launching it through the Homebrew Channel and making sure it works as intended. If you get a black screen or it exits straight away then you either don't have the IPL file at the root of the SD card, or it is a bad IPL dump (File size should be 2 MB).
- If you want to have it as a Wii start-up animation
