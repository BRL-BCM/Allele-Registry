#!/bin/bash

set -e

# build mysql connector
tar xzf mysql-connector-c-6.1.6-src.tar.gz
cd mysql-connector-c-6.1.6-src
mkdir build
cd build
CC="gcc" CXX="g++" cmake .. -C../../mysql.cmake
make all
make install
cd ../../

# build yaml-cpp
tar xzf yaml-cpp-0.5.3.tar.gz
cd yaml-cpp-yaml-cpp-0.5.3
mkdir build
cd build
CC="gcc" CXX="g++" cmake .. -C../../yaml.cmake
make all
make install
cd ../../

# build kyotocabinet
#tar xf  kyotocabinet-1.2.76.tar.gz
#cd kyotocabinet-1.2.76
#./configure --prefix=`pwd`/../kyotocabinet --enable-static --disable-shared
#make all
#make install
#cd ../

# build rocksdb
#tar xf rocksdb-rocksdb-5.0.1.tar.gz
#cd rocksdb-rocksdb-5.0.1
#PORTABLE=1 make static_lib
#make install INSTALL_PATH=`pwd`/../rocksdb
#cd ../
