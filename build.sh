#!/bin/bash

set -x

SOURCE_DIR=`pwd`
BUILD_DIR=${SOURCE_DIR:-/build}
BUILD_TYPE=${BUILD_TYPE:-release}
CXX={CXX:-gcc}

mkdir -p $BUILD_DIR/$BUILD_TYPE \
  && cd $BUILD_DIR/$BUILD_TYPE \
  && cmake \
          -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
          -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
          $SOURCE_DIR \
  && make $*
