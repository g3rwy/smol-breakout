# Smol Breakout
### My try at making very small Breakout game, similar to my [smol Pong](https://github.com/g3rwy/smol-pong), but now with colors and for Windows too

## The final size is only *27KB*!!

![Some "gameplay" lol](https://cdn.discordapp.com/attachments/845631328656293928/935640401690394645/Breakout.gif)

---
Some Controls:
`Tab` - Shows what those magic numbers are on the top

`Space or Up` - Shoot the ball when you are holding it

`Left or Right` - Pretty obvious

Of course its *buggy* but its playable
Im working on Linux so i used Mingw to compile for Windows, maybe its not the best way but it was the best for my case. The style of it was inspired by some of the old Atari 2600 Breakout style.I also used [PixelGameEngine](https://github.com/OneLoneCoder/olcPixelGameEngine) as my engine/framework to display stuff, previously i used X11 which wasn't that bad but PGE is much better in this case, made it possible for me to port it to Windows.

So i also provide binaries of the game in releases tab, keep in mind they are not **smallest** build, they are statically linked and not compressed so your machine won't have difficulties in opening it and also it won't get recognized as a virus (a lot of viruses use upx to compress binaries, Windows Defender is not a fun of it)

# Building
But if you want to build it yourself sure, go on

## Linux
on linux its pretty easy just compile it with 
(i have **build.sh** exactly doing those stuff so you can run it instead)
`g++ -o breakout breakout.cpp -lX11 -lGL -lpthread -lpng -lstdc++fs -std=c++17 -s -Os -flto`

there are some libraries we want and also flags for size optimization
after that compress it with upx

`upx --lzma breakout`

for me --lzma works the best

## Windows
Well because i used mingw i will show how i did it with mingw, probably you are able to use the above command when using cygwin or something, but if you want to compile it you will figure it out
using mingw32-g++, also you need to compile **breakout_Win.cpp** i made some changes there for it to be playable on Windows


`x86_64-w64-mingw32-g++-win32 breakout_Win.cpp -luser32 -lgdi32 -lopengl32 -lgdiplus -lshlwapi -ldwmapi -lstdc++fs -std=c++17 -static`


linking it statically, but you can try without it and see if it works for you, it should if you have all stuff installed
after that you can also compress it with upx using --lzma flag 

## Mac
Currently i have no idea how to compile it for OSX, i have a machine to compile it on so i might place a binary in a release. If you have g++ installed (via homebrew probably) you should be able to do the same command to compile as on Linux, remember to have all requirements installed.

***
Levels are currently made out of all bricks being 3 bits (this way i was able to make levels a png's with all brick types being different color)
I can add more levels in future.
### Have fun playing!

