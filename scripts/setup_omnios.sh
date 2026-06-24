#!/bin/bash
# setup_omnios.sh
# Sets up the OmniOS development environment for ytnova.

set -euo pipefail
PATH=/opt/local/sbin:/opt/local/bin:$PATH

echo ">>> Refreshing OmniOS package catalogs..."
sudo pkg refresh --full

echo ">>> Installing OmniOS build dependencies..."
sudo pkg install developer/omnios-build-tools ooce/library/libarchive library/ncurses library/readline

if ! command -v pandoc >/dev/null 2>&1; then
    echo ">>> Installing pandoc via pkgsrc/pkgin..."
    if ! command -v pkgin >/dev/null 2>&1; then
        # pkgsrc/pkgin are bootstrapped here because OmniOS does not ship pandoc.
        BOOTSTRAP_TAR="bootstrap-trunk-x86_64-20240116.tar.gz"
        BOOTSTRAP_SHA="4d92a333587d9dcc669ff64264451ca65da701b7"
        TMPDIR="$(mktemp -d)"

        (
            cd "$TMPDIR"
            curl -O "https://pkgsrc.smartos.org/packages/SmartOS/bootstrap/${BOOTSTRAP_TAR}"
            [ "${BOOTSTRAP_SHA}" = "$(/bin/digest -a sha1 "${BOOTSTRAP_TAR}")" ] || {
                echo "ERROR: checksum failure for ${BOOTSTRAP_TAR}" >&2
                exit 1
            }
            sudo tar -zxpf "${BOOTSTRAP_TAR}" -C /
        )

        PATH=/opt/local/sbin:/opt/local/bin:$PATH
        sudo /opt/local/sbin/pkg_add -U pkg_install pkgin libarchive
        sudo /opt/local/bin/pkgin -y update
        echo ">>> pkgsrc/pkgin bootstrapped to install hs-pandoc."
    fi

    sudo /opt/local/bin/pkgin -y install hs-pandoc
fi

echo
echo "Setup complete."
echo "You may now build ytnova with:"
echo "  cd ~/ytreenova"
echo "  gmake"
