#!/usr/bin/env bash

# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2024-2025 <Nitrux Latinoamericana S.C. <hello@nxos.org>>


# -- Exit on errors.

set -e


# -- Download Source.

SRC_DIR="$(mktemp -d)"

git clone --depth 1 --branch "$QMLGREET_BRANCH" https://gitlab.com/Nitrux/qmlgreet.git "$SRC_DIR/qmlgreet-src"

cd "$SRC_DIR/qmlgreet-src"


# -- Configure Build.

meson setup .build --prefix=/usr --buildtype=release -Dcpp_args='-march=x86-64-v3'


# -- Compile Source.

ninja -C .build -k 0 -j "$(nproc)"


# -- Create a temporary DESTDIR.

DESTDIR="$(pwd)/pkg"

rm -rf "$DESTDIR"


# -- Install to DESTDIR.

DESTDIR="$DESTDIR" ninja -C .build install


# -- Create DEBIAN control file.

mkdir -p "$DESTDIR/DEBIAN"

PKGNAME="greetd-qmlgreet"
MAINTAINER="uri_herrera@nxos.org"
ARCHITECTURE="$(dpkg --print-architecture)"
DESCRIPTION="QML-based greeter for greetd and wlr-based compositors."

cat > "$DESTDIR/DEBIAN/control" <<EOF
Package: $PKGNAME
Version: $PACKAGE_VERSION
Section: utils
Priority: optional
Architecture: $ARCHITECTURE
Maintainer: $MAINTAINER
Description: $DESCRIPTION
Depends: greetd, libqt6core5compat6, libqt6core6t64, libqt6dbus6, libqt6gui6, libqt6opengl6, libqt6openglwidgets6, libqt6qml6, libqt6waylandclient6, libwayland-client0, libwayland-cursor0, libwayland-egl1, libwayland-server0, mauikit, qml6-module-qt5compat-graphicaleffects, qt6-wayland, wayland-protocols, wayland-scanner++
EOF


# -- Build the Debian package.

cd "$(dirname "$DESTDIR")"

dpkg-deb --build "$(basename "$DESTDIR")" "${PKGNAME}_${PACKAGE_VERSION}_${ARCHITECTURE}.deb"


# -- Move .deb to ./build/ for CI consistency.

mkdir -p "$GITHUB_WORKSPACE/build"

mv "${PKGNAME}_${PACKAGE_VERSION}_${ARCHITECTURE}.deb" "$GITHUB_WORKSPACE/build/"

echo "Debian package created: $(pwd)/build/${PKGNAME}_${PACKAGE_VERSION}_${ARCHITECTURE}.deb"
