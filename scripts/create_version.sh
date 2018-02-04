#!/bin/bash

echo -e "// AUTOMATICALLY GENERATED FILE! DO NOT EDIT!\n" > leaguedisplays_version.h
echo "#ifndef LD_VERSION_H" >> leaguedisplays_version.h
echo -e "#define LD_VERSION_H\n" >> leaguedisplays_version.h

echo -en "#define LD_APPVERSION \"L1.0.928-beta_x86_64-" >> leaguedisplays_version.h
printf "%s" $(git branch --no-color 2> /dev/null | sed -e '/^[^*]/d' -e "s/* \(.*\)/\1/") >> leaguedisplays_version.h
echo -n "/" >> leaguedisplays_version.h
printf "%s" $(git rev-parse --short HEAD 2> /dev/null | sed "s/\(.*\)/\1/") >> leaguedisplays_version.h
echo -en "\"\n\n#endif // LD_VERSION_H" >> leaguedisplays_version.h
