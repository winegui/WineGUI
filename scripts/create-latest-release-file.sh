#!/usr/bin/env sh
# By: Melroy van den Berg
# Description: Create a latest_release.txt file on winegui.melroy.org,
#  which can be used to verify if the user is using the latest version of WineGUI (client-side check).

if [ -z ${APP_VERSION} ]; then
    echo "ERROR: App_version env. variable is not set! Exit"
    exit 1
fi

# Remove the first char ('v') from APP_VERSION
VERSION="${APP_VERSION:1}"
# Create new folder (release)
mkdir release
# Create text file with the latest version
echo "$VERSION" >release/latest_release.txt
