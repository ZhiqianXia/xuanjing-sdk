# CI/CD Pipeline Configuration

本文档说明了Xuanjing SDK项目的持续集成和持续部署（CI/CD）流程。

## 概述

项目使用GitHub Actions自动化以下任务：
- ✅ 多配置编译和测试
- ✅ 代码质量检查（linting）
- ✅ 代码覆盖率分析
- ✅ 安全检查和依赖分析
- ✅ 性能基准测试
- ✅ 变更日志验证

## 工作流说明

### 1. Main CI Pipeline (`ci.yml`)

在**push到main/master/dev分支**或**创建PR到main/master分支**时触发。

#### 构建和测试矩阵
使用3种不同的CMake预设编译：

| 预设 | 特性 | 用途 |
|------|------|------|
| `dev` | Debug + 完整警告 | 开发构建，检测问题 |
| `release` | Release优化 + 示例 | 性能测试，发布验证 |
| `asan` | AddressSanitizer + UBSan | 内存错误检测 |

**构建步骤：**
```bash
cmake --preset <preset> -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_C_COMPILER=clang
cmake --build --preset <preset> -j$(nproc)
ctest --preset <preset> --output-on-failure
```

#### Lint检查
运行`tools/lint.sh`脚本，包括：
- `clang-format`: 代码格式检查
- `clang-tidy`: 静态分析和最佳实践检查

**配置文件：**
- [.clang-format](.clang-format) - 格式规则
- [.clang-tidy](.clang-tidy) - 代码检查规则

#### 代码覆盖率
运行`tools/coverage.sh`脚本：
- 使用lcov生成覆盖率报告
- 生成HTML覆盖率报告
- 上传为工件供查看

**输出位置：** `build-coverage/coverage-report/index.html`

#### Changelog验证
PR必须更新`CHANGELOG.md`文件，否则CI失败。

### 2. Security Pipeline (`security.yml`)

在**push、PR或每周一次**触发安全检查。

#### CodeQL分析
使用GitHub CodeQL引擎进行C++代码安全扫描，检测：
- 内存错误
- 注入漏洞
- 资源泄漏
- 安全弱点

#### 依赖检查
检查第三方依赖的已知漏洞。

### 3. Performance Pipeline (`benchmarks.yml`)

在**push到main/master分支**或**非草稿PR**时运行性能基准测试。

- 编译优化的Release版本
- 运行性能基准测试
- 保存基准结果作为工件

## 本地测试

在提交前，在本地运行相同的检查：

```bash
# 安装依赖
sudo apt-get install -y ninja-build cmake clang clang-tidy clang-format lcov gcc g++

# 运行Lint检查
bash tools/lint.sh

# 运行Dev构建测试
cmake --preset dev
cmake --build --preset dev -j
ctest --preset dev --output-on-failure

# 运行内存检查
cmake --preset asan
cmake --build --preset asan -j
ctest --preset asan --output-on-failure

# 生成覆盖率报告
bash tools/coverage.sh
```

## CMake预设

详见[CMakePresets.json](CMakePresets.json)，包括：

- `dev` - 调试构建，启用所有警告和测试
- `release` - 发布构建，优化性能
- `asan` - AddressSanitizer检测
- `coverage` - 代码覆盖率分析

## 缓存策略

CI流程使用GitHub Actions缓存加速构建：
- APT包缓存：避免重复下载依赖
- CMake构建缓存：基于CMakeLists.txt和预设的变化

## 关键文件

| 文件 | 说明 |
|------|------|
| `.github/workflows/ci.yml` | 主CI流程 |
| `.github/workflows/security.yml` | 安全检查 |
| `.github/workflows/benchmarks.yml` | 性能测试 |
| `.clang-format` | 代码格式规则 |
| `.clang-tidy` | 代码检查配置 |
| `CMakePresets.json` | 构建预设定义 |
| `tools/lint.sh` | Lint脚本 |
| `tools/coverage.sh` | 覆盖率生成脚本 |
| `cmake/CompilerWarnings.cmake` | 编译器警告设置 |
| `cmake/Sanitizers.cmake` | Sanitizer配置 |
| `cmake/Coverage.cmake` | 覆盖率配置 |

## 常见问题

### Q: 如何跳过CI检查？
A: 在commit信息中添加`[skip ci]`标签（不推荐）。

### Q: 如何只运行特定的工作流？
A: 修改或注释`.github/workflows/`中的触发条件。

### Q: 构建失败了怎么办？
A: 检查工作流日志，通常是：
1. 依赖缺失（更新apt）
2. 编译错误（检查代码）
3. 测试失败（运行本地测试）
4. Lint失败（运行`bash tools/lint.sh`）

### Q: 如何自定义构建？
A: 编辑[CMakePresets.json](CMakePresets.json)或相关工作流文件。

## 扩展CI/CD

### 添加新的测试套件
1. 在`tests/`目录中添加CMakeLists.txt
2. 更新主`CMakeLists.txt`的`XUANJING_BUILD_TESTS`部分
3. CI会自动运行新测试

### 集成外部工具
1. 在工作流中添加安装步骤
2. 在相应的构建步骤中调用工具
3. 根据需要上传结果作为工件

### 添加发布工作流
可创建新的`.github/workflows/release.yml`来：
- 打标签时自动构建
- 生成发布包
- 上传到GitHub Releases

## 参考链接

- [GitHub Actions文档](https://docs.github.com/en/actions)
- [CMake预设文档](https://cmake.org/cmake/help/latest/manual/cmake-presets.7.html)
- [clang-format文档](https://clang.llvm.org/docs/ClangFormat/)
- [clang-tidy文档](https://clang.llvm.org/extra/clang-tidy/)
