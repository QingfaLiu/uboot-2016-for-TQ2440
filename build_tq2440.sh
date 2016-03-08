#!/bin/bash

export ARCH=arm
export CROSS_COMPILE=arm-linux-

cp TQ2440_config .config
make -j 4
