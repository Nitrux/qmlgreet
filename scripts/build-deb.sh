#!/usr/bin/env bash

# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2024-2025 <Nitrux Latinoamericana S.C. <hello@nxos.org>>


# -- Exit on errors.

set -euo pipefail


# -- Prepare source.

SRC_DIR="$(pwd)"

BUILD_WORK_DIR="$(mktemp -d)"

cp -r "$SRC_DIR"/* "$BUILD_WORK_DIR/"

cd "$BUILD_WORK_DIR"


# -- Configure build.

# MauiKit software MUST be built with CMake and ECM; NEVER use Meson.
DEB_HOST_MULTIARCH="$(dpkg-architecture -qDEB_HOST_MULTIARCH)"
CMAKE_ARGS=(
    -DCMAKE_INSTALL_PREFIX=/usr
    -DKDE_INSTALL_LIBDIR="lib/$DEB_HOST_MULTIARCH"
    -DCMAKE_BUILD_TYPE=Release
)

if [ "$(dpkg-architecture -qDEB_HOST_ARCH)" = "amd64" ]; then
    CMAKE_ARGS+=(-DCMAKE_CXX_FLAGS=-march=x86-64-v3)
fi

cmake -S . -B .build "${CMAKE_ARGS[@]}"


# -- Compile source.

cmake --build .build --parallel "$(nproc)"


# -- Create a temporary DESTDIR for packaging.

DESTDIR="$(pwd)/pkg"
rm -rf "$DESTDIR"


# -- Install binary to DESTDIR.

DESTDIR="$DESTDIR" cmake --install .build


# -- Install configuration file.

install -Dm644 "qmlgreet.conf" "$DESTDIR/etc/qmlgreet/qmlgreet.conf"


# -- Create DEBIAN control file.

mkdir -p "$DESTDIR/DEBIAN"

PKGNAME="greetd-qmlgreet"
VERSION="${PACKAGE_VERSION:?PACKAGE_VERSION must be supplied by CI}"
MAINTAINER="uri_herrera@nxos.org"
ARCHITECTURE="$(dpkg --print-architecture)"
DESCRIPTION="QML-based greeter for greetd and wlr-based compositors."

cat > "$DESTDIR/DEBIAN/control" <<EOF
Package: $PKGNAME
Version: $VERSION
Section: utils
Priority: optional
Architecture: $ARCHITECTURE
Maintainer: $MAINTAINER
Description: $DESCRIPTION
Depends: libkf6i18n6,libqt6quick6,libqt6quickcontrols2-6,greetd, libqt6core5compat6, libqt6core6t64, libqt6dbus6, libqt6gui6, libqt6opengl6, libqt6openglwidgets6, libqt6qml6, libqt6waylandclient6, qml6-module-qtquick, qml6-module-qtquick-controls, qml6-module-qtquick-layouts, libwayland-client0, libwayland-cursor0, libwayland-egl1, libwayland-server0, mauikit (>= 4.0.5), qml6-module-qt5compat-graphicaleffects, qt6-wayland, wayland-protocols, wayland-scanner++
EOF


echo "/etc/qmlgreet/qmlgreet.conf" > "$DESTDIR/DEBIAN/conffiles"

cd "$(dirname "$DESTDIR")"

dpkg-deb --build "$(basename "$DESTDIR")" "${PKGNAME}_${VERSION}_${ARCHITECTURE}.deb"


# -- Move .deb to ./build/ for CI consistency.

TARGET_DIR="${GITHUB_WORKSPACE:-$SRC_DIR}/build"

mkdir -p "$TARGET_DIR"

mv "${PKGNAME}_${VERSION}_${ARCHITECTURE}.deb" "$TARGET_DIR/"

echo "Debian package created: $(pwd)/build/${PKGNAME}_${VERSION}_${ARCHITECTURE}.deb"
