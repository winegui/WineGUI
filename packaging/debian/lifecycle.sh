
APT_ENABLED='@APT_ENABLED@'
PACKAGE_KEY_GENERATION='@KEY_GENERATION@'
PACKAGE_KEY_BASE64='@KEY_BASE64@'
PACKAGE_KEY_SHA256='@KEY_SHA256@'

root="${WINEGUI_MAINTAINER_ROOT:-}"
state_dir="${root}/var/lib/winegui"
source_file="${root}/etc/apt/sources.list.d/winegui.sources"
key_file="${root}/usr/share/keyrings/winegui-archive-keyring.gpg"
source_state="${state_dir}/repository.sources"
setup_marker="${state_dir}/repository.setup"
generation_file="${state_dir}/key.generation"
key_hash_file="${state_dir}/key.sha256"
os_release_file="${root}/etc/os-release"
umask 022

winegui_info() {
    echo "WineGUI APT: $*" >&2
}

winegui_mkdir() {
    mkdir -p "$1" || return 1
    chmod 0755 "$1" || return 1
}

winegui_atomic_file() {
    destination=$1
    mode=$2
    temporary="${destination}.tmp.$$"
    winegui_mkdir "$(dirname "$destination")" || return 1
    umask 022
    cat >"$temporary" || return 1
    chmod "$mode" "$temporary" || return 1
    mv -f "$temporary" "$destination" || return 1
}

winegui_read_generation() {
    if [ ! -e "$generation_file" ]; then
        echo 0
        return
    fi
    if [ ! -f "$generation_file" ] || [ -L "$generation_file" ]; then
        winegui_info "invalid installed key generation marker; preserving existing trust files"
        echo invalid
        return
    fi
    generation=$(sed -n '1p' "$generation_file") || {
        echo invalid
        return
    }
    case "$generation" in
        ''|*[!0-9]*)
            winegui_info "invalid installed key generation marker; preserving existing trust files"
            echo invalid
            ;;
        *) echo "$generation" ;;
    esac
}

winegui_install_package_key() {
    embedded_key="${state_dir}/key.package.$$"
    trap 'rm -f "$embedded_key"' EXIT HUP INT TERM
    winegui_mkdir "$state_dir" || return 1
    printf '%s' "$PACKAGE_KEY_BASE64" | base64 -d >"$embedded_key" || return 1
    chmod 0644 "$embedded_key" || return 1
    actual_hash=$(sha256sum "$embedded_key" | awk '{print $1}') || return 1
    if [ "$actual_hash" != "$PACKAGE_KEY_SHA256" ]; then
        winegui_info "embedded repository key failed its integrity check"
        return 1
    fi

    installed_generation=$(winegui_read_generation)
    [ "$installed_generation" != invalid ] || return 1
    if [ "$installed_generation" -gt 0 ]; then
        if [ ! -f "$key_hash_file" ] || ! grep -Eq '^[0-9a-f]{64}$' "$key_hash_file"; then
            winegui_info "installed key generation has no valid integrity marker"
            return 1
        fi
        if [ -e "$key_file" ]; then
            if [ ! -f "$key_file" ] || [ -L "$key_file" ] || [ "$(sha256sum "$key_file" | awk '{print $1}')" != "$(sed -n '1p' "$key_hash_file")" ]; then
                winegui_info "the managed keyring was modified; refusing to overwrite it"
                return 1
            fi
        fi
    fi
    if [ "$installed_generation" -gt "$PACKAGE_KEY_GENERATION" ]; then
        if [ ! -f "$key_file" ]; then
            winegui_info "a newer key generation is recorded but its keyring is absent; repository setup is skipped"
            return 1
        fi
        winegui_info "preserving newer repository key generation ${installed_generation}"
        return 0
    fi

    if [ "$installed_generation" -eq "$PACKAGE_KEY_GENERATION" ] && [ "$installed_generation" -ne 0 ]; then
        if [ ! -f "$key_hash_file" ] || [ "$(sed -n '1p' "$key_hash_file")" != "$PACKAGE_KEY_SHA256" ]; then
            winegui_info "key generation ${installed_generation} conflicts with its recorded fingerprint"
            return 1
        fi
    elif [ "$installed_generation" -eq 0 ] && [ -e "$key_file" ]; then
        winegui_info "preserving a pre-existing untracked keyring; repository setup is skipped"
        return 1
    fi

    winegui_atomic_file "$key_file" 0644 <"$embedded_key" || return 1
    printf '%s\n' "$PACKAGE_KEY_GENERATION" | winegui_atomic_file "$generation_file" 0644 || return 1
    printf '%s\n' "$PACKAGE_KEY_SHA256" | winegui_atomic_file "$key_hash_file" 0644 || return 1
    rm -f "$embedded_key" || return 1
    trap - EXIT HUP INT TERM
    return 0
}

winegui_suite() {
    [ -r "$os_release_file" ] || return 1
    suite=$(sed -n 's/^VERSION_CODENAME=["'\'']\{0,1\}\([^"'\'']*\)["'\'']\{0,1\}$/\1/p' "$os_release_file" | head -n 1)
    case "$suite" in
        noble|plucky|resolute|trixie|forky) echo "$suite" ;;
        *) return 1 ;;
    esac
}

winegui_source_content() {
    cat <<EOF
Types: deb
URIs: https://apt.winegui.melroy.org
Suites: $1
Components: main
Architectures: amd64
Signed-By: /usr/share/keyrings/winegui-archive-keyring.gpg
EOF
}

winegui_configure_repository() {
    [ "$APT_ENABLED" = 1 ] || return 0
    if ! winegui_install_package_key; then
        winegui_info "repository source configuration was skipped to preserve trust state"
        return 0
    fi
    if ! suite=$(winegui_suite); then
        winegui_info "this OS codename is unsupported; no APT source was created"
        return 0
    fi

    desired="${state_dir}/repository.desired.$$"
    winegui_mkdir "$state_dir"
    winegui_source_content "$suite" >"$desired"
    chmod 0644 "$desired"

    if [ ! -e "$source_file" ]; then
        if [ -e "$setup_marker" ]; then
            winegui_info "repository source remains absent (administrator opt-out)"
        else
            winegui_atomic_file "$source_file" 0644 <"$desired"
        fi
    elif [ -f "$source_file" ] && [ ! -L "$source_file" ] && [ -f "$source_state" ] && cmp -s "$source_file" "$source_state"; then
        winegui_atomic_file "$source_file" 0644 <"$desired"
    elif [ ! -L "$source_file" ] && [ ! -e "$setup_marker" ] && cmp -s "$source_file" "$desired"; then
        chmod 0644 "$source_file"
    else
        winegui_info "preserving a pre-existing or administrator-edited source"
    fi

    winegui_atomic_file "$source_state" 0644 <"$desired"
    printf '1\n' | winegui_atomic_file "$setup_marker" 0644
    rm -f "$desired"
}

winegui_source_references_key() {
    [ -f "$source_file" ] || return 1
    awk '
        tolower($1) == "signed-by:" {
            for (i = 2; i <= NF; i++)
                if ($i == "/usr/share/keyrings/winegui-archive-keyring.gpg") found = 1
        }
        END { exit(found ? 0 : 1) }
    ' "$source_file"
}

winegui_purge_repository() {
    [ "$APT_ENABLED" = 1 ] || return 0
    installed_generation=$(winegui_read_generation)
    if [ "$installed_generation" = invalid ] || [ "$installed_generation" -gt "$PACKAGE_KEY_GENERATION" ]; then
        winegui_info "preserving repository state owned by a newer key generation"
        return 0
    fi

    preserve_source=0
    if [ -e "$source_file" ]; then
        if [ -f "$source_file" ] && [ ! -L "$source_file" ] && [ -f "$source_state" ] && cmp -s "$source_file" "$source_state"; then
            rm -f "$source_file"
        else
            preserve_source=1
            winegui_info "preserving a pre-existing or administrator-edited source during purge"
        fi
    fi

    if [ "$preserve_source" -eq 1 ] && winegui_source_references_key; then
        rm -f "$source_state" "$setup_marker"
        winegui_info "preserving the keyring and generation marker referenced by the retained source"
        return 0
    fi

    rm -f "$key_file" "$source_state" "$setup_marker" "$generation_file" "$key_hash_file"
    rmdir "$state_dir" 2>/dev/null || true
}
