#!/bin/bash
set -e

export CC="gcc -m32"
export CXX="g++ -m32"
export PKG_CONFIG_PATH="/usr/lib32/pkgconfig"

meson setup build32 --prefix=/usr --libdir=lib32 -Dwith_xnvctrl=disabled -Dwith_server=false --reconfigure
ninja -C build32
sudo ninja -C build32 install

unset CC CXX PKG_CONFIG_PATH
meson setup build64 --prefix=/usr --libdir=lib -Dwith_server=true --reconfigure
ninja -C build64
sudo ninja -C build64 install
