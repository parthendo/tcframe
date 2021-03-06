#!/usr/bin/env bash

set -e

export TCFRAME_HOME=../../tcframe

g++ -o solution solution.cpp
g++ -o scorer scorer.cpp
$TCFRAME_HOME/scripts/tcframe build
./runner --solution=./solution --scorer=./scorer
