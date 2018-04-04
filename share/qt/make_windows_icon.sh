#!/bin/bash
# create multiresolution windows icon
ICON_SRC=../../src/qt/res/icons/evolvechain.png
ICON_DST=../../src/qt/res/icons/evolvechain.ico
convert ${ICON_SRC} -resize 16x16 evolvechain-16.png
convert ${ICON_SRC} -resize 32x32 evolvechain-32.png
convert ${ICON_SRC} -resize 48x48 evolvechain-48.png
convert evolvechain-16.png evolvechain-32.png evolvechain-48.png ${ICON_DST}

