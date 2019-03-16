#!/bin/sh
set -e

GIT_REV=$(git log --pretty=format:'%h' -n 1)
GIT_DIFF=$(git diff --quiet --exit-code || echo +)
GIT_TAG=$(git describe --exact-match --tags 2> /dev/null || echo "")
GIT_BRANCH=$(git rev-parse --abbrev-ref HEAD)

VERSION=$(cat << EOF
const char* GIT_REV="${GIT_REV}${GIT_DIFF}";
const char* GIT_TAG="${GIT_TAG}";
const char* GIT_BRANCH="${GIT_BRANCH}";
EOF
)

PREV=$(cat build/version.c 2> /dev/null || echo '')
if [ "$VERSION" != "$PREV" ]
then
    echo "$VERSION" > build/version.c
fi
