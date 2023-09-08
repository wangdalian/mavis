#!/bin/bash
set -ex

docker_cmd="docker"
image_version="0.0.1"
compile_image=${1:-"wangdalian/mavis-riscv32:${image_version}"}

work_dir=`pwd`

${docker_cmd} run --rm \
  -v ${work_dir}:/root/mavis \
  -w /root/mavis \
  ${compile_image} \
  /usr/bin/make

echo "make done"