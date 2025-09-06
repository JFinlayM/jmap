#!/bin/bash
set -e

PKGNAME="libjmap-dev"
VERSION="1.0.0-beta"
FULLNAME="JFinlayM"
USER="JFinlayM"
DATE=$(date -R)

# Nettoyer un éventuel vieux dossier debian/
rm -rf debian
mkdir -p debian

# ===== FICHIER control =====
cat > debian/control <<EOF
Source: jmap
Section: libs
Priority: optional
Maintainer: $FULLNAME
Build-Depends: debhelper-compat (= 13), cmake, gcc, make
Standards-Version: 4.6.2
Homepage: https://github.com/$USER/jmap
Rules-Requires-Root: no

Package: $PKGNAME
Architecture: any
Depends: \${shlibs:Depends}, \${misc:Depends}
Description: Dynamic array library for C
 A hash map library for C with key-value storage, iteration, and various utility functions. The library is designed to be generic, allowing storage of any data type by specifying the element size during initialization. 
 It provides a variety of utility functions and supports user-defined callbacks for custom behavior. The functions return a struct containing either the result or an error code/message, which can be checked using provided macros. 
 The functions are inspired by higher-level languages like JavaScript or Java.
 This package contains the static and shared libraries, along
 with the development header file.
 Note that I'm a beginner programmer, so don't take this code too seriously. This is a learning project!
EOF

# ===== FICHIER rules =====
cat > debian/rules <<'EOF'
#!/usr/bin/make -f
export DEB_BUILD_MAINT_OPTIONS = hardening=+all
export DEB_CFLAGS_MAINT_APPEND  = -Wall -Wextra
export DEB_LDFLAGS_MAINT_APPEND =

%:
	dh $@ --buildsystem=cmake
EOF
chmod +x debian/rules

# ===== FICHIER changelog =====
cat > debian/changelog <<EOF
jmap ($VERSION-1) unstable; urgency=medium

  * Initial release.

 -- $FULLNAME <$EMAIL>  $DATE
EOF

# ===== FICHIER copyright =====
cat > debian/copyright <<EOF
Format: https://www.debian.org/doc/packaging-manuals/copyright-format/1.0/
Upstream-Name: jmap
Source: https://github.com/$USER/jmap

Files: *
Copyright: $(date +%Y), $FULLNAME <$EMAIL>
License: MIT

License: MIT
 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:
 .
 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.
 .
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
EOF

echo "[OK] Dossier debian/ généré ✅"
echo "Pour construire ton .deb :"
echo "  dpkg-buildpackage -us -uc"
