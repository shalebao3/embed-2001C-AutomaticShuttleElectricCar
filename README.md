# STM32F103C8T6 电赛标准库工程模板

用于后续电子设计竞赛项目的 STM32F103C8T6 基础脚手架。

当前 `main` 固定为：

- MCU：STM32F103C8T6
- STM32F10x Standard Peripheral Library V3.5.0
- arm-none-eabi-gcc
- CMake + Ninja
- VSCode + Cortex-Debug
- ST-Link
- 不使用 CubeMX
- 不使用 HAL

模板只保留可跨题复用的工程基础设施。ADC、TIM、DMA、量程、电机等具体业务配置不预置，避免上一道题的实现污染下一道题。

## 目录分层

```text
.
├── .github/
│   └── workflows/
│       └── firmware-build.yml
├── .vscode/
├── firmware/
│   ├── Start/
│   │   ├── startup.cmake
│   │   ├── cmsis-compat.cmake
│   │   └── STM32F103xx_FLASH.ld
│   ├── Libraries/
│   │   └── STM32F10x_StdPeriph_Lib
│   ├── cmake/
│   ├── src/
│   │   ├── User/
│   │   ├── App/
│   │   ├── Driver/
│   │   ├── Bsp/
│   │   └── Common/
│   ├── CMakeLists.txt
│   └── CMakePresets.json
├── tests/
└── README.md
```

### 分层职责

| 目录 | 职责 |
| --- | --- |
| `Start` | 启动文件、CMSIS 兼容、链接脚本 |
| `User` | main、中断入口、标准库配置 |
| `App` | 赛题业务流程、状态机、算法编排 |
| `Driver` | STM32 片内 ADC/TIM/DMA/UART 等驱动 |
| `Bsp` | 板级 GPIO 与外部器件映射 |
| `Common` | 与赛题无关的通用组件 |

当前 `Common` 预置 `Com_Time`，统一提供 1ms SysTick 时间基准。

## 为什么不把 2011G 的 ADC 和自动量程一起带进来

2011G 的 `Driver_ADC` 固定了 ADC1、PA0、Channel 0、软件触发和轮询 EOC；`bsp_Range` 又固定了继电器量程引脚。这些都属于具体赛题实现，不是模板基础设施。

下一道题可能需要：

- TIM 触发 ADC
- ADC + DMA Circular
- 多通道扫描
- 输入捕获
- PWM
- 编码器接口

因此模板只保留分层位置，不预设外设方案。

## 首次使用

```bash
git submodule update --init --recursive
```

确认工具链：

```bash
arm-none-eabi-gcc --version
cmake --version
ninja --version
```

## 构建

从仓库根目录执行：

```bash
cmake -S firmware -B firmware/build/Debug -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_TOOLCHAIN_FILE=firmware/cmake/gcc-arm-none-eabi.cmake

cmake --build firmware/build/Debug
```

构建输出：

```text
firmware/build/Debug/
├── stm32f103_std_template.elf
├── stm32f103_std_template.hex
├── stm32f103_std_template.bin
└── stm32f103_std_template.map
```

GitHub Actions 会同时验证 Debug / Release，并检查 CMSIS 兼容处理、源码分层和 BIN/HEX/MAP 产物。

## 新电赛题的推荐起步方式

1. 通过 GitHub 的 **Use this template** 创建新的题目仓库。
2. 修改 `CMAKE_PROJECT_NAME` 为题目仓库名。
3. 在 `App/` 创建业务入口，例如 `App_xxx.c/.h`。
4. 按实际方案在 `Driver/` 增加 ADC、TIM、DMA 等驱动。
5. 按实际硬件在 `Bsp/` 增加板级控制。
6. 在 `main.c` 中完成初始化，并在主循环调用 `App_xxx_Task()`。
7. 本地 Debug 构建通过后再提交，由 GitHub Actions 再验证 Debug / Release。

CMake 已使用 `CONFIGURE_DEPENDS` 自动发现上述源码目录中新加入的 `.c` 文件，因此通常不需要每增加一个模块就手工修改源码列表。

## 架构原则

```text
main / IRQ
    │
    ▼
   App
  / | \
 ▼  ▼  ▼
Driver Bsp Common
   \   |   /
    标准外设库
        │
        ▼
      寄存器
        │
        ▼
      STM32
```

核心边界：

- App 决定“做什么”。
- Driver 决定“STM32 片内外设怎么工作”。
- Bsp 决定“当前板子具体接到哪里、怎么驱动外部器件”。
- Common 只放真正跨题通用能力。
- 不修改固定版本标准库子模块源码。

## GitHub Template Repository

建议在仓库 Settings → General 中勾选 **Template repository**。

这样以后不要复制旧项目历史，也不需要重新搭工程：

```text
embed-stm32c8t6-template
        ↓
Use this template
        ↓
embed-xxxx-xxxx
        ↓
App + Driver + Bsp
```
