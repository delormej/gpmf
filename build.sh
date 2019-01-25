
# create directory if it doesn't already exist
mkdir -p build/ 

# run a container to build the gpmf app
docker run --rm -v "$PWD"/:/gpmf -w /gpmf/demo gcc:4.9 make
# copy make output binary to local dir
cp ./demo/gpmfdemo ./build
