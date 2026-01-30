#!/bin/bash
./build.sh release windows
./build.sh release

mkdir -p Lyte/DEBIAN Lyte/usr/bin Lyte/usr/share/lyte Lyte/usr/share/icons/hicolor/64x64/apps Lyte/usr/share/applications

strip lyte
cp lyte Lyte/usr/bin/lyte

cat <<EOL > "Lyte/DEBIAN/control"
Package: lyte
Version: 1.12
Section: editors
Priority: optional
Architecture: amd64
Maintainer: Hedrahexon <johanthomasjiji@gmail.com>
Description: A lightweight text editor written in Lua.
EOL

cat > Lyte/usr/share/applications/Lyte.desktop << EOF
[Desktop Entry]
Version=1.12
Type=Application
Name=Lyte
Exec=lyte
Icon=lyte
Terminal=false
Categories=Utility;TextEditor;
EOF
chmod 644 Lyte/usr/share/applications/Lyte.desktop

cp icon.png Lyte/usr/share/icons/hicolor/64x64/apps/lyte.png
chmod 644 Lyte/usr/share/icons/hicolor/64x64/apps/lyte.png

cat > Lyte/DEBIAN/postinst << EOF
#!/bin/sh
set -e
if command -v update-desktop-database >/dev/null 2>&1; then
    update-desktop-database -q /usr/share/applications
fi
if command -v gtk-update-icon-cache >/dev/null 2>&1; then
    gtk-update-icon-cache -q /usr/share/icons/hicolor
fi
exit 0
EOF
chmod 755 Lyte/DEBIAN/postinst

cp -a data/. Lyte/usr/share/lyte/
dpkg-deb --build "Lyte"

rm lyte.zip 2>/dev/null
rm -r Lyte
cp winlib/SDL3-3.2.16/x86_64-w64-mingw32/bin/SDL3.dll SDL3.dll
strip lyte.exe
strip SDL3.dll
zip lyte.zip lyte Lyte.deb lyte.exe SDL3.dll data -r
