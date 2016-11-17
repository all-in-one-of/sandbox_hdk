#!/bin/bash

echo 'This is a sample install script.'
echo 'Ensure the IMG library directory exists.'

HOME_DIR="$HOME/houdini15.0/"
DSO_DIR="$HOME_DIR/dso/fb"

mkdir -p "$DSO_DIR"

echo
echo 'Build the IMG library files.'
hcustom -i "$DSO_DIR" IMG_Raw.C 

echo
echo 'Copying FBformats file into home directory.'
echo 'This will overwrite existing files.'
cp -i FBformats "$HOME_DIR"

echo
echo "Now, the IMG library files have been installed in $HOME/houdini15.0/dso_fb"
