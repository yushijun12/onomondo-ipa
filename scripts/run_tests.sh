#!/bin/bash
# IPAd 自动化测试脚本
# 用于编译和运行所有测试

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build_test"

echo "=========================================="
echo "IPAd 自动化测试脚本"
echo "=========================================="

# 清理旧的构建目录
if [ -d "${BUILD_DIR}" ]; then
    echo "清理旧的构建目录..."
    rm -rf "${BUILD_DIR}"
fi

# 创建构建目录
mkdir -p "${BUILD_DIR}"
cd "${BUILD_DIR}"

# 配置
echo ""
echo "配置 CMake..."
cmake .. \
    -DENABLE_SANITIZE=ON \
    -DSHOW_ASN_OUTPUT=OFF \
    -DMEM_EMIT_DEBUG=ON \
    -DCMAKE_BUILD_TYPE=Debug

# 编译
echo ""
echo "编译项目..."
cmake --build . -j$(nproc)

# 运行测试
echo ""
echo "运行测试..."
ctest --output-on-failure

# 运行单元测试
echo ""
echo "运行缓冲区单元测试..."
./tests/unit/test_ipa_buf

# 显示内存统计（如果启用）
echo ""
echo "=========================================="
echo "测试完成!"
echo "=========================================="

# 可选：生成代码覆盖率报告
if command -v gcov &> /dev/null && [ -f "CMakeFiles/CMakeDirectoryInformation.cmake" ]; then
    echo ""
    echo "生成代码覆盖率报告..."
    # 这里可以添加 gcov/lcov 命令
fi

exit 0
