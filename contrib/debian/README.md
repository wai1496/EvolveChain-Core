
Debian
====================
This directory contains files used to package evolvechaind/evolvechain-qt
for Debian-based Linux systems. If you compile evolvechaind/evolvechain-qt yourself, there are some useful files here.

## evolvechain: URI support ##


evolvechain-qt.desktop  (Gnome / Open Desktop)
To install:

	sudo desktop-file-install evolvechain-qt.desktop
	sudo update-desktop-database

If you build yourself, you will either need to modify the paths in
the .desktop file or copy or symlink your evolvechain-qt binary to `/usr/bin`
and the `../../share/pixmaps/evolvechain128.png` to `/usr/share/pixmaps`

evolvechain-qt.protocol (KDE)

