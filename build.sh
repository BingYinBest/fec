#!/usr/bin/env bash
# Build a static 'fec' binary for Android (or the local host for testing).
#
# Usage: ./build.sh [arch]
#   arch: aarch64 (default) | armv7a | x86_64 | native
#
# For cross builds, NDK must point to an Android NDK root (e.g. /opt/android-ndk-r26d).
# Output: out/<arch>/fec
set -euo pipefail

ARCH="${1:-aarch64}"
ROOT="$(cd "$(dirname "$0")" && pwd)"
OUT="$ROOT/out/$ARCH"
OBJ="$OUT/obj"
mkdir -p "$OBJ"

COMMON_FLAGS=(-O2 -g -D_LARGEFILE64_SOURCE -DFEC_NO_KLOG
  -Isrc -Isrc/libfec -Isrc/libfec/include -Isrc/libfec_rs
  -Ideps/include -Ideps/zlib)

C_SRCS=(
  src/libfec_rs/encode_rs_char.c
  src/libfec_rs/decode_rs_char.c
  src/libfec_rs/init_rs_char.c
  deps/squashfs_utils.c
  deps/zlib/adler32.c
  deps/zlib/compress.c
  deps/zlib/crc32.c
  deps/zlib/deflate.c
  deps/zlib/gzclose.c
  deps/zlib/gzlib.c
  deps/zlib/gzread.c
  deps/zlib/gzwrite.c
  deps/zlib/infback.c
  deps/zlib/inffast.c
  deps/zlib/inflate.c
  deps/zlib/inftrees.c
  deps/zlib/trees.c
  deps/zlib/uncompr.c
  deps/zlib/zutil.c
  deps/mini/sha1.c
  deps/mini/sha256.c
  deps/mini/evp.c
  deps/mini/ext4_sb.c
  deps/mini/klog.c
)

CPP_SRCS=(
  src/main.cpp
  src/image.cpp
  src/libfec/fec_open.cpp
  src/libfec/fec_read.cpp
  src/libfec/fec_verity.cpp
  src/libfec/fec_process.cpp
  src/libfec/avb_utils_stub.cpp
  deps/libsparse/backed_block.cpp
  deps/libsparse/output_file.cpp
  deps/libsparse/sparse.cpp
  deps/libsparse/sparse_crc32.cpp
  deps/libsparse/sparse_err.cpp
  deps/libsparse/sparse_read.cpp
  deps/mini/libbase.cpp
)

case "$ARCH" in
  aarch64) TRIPLE="aarch64-linux-android" ;;
  armv7a)  TRIPLE="armv7a-linux-androideabi" ;;
  x86_64)  TRIPLE="x86_64-linux-android" ;;
  native)  TRIPLE="" ;;
  *) echo "unknown arch: $ARCH (aarch64|armv7a|x86_64|native)" >&2; exit 1 ;;
esac

if [ -z "$TRIPLE" ]; then
  CC="${CC:-cc}"
  CXX="${CXX:-c++}"
  AR="${AR:-ar}"
else
  : "${NDK:?set NDK to the Android NDK root (e.g. export NDK=/opt/android-ndk-r26d)}"
  HOST_OS="$(uname -s | tr '[:upper:]' '[:lower:]')"
  case "$HOST_OS" in
    darwin) HOST_OS="darwin" ;;
    linux)  HOST_OS="linux" ;;
    *) echo "unsupported host OS: $HOST_OS" >&2; exit 1 ;;
  esac
  TOOLCHAIN="$NDK/toolchains/llvm/prebuilt/$HOST_OS-x86_64"
  if [ ! -x "$TOOLCHAIN/bin/${TRIPLE}${API:-21}-clang" ]; then
    echo "NDK toolchain not found at $TOOLCHAIN" >&2
    exit 1
  fi
  API="${API:-21}"
  CC="$TOOLCHAIN/bin/${TRIPLE}${API}-clang"
  CXX="$TOOLCHAIN/bin/${TRIPLE}${API}-clang++"
  AR="$TOOLCHAIN/bin/llvm-ar"
fi

echo "arch: $ARCH"
echo "cc:  $CC"
echo "cxx: $CXX"

objs=()
for s in "${C_SRCS[@]}"; do
  o="$OBJ/$(echo "$s" | tr '/.' '__').o"
  "$CC" "${COMMON_FLAGS[@]}" -c "$ROOT/$s" -o "$o"
  objs+=("$o")
done
for s in "${CPP_SRCS[@]}"; do
  o="$OBJ/$(echo "$s" | tr '/.' '__').o"
  "$CXX" "${COMMON_FLAGS[@]}" -std=c++17 -pthread -c "$ROOT/$s" -o "$o"
  objs+=("$o")
done

"$CXX" -static -pthread -o "$OUT/fec" "${objs[@]}" -lm

echo "built: $OUT/fec"
"$ROOT/scripts/check-binary.sh" "$OUT/fec" 2>/dev/null || true
