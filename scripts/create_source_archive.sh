#!/usr/bin/env bash
# By: Melroy van den Berg
# Description: Create custom source code archive with version.txt
# Depends on one environment variable: $APP_VERSION

if [ -z ${APP_VERSION} ]; then
    echo "ERROR: App_version env. variable is not set! Exit"
    exit 1
fi

# Create version.txt
echo "${APP_VERSION}" > version.txt
# Create source archive (tar.gz)
git archive --format=tar.gz --output=build_prod/WineGUI-Source-${APP_VERSION}.tar.gz --add-file=version.txt HEAD
