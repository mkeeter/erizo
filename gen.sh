#!/bin/sh
set -e

################################################################################
# Write the Git revision (with trailing '+' to mark changes), tag,
# and branch to a file as C strings.
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

TARGET=src/version.c
if ! echo "$VERSION" | cmp $TARGET &> /dev/null
then
    echo "$VERSION" > $TARGET
fi

################################################################################
# Find the largest filename + line number possible in the logs,
# then write it to a file so that log columns can be nicely aligned.
LARGEST=$(grep -nr "log_" $@ | sed -e 's/\([^:]*:[^:]*\):.*$/\1/g' |
          awk '{print length($1)}' | sort | tail -n 1)
TARGET=inc/log_align.h
LOGSIZE="#define LOG_ALIGN $LARGEST"
if ! echo "$LOGSIZE" | cmp $TARGET &> /dev/null
then
    echo "$LOGSIZE" > $TARGET
fi
