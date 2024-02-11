#!/usr/bin/env bash
# By: Melroy van den Berg
# Description: Create custom source code archive with version.txt
# Depends on one environment variable: $APP_VERSION

if [ -z ${APP_VERSION} ]; then
    echo "ERROR: App_version env. variable is not set! Exit"
    exit 1
fi
# Remove the first char ('v') from APP_VERSION
VERSION="${APP_VERSION:1}"
# Create version.txt
echo "${VERSION}" > version.txt
# Create source archive (tar.gz)
git archive --format=tar.gz --output=build_prod/WineGUI-Source-${APP_VERSION}.tar.gz --add-file=version.txt HEAD
