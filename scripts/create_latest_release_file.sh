#!/usr/bin/env bash
# By: Melroy van den Berg
# Description: Create a latest_release.txt file on winegui.melroy.org,
#  which can be used to verify if the user is using the latest version of WineGUI.

if [ -z ${APP_VERSION} ]; then
    echo "ERROR: App_version env. variable is not set! Exit"
    exit 1
fi

# Remove the first char ('v') from APP_VERSION
VERSION="${APP_VERSION:1}"
# Create text file
echo "$VERSION" >latest_release.txt

echo "Upload latest_release.txt file..."
scp -i ./id_gitlab_rsa -o stricthostkeychecking=no ./latest_release.txt melroy@gitlab.melroy.org:/var/www/winegui.melroy.org/html
