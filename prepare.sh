#!/bin/bash

function silent_pushd() {
	pushd "$1" > /dev/null
}

function silent_popd() {
	popd > /dev/null
}

function assert_success() {
	if [[ $? != 0 ]]; then
		echo "Terminating."
		exit 1
	fi
}

function usage() {
	echo "usage: $0 [--cef-path PATH] [--rapidjons-install]"
}

export _cef_path=""
export _rapidjson_in_include=false

OPTS=$(getopt -o "h" -l "help,cef-path:,rapidjson-install" -- "$@")
if [[ $? != 0 ]]; then
	echo "Failed to parse options"
	exit 2
fi
eval set -- "$OPTS"

export _cef_path=
export _rapidjson_in_include=false
while true; do
	case "$1" in
		-h|--help)
			usage
			exit
			shift
			;;
		--cef-path)
			_cef_path=$2
			shift 2
			;;
		--rapidjson-install)
			_rapidjson_in_include=true
			if [[ ! -d "/usr/include/rapidjson" ]]; then
				echo "--rapidjson-installed is defined but cannot find directory in /usr/include!"
				exit 1
			fi
			shift
			;;
		--) shift; break ;;
		*)
			echo "Error"
			exit 2
			;;
	esac
done

echo "[*] Creating directories..."

mkdir -p "$PWD/bin/"
assert_success

mkdir -p "$PWD/thirdparty/cef"
assert_success

if [[ $_cef_path == "" ]]; then
	echo "[*] Downloading and building CEF api..."

	if [[ ! -f "$PWD/thirdparty/cef_binary.tar.bz2" ]]; then
		wget --quiet --show-progress -O "$PWD/thirdparty/cef_binary.tar.bz2" $(printf "http://opensource.spotify.com/cefbuilds/%s" $(curl -s http://opensource.spotify.com/cefbuilds/index.html | grep -oP --regex="cef_binary_3.*?_linux64.tar.bz2" | head -1))
		assert_success
	fi

	if [[ ! -d "$PWD/thirdparty/cef/include/" ]]; then
		echo "[*] Uncompressing..."

		tar -vxjf "$PWD/thirdparty/cef_binary.tar.bz2" -C "$PWD/thirdparty/cef/"

		archivedir="$PWD/thirdparty/cef/"$(ls -1 "$PWD/thirdparty/cef/" | head -1)"/"
		mv "$archivedir"* "$PWD/thirdparty/cef/"
		assert_success
		rmdir "$archivedir"
		assert_success
	fi

	silent_pushd "$PWD/thirdparty/cef"
	assert_success

	cmake . -DCMAKE_BUILD_TYPE="Release"
	assert_success

	make -j$(nproc --all) libcef_dll_wrapper

	silent_popd
fi

if [[ $_rapidjson_in_include == false ]]; then
	echo "[*] Downloading rapidjson..."

	silent_pushd "$PWD/thirdparty/"

	if [[ ! -d rapidjson ]]; then
		git clone https://github.com/Tencent/rapidjson.git rapidjson
		assert_success
	fi

	silent_popd
fi

if [[ $_cef_path == "" ]] || [[ $_rapidjson_in_include == false ]]; then
	echo "[*] Creating include directory..."

	if [[ $_cef_path == "" ]]; then
		cp -r "$PWD/thirdparty/cef/include/" "$PWD/thirdparty/"
	fi

	if [[ $_rapidjson_in_include == false ]]; then
		cp -r "$PWD/thirdparty/rapidjson/include/" "$PWD/thirdparty/"
	fi
fi

echo "[*] Copying CEF resources and libs..."

if [[ $_cef_path == "" ]]; then
	cp -fr "$PWD/thirdparty/cef/Release/"* "$PWD/bin/"
	cp -fr "$PWD/thirdparty/cef/Resources/"* "$PWD/bin/"
else
	cp -frs "$_cef_path/Release/"* "$PWD/bin/"
	cp -frs "$_cef_path/Resources/"* "$PWD/bin/"
fi

find "$PWD/bin/" -type f -name "*.so" -exec strip {} +
strip "$PWD/bin/chrome-sandbox"

echo "[*] Creating makefile..."
./scripts/create_makefile.sh "$@"

echo "[*] We are done!"
echo -e "[*] Use \e[1m\e[94mmake -j$(nproc --all)\e[39m\e[21m to compile"
