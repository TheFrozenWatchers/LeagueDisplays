#!/bin/bash

cd src
find . -type f -exec sed -i 's/\t/    /g' {} +
cd ..
