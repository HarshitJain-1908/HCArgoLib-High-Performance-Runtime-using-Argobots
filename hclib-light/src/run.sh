#!/bin/bash

export ARGOBOTS_INSTALL_DIR=/home/`whoami`/HCArgoLib-High-Performance-Runtime-using-Argobots/argobots-install
LD_LIBRARY_PATH=$ARGOBOTS_INSTALL_DIR/lib:$LD_LIBRARY_PATH ./argobots-runtime