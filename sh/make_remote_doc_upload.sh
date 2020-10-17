#!/bin/sh
# 
# File:   make_remote_doc_upload.sh
# Author: Kirill Scherba <kirill@scherba.ru>
#
# Make doxygen documentation and upload it to web-site (repository) by FTP
#
# Created on Sep 3, 2015, 1:02:36 AM
#
ANSI_BROWN="\033[22;33m"
ANSI_NONE="\033[0m"

DOC_MAKE="make doxygen-doc"
DOC_FOLDER=docs/html

REPO_DOC_FOLDER=/docs/$PROJECT_NAME

# Make documentation
echo $ANSI_BROWN"Make documentation:"$ANSI_NONE
echo ""
$DOC_MAKE
echo ""
# Upload project documentation to github pages
if [ ! -z "$CI_BUILD_REF_BT" ]; then

	echo $ANSI_BROWN"Upload Packet documentation to GitHub pages:"$ANSI_NONE
	echo ""

	exists=$(git show-ref refs/remotes/origin/gh-pages)
	if [ -n "$exists" ]; then
    	echo 'gh-pages branch exists!'
    	#make doxygen-doc
    	mv docs/ ../.docs

		git config --global user.email "repo@ksproject.org"
  		git config --global user.name  "repository"

    	git checkout gh-pages

    	rm -r *
    	cp -r ../.docs/html/* ./

    	git add .
    	git commit -m "Project documentation updated" -m "[skip ci]"
    	printenv
    	git push origin gh-pages
    	rm -r ../.docs
    	
    	# Resotre project state
    	# git checkout master
    	# git submodule update --init --recursive
    	# ./autogen.sh
    	# make

    	echo 'Packet documentation upload to GitHub pages. Use htts://$PACKET_NAME.github.io'
    else 
    	echo "Can't upload Packet documentation to GitHub pages. Create gh-pages branch to publish documentation to GitHub pages."	
	fi
	echo ""
fi
