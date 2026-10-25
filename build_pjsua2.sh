#!/usr/bin/env bash

# EventGamer - telephone controls for Snake in MadMapper.
# Copyright (C) 2026 Stefan Schuermans
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program. If not, see <https://www.gnu.org/licenses/>.
#
# SPDX-License-Identifier: GPL-3.0-or-later

set -euo pipefail

# PJPROJECT contains the PJSUA2 C++ API. Everything stays inside this repository.
readonly pjproject_version="2.17"
repo_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
readonly repo_dir
readonly dependency_dir="$repo_dir/.deps/pjproject"
readonly source_dir="$dependency_dir/source-$pjproject_version"
readonly install_dir="$dependency_dir/install"

if [[ ${1:-} == --help ]]; then
    echo "Usage: ./build_pjsua2.sh"
    echo "Builds PJPROJECT $pjproject_version for Linux/macOS in .deps/pjproject."
    echo "Optional environment: JOBS, CC, CXX, CFLAGS, CXXFLAGS, LDFLAGS."
    exit 0
fi
if [[ $# -ne 0 ]]; then
    echo "Unexpected arguments; use --help." >&2
    exit 1
fi
case "$(uname -s)" in
    Linux|Darwin) ;;
    *) echo "This script supports Linux/macOS. Use a native PJPROJECT SDK on Windows." >&2; exit 1 ;;
esac
for tool in git make pkg-config; do
    if ! command -v "$tool" >/dev/null 2>&1; then
        echo "Missing required tool: $tool" >&2
        exit 1
    fi
done
jobs="${JOBS:-$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 2)}"
if [[ ! $jobs =~ ^[1-9][0-9]*$ ]]; then
    echo "JOBS must be a positive integer." >&2
    exit 1
fi

mkdir -p "$dependency_dir"
if [[ ! -e $source_dir ]]; then
    git clone --depth 1 --branch "$pjproject_version" \
        https://github.com/pjsip/pjproject.git "$source_dir"
fi
if [[ ! -d $source_dir/.git ]] ||
   [[ $(git -C "$source_dir" rev-parse HEAD) != $(git -C "$source_dir" rev-parse "$pjproject_version^{commit}") ]]; then
    echo "Expected an unmodified PJPROJECT $pjproject_version checkout at $source_dir." >&2
    exit 1
fi
if ! git -C "$source_dir" diff --quiet HEAD --; then
    echo "PJPROJECT has local changes at $source_dir; refusing to build." >&2
    exit 1
fi

cd "$source_dir"
# WAV playback into calls needs no physical audio device. Keep bundled codecs.
./configure --prefix="$install_dir" --disable-video --disable-sound
make dep
make -j "$jobs"
make install

if [[ ! -f $install_dir/include/pjsua2.hpp || ! -f $install_dir/lib/pkgconfig/libpjproject.pc ]]; then
    echo "PJPROJECT installation is missing PJSUA2 headers or pkg-config metadata." >&2
    exit 1
fi
echo "PJSUA2 installed in $install_dir"
echo "Build EventGamer with:"
echo "  cmake -S \"$repo_dir/EventGamer\" -B \"$repo_dir/EventGamer/build\" -G Ninja"
echo "  cmake --build \"$repo_dir/EventGamer/build\""
