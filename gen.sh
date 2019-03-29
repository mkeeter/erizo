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
PREV=$(cat $TARGET 2> /dev/null || echo '')
if [ "$VERSION" != "$PREV" ]
then
    echo "$VERSION" > $TARGET
fi

################################################################################
# Find the largest filename + line number possible in the logs,
# then write it to a file so that log columns can be nicely aligned.
LARGEST=0
for s in "$@"
do
    LINECOUNT=`wc -l src/$s.c | awk '{print $1}'`
    let "LENGTH = ${#s} + ${#LINECOUNT} + 2"
    if (( $LENGTH > $LARGEST ))
    then
        LARGEST=$LENGTH
    fi
done
TARGET=inc/log_align.h
LOGSIZE="#define LOG_ALIGN $LARGEST"
PREV=$(cat $TARGET 2> /dev/null || echo '')
if [ "$LOGSIZE" != "$PREV" ]
then
    echo "$LOGSIZE" > $TARGET
fi
