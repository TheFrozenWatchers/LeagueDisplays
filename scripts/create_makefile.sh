#!/bin/bash

echo "CXX = clang++" > Makefile

cflags=()
cflags+=('-O3')
cflags+=('-g')
cflags+=('-w')
cflags+=('-I.')
cflags+=('-I./src/')
cflags+=('-Wl,-rpath,.')
cflags+=('-L./bin/')
cflags+=('-std=c++14')
cflags+=('-Wall')
cflags+=('-lX11')
cflags+=('-lXt')
cflags+=('-lcef')
cflags+=('-pthread')
cflags+=('-lrt')
cflags+=('-lz') # zlib
cflags+=('`pkg-config --libs --cflags gtk+-2.0`')

if [[ -d "thirdparty" ]]; then
	cflags+=('-I./thirdparty/')
fi

if [[ $_cef_path != "" ]]; then
	cflags+=("-I$_cef_path")
fi

if [[ $(find /usr/include/ | grep appindicator) != "" ]]; then
	echo "[*] Looks like you have appindicator installed"
	cflags+=('`pkg-config --libs --cflags appindicator-0.1`')
	cflags+=('-DLD_USE_APPINDICATOR')
fi

echo -e "CFLAGS = "${cflags[@]}"\n" >> Makefile

echo "[*] Adding predefined rules..."

echo -e >> Makefile "
all: leaguedisplays assets

leaguedisplays: bin/leaguedisplays

clean: clean-assets
\trm -f *.o
\trm -f ./bin/leaguedisplays

strip:
\tstrip ./bin/leaguedisplays

leaguedisplays_version.h: ./scripts/create_version.sh
\t./scripts/create_version.sh

"

echo "[*] Creating rules for assets..."

sources=($(ls assets))
objects=()

for f in ${sources[@]}; do
	if [[ -d "assets/$f" ]] || [[ -d "assets/"$(echo $f | sed -r 's/\.bin//g') ]]; then
		if [[ -f "assets/$f" ]]; then
			continue
		fi

		echo "[*] Packed asset group: $f"

		object="bin/$f.bin"
		objects+=("$object")

		requested_files=$(find "assets/$f" -type f)
		files_to_compress=$(printf "%s " $(echo $requested_files | sed -r 's/assets\/'$f'\///g'))

		echo "$object: "$(printf "%s " $requested_files) >> Makefile
		echo -e "\tcd \"assets/$f\" && zip ../$f.bin $files_to_compress" >> Makefile
		echo -e "\tcp assets/$f.bin $object\n" >> Makefile
		continue
	fi

	object="bin/$f"
	objects+=("$object")

	echo "$object: assets/$f" >> Makefile
	echo -e "\tcp assets/$f $object\n" >> Makefile
done

echo -e "assets: "${objects[@]}"\n" >> Makefile

echo "clean-assets:" >> Makefile

for o in ${objects[@]}; do
	echo -e "\trm -f $o" >> Makefile
done

echo "" >> Makefile

echo "[*] Creating rules for source files..."

sources=($(find src | grep --regex="log\.cc$"))
sources+=($(find src | grep --regex="\.cc$"))
objects=()

for f in "${sources[@]}"; do
	object=$(echo $f | sed -r 's/\.cc$/\.o/g' | sed -r 's/src\///g')
	if [[ $(echo ${objects[@]} | grep $object) != "" ]]; then continue; fi
	objects+=("$object")

	rule_header="$object: "
	rule_header+="$f "

	includes=($(cat "$f" | grep -oP --regex="#include.*?\".*?\"" | sed -r 's/#include\s+//g' | cut -c 2- | sed -r 's/\"$//g'))
	for i in "${includes[@]}"; do
		if [[ ! $i =~ ^"include/" ]]; then
			if [[ -f "src/$i" ]]; then
				i="src/$i"
			fi

			rule_header+="$i "
		fi
	done

	echo "$rule_header" >> Makefile
	echo -e "\t\$(CXX) \$(CFLAGS) -c $f\n" >> Makefile
done

objects+=("./thirdparty/cef/libcef_dll_wrapper/libcef_dll_wrapper.a")

echo "[*] Adding linker rule..."

echo -e >> Makefile "
bin/leaguedisplays: ${objects[@]}
\t\$(CXX) \$(CFLAGS) -o ./bin/leaguedisplays ${objects[@]}
"
