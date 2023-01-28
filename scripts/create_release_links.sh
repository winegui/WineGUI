#!/usr/bin/env bash
# By: Melroy van den Berg
# Description: Create new links in the Release page of Gitlab
# Depends on one environment variable: $APP_VERSION

# Location where you can find the WineGUI binaries
URL_PREFIX_LOCATION="https://winegui.melroy.org/downloads"

if [ -z ${APP_VERSION} ]; then
    echo "ERROR: App_version env. variable is not set! Exit"
    exit 1
fi

output=$(curl -s --header "JOB-TOKEN: $CI_JOB_TOKEN" "https://${CI_SERVER_HOST}/api/v4/projects/${CI_PROJECT_ID}/releases/$APP_VERSION/assets/links")
if [[ "$output" == "" ]]; then
    echo "ERROR: Retrieving links from API returns an empty request! Something is wrong."
    exit 1
fi

if [[ "$output" == "[]" ]]; then
    # Upload new links if the request returns an empty list of links
    echo "INFO: Creating new release links for WineGUI $APP_VERSION!"

    curl --request POST \
        --header "JOB-TOKEN: $CI_JOB_TOKEN" \
        --data name="WineGUI Compressed binary (tar)" \
        --data url="${URL_PREFIX_LOCATION}/WineGUI-$APP_VERSION.tar.gz" \
        "https://${CI_SERVER_HOST}/api/v4/projects/${CI_PROJECT_ID}/releases/$APP_VERSION/assets/links"

    curl --request POST \
        --header "JOB-TOKEN: $CI_JOB_TOKEN" \
        --data name="WineGUI RPM Package (rpm)" \
        --data url="${URL_PREFIX_LOCATION}/WineGUI-$APP_VERSION.rpm" \
        "https://${CI_SERVER_HOST}/api/v4/projects/${CI_PROJECT_ID}/releases/$APP_VERSION/assets/links"

    curl --request POST \
        --header "JOB-TOKEN: $CI_JOB_TOKEN" \
        --data name="WineGUI Debian package (deb)" \
        --data url="${URL_PREFIX_LOCATION}/WineGUI-$APP_VERSION.deb" \
        "https://${CI_SERVER_HOST}/api/v4/projects/${CI_PROJECT_ID}/releases/$APP_VERSION/assets/links"
elif [[ "$output" == "{\"message\":\"404 Not found\"}" ]]; then
    echo "WARN: Release doesn't yet exist yet/can't be found yet in Gitlab: $APP_VERSION..."
else
    echo "INFO: Links already exists. Skipping creating new links for WineGUI $APP_VERSION!"
fi
