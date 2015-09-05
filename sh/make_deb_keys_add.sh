#!/bin/sh
# Import repository keys to this host

ANSI_BROWN="\033[22;33m"
ANSI_NONE="\033[0m"

# Import repository keys to this host
#echo "Before Import repository keys..."
echo $ANSI_BROWN"Check host repository keys:"$ANSI_NONE
echo ""
str=`gpg --list-keys | grep "repository <repo@ksproject.org>"`;
echo ""
if [ -z "$str" ]; then 
  echo $ANSI_BROWN"Add repository keys to this host:"$ANSI_NONE
  echo ""  
  gpg --allow-secret-key-import --import sh/gpg_key/deb-sec.gpg.key
  gpg --import sh/gpg_key/deb.gpg.key
  echo ""
fi
#echo "After Import repository keys..."
