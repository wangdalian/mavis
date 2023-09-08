image_version=0.0.1

docker build -t wangdalian/mavis-riscv32 -f Dockerfile.riscv32 .
docker tag wangdalian/mavis-riscv32 wangdalian/mavis-riscv32:${image_version}