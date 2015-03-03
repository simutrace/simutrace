#!/bin/sh
mkdir -p build/documentation
doxygen simutrace/documentation/PublicApi.doxyfile
cp -rv simutrace/documentation/theme/. build/documentation/html/ 
