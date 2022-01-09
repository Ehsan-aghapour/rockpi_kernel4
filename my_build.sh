
export Rockpi_cross_compiler_path=/home/ehsan/UvA/DRLPM/Master_Backup/NUS/android-ndk-r15c/toolchains/aarch64-linux-android-4.9/prebuilt/linux-x86_64/bin/aarch64-linux-android-
export CROSS_COMPILE=${Rockpi_cross_compiler_path}
export ARCH=arm64
#make rockchip_defconfig
make menuconfig
make rk3399pro-rockpi-n10-v1.3-android.img -j$(nproc)
