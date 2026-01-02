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

output=$(curl -s --header "JOB-TOKEN: $CI_JOB_TOKEN" "${CI_API_V4_URL}/projects/${CI_PROJECT_ID}/releases/$APP_VERSION/assets/links")
if [[ "$output" == "" ]]; then
    echo "ERROR: Retrieving links from API returns an empty request! Something is wrong."
    exit 1
fi

if [[ "$output" == "[]" ]]; then
    # Upload new links if the request returns an empty list of links
    echo "INFO: Creating new release links for WineGUI $APP_VERSION!"

    # !! In the reverse order of how we want the links to be displayed !!
    # Meaning the first added, will be the displayed last.
    curl --request POST \
        --header "JOB-TOKEN: $CI_JOB_TOKEN" \
        --data link_type="package" \
        --data name="WineGUI - Source Code Archive (.tar.gz)" \
        --data url="${URL_PREFIX_LOCATION}/WineGUI-Source-$APP_VERSION.tar.gz" \
        "${CI_API_V4_URL}/projects/${CI_PROJECT_ID}/releases/$APP_VERSION/assets/links"

    curl --request POST \
        --header "JOB-TOKEN: $CI_JOB_TOKEN" \
        --data link_type="package" \
        --data name="WineGUI - Compressed Binary (.tar.gz)" \
        --data url="${URL_PREFIX_LOCATION}/WineGUI-$APP_VERSION.tar.gz" \
        "${CI_API_V4_URL}/projects/${CI_PROJECT_ID}/releases/$APP_VERSION/assets/links"

    curl --request POST \
        --header "JOB-TOKEN: $CI_JOB_TOKEN" \
        --data link_type="package" \
        --data name="WineGUI - Fedora/openSUSE/CentOS (.rpm)" \
        --data url="${URL_PREFIX_LOCATION}/WineGUI-$APP_VERSION.rpm" \
        "${CI_API_V4_URL}/projects/${CI_PROJECT_ID}/releases/$APP_VERSION/assets/links"

    # Deb package links
    curl --request POST \
        --header "JOB-TOKEN: $CI_JOB_TOKEN" \
        --data link_type="package" \
        --data name="WineGUI - Debian 12 Bookworm/MX Linux 23 (.deb)" \
        --data url="${URL_PREFIX_LOCATION}/WineGUI-$APP_VERSION-bookworm.deb" \
        "${CI_API_V4_URL}/projects/${CI_PROJECT_ID}/releases/$APP_VERSION/assets/links"

    curl --request POST \
        --header "JOB-TOKEN: $CI_JOB_TOKEN" \
        --data link_type="package" \
        --data name="WineGUI - Debian 13 Trixie/MX Linux 25 (.deb)" \
        --data url="${URL_PREFIX_LOCATION}/WineGUI-$APP_VERSION-trixie.deb" \
        "${CI_API_V4_URL}/projects/${CI_PROJECT_ID}/releases/$APP_VERSION/assets/links"

    curl --request POST \
        --header "JOB-TOKEN: $CI_JOB_TOKEN" \
        --data link_type="package" \
        --data name="WineGUI - Debian 14 Forky/Kali Linux (.deb)" \
        --data url="${URL_PREFIX_LOCATION}/WineGUI-$APP_VERSION-forky.deb" \
        "${CI_API_V4_URL}/projects/${CI_PROJECT_ID}/releases/$APP_VERSION/assets/links"

    curl --request POST \
        --header "JOB-TOKEN: $CI_JOB_TOKEN" \
        --data link_type="package" \
        --data name="WineGUI - Ubuntu 24.04 (Noble Numbat)/Linux Mint 22/Zorin OS 18/elementary OS 8 (.deb)" \
        --data url="${URL_PREFIX_LOCATION}/WineGUI-$APP_VERSION-noble.deb" \
        "${CI_API_V4_URL}/projects/${CI_PROJECT_ID}/releases/$APP_VERSION/assets/links"

    curl --request POST \
        --header "JOB-TOKEN: $CI_JOB_TOKEN" \
        --data link_type="package" \
        --data name="WineGUI - Ubuntu 25.04 (Plucky Puffin) (.deb)" \
        --data url="${URL_PREFIX_LOCATION}/WineGUI-$APP_VERSION-plucky.deb" \
        "${CI_API_V4_URL}/projects/${CI_PROJECT_ID}/releases/$APP_VERSION/assets/links"

elif [[ "$output" == "{\"message\":\"404 Not found\"}" ]]; then
    echo "WARN: Release doesn't yet exist yet/can't be found yet in Gitlab: $APP_VERSION..."
else
    echo "INFO: Links already exists. Skipping creating new links for WineGUI $APP_VERSION!"
fi
