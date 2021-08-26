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

echo -n "Version " >ChangeLog.tmp
git describe --tags >>ChangeLog.tmp
echo "==============\n" >>ChangeLog.tmp
git log --pretty=format:'  - %s' $1..HEAD >>ChangeLog.tmp
echo "\n" >> ChangeLog.tmp
cat ChangeLog >>ChangeLog.tmp
mv ChangeLog.tmp ChangeLog