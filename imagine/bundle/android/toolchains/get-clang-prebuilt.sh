#!/bin/bash
cd "$(dirname "$0")"
CLANG_VERSION=r487747c
OUTPUT_DIR=llvm/clang-$CLANG_VERSION
if [ -d $OUTPUT_DIR/bin ] 
then
    echo "clang-$CLANG_VERSION is already present" 
    exit 0
fi
mkdir -p $OUTPUT_DIR
wget -qO- "https://android.googlesource.com/platform/prebuilts/clang/host/linux-x86/+archive/refs/heads/master/clang-$CLANG_VERSION.tar.gz" | tar xvz -C $OUTPUT_DIR
