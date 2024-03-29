#!/bin/bash

# Just exit if this is clang-format
if [[ "$NAME" == "clang-format" ]]; then
  exit 0
fi

if [[ "$TRAVIS_OS_NAME" == "osx" ]]; then
  # Before install
  brew update

  # Install
  brew upgrade git cmake
  brew install qt5 qwt eigen # libssh should already be installed...
else
  # Before install
  eval "${MATRIX_EVAL}"
  sudo apt-get update -qq
  wget https://github.com/xtalopt/xtalopt-dependencies/releases/download/1.0/qwt_6.1.3-2-Ubuntu14.04-gcc4.8.4.deb

  # Install
  sudo apt-get install -qq qt5-default libeigen3-dev libssh-dev \
                           libqt5svg5 lcov
  sudo dpkg -i qwt_6.1.3-2-Ubuntu14.04-gcc4.8.4.deb
fi
