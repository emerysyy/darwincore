# ========================================
# MyLib 项目构建脚本
# ========================================
# 功能: 自动化构建 MyLib 项目
# 使用方式:
#   ./build.sh        # Release 模式构建（默认）
#   ./build.sh dev    # Debug 模式构建（生成 Xcode 工程）
# ========================================

# 设置构建类型，默认为 Release
BUILD_TYPE="Release"
if [ x"$1" == x"dev" ]; then
  BUILD_TYPE="Debug"
fi

# 输出当前构建类型
echo "BUILD_TYPE: ${BUILD_TYPE}"

# 清理并重新创建 build 目录
rm -rf build
mkdir build

# 根据构建类型执行不同的构建流程
if [ x"${BUILD_TYPE}" == x"Debug" ]; then
  # Debug 模式：生成 Xcode 工程，便于调试
  pushd build || exit
    cmake -G "Xcode" ..
  popd || exit
else
  # Release 模式：支持 x86_64 和 arm64 双架构编译
  pushd build || exit
      cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_OSX_ARCHITECTURES="x86_64;arm64" ..
      make -j$(sysctl -n hw.ncpu)
      make install
#      make package  # 可选：打包
  popd || exit
fi
