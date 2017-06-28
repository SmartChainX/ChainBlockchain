#!/usr/bin/env sh

#------------------------------------------------------------------------------
# Bash script for installing pre-requisite packages for cpp-chainx on a
# variety of Linux and other UNIX-derived platforms.
#
# This is an "infrastucture-as-code" alternative to the manual build
# instructions pages which we previously maintained, first as Wiki pages
# and later as readthedocs pages
#
# The aim of this script is to simplify things down to the following basic
# flow for all supported operating systems:
#
# - git clone --recursive
# - ./install_deps.sh
# - cmake && make
#
# At the time of writing we are assuming that 'lsb_release' is present for all
# Linux distros, which is not a valid assumption.  We will need a variety of
# approaches to actually get this working across all the distros which people
# are using.
#
# See http://unix.stackexchange.com/questions/92199/how-can-i-reliably-get-the-operating-systems-name
# for some more background on this common problem.
#
# TODO - There is no support here yet for cross-builds in any form, only
# native builds.  Expanding the functionality here to cover the mobile,
# wearable and SBC platforms covered by doublethink and chainXEmbedded would
# also bring in support for Android, iOS, watchOS, tvOS, Tizen, Sailfish,
# Maemo, MeeGo and Yocto.
#
# ------------------------------------------------------------------------------
# This file is part of cpp-chainx

#------------------------------------------------------------------------------

set -e

# Check for 'uname' and abort if it is not available.
uname -v > /dev/null 2>&1 || { echo >&2 "ERROR - cpp-chainx requires 'uname' to identify the platform."; exit 1; }

case $(uname -s) in

#------------------------------------------------------------------------------
# macOS
#------------------------------------------------------------------------------

Darwin)
    case $(sw_vers -productVersion | awk -F . '{print $1"."$2}') in
        10.9)
            echo "Installing cpp-chainx dependencies on OS X 10.9 Mavericks."
            ;;
        10.10)
            echo "Installing cpp-chainx dependencies on OS X 10.10 Yosemite."
            ;;
        10.11)
            echo "Installing cpp-chainx dependencies on OS X 10.11 El Capitan."
            ;;
        10.12)
            echo "Installing cpp-chainx dependencies on macOS 10.12 Sierra."
            echo ""
            echo "NOTE - You are in unknown territory with this preview OS."
            echo "Even Homebrew doesn't have official support yet, and there are
            echo "If you would like to partner with us to work through these issues, that"
            echo "would be fantastic.  Please just comment on that issue.  Thanks!"
            ;;
        *)
            echo "Unsupported macOS version."
            echo "We only support Mavericks, Yosemite and El Capitan, with work-in-progress on Sierra."
            exit 1
            ;;
    esac

    if [ "$TRAVIS" ]; then
        TRAVIS_PACKAGES="ccache"
    fi

    # Check for Homebrew install and abort if it is not installed.
    brew -v > /dev/null 2>&1 || 

    # And finally install all the external dependencies.
    brew install \
        leveldb \
        libmicrohttpd \
        $TRAVIS_PACKAGES

    ;;

#------------------------------------------------------------------------------
# FreeBSD
#------------------------------------------------------------------------------
FreeBSD)
    echo "Installing dependencies on FreeBSD."
    echo "ERROR - 'install_deps.sh' doesn't have FreeBSD support yet."
    echo "Please let us know if you see this error message, and we can work out what is missing."
    echo 
    exit 1
    ;;

#------------------------------------------------------------------------------
# Linux
#------------------------------------------------------------------------------
Linux)

    # Detect if sudo is needed.
    if [ $(id -u) != 0 ]; then
        SUDO="sudo"
    fi

#------------------------------------------------------------------------------
# Arch Linux
#------------------------------------------------------------------------------

    if [ -f "/etc/arch-release" ]; then

        echo 

        # The majority of our dependencies can be found in the
        # Arch Linux official repositories.
        # See https://wiki.archlinux.org/index.php/Official_repositories
        $SUDO pacman -Sy --noconfirm \
            autoconf \
            automake \
            gcc \
            libtool \
            leveldb \
            libmicrohttpd

    elif [ -f "/etc/os-release" ]; then

        DISTRO_NAME=$(. /etc/os-release; echo $NAME)
        case $DISTRO_NAME in

        Debian*)
            echo 

            $SUDO apt-get -q update
            $SUDO apt-get -qy install \
                build-essential \
                libgmp-dev \
                libleveldb-dev \
                libmicrohttpd-dev
            ;;

        Fedora)
            echo 
            $SUDO dnf -qy install \
                gcc-c++ \
                leveldb-devel \
                libmicrohttpd-devel \
                gmp-devel
            ;;

#------------------------------------------------------------------------------
# Ubuntu
#
# TODO - I wonder whether all of the Ubuntu-variants need some special
# treatment?
#
# TODO - We should also test this code on Ubuntu Server, Ubuntu Snappy Core
# and Ubuntu Phone.
#
# TODO - Our Ubuntu build is only working for amd64 and i386 processors.
# It would be good to add armel, armhf and arm64.
# See 
#------------------------------------------------------------------------------
        Ubuntu|"Linux Mint")
            echo 
            if [ "$TRAVIS" ]; then
                # Setup prebuilt LLVM on Travis CI:
                $SUDO apt-get -qy remove llvm  # Remove confilicting package.
                echo "deb http://apt.llvm.org/trusty/ llvm-toolchain-trusty-3.9 main" | \
                    $SUDO tee -a /etc/apt/sources.list > /dev/null
                TRAVIS_PACKAGES="llvm-3.9-dev libz-dev"
            fi
            $SUDO apt-get -q update
            $SUDO apt-get install -qy --no-install-recommends --allow-unauthenticated \
                build-essential \
                libgmp-dev \
                libleveldb-dev \
                libmicrohttpd-dev \
                $TRAVIS_PACKAGES
            ;;

        CentOS*)
            echo 
            # Enable EPEL repo that contains leveldb-devel
            $SUDO yum -y -q install epel-release
            $SUDO yum -y -q install \
                make \
                gcc-c++ \
                leveldb-devel \
                libmicrohttpd-devel \
                gmp-devel
            ;;

        *)
            echo "Unsupported Linux distribution: $DISTRO_NAME."
            exit 1
            ;;

        esac

    elif [ -f "/etc/alpine-release" ]; then

        # Alpine Linux
        echo 
        $SUDO apk add --no-cache --repository http://dl-cdn.alpinelinux.org/alpine/edge/testing/ \
            g++ \
            make \
            libmicrohttpd-dev \
            leveldb-dev

    else

        case $(lsb_release -is) in

#------------------------------------------------------------------------------
# OpenSUSE
#------------------------------------------------------------------------------
        openSUSE*)
            #openSUSE
            echo 
            echo "ERROR - 'install_deps.sh' doesn't have openSUSE support yet."
            echo "If you would like to get 'install_deps.sh' working for openSUSE, that would be fantastic."
            echo
            exit 1
            ;;

#------------------------------------------------------------------------------
# Other (unknown) Linux
# Major and medium distros which we are missing would include Mint, CentOS,
# RHEL, Raspbian, Cygwin, OpenWrt, gNewSense, Trisquel and SteamOS.
#------------------------------------------------------------------------------
        *)
            #other Linux
            echo "ERROR - Unsupported or unidentified Linux distro."
      echo "If you would like to get your distro working, that would be fantastic."
      exit 1
            ;;
        esac
    fi
    ;;

#------------------------------------------------------------------------------
# Other platform (not Linux, FreeBSD or macOS).
# Not sure what might end up here?
# Maybe OpenBSD, NetBSD, AIX, Solaris, HP-UX?
#------------------------------------------------------------------------------
*)
    #other
    echo "ERROR - Unsupported or unidentified operating system."
   "
    ;;
esac
