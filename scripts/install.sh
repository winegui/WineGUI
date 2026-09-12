#!/usr/bin/env bash

set -Eeuo pipefail

readonly WINEGUI_APT_URI="https://apt.winegui.melroy.org"
readonly WINEGUI_KEY_FINGERPRINTS="DC1522C22AC2908D6AF1D0A54AD49EDC219C57F7"
readonly WINEGUI_KEY_SHA256="e074b9409ad5aa58ce76846179c7857698bacbe6ae384cacd95b6570c3a2dcf3"
readonly WINEGUI_KEY_BASE64='mDMEaqWAexYJKwYBBAHaRw8BAQdAf7pkqhvo4tKzV/BtV/j4OsxMNffYPHBZogqM/+vBIDK0KldpbmVHVUkgQVBUIFJlcG9zaXRvcnkgPG1lbHJveUBtZWxyb3kub3JnPoiZBBMWCgBBFiEE3BUiwirCkI1q8dClStSe3CGcV/cFAmqlgHsCGwEFCQlmAYAFCwkIBwICIgIGFQoJCAsCBBYCAwECHgcCF4AACgkQStSe3CGcV/e5PwEA56Q0Kaee9GkJN6xivaO7Z6zBgUhl+qJ8/Umx9KNWB6AA/iIf0hsokm2wyZJSywF+cvBpu3r4FREhZvIWadH0NZgAuDMEaqWA0hYJKwYBBAHaRw8BAQdAjPlJiYzWVMpcPq3HiAmrBr8TcF9hcg//myICuRxSx9eI9QQYFgoAJhYhBNwVIsIqwpCNavHQpUrUntwhnFf3BQJqpYDSAhsCBQkDwmcAAIEJEErUntwhnFf3diAEGRYKAB0WIQRCCooUS3VamIvNv2FrCTtXO9rO8QUCaqWA0gAKCRBrCTtXO9rO8UstAQDmhZFo0gdHbrAfKKuqBWvaifYeNcYyNXTzfu7NbRDJGwD9Fdi4yYtbme/cdImLOjeskJVvwGJk/RHAs0VdP4esagABLwEAsaLKkxLQM5B5Nbh9SQHi4+QEDs/eqq7ECK9PjJgC0cEA/iOJwb+f1EWqOxrRI5BPwqv1uPi0HMMW36qSE2xcAukJ'

die() {
    echo "WineGUI installer: $*" >&2
    exit 1
}

os_release_value() {
    local file=$1
    local wanted=$2

    [[ -r "$file" ]] || return 1
    awk -v wanted="$wanted" '
        index($0, wanted "=") == 1 {
            value = substr($0, length(wanted) + 2)
            first = substr(value, 1, 1)
            last = substr(value, length(value), 1)
            if (length(value) >= 2 && ((first == "\"" && last == "\"") || (first == "\047" && last == "\047")))
                value = substr(value, 2, length(value) - 2)
            found = value
        }
        END { print found }
    ' "$file"
}

id_like_contains() {
    local wanted=$1
    local id_like=$2

    awk -v wanted="$wanted" '
        {
            for (i = 1; i <= NF; i++)
                if ($i == wanted) found = 1
        }
        END { exit(found ? 0 : 1) }
    ' <<<"$id_like"
}

detect_suite() {
    local file=$1
    local os_id id_like version_codename ubuntu_codename

    os_id=$(os_release_value "$file" ID)
    id_like=$(os_release_value "$file" ID_LIKE)
    version_codename=$(os_release_value "$file" VERSION_CODENAME)
    ubuntu_codename=$(os_release_value "$file" UBUNTU_CODENAME)

    if [[ -n "$ubuntu_codename" ]]; then
        case "$ubuntu_codename" in
            noble|plucky|resolute) echo "$ubuntu_codename"; return 0 ;;
            *) return 1 ;;
        esac
    fi

    if [[ "$os_id" == ubuntu ]] || id_like_contains ubuntu "$id_like"; then
        case "$version_codename" in
            noble|plucky|resolute) echo "$version_codename"; return 0 ;;
            *) return 1 ;;
        esac
    fi

    if [[ "$os_id" == debian ]] || id_like_contains debian "$id_like"; then
        case "$version_codename" in
            trixie|forky) echo "$version_codename"; return 0 ;;
            *) return 1 ;;
        esac
    fi

    return 1
}

if [[ ${1:-} == "--print-suite" ]]; then
    [[ $# -le 2 ]] || die "usage: install.sh --print-suite [os-release-file]"
    detect_suite "${2:-/etc/os-release}" || die "this distribution does not report a supported Ubuntu or Debian suite"
    exit 0
fi

[[ $# -eq 0 ]] || die "this installer does not accept arguments"
[[ $EUID -eq 0 ]] || die "run this script as root, for example with: curl -fsSL https://raw.githubusercontent.com/winegui/WineGUI/main/scripts/install.sh | sudo bash"

for command in apt-cache apt-get awk base64 cat chmod dpkg dpkg-query grep mv rm sha256sum; do
    command -v "$command" >/dev/null 2>&1 || die "required command not found: $command"
done

architecture=$(dpkg --print-architecture)
[[ "$architecture" == amd64 ]] || die "unsupported architecture '$architecture'; the WineGUI APT repository currently publishes amd64 packages"

suite=$(detect_suite /etc/os-release) || {
    os_id=$(os_release_value /etc/os-release ID || true)
    version_codename=$(os_release_value /etc/os-release VERSION_CODENAME || true)
    ubuntu_codename=$(os_release_value /etc/os-release UBUNTU_CODENAME || true)
    die "unsupported distribution (ID=${os_id:-unknown}, VERSION_CODENAME=${version_codename:-unknown}, UBUNTU_CODENAME=${ubuntu_codename:-none})"
}

bootstrap_key="/usr/share/keyrings/winegui-bootstrap-keyring.gpg"
bootstrap_key_temporary="${bootstrap_key}.tmp.$$"
bootstrap_source="/etc/apt/sources.list.d/winegui-bootstrap.sources"
bootstrap_source_temporary="${bootstrap_source}.tmp.$$"
bootstrap_key_created=0
bootstrap_source_created=0

cleanup() {
    rm -f -- "$bootstrap_source_temporary" "$bootstrap_key_temporary"
    if [[ $bootstrap_source_created -eq 1 ]]; then
        rm -f -- "$bootstrap_source"
    fi
    if [[ $bootstrap_key_created -eq 1 ]]; then
        rm -f -- "$bootstrap_key"
    fi
}
trap cleanup EXIT
trap 'exit 129' HUP
trap 'exit 130' INT
trap 'exit 143' TERM

repository_configured=0
source_locations=()
[[ -f /etc/apt/sources.list ]] && source_locations+=(/etc/apt/sources.list)
[[ -d /etc/apt/sources.list.d ]] && source_locations+=(/etc/apt/sources.list.d)
if [[ ${#source_locations[@]} -gt 0 ]] && grep -Riqs --include='*.list' --include='*.sources' \
    'apt\.winegui\.melroy\.org' "${source_locations[@]}"; then
    repository_configured=1
fi

if [[ $repository_configured -eq 0 ]]; then
    [[ ! -e "$bootstrap_source" && ! -L "$bootstrap_source" ]] || die "$bootstrap_source already exists; remove or inspect it before retrying"
    [[ ! -e "$bootstrap_key" && ! -L "$bootstrap_key" ]] || die "$bootstrap_key already exists; remove or inspect it before retrying"

    printf '%s' "$WINEGUI_KEY_BASE64" | base64 --decode >"$bootstrap_key_temporary"
    chmod 0644 "$bootstrap_key_temporary"
    actual_hash=$(sha256sum "$bootstrap_key_temporary" | awk '{print $1}')
    [[ "$actual_hash" == "$WINEGUI_KEY_SHA256" ]] || die "the embedded repository key failed its integrity check"
    mv -f -- "$bootstrap_key_temporary" "$bootstrap_key"
    bootstrap_key_created=1

    cat >"$bootstrap_source_temporary" <<EOF
Types: deb
URIs: $WINEGUI_APT_URI
Suites: $suite
Components: main
Architectures: amd64
Signed-By: $bootstrap_key
EOF
    chmod 0644 "$bootstrap_source_temporary"
    mv -f -- "$bootstrap_source_temporary" "$bootstrap_source"
    bootstrap_source_created=1
    echo "WineGUI installer: bootstrapping the $suite repository"
else
    echo "WineGUI installer: using the existing WineGUI APT source"
fi

export DEBIAN_FRONTEND=noninteractive
apt-get update

if dpkg-query -W -f='${Status}' winegui 2>/dev/null | grep -qx 'install ok installed'; then
    apt-get install --yes --reinstall winegui
else
    apt-get install --yes winegui
fi

if [[ $bootstrap_source_created -eq 1 ]]; then
    managed_source="/etc/apt/sources.list.d/winegui.sources"
    managed_key="/usr/share/keyrings/winegui-archive-keyring.gpg"
    if [[ ! -f "$managed_source" || -L "$managed_source" ]]; then
        die "WineGUI was installed, but its permanent APT source was not created"
    fi
    if [[ ! -f "$managed_key" || -L "$managed_key" || ! -r "$managed_key" ]]; then
        die "WineGUI was installed, but its permanent APT keyring was not created"
    fi
    if ! grep -Fqx "URIs: $WINEGUI_APT_URI" "$managed_source"; then
        die "WineGUI installed an unexpected permanent APT source"
    fi
    if ! grep -Fqx "Suites: $suite" "$managed_source"; then
        die "WineGUI installed an unexpected repository suite"
    fi
    if ! grep -Fqx "Signed-By: $managed_key" "$managed_source"; then
        die "WineGUI installed an unexpected repository keyring reference"
    fi

    rm -f -- "$bootstrap_source"
    bootstrap_source_created=0
    rm -f -- "$bootstrap_key"
    bootstrap_key_created=0
    apt-get update
fi

apt-cache policy winegui
echo "WineGUI installer: installation complete"
