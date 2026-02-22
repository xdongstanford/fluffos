# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

FluffOS is an LPMUD driver based on the last release of MudOS (v22.2b14) with 10+ years of bug fixes and performance enhancements. It supports all LPC-based MUDs with minimal code changes and includes modern features like UTF-8 support, TLS, WebSocket protocol, async IO, and database integration.

## Architecture

### Core Components

**Driver Layer** (`src/`)

- `main.cc` - Entry point for the driver executable
- `backend.cc` - Main game loop and event handling
- `comm.cc` - Network communication layer
- `user.cc` - User/session management
- `symbol.cc` - Symbol table management
- `ofile.cc` - Object file handling

**VM Layer** (`src/vm/`)

- `vm.cc` - Virtual machine initialization and management
- `interpret.cc` - LPC bytecode interpreter
- `simulate.cc` - Simulation engine for object lifecycle
- `master.cc` - Master object management
- `simul_efun.cc` - Simulated external functions

**Compiler Layer** (`src/compiler/`)

- `compiler.cc` - LPC-to-bytecode compiler
- `grammar.y` - Grammar definition (Bison)
- `lex.cc` - Lexical analyzer
- `generate.cc` - Code generation

**Packages** (`src/packages/`)

- Modular functionality organized by feature (async, db, crypto, etc.)
- Each package has `.spec` files defining available functions
- Core packages: core, crypto, db, math, parser, sockets, etc.

**Networking** (`src/net/`)

- `telnet.cc` - Telnet protocol implementation
- `websocket.cc` - WebSocket support for web clients
- `tls.cc` - SSL/TLS encryption support
- `msp.cc` - MUD Sound Protocol support

### Build System

**CMake Configuration** (`CMakeLists.txt`, `src/CMakeLists.txt`)

- CMake 3.22+ required
- C++17 and C11 standards
- Jemalloc for memory management (recommended)
- ICU for UTF-8 support
- OpenSSL for crypto features

**Build Options** (key flags)

- `MARCH_NATIVE=ON` (default) - Optimize for current CPU
- `STATIC=ON/OFF` - Static vs dynamic linking
- `USE_JEMALLOC=ON` - Use jemalloc memory allocator
- `PACKAGE_*` - Enable/disable specific packages
- `ENABLE_SANITIZER=ON` - Enable address sanitizer for debugging

## Build Commands

### Development Build

```bash
# Standard development build
mkdir build && cd build
cmake ..
make -j$(nproc) install

# Debug build with sanitizer
cmake .. -DCMAKE_BUILD_TYPE=Debug -DENABLE_SANITIZER=ON
```

### Production Build

```bash
# Production-ready static build
cmake .. -DCMAKE_BUILD_TYPE=Release -DSTATIC=ON -DMARCH_NATIVE=OFF
make install
```

### Package-Specific Builds

```bash
# Build without database support
cmake .. -DPACKAGE_DB=OFF

# Build with specific packages disabled
cmake .. -DPACKAGE_CRYPTO=OFF -DPACKAGE_COMPRESS=OFF
```

## Testing

### Unit Tests

```bash
# Run all tests (requires GTest)
cd build
make test

# Run specific test executable
./src/lpc_tests
./src/ofile_tests
```

### LPC Tests

```bash
# Run LPC test suite
cd testsuite
../build/bin/driver etc/config.test -ftest
```

### Integration Testing

```bash
# Run driver with test configuration
./build/bin/driver testsuite/etc/config.test
```

## Continuous Integration

FluffOS uses GitHub Actions for comprehensive CI/CD across multiple platforms and configurations.

### CI Matrix

**Ubuntu CI** (`.github/workflows/ci.yml`)

- **Compilers**: GCC and Clang
- **Build Types**: Debug and RelWithDebInfo
- **Platform**: ubuntu-22.04
- **Key Features**: SQLite support, GTest integration
- **Steps**: Install dependencies → CMake configure → Build → Unit tests → LPC testsuite

**macOS CI** (`.github/workflows/ci-osx.yml`)

- **Build Types**: Debug and RelWithDebInfo
- **Platform**: macos-14 (Apple Silicon)
- **Environment Variables**:
  - `OPENSSL_ROOT_DIR=/usr/local/opt/openssl`
  - `ICU_ROOT=/opt/homebrew/opt/icu4c`
- **Dependencies**: cmake, pkg-config, pcre, libgcrypt, openssl, jemalloc, icu4c, mysql, sqlite3, googletest

**Windows CI** (`.github/workflows/ci-windows.yml`)

- **Build Types**: Debug and RelWithDebInfo
- **Platform**: windows-latest with MSYS2/MINGW64
- **Build Options**:
  - `-DMARCH_NATIVE=OFF` (for portability)
  - `-DPACKAGE_CRYPTO=OFF`
  - `-DPACKAGE_DB_MYSQL=""` (disabled)
  - `-DPACKAGE_DB_SQLITE=1`
- **Dependencies**: mingw-w64 toolchain, cmake, zlib, pcre, icu, sqlite3, jemalloc, gtest

**Sanitizer CI** (`.github/workflows/ci-sanitizer.yml`)

- **Purpose**: Memory safety and bug detection
- **Compiler**: Clang only
- **Build Types**: Debug and RelWithDebInfo
- **Special Flag**: `-DENABLE_SANITIZER=ON`
- **Additional Dependencies**: libdw-dev, libbz2-dev

**Docker CI** (`.github/workflows/docker-publish.yml`)

- **Base Image**: Alpine 3.18
- **Build Configuration**: Static linking with `-DSTATIC=ON -DMARCH_NATIVE=OFF`
- **Registry**: GitHub Container Registry (ghcr.io)
- **Triggers**: Push to master, version tags (v*.*), pull requests

**Code Quality CI**

- **CodeQL Analysis** (`.github/workflows/codeql-analysis.yml`): Security vulnerability detection
- **Coverity Scan** (`.github/workflows/coverity-scan.yml`): Static analysis (weekly schedule + push)

**Documentation CI** (`.github/workflows/gh-pages.yml`)

- **Framework**: VitePress
- **Build**: Node.js 20 with npm
- **Deploy**: GitHub Pages
- **Path**: `docs/` directory

### Running CI-Equivalent Builds Locally

**Ubuntu Debug Build (GCC)**

```bash
export CC=gcc CXX=g++
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Debug -DPACKAGE_DB_SQLITE=2 ..
make -j$(nproc) install
make test
cd ../testsuite && ../build/bin/driver etc/config.test -ftest
```

**Ubuntu with Sanitizer (Clang)**

```bash
export CC=clang CXX=clang++
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Debug -DPACKAGE_DB_SQLITE=2 -DENABLE_SANITIZER=ON ..
make -j$(nproc) install
make test
```

**macOS Build**

```bash
mkdir build && cd build
OPENSSL_ROOT_DIR="/usr/local/opt/openssl" ICU_ROOT="/opt/homebrew/opt/icu4c" \
  cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -DPACKAGE_DB_SQLITE=2 ..
make -j$(sysctl -n hw.ncpu) install
make test
```

**Windows Build (MSYS2/MINGW64)**

```bash
mkdir build && cd build
cmake -G "MSYS Makefiles" -DCMAKE_BUILD_TYPE=Debug \
  -DMARCH_NATIVE=OFF -DPACKAGE_CRYPTO=OFF \
  -DPACKAGE_DB_MYSQL="" -DPACKAGE_DB_SQLITE=1 ..
make -j$(nproc) install
```

**Docker Build (Static)**

```bash
docker build -t fluffos:local .
# Or build manually in Alpine container
cmake .. -DMARCH_NATIVE=OFF -DSTATIC=ON
make install
ldd bin/driver  # Should show "not a dynamic executable"
```

### CI Dependencies by Platform

**Ubuntu/Debian**

```bash
sudo apt update
sudo apt install -y build-essential autoconf automake bison expect \
  libmysqlclient-dev libpcre3-dev libpq-dev libsqlite3-dev \
  libssl-dev libtool libz-dev telnet libgtest-dev libjemalloc-dev \
  libdw-dev libbz2-dev  # For sanitizer builds
```

**macOS (Homebrew)**

```bash
brew install cmake pkg-config pcre libgcrypt openssl jemalloc icu4c \
  mysql sqlite3 googletest
```

**Windows (MSYS2 - MINGW64 shell)**

```bash
pacman --noconfirm -S --needed \
  mingw-w64-x86_64-toolchain mingw-w64-x86_64-cmake \
  mingw-w64-x86_64-zlib mingw-w64-x86_64-pcre \
  mingw-w64-x86_64-icu mingw-w64-x86_64-sqlite3 \
  mingw-w64-x86_64-jemalloc mingw-w64-x86_64-gtest \
  bison make
```

**Alpine (Docker/Static)**

```bash
apk add --no-cache linux-headers gcc g++ clang-dev make cmake bash \
  mariadb-dev mariadb-static postgresql-dev sqlite-dev sqlite-static \
  openssl-dev openssl-libs-static zlib-dev zlib-static icu-dev icu-static \
  pcre-dev bison git musl-dev libelf-static elfutils-dev \
  zstd-static bzip2-static xz-static
```

## Development Workflow

### Code Generation

Several source files are auto-generated during build:

- `grammar.autogen.cc/.h` - From `grammar.y` (Bison)
- `efuns.autogen.cc/.h` - From package specifications
- `applies_table.autogen.cc/.h` - From applies definitions
- `options.autogen.h` - From configuration options

### Adding New Functions

1. Add function to appropriate package `.spec` file
2. Implement function in corresponding `.cc` file
3. Run build to regenerate autogenerated files
4. Add tests in `testsuite/` directory

### Debugging

```bash
# Build with debug symbols and sanitizer
cmake .. -DCMAKE_BUILD_TYPE=Debug -DENABLE_SANITIZER=ON

# Run with GDB
gdb --args ./build/bin/driver config.test

# Memory debugging with Valgrind
valgrind --leak-check=full ./build/bin/driver config.test
```

## Platform-Specific Notes

### Ubuntu/Debian

```bash
# Install dependencies
sudo apt install build-essential bison libmysqlclient-dev libpcre3-dev \
  libpq-dev libsqlite3-dev libssl-dev libz-dev libjemalloc-dev libicu-dev \
  libgtest-dev  # For testing
```

### macOS

```bash
# Install dependencies
brew install cmake pkg-config mysql pcre libgcrypt openssl jemalloc icu4c \
  sqlite3 googletest  # Added sqlite3 and googletest for testing

# Build with environment variables (for Apple Silicon)
OPENSSL_ROOT_DIR="/usr/local/opt/openssl" ICU_ROOT="/opt/homebrew/opt/icu4c" cmake ..
# For Intel Macs, use:
# OPENSSL_ROOT_DIR="/usr/local/opt/openssl" ICU_ROOT="/usr/local/opt/icu4c" cmake ..
```

### Windows (MSYS2)

```bash
# Install MSYS2 packages (run in MSYS2 shell, then switch to MINGW64 shell for build)
pacman --noconfirm -S --needed \
  git mingw-w64-x86_64-toolchain mingw-w64-x86_64-cmake \
  mingw-w64-x86_64-zlib mingw-w64-x86_64-pcre \
  mingw-w64-x86_64-icu mingw-w64-x86_64-sqlite3 \
  mingw-w64-x86_64-jemalloc mingw-w64-x86_64-gtest \
  bison make

# Note: PACKAGE_CRYPTO is typically disabled on Windows
# Build in MINGW64 terminal
cmake -G "MSYS Makefiles" -DMARCH_NATIVE=OFF \
  -DPACKAGE_CRYPTO=OFF -DPACKAGE_DB_MYSQL="" -DPACKAGE_DB_SQLITE=1 ..
```

## Key Directories

- `src/` - Main driver source code
- `src/packages/` - Modular package implementations
- `src/vm/` - Virtual machine and interpreter
- `src/compiler/` - LPC compiler
- `src/net/` - Network protocol implementations
- `testsuite/` - LPC test programs and configurations
- `docs/` - Documentation (Markdown and Jekyll)
- `build/` - Build output directory (auto-generated)

## Configuration Files

- `Config.example` - Example driver configuration
- `src/local_options` - Local build options (copy from `local_options.README`)
- `testsuite/etc/config.test` - Test configuration
- Package-specific `.spec` files define available functions

## Common Development Tasks

### Adding a New Package

1. Create directory in `src/packages/[package-name]/`
2. Add `CMakeLists.txt`, `.spec` file, and source files
3. Update `src/packages/CMakeLists.txt`
4. Add tests in `testsuite/`

### Modifying Compiler

1. Edit `src/compiler/grammar.y` for syntax changes
2. Regenerate grammar with Bison if available
3. Update corresponding compiler components

### Debugging Memory Issues

1. Build with `-DENABLE_SANITIZER=ON`
2. Use `mud_status()` efun in LPC for runtime memory info
3. Check `testsuite/log/debug.log` for detailed logs

## XK Package (XKOS Extension)

The XK package (`src/packages/xk/`) is a custom extension providing enhanced PostgreSQL support, JSON handling, and utility functions. It is designed as an alternative to the standard `PACKAGE_DB` for PostgreSQL-focused projects.

### Configuration

To use the XK package, configure `src/local_options`:

```c
#define XKOS 160622
#define PACKAGE_XK
#undef PACKAGE_DB        // Disable standard DB package (conflicts with XK PostgreSQL)
#undef PACKAGE_PCRE      // Optional: disable if not needed
```

Build command:

```bash
cmake .. -DCMAKE_BUILD_TYPE=Debug -DPACKAGE_DB=OFF -DPACKAGE_PCRE=OFF
make -j$(nproc) install
```

### PostgreSQL Functions

The XK package provides a feature-rich PostgreSQL interface using libpq with parameterized queries and async support.

| Function | Description |
|----------|-------------|
| `int pg_connect(string conninfo)` | Connect to PostgreSQL. Returns connection handle (0-9) or -1 on failure |
| `int pg_close(int handle)` | Close a connection. Returns 1 on success |
| `int pg_ok(int handle)` | Check if connection is valid. Returns 1 if OK |
| `mixed pg_call(int handle, string query, ...)` | Execute sync query with parameters. Returns array of results or error string |
| `void pg_send(int handle, string query, ...)` | Send async query (non-blocking) |
| `mixed pg_get(int handle)` | Get results from async query. Returns array, 0 (busy), or error string |
| `string pg_look()` | Debug: show all connection statuses |

**Key Features vs Standard PACKAGE_DB:**

- **Parameterized Queries**: Uses `PQexecParams` with `$1, $2, ...` placeholders for SQL injection protection
- **Async/Streaming Support**: `pg_send`/`pg_get` for non-blocking queries
- **Smart Type Conversion**: Automatically converts PostgreSQL types to LPC types:
  - `INT4/INT8` → LPC int
  - `FLOAT4/FLOAT8/NUMERIC` → LPC float
  - `JSON/JSONB` → Parsed LPC mixed (array/mapping)
  - `BOOL` → LPC int (0/1)
  - Others → LPC string
- **Connection Pooling**: Supports up to 10 concurrent connections (handles 0-9)

**Example Usage:**

```c
// Connect
int db = pg_connect("host=localhost dbname=mydb user=myuser");
if (db < 0) error("Connection failed");

// Parameterized query (safe from SQL injection)
mixed result = pg_call(db, "SELECT * FROM users WHERE id = $1 AND status = $2", 123, "active");

// Async query
pg_send(db, "SELECT * FROM large_table WHERE category = $1", "items");
// ... do other work ...
mixed rows = pg_get(db);  // 0 if still busy, array if done, string if error

// Cleanup
pg_close(db);
```

### JSON Functions

| Function | Description |
|----------|-------------|
| `string json_stringify(mixed data, int strict = 0)` | Convert LPC value to JSON string. Set strict=1 for strict type checking |
| `mixed json_parse(string json)` | Parse JSON string to LPC value (array/mapping/string/int/float) |

**Features:**

- Lightweight pure C implementation (no external library dependency)
- Max input size: 600KB (`JSON_MAX_SIZE`)
- Max nesting depth: 25 levels (`JSON_MAX_DEPTH`)
- Strict mode: optionally error on non-string mapping keys or unsupported types

**Type Mapping:**

```
LPC → JSON:
  T_STRING   → "string"
  T_NUMBER   → 123
  T_REAL     → 1.5 (15 digits precision)
  T_ARRAY    → [...]
  T_MAPPING  → {...}  (string keys only, non-string keys skipped in non-strict mode)
  Other      → null

JSON → LPC:
  "string"   → T_STRING
  123        → T_NUMBER (int)
  1.5        → T_REAL (float)
  true/false → 1/0 (T_NUMBER)
  null       → 0
  [...]      → T_ARRAY
  {...}      → T_MAPPING
```

**XK JSON vs FluffOS Official JSON Support:**

FluffOS does **not** provide JSON efuns. The official JSON support is limited to:

| Feature | FluffOS Official | XK Package |
|---------|------------------|------------|
| LPC efun | **No** | **Yes** ✓ |
| Runtime usage | Not available | Available ✓ |
| Purpose | Save file conversion (CLI tools `o2json`/`json2o`) | General JSON processing |
| Implementation | nlohmann/json (C++) | Pure C parser |
| Use case | Convert `.o` files offline | API calls, config, data exchange |

**Note**: If your MUD needs to process JSON at runtime (API calls, configuration files, data exchange), you must use the XK package or implement similar functionality yourself.

### Utility Functions

| Function | Description |
|----------|-------------|
| `void stdout(string format, ...)` | Print formatted string to stdout |
| `void stderr(string format, ...)` | Print formatted string to stderr |
| `float ftime()` | Get current time as float with microsecond precision |
| `float fuptime()` | Get driver uptime as float with microsecond precision |
| `mixed *shuffle_array(mixed *arr)` | Randomly shuffle array elements (Fisher-Yates algorithm) |

### Debug/System Functions

| Function | Description |
|----------|-------------|
| `void let_us_make_a_crash()` | Intentionally crash the driver (debugging only) |
| `string xk_cmdrc4(string data)` | RC4 encrypt/decrypt string |
| `string malloc_stats_print(string opts)` | Print jemalloc memory statistics |

### XK vs Standard PACKAGE_DB Comparison

| Feature | XK Package | PACKAGE_DB (PostgreSQL) |
|---------|------------|-------------------------|
| Parameterized Queries | Yes (`$1, $2, ...`) | No (string concatenation) |
| Async Queries | Yes (`pg_send`/`pg_get`) | No |
| Type Conversion | Automatic (int, float, JSON) | All strings |
| JSON Auto-Parse | Yes (JSONB → LPC mapping) | No |
| Connection Limit | 10 handles | Configurable |
| Multi-DB Support | PostgreSQL only | MySQL, PostgreSQL, SQLite |
| Security Validation | Direct access | `valid_database` apply |

**Recommendation**: Use XK package for pure PostgreSQL projects requiring parameterized queries, async operations, or automatic JSON handling. Use standard PACKAGE_DB for multi-database projects or when `valid_database` security validation is needed.

## 开发环境

### 路径对应关系

| 用途 | Windows 路径 | Ubuntu (WSL) 路径 |
|------|-------------|------------------|
| FluffOS 源码 | `d:/ai/fluffos` | `/mnt/d/ai/fluffos` |
| Mudlib 代码 | `d:/ai/xktx` | `/mnt/d/ai/xktx` |
| Mudlib 运行目录 | - | `/home/xktx/mud/cross` (符号链接到 `/mnt/d/ai/xktx`) |
| FluffOS 可执行文件 | - | `/home/xktx/bin/driver` |
| 配置文件 | - | `/home/xktx/bin/xklib.conf` |

### WSL 使用方法

使用 WSL Ubuntu 24.04 运行 FluffOS：

```bash
# 在 Windows 中执行 WSL 命令
wsl -d ubuntu-24.04 -- bash -c "命令"
```

### FluffOS 编译

```bash
# 完整重新编译
wsl -d ubuntu-24.04 -- bash -c "cd /mnt/d/ai/fluffos && rm -rf build && mkdir build && cd build && cmake .. -DCMAKE_BUILD_TYPE=Debug -DPACKAGE_DB=OFF -DPACKAGE_PCRE=OFF -DCMAKE_INSTALL_PREFIX=/home/xktx/bin && make -j4 install"

# 增量编译
wsl -d ubuntu-24.04 -- bash -c "cd /mnt/d/ai/fluffos/build && make -j4 install"
```

### 编译类型选择

| 类型 | 优化 | 调试符号 | 速度 | 用途 |
|------|------|----------|------|------|
| `Debug` | 无 (`-O0`) | 完整 | 慢 2-10x | 开发调试 |
| `Release` | 高 (`-O3`) | 无 | 最快 | 生产环境 |
| `RelWithDebInfo` | 高 (`-O2`) | 有 | 快 | 生产+可调试 |

**建议**：开发用 `Debug`，生产用 `RelWithDebInfo`（有优化且保留堆栈跟踪）：

```bash
cmake .. -DCMAKE_BUILD_TYPE=RelWithDebInfo -DPACKAGE_DB=OFF -DPACKAGE_PCRE=OFF -DCMAKE_INSTALL_PREFIX=/home/xktx/bin
```

### 命令行参数

FluffOS driver 支持以下命令行参数：

```bash
driver config_file [-m<mudlib>] [-D<define>] [-p<port>] [-d<debug>] [-f<flag>]
```

| 参数 | 说明 | 示例 |
|------|------|------|
| `-m<path>` | 覆盖 mudlib 目录 | `-m/home/xktx/mud/yitong` |
| `-D<define>` | 添加 LPC 预定义宏 | `-DXKZONE__yitong` 或 `-DXKZONE_PORT=9003` |
| `-p<port>` | 覆盖监听端口 | `-p9003` |
| `-d<level>` | 设置调试级别 | `-d` 或 `-dall` |
| `-f<flag>` | 调用 master::flag() | `-ftest` |

### 运行 MUD

```bash
# 启动 MUD 服务器 (完整示例)
wsl -d ubuntu-24.04 -- bash -c "/home/xktx/bin/driver /home/xktx/bin/xklib.conf -m/home/xktx/mud/cross -DXKZONE__cross -DXKZONE_PORT=9001 -p9001"

# 或使用编译目录的 driver
wsl -d ubuntu-24.04 -- bash -c "/mnt/d/ai/fluffos/build/bin/driver /home/xktx/bin/xklib.conf -m/home/xktx/mud/cross -DXKZONE__cross -p9001"
```

### 注意事项

- 配置文件 `xklib.conf` 必须是 UTF-8 编码
- Mudlib 源文件如包含中文，需使用 UTF-8 编码（或启用 `USE_ICONV`）
- `-m` 参数会覆盖配置文件中的 `mudlib directory` 设置
- `-D` 参数添加的宏可在 LPC 代码中用 `#ifdef` 检测
- PACKAGE_XK 与 PACKAGE_DB 冲突，编译时需禁用 PACKAGE_DB
