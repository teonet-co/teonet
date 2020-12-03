#!/bin/sh
# 
# File:   change_log_prepend.sh
# Author: Kirill Scherba <kirill@scherba.ru>
#
# Created on Feb 29, 2016, 4:34:03 PM
#

die () {
    echo >&2 "$@"
    exit 1
}

# Empty line
echo "\n" | cat - ChangeLog > temp && mv temp ChangeLog

# List of changes
git log --pretty=format:'  - %s' $1..HEAD | cat - ChangeLog > temp && mv temp ChangeLog

# Empty line
echo "" | cat - ChangeLog > temp && mv temp ChangeLog

# Last tag
git describe --tags | cat - ChangeLog > temp && mv temp ChangeLog
echo "### " | cat - ChangeLog > temp && mv temp ChangeLog

# git rev-parse v0.1.39-1-g197cc61
# git describe --tags
#