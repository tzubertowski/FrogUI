#!/bin/sh
set -e
MIPS="$HOME/sf3000-work/sf3000toolchain/mipsel-buildroot-linux-gnu_sdk-buildroot/opt/ext-toolchain/bin/mips-mti-linux-gnu-"
SYSROOT="$HOME/sf3000-work/sf3000toolchain/mipsel-buildroot-linux-gnu_sdk-buildroot/mipsel-buildroot-linux-gnu/sysroot"
CC="${MIPS}gcc"
cd "$(dirname "$0")"

CFLAGS="-mips32r2 -march=mips32r2 -mtune=24kc -mfp32 -mhard-float -mlong-calls -EL --sysroot=$SYSROOT -fPIC -G0 -Wall -I./ -I$SYSROOT/usr/include -Ofast -DPLATFORM_SF3000 -DNDEBUG -D__LIBRETRO__"

for src in frogui_libretro.c render.c font.c recent_games.c theme.c favorites.c banner.c backlight.c input.c core_override.c ext_filter.c i18n.c; do
    obj="${src%.c}.lo"
    echo "CC $src"
    $CC $CFLAGS -c -o "$obj" "$src"
done

echo "Linking..."
$CC -mips32r2 -mhard-float -mfp32 -EL -fPIC -shared -nostdlib -Wl,--no-undefined \
    frogui_libretro.lo render.lo font.lo recent_games.lo theme.lo favorites.lo banner.lo backlight.lo input.lo core_override.lo ext_filter.lo i18n.lo \
    -L"$SYSROOT/usr/lib" --sysroot="$SYSROOT" -lm -lc -ldl -lpthread \
    -o frogui_libretro.so

ls -la frogui_libretro.so
echo "Done."
