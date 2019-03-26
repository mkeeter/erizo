#!/bin/sh
set -x -e

EXE=Erizo
APP=$EXE.app

cd ../..
make clean
make -j8

# Parse the revision, branch, and tag from version.c
REV=$(cat src/version.c | sed -ne 's/.*GIT_REV="\(.*\)".*/\1/gp')
TAG=$(cat src/version.c | sed -ne 's/.*GIT_TAG="\(.*\)".*/\1/gp')
if [ -z $TAG ]
then
    TAG=$(cat src/version.c | sed -ne 's/.*GIT_BRANCH="\(.*\)".*/\1/gp')
fi
VERSION="$TAG [$REV]"

rm -rf $APP
mkdir -p $APP/Contents/MacOS
mkdir -p $APP/Contents/Resources
mkdir -p $APP/Contents/Frameworks

cp erizo $APP/Contents/MacOS
cp deploy/darwin/erizo.icns $APP/Contents/Resources
sed "s/VERSION/$VERSION/g" deploy/darwin/Info.plist > $APP/Contents/Info.plist

for LIB in $(otool -L erizo | grep /usr/local | awk '{print $1}')
do
    cp $LIB $APP/Contents/Frameworks/$(basename $LIB)
    install_name_tool -change $LIB @executable_path/../Frameworks/$(basename $LIB) $APP/Contents/MacOS/erizo
done

if [ "$1" == "dmg" ]
then
    rm -rf dmg $EXE.dmg
    mkdir dmg
    cp -r $APP dmg
    hdiutil create $EXE.dmg -volname "$EXE $VERSION" -srcfolder dmg

    # Clean up
    rm -rf dmg
fi

make clean
