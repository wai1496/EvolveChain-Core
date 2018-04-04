#!/bin/sh
# Copyright (c) 2013 The EvolveChain Core developers
# Distributed under the MIT/X11 software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
#
# Helper script for pull-tester.
#Param 1: path to evolvechain srcroot
#Param ...: arguments for build-test.sh

if [ $# -lt 1 ]; then
  echo "usage: $0 [evolvechain srcroot] build-test arguments..."
fi

killall -q evolvechain-cli
killall -q evolvechaind

cd $1
shift

./autogen.sh
./configure
./qa/pull-tester/build-tests.sh "$@"
