#!/usr/bin/env bash
# By: Melroy van den Berg
# Description: Create new links in the Release page of Gitlab
# Depends on two env. variables: $PRIVATE_TOKEN & $APP_VERSION

# Gitlab WineGUI project ID
project_id=66

# Gitlab URL
gitlab_url="https://gitlab.melroy.org"

# Website location where you can find the WineGUI binaries
webpage_prefix="https://winegui.melroy.org/downloads"

if [ -z ${PRIVATE_TOKEN} ]; then
    echo "ERROR: Private_token env. variable is not set! Exit"
    exit 1
fi

if [ -z ${APP_VERSION} ]; then
    echo "ERROR: App_version env. variable is not set! Exit"
    exit 1
fi

output=$(curl -s "$gitlab_url/api/v4/projects/$project_id/releases/$APP_VERSION/assets/links?private_token=$PRIVATE_TOKEN")
if [[ "$output" == "" ]]; then
    echo "ERROR: Retrieving links from API returns an empty request! Something is wrong."
    exit 1
fi

if [[ "$output" == "[]" ]]; then
    # Upload new links if the request returns an empty list of links
    echo "INFO: Creating new release links for WineGUI $APP_VERSION!"

    curl --request POST \
        --header "PRIVATE-TOKEN: $PRIVATE_TOKEN" \
        --data name="WineGUI Compressed binary (tar)" \
        --data url="$webpage_prefix/WineGUI-$APP_VERSION.tar.gz" \
        "$gitlab_url/api/v4/projects/$project_id/releases/$APP_VERSION/assets/links"

    curl --request POST \
        --header "PRIVATE-TOKEN: $PRIVATE_TOKEN" \
        --data name="WineGUI RPM Package (rpm)" \
        --data url="$webpage_prefix/WineGUI-$APP_VERSION.rpm" \
        "$gitlab_url/api/v4/projects/$project_id/releases/$APP_VERSION/assets/links"

    curl --request POST \
        --header "PRIVATE-TOKEN: $PRIVATE_TOKEN" \
        --data name="WineGUI Debian package (deb)" \
        --data url="$webpage_prefix/WineGUI-$APP_VERSION.deb" \
        "$gitlab_url/api/v4/projects/$project_id/releases/$APP_VERSION/assets/links"
elif [[ "$output" == "{\"message\":\"404 Not found\"}" ]]; then
    echo "WARN: Release doesn't yet exist yet/can't be found yet in Gitlab: $APP_VERSION..."
else
    echo "INFO: Links already exists. Skipping creating new links for WineGUI $APP_VERSION!"
fi