#!/bin/sh

docker build . -t dxloc/alpine-cmake:0.1.0
docker push dxloc/alpine-cmake:0.1.0
