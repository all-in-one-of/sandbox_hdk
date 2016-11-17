#!/bin/bash

echo 'When making a stand alone application, we need to specify the -s'
echo 'option on the hcustom command.'

hcustom -s standalone.C
hcustom -s traverse.C
hcustom -s i3ddsmgen.C
hcustom -s gengeovolume.C
hcustom -s tiledevice.C
