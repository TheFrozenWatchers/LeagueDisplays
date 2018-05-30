#!/bin/bash

branch=$(git branch --no-color 2> /dev/null | sed -e '/^[^*]/d' -e "s/* \(.*\)/\1/")
version=$(git rev-parse --short HEAD 2> /dev/null | sed "s/\(.*\)/\1/")

echo -e > leaguedisplays_version.h "
// AUTOMATICALLY GENERATED FILE! DO NOT EDIT!
#ifndef LD_VERSION_H
#define LD_VERSION_H

#define LD_APPVERSION \"L1.0.992-beta_x86_64-${branch}/${version}\"
#endif // LD_VERSION_H
"
