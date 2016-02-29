#!/bin/sh
# 
# File:   change_log_prepend.sh
# Author: kirill
#
# Created on Feb 29, 2016, 4:34:03 PM
#

git log --pretty=format:'  - %s' 776465a35a7954b962b3d017883200a6dfc85cc4..HEAD | cat - ChangeLog > temp && mv temp ChangeLog
echo -e "" | cat - ChangeLog > temp && mv temp ChangeLog
git describe --tags | cat - ChangeLog > temp && mv temp ChangeLog
echo "### " | cat - ChangeLog > temp && mv temp ChangeLog

# git rev-parse v0.1.39-1-g197cc61
# git describe --tags
#