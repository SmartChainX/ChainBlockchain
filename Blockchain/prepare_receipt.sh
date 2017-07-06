ble File  72 lines (61 sloc)  2.49 KB
#/bin/bash

# this script should be run from build directory

VERSION=1 # eg 1.0rc2
NUMBER=1 # jenkins build number

# Detect whether we are running on a Yosmetie and generate
# an appropriately named ZIP file for the Homebrew receipt to point at.
if echo `sw_vers` | grep "10.11"; then
    OSX_VERSION=el_capitan
elif echo `sw_vers` | grep "10.10"; then
    OSX_VERSION=yosemite
else
    echo Unsupported OS X version.  
    exit 1
fi

while [ "$1" != "" ]; do
    case $1 in
        --version )
            shift
            VERSION=$1 
            ;;
        --number )
            shift
            NUMBER=$1
            ;;
    esac
    shift
done
