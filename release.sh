#!/bin/bash
# Deploy the tar, deb and rpm files to the WineGUI website download directory

# TODO: Check if the version already exists

# Roll-out the new release
sshpass -e scp -o stricthostkeychecking=no ./build/WineGUI-v* melroy@server.melroy.org:/var/www/winegui.melroy.org/html/downloads