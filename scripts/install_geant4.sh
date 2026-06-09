#!/usr/bin/env bash
# Build GEANT4 from source on macOS (Apple Silicon) using the Homebrew toolchain.
# Homebrew dropped the geant4 formula, so we build the latest stable release ourselves.
set -euo pipefail

VERSION="${GEANT4_VERSION:-v11.4.1}"
SRC_ROOT="${GEANT4_SRC_ROOT:-/tmp/geant4_src}"
INSTALL_PREFIX="${GEANT4_PREFIX:-$HOME/geant4/$VERSION}"
NPROC="$(sysctl -n hw.ncpu)"

echo "==> GEANT4 $VERSION  |  install -> $INSTALL_PREFIX  |  jobs=$NPROC"

echo "==> Installing build dependencies via Homebrew (idempotent)"
brew install cmake xerces-c qt expat >/dev/null 2>&1 || brew install cmake xerces-c qt expat

QT_PREFIX="$(brew --prefix qt)"

mkdir -p "$SRC_ROOT"
cd "$SRC_ROOT"

TARBALL="geant4-$VERSION.tar.gz"
if [[ ! -f "$TARBALL" ]]; then
  echo "==> Downloading GEANT4 source"
  curl -L -o "$TARBALL" "https://github.com/Geant4/geant4/archive/refs/tags/$VERSION.tar.gz"
fi

SRC_DIR="$SRC_ROOT/geant4-${VERSION#v}"
if [[ ! -d "$SRC_DIR" ]]; then
  echo "==> Extracting"
  tar xzf "$TARBALL"
fi

BUILD_DIR="$SRC_ROOT/build-$VERSION"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

echo "==> Configuring with CMake"
cmake "$SRC_DIR" \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX="$INSTALL_PREFIX" \
  -DCMAKE_PREFIX_PATH="$QT_PREFIX" \
  -DGEANT4_INSTALL_DATA=ON \
  -DGEANT4_BUILD_MULTITHREADED=ON \
  -DGEANT4_USE_QT=ON \
  -DGEANT4_USE_OPENGL_X11=OFF \
  -DGEANT4_USE_GDML=ON \
  -DGEANT4_USE_SYSTEM_EXPAT=OFF \
  -DGEANT4_BUILD_TLS_MODEL=global-dynamic

echo "==> Building (this is the long step)"
cmake --build . --parallel "$NPROC"

echo "==> Installing"
cmake --install .

echo "==> DONE. Source the environment with:"
echo "    source $INSTALL_PREFIX/bin/geant4.sh"
