#!/usr/bin/env bash
set -euo pipefail

: "${APP_VERSION:?APP_VERSION is required}"
: "${CI_JOB_TOKEN:?CI_JOB_TOKEN is required}"
: "${CI_API_V4_URL:?CI_API_V4_URL is required}"
: "${CI_PROJECT_ID:?CI_PROJECT_ID is required}"
URL_PREFIX_LOCATION="https://winegui.melroy.org/downloads"
endpoint="${CI_API_V4_URL}/projects/${CI_PROJECT_ID}/releases/${APP_VERSION}/assets/links"
curl_args=(--fail-with-body --silent --show-error --header "JOB-TOKEN: $CI_JOB_TOKEN")
links='[]'
page=1
while true; do
    batch=$(curl "${curl_args[@]}" "$endpoint?per_page=100&page=$page")
    jq -e 'type == "array" and all(.[]; (.name | type == "string") and (.url | type == "string") and (.link_type | type == "string"))' <<< "$batch" >/dev/null
    links=$(jq -cn --argjson old "$links" --argjson batch "$batch" '$old + $batch')
    [[ $(jq 'length' <<< "$batch") -lt 100 ]] && break
    page=$((page + 1))
done

add_link() {
    local type=$1 name=$2 url=$3 matches response
    matches=$(jq -c --arg name "$name" --arg url "$url" '[.[] | select(.name == $name or .url == $url)]' <<< "$links")
    if [[ "$matches" != '[]' ]]; then
        if jq -e --arg name "$name" --arg url "$url" --arg type "$type" 'length == 1 and .[0].name == $name and .[0].url == $url and .[0].link_type == $type' <<< "$matches" >/dev/null; then
            echo "INFO: Release link already exists: $name"
            return
        fi
        echo "ERROR: Conflicting release link: $name" >&2
        return 1
    fi
    response=$(curl "${curl_args[@]}" --request POST --data-urlencode "link_type=$type" --data-urlencode "name=$name" --data-urlencode "url=$url" "$endpoint")
    jq -e --arg name "$name" --arg url "$url" --arg type "$type" 'type == "object" and .name == $name and .url == $url and .link_type == $type' <<< "$response" >/dev/null
    echo "INFO: Created release link: $name"
}

# Create in reverse display order, preserving the existing release layout.
add_link "other" "WineGUI - Source Code Archive (.tar.gz)" "${URL_PREFIX_LOCATION}/WineGUI-Source-$APP_VERSION.tar.gz"
add_link "package" "WineGUI - Compressed Binary (.tar.gz)" "${URL_PREFIX_LOCATION}/WineGUI-$APP_VERSION.tar.gz"
add_link "package" "WineGUI - Fedora/openSUSE/CentOS (.rpm)" "${URL_PREFIX_LOCATION}/WineGUI-$APP_VERSION.rpm"
add_link "package" "WineGUI - Debian 13 Trixie/MX Linux 25 (.deb)" "${URL_PREFIX_LOCATION}/WineGUI-$APP_VERSION-trixie.deb"
add_link "package" "WineGUI - Debian 14 Forky/Kali Linux (.deb)" "${URL_PREFIX_LOCATION}/WineGUI-$APP_VERSION-forky.deb"
add_link "package" "WineGUI - Ubuntu 24.04/Mint 22/Zorin 18/elementary 8/Pop!_OS 24.04 (.deb)" "${URL_PREFIX_LOCATION}/WineGUI-$APP_VERSION-noble.deb"
add_link "package" "WineGUI - Ubuntu 25.04 (Plucky Puffin)/AnduinOS 1.3 (.deb)" "${URL_PREFIX_LOCATION}/WineGUI-$APP_VERSION-plucky.deb"
add_link "package" "WineGUI - Ubuntu 26.04 (Resolute Raccoon)/AnduinOS 2 (.deb)" "${URL_PREFIX_LOCATION}/WineGUI-$APP_VERSION-resolute.deb"
add_link "package" "WineGUI - AppImage (.AppImage)" "${URL_PREFIX_LOCATION}/WineGUI-$APP_VERSION-x86_64.AppImage"
