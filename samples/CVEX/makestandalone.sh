#!/bin/bash

echo 'When making a stand alone application, we need to specify the -s'
echo 'option on the hcustom command.'

vcc test.vfl
vcc simple.vfl
hcustom -g -s cvexsample.C
hcustom -g -s simple.C

# Sample run
echo 'Running cvexsample'
./cvexsample test unbound 32

echo 'Running simple'
./simple simple
