#!/bin/sh

# get cbp2make from here:
# https://github.com/dmpas/cbp2make

SCRIPT_DIR=`dirname $0`
ROOT_DIR="${SCRIPT_DIR}/.."

cbp2make -in ${ROOT_DIR}/thinker.cbp -out ${ROOT_DIR}/Makefile_dll
patch -u ${ROOT_DIR}/Makefile_dll -i ${SCRIPT_DIR}/Makefile_dll.patch

echo "=============================================="
cbp2make -in ${ROOT_DIR}/thinkerlaunch.cbp -out ${ROOT_DIR}/Makefile_exe
patch -u ${ROOT_DIR}/Makefile_exe -i ${SCRIPT_DIR}/Makefile_exe.patch

pushd "${ROOT_DIR}"
echo "=============================================="
make -j`nproc` -f Makefile_dll

echo "=============================================="
make -j`nproc` -f Makefile_exe

rm Makefile_exe Makefile_dll
popd
