#!/bin/bash

echo apt install libsndfile1-dev
autogen
./configure
make
