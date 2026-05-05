#!/usr/bin/bash
set -ex

# Detect architecture or use argument
ARCH=${1:-$(uname -m)}

case "$ARCH" in
    x86_64|amd64)
        CONTAINERFILE="Containerfile.x86_64"
        CONFIG="x86_64-autosd"
        ;;
    aarch64|arm64)
        CONTAINERFILE="Containerfile.aarch64"
        CONFIG="aarch64-autosd"
        ;;
    *)
        echo "Unsupported architecture: $ARCH"
        echo "Usage: $0 [x86_64|aarch64]"
        exit 1
        ;;
esac

echo "Building for architecture: $ARCH using $CONTAINERFILE"

podman build -f "$CONTAINERFILE" -t bazel:8.3.1 .

podman run -ti --rm -v $(pwd):/app:z localhost/bazel:8.3.1 bazel build --config ${CONFIG} //score/...
