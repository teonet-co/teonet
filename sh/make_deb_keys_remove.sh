#!/bin/sh
# 
# File:   make_deb_keys_remove.sh
# Author: Kirill Scherba <kirill@scherba.ru>
#
# Remove repository keys from this host
#
# Created on Aug 26, 2015, 7:59:32 PM
#

gpg --delete-secret-key  "repository <repo@ksproject.org>"
gpg --delete-key  "repository <repo@ksproject.org>"
