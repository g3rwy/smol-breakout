#!/bin/sh

g++ -o breakout breakout.cpp -lX11 -lGL -lpthread -lpng -lstdc++fs -std=c++17 -s -Os -flto
upx --lzma breakout
