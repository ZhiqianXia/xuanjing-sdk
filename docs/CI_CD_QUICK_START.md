# CI/CD 推送指南

将代码推送到GitHub后，CI/CD流程会自动开始运行。本指南说明如何设置、监控和调试CI/CD流程。

## 快速开始

### 1️⃣ 设置本地环境

```bash
# 使用提供的脚本安装所有依赖
bash tools/setup-cicd.sh

# 或手动安装
sudo apt-get install -y cmake ninja-build clang clang-tidy clang-format lcov gcc g++
```

### 2️⃣ 本地验证（推送前）

在提交和推送之前，在本地运行相同的检查：

```bash
# 检查代码格式和静态分析
bash tools/lint.sh

# 快速debug构建测试
cmake --preset dev && cmake --build --preset dev -j && ctest --preset dev

# 内存安全检查（可选，较慢）
cmake --preset asan && cmake --build --preset asan -j && ctest --preset asan
```

### 3️⃣ 推送代码

```bash
# 更新CHANGELOG.md（PR时必需）
git add .
git commit -m "feat: your changes

- Detail 1
- Detail 2"

# 推送到GitHub
git push origin main
```

## GitHub Actions 监控

### 查看工作流状态

1. 访问 GitHub仓库 → **Actions** 标签
2. 选择最新的工作流运行
3. 查看各个job的状态：
   - ✅ 绿色：通过
   - ❌ 红色：失败
   - ⏳ 黄色：运行中

### 工作流详情

| 工作流 | 触发条件 | 耗时 | 作用 |
|--------|---------|------|------|
| **ci.yml** | Push到main/dev，PR到main | 5-10分钟 | 构建、测试、Lint、覆盖率 |
| **security.yml** | Push到main，每周一 | 3-5分钟 | CodeQL扫描、依赖检查 |
| **benchmarks.yml** | Push到main，PR | 2-3分钟 | 性能测试 |

### 下载工件

工作流完成后，可以下载工件：

1. 在Actions页面点击工作流运行
2. 向下滚动到**Artifacts**部分
3. 下载所需的工件（如coverage-report）

## 常见问题排查

### ❌ Lint检查失败

**错误信息：** `clang-format check failed` 或 `clang-tidy errors`

**解决方案：**
```bash
# 本地运行Lint检查查看具体错误
bash tools/lint.sh

# 自动修复格式问题
find modules -name "*.cpp" -o -name "*.h" | xargs clang-format -i
```

**常见原因：**
- 代码格式不一致
- 使用了不推荐的C++特性
- 未包含必要的头文件

### ❌ 构建失败

**解决方案：**

1. 查看CI日志中的完整错误消息
2. 在本地重现构建：
```bash
cmake --preset dev
cmake --build --preset dev 2>&1 | tee build.log
# 检查build.log寻找错误
```

3. 常见原因：
   - 缺少头文件：检查`#include`
   - 链接错误：检查CMakeLists.txt
   - 编译器版本：使用clang++

### ❌ 测试失败

**解决方案：**

```bash
# 本地运行失败的测试
ctest --preset dev -V --rerun-failed

# 或单独运行测试
./build-dev/path/to/test_executable
```

### ❌ Changelog检查失败

**错误：** `ERROR: CHANGELOG.md was not updated in this PR`

**解决方案：**
```bash
# 编辑CHANGELOG.md并提交
# 格式遵循：https://keepachangelog.com/

git add CHANGELOG.md
git commit --amend --no-edit
git push -f
```

### ⏳ 工作流超时

**原因：** 通常是因为infinite loop或资源不足

**解决方案：**
- 检查是否有死循环
- 本地测试是否正常完成
- 检查是否有资源泄漏

## 工作流文件说明

### 主工作流：`.github/workflows/ci.yml`

```yaml
# 触发条件
on:
  push:
    branches: [main, dev, master]
  pull_request:
    branches: [main, master]

# 并发控制：同一分支的多个推送只运行最新的
concurrency:
  group: ${{ github.workflow }}-${{ github.ref }}
  cancel-in-progress: true

# 构建矩阵：同时测试dev、release、asan三种配置
jobs:
  build-and-test:
    strategy:
      matrix:
        preset: [dev, release, asan]
```

### 修改要点

若要自定义CI/CD，可修改：

1. **添加新的构建预设：**
   - 编辑 [CMakePresets.json](../../CMakePresets.json)
   - 在ci.yml的matrix中添加

2. **修改检查工具：**
   - 编辑 [.clang-tidy](.clang-tidy)（静态分析规则）
   - 编辑 [.clang-format](.clang-format)（代码格式）

3. **添加新工作流：**
   - 在`.github/workflows/`创建新的YAML文件
   - 定义触发条件和工作步骤

## 最佳实践

✅ **推荐做法：**
- 在提交前本地运行`bash tools/lint.sh`
- 更新CHANGELOG.md记录所有变更
- 为PR提供有意义的提交信息
- 定期检查Actions工作流日志

❌ **避免：**
- 禁用CI/CD检查
- 跳过本地测试直接推送
- 忽略Lint警告
- 修改CI/CD配置而不测试

## 性能优化建议

1. **利用缓存：**
   - CI已配置APT和CMake缓存
   - 相同hash的CMakeLists.txt将重用构建缓存

2. **并行构建：**
   - 使用`-j$(nproc)`充分利用CPU核心

3. **矩阵并行：**
   - dev、release、asan同时运行
   - 一个失败不影响其他配置

## 获取帮助

遇到问题时的排查步骤：

1. 查看完整的CI日志（Actions页面）
2. 本地重现问题
3. 检查相关文档（见[CI/CD_SETUP.md](../docs/CI_CD_SETUP.md)）
4. 查看提交历史中的类似问题

## 进一步阅读

- [CI/CD详细配置](../docs/CI_CD_SETUP.md)
- [编码标准](../docs/CODING_STANDARDS.md)
- [项目结构](../docs/PROJECT_STRUCTURE.md)
- [GitHub Actions文档](https://docs.github.com/actions)

---

**更新于：** 2026-04-23
