#!/bin/sh
set -x -e

EXE=Hedgehog
APP=$EXE.app

VERSION=`git describe --exact-match --tags || echo "($(git rev-parse --abbrev-ref HEAD))"`
VERSION=`echo $VERSION|sed s:/:-:g`

cd ../..
make clean
make -j8

mkdir -p $APP/Contents/MacOS
mkdir -p $APP/Contents/Resources
mkdir -p $APP/Contents/Frameworks

cp hedgehog $APP/Contents/MacOS
cp app/darwin/hedgehog.icns $APP/Contents/Resources
cp app/darwin/Info.plist $APP/Contents

for LIB in $(otool -L hedgehog | grep /usr/local | awk '{print $1}')
do
    cp $LIB $APP/Contents/Frameworks/$(basename $LIB)
    install_name_tool -change $LIB @executable_path/../Frameworks/$(basename $LIB) $APP/Contents/MacOS/hedgehog
done

rm -rf deploy $EXE.dmg
mkdir deploy
cp -r $APP deploy
hdiutil create $EXE.dmg -volname "$EXE $VERSION" -srcfolder deploy

# Clean up
rm -rf deploy
make clean
