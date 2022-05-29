#!/usr/bin/env bash
# By: Melroy van den Berg
# Description: Deploy the tar, deb and rpm files to the WineGUI website download directory

if [ -z ${APP_VERSION} ]; then
    echo "ERROR: App_version env. variable is not set! Exit"
    exit 1
fi

echo "Listing files in build folder:"
ls ./build_prod/WineGUI-v*


# Check if the version already exists
output=$(sshpass -e ssh -o StrictHostKeyChecking=no melroy@gitlab.melroy.org 'cd /var/www/winegui.melroy.org/html/downloads; ls')
name="WineGUI-${APP_VERSION}"
if [[ "$output" == *"$name"* ]]; then
    echo "INFO: Release is already rolled-out to the downloads folder."
else
    echo "Transfer files to website..."
    echo ""
    # Roll-out the new release
    sshpass -e scp -o stricthostkeychecking=no ./build_prod/WineGUI-v* melroy@gitlab.melroy.org:/var/www/winegui.melroy.org/html/downloads
fi
