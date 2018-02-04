#!/bin/bash

function silent_pushd() {
	pushd $1 > /dev/null
}

function silent_popd() {
	popd > /dev/null
}

function assert_success() {
	if [[ $? != 0 ]]; then
		echo "Terminating."
		silent_popd
		exit -1
	fi
}

export _cef_path=""
export _rapidjson_in_include=false

for arg in $@; do
	if [[ $arg == "--cef-path="* ]]; then
		export _cef_path=$(echo $arg | cut -c 12-)
		if [[ ! -d $_cef_path ]]; then
			echo "Invalid cef path!"
			exit -1
		fi
	elif [[ $arg == "--rapidjson-installed" ]]; then
		export _rapidjson_in_include=true
		if [[ ! -d "/usr/include/rapidjson" ]]; then
			echo "--rapidjson-installed is defined but cannot find directory in /usr/include!"
			exit -1
		fi
	fi
done

echo "[*] Creating directories..."

mkdir -p $(pwd)"/bin/"
#rm -rf $(pwd)"/bin/"
#mkdir -p $(pwd)"/bin/"
assert_success

mkdir -p $(pwd)"/thirdparty/"
#rm -rf $(pwd)"/thirdparty/"
#mkdir -p $(pwd)"/thirdparty/"
assert_success

mkdir -p $(pwd)"/thirdparty/cef"
assert_success

if [[ $_cef_path == "" ]]; then
	echo "[*] Downloading and building CEF api..."

	if [[ ! -f $(pwd)"/thirdparty/cef_binary.tar.bz2" ]]; then
		wget --quiet --show-progress -O $(pwd)"/thirdparty/cef_binary.tar.bz2" $(printf "http://opensource.spotify.com/cefbuilds/%s" $(curl -s http://opensource.spotify.com/cefbuilds/index.html | grep -oP --regex="cef_binary_3.*?_linux64.tar.bz2" | head -1))
		assert_success
	fi

	if [[ ! -d $(pwd)"/thirdparty/cef/include/" ]]; then
		echo "[*] Uncompressing..."

		tar -vxjf $(pwd)"/thirdparty/cef_binary.tar.bz2" -C $(pwd)"/thirdparty/cef/"

		archivedir=$(pwd)"/thirdparty/cef/"$(ls -1 $(pwd)"/thirdparty/cef/" | head -1)"/"
		mv $archivedir* $(pwd)"/thirdparty/cef/"
		assert_success
		rmdir $archivedir
		assert_success
	fi

	silent_pushd $(pwd)"/thirdparty/cef"
	assert_success

	cmake . -DCMAKE_BUILD_TYPE="Release"
	assert_success

	make -j`nproc --all` libcef_dll_wrapper

	silent_popd
fi

if [[ $_rapidjson_in_include == false ]]; then
	echo "[*] Downloading rapidjson..."

	silent_pushd $(pwd)"/thirdparty/"

	if [[ ! -d  rapidjson ]]; then
		git clone https://github.com/Tencent/rapidjson.git rapidjson
		assert_success
	fi

	silent_popd
fi

if [[ $_cef_path == "" ]] || [[ $_rapidjson_in_include == false ]]; then
	echo "[*] Creating include directory..."

	if [[ $_cef_path == "" ]]; then
		cp -r $(pwd)"/thirdparty/cef/include/" $(pwd)"/thirdparty/"
	fi

	if [[ $_rapidjson_in_include == false ]]; then
		cp -r $(pwd)"/thirdparty/rapidjson/include/" $(pwd)"/thirdparty/"
	fi
fi

echo "[*] Copying CEF resources and libs..."

if [[ $_cef_path == "" ]]; then
	cp -fr $(pwd)"/thirdparty/cef/Release/"* $(pwd)"/bin/"
	cp -fr $(pwd)"/thirdparty/cef/Resources/"* $(pwd)"/bin/"
else
	cp -frs "$_cef_path/Release/"* $(pwd)"/bin/"
	cp -frs "$_cef_path/Resources/"* $(pwd)"/bin/"
fi

echo "[*] Creating makefile..."
./scripts/create_makefile.sh $@

echo "[*] We are done!"
echo -e "[*] Use \e[1m\e[94mmake -j`nproc --all`\e[39m\e[21m to compile"
