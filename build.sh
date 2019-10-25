#!/bin/bash
# Remove existing build artifact if dir exists. 
rm -rf build

# create directory if it doesn't already exist
mkdir -p build/ 

# run a container to build the gpmf app
docker run --rm -v "$PWD"/:/gpmf -w /gpmf/demo gcc:9.2 make
# copy make output binary to local dir
cp ./demo/gpmfdemo ./build
