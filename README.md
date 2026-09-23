# 2001C 自动往返电动小汽车

基于 STM32F103C8T6 + STM32F10x Standard Peripheral Library V3.5.0 的电赛项目。

当前目标不是一次性完成整车，而是按“PWM 输出 → 电机驱动 → 编码器测速 → 速度纠偏 → 黑线位置识别 → 自动往返状态机”的顺序逐步实现，并在每一步完成独立验证。

## 当前技术基线

- MCU：STM32F103C8T6
- 标准库：STM32F10x Standard Peripheral Library V3.5.0
- 工具链：arm-none-eabi-gcc
- 构建：CMake + Ninja
- 调试：VSCode + Cortex-Debug + ST-Link
- 不使用 CubeMX
- 不使用 HAL
- 当前阶段优先掌握 STM32 GPIO、TIM PWM、Encoder Mode 等片上外设
- 不为了分层形式强制要求 `Driver/` 存在；具体硬件模块可直接放在 `Bsp/`

## 当前总体方案

### 1. 底盘与驱动

采用左右两轮独立驱动的差速小车：

```text
                STM32F103C8T6
                     │
          ┌──────────┴──────────┐
          │                     │
     左电机 PWM              右电机 PWM
          │                     │
          ▼                     ▼
        H 桥                  H 桥
          │                     │
          ▼                     ▼
      左直流减速电机        右直流减速电机
          │                     │
          ▼                     ▼
        左轮                  右轮
```

电机计划使用带 AB 相编码器的有刷直流减速电机。

H 桥驱动芯片目前尚未确定，因此正转、反转、刹车相关 GPIO 和逻辑暂不写死。

---

### 2. TIM1 双路 PWM

当前已实现：

```text
TIM1_CH1 → PA8 → 左电机 PWM
TIM1_CH2 → PA9 → 右电机 PWM
```

PWM 参数：

```text
TIM1 Clock = 72 MHz
PSC = 0
ARR = 3599
```

因此：

$$
f_{PWM}
=
\frac{72MHz}{(0+1)(3599+1)}
=
20kHz
$$

左右通道共用 TIM1 的：

```text
PSC
ARR
CNT
```

因此 PWM 频率一致。

左右速度分别通过：

```text
CCR1 → 左电机占空比
CCR2 → 右电机占空比
```

独立调整。

BSP 对外统一使用：

```text
0    → 0%
500  → 50%
1000 → 100%
```

占空比换算：

$$
compare
=
\frac{(ARR+1)\times duty}{1000}
$$

例如：

```text
duty = 500
ARR = 3599
```

则：

$$
compare
=
\frac{3600\times500}{1000}
=
1800
$$

对应约 50% PWM。

当前实现文件：

```text
firmware/src/Bsp/bsp_Motor.c
firmware/src/Bsp/bsp_Motor.h
```

### 当前验证状态

软件配置已完成，但示波器尚未到货。

暂时按以下结果继续开发，后续必须补实测：

| 引脚 | 预期频率 | 预期占空比示例 |
| --- | ---: | ---: |
| PA8 / TIM1_CH1 | 20 kHz | 50% |
| PA9 / TIM1_CH2 | 20 kHz | 75% |

示波器到货后需要验证：

- PA8、PA9 是否真实输出 PWM
- PWM 频率是否约为 20 kHz
- 修改 CCR1 / CCR2 时，占空比是否独立变化
- 修改 CCR 时 PWM 频率是否保持不变

---

### 3. 电机方向控制

PWM 只负责调速，正反转由 H 桥方向输入控制。

逻辑抽象为：

```text
GPIO → H桥方向输入 → 电机正反转
TIM1 PWM → H桥 PWM/EN → 电机速度
```

最终 BSP 预计提供：

```c
Bsp_Motor_SetLeftDuty(...);
Bsp_Motor_SetRightDuty(...);

Bsp_Motor_Forward();
Bsp_Motor_Reverse();
Bsp_Motor_Stop();
```

具体引脚和高低电平逻辑等 H 桥型号确定后再实现。

---

### 4. 左右轮编码器

不依赖侧边挡板作为方向基准。

计划给左右有刷直流减速电机分别使用 AB 相正交编码器。

资源规划：

```text
左编码器：
A → TIM2_CH1
B → TIM2_CH2
TIM2 → Encoder Mode

右编码器：
A → TIM3_CH1
B → TIM3_CH2
TIM3 → Encoder Mode
```

Timer Encoder Mode 负责根据 A/B 两相信号的状态变化顺序自动：

```text
正方向 → CNT++
反方向 → CNT--
```

CPU 不逐脉冲处理中断。

累计 CNT 可用于估算轮子转角和行驶距离。

单位时间内的 CNT 增量：

$$
\Delta CNT
=
CNT_{now}
-
CNT_{last}
$$

可用于表示轮速。

第一阶段速度控制不要求立即换算成 RPM，可直接使用：

```text
counts / 10ms
```

作为速度反馈量。

---

### 5. 速度采样

计划使用 TIM4 产生固定周期任务，例如每 10 ms：

```text
TIM4
 ↓
每 10 ms
 ↓
读取 TIM2 CNT
读取 TIM3 CNT
 ↓
计算左右 ΔCNT
 ↓
得到左右轮速度
```

资源初步规划：

| 功能 | STM32 资源 | 引脚/说明 |
| --- | --- | --- |
| 左电机 PWM | TIM1_CH1 | PA8 |
| 右电机 PWM | TIM1_CH2 | PA9 |
| 左编码器 | TIM2_CH1 + CH2 | PA0 + PA1（默认映射） |
| 右编码器 | TIM3_CH1 + CH2 | PA6 + PA7（默认映射） |
| 速度采样周期 | TIM4 | 不需要输出引脚 |
| 电机方向 | GPIO | 待 H 桥型号确定 |
| 黑线检测 | GPIO 输入 | 待传感器确定 |

当前规划尽量使用默认复用功能，暂不引入 AFIO Remap。

---

### 6. 小车直行与方向纠偏

不采用“测左右挡板距离”作为主方案，避免控制依赖比赛环境。

第一阶段先验证：

```text
左 PWM = 右 PWM
```

时实际是否能大致直行。

如果左右轮机械差异导致跑偏，先做简单开环标定，例如：

```text
左 PWM = 58%
右 PWM = 60%
```

随后加入编码器反馈。

基础纠偏思路：

```text
左轮 ΔCNT > 右轮 ΔCNT
        ↓
左轮偏快
        ↓
左 PWM ↓ / 右 PWM ↑
```

反之亦然。

第一版先使用简单阈值或比例修正，不立即引入完整 PID。

需要注意：

> 左右轮编码器速度相等，不代表小车一定长期保持绝对航向不漂移。

轮径误差、轮胎打滑、安装误差和地面摩擦差异都会造成累计航向误差。

因此 IMU 作为后续备选方案：只有实车验证发现编码器速度闭环仍无法满足直行精度时，再考虑增加陀螺仪航向反馈。

当前阶段不提前引入 IMU、姿态解算或多环 PID。

---

### 7. 黑线检测与位置识别

赛道横向黑线主要用于：

> 识别小车“走到了哪个位置”。

它不承担连续方向控制。

第一版计划使用带数字比较输出的反射式红外传感器：

```text
地面黑/白
   ↓
反射式红外传感器
   ↓
数字输出
   ↓
STM32 GPIO Input
```

初版优先采用 GPIO 轮询，不立即使用 EXTI。

软件需要检测：

```text
WHITE → BLACK
```

这个状态变化，而不是在黑线上持续累加，否则同一条 2 cm 黑线会被重复计数。

位置判断采用：

```text
行驶方向
+
已经过的黑线序号
+
当前黑线事件
```

共同确定。

后续如数字阈值不稳定，再考虑 ADC 读取原始反射强度和软件阈值。

---

### 8. 自动往返状态机

最终业务流程计划由 App 层管理。

核心状态顺序：

```text
起点
 ↓
高速前进
 ↓
进入 D~E 限速区
 ↓
降低 PWM
 ↓
离开限速区
 ↓
恢复高速
 ↓
到达终点
 ↓
停车
 ↓
等待 10 秒
 ↓
反向
 ↓
原路返回
```

状态机只负责决定：

```text
现在该前进 / 后退 / 停止
现在使用高速 / 低速
当前位置是什么
```

具体 PWM、GPIO、编码器等硬件操作留在 BSP / 底层模块。

---

## 当前开发进度

### 已完成

- STM32F103C8T6 标准库工程
- CMake + Ninja 构建
- GitHub Actions Debug / Release 构建
- `bsp_Motor.c/.h`
- TIM1_CH1 / CH2 双路 PWM 配置
- PA8 / PA9 复用推挽输出
- PWM 频率设计为 20 kHz
- 左右占空比独立接口
- 对 `GPIO_Init()`、`GPIOA_CRH`、CNF/MODE、RCC/APB2 地址映射进行了源码与 RM0008 对照

### 待验证

- PA8 / PA9 实际 PWM 波形
- 20 kHz 实测频率
- 50% / 75% 等占空比实测

### 下一步

1. 进入 TIM2 Encoder Mode，先完成左编码器配置。
2. 再完成 TIM3 右编码器。
3. 使用 TIM4 建立 10 ms 固定速度采样周期。
4. 读取左右 `ΔCNT`，先不做 PID，只验证测速。
5. H 桥型号确定后补充正反转与停车 GPIO。
6. 接真实电机，验证左右独立调速与编码器反馈。
7. 加黑线检测。
8. 最后实现自动往返状态机。

---

## 软件分层

当前项目按需求使用以下分层：

```text
main / IRQ
    │
    ▼
   App
  / | \
 ▼  ▼  ▼
Bsp Driver Common
   \   |   /
    标准外设库
        │
        ▼
      寄存器
        │
        ▼
      STM32
```

其中 `Driver` 为按需层，不要求为了目录完整性强行创建。

当前电机模块直接放在：

```text
Bsp/
└── bsp_Motor.c/.h
```

因为它已经包含：

```text
TIM1_CH1 = 左电机
TIM1_CH2 = 右电机
```

这种明确的板级硬件语义。

如果后续出现多个模块需要复用通用 PWM 驱动，再考虑抽出：

```text
Driver_PWM
```

避免为了分层而提前抽象。

---

## 目录结构

```text
.
├── .github/
│   └── workflows/
│       └── firmware-build.yml
├── .vscode/
├── firmware/
│   ├── Start/
│   ├── Libraries/
│   │   └── STM32F10x_StdPeriph_Lib/
│   ├── cmake/
│   ├── src/
│   │   ├── User/
│   │   ├── App/
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
| `App` | 自动往返流程、状态机、控制策略 |
| `Driver` | 按需存在；纯 STM32 片内外设通用驱动 |
| `Bsp` | 板级 GPIO、PWM、电机和外部器件映射 |
| `Common` | 与赛题无关的通用组件 |

当前 `Common` 预置 `Com_Time`，统一提供 1 ms SysTick 时间基准。

## 标准库管理

`firmware/Libraries/STM32F10x_StdPeriph_Lib` 作为普通 Git tracked files 固化在仓库中，不使用 Git Submodule。

标准库来源与固定上游提交记录在：

```text
VENDOR_INFO.md
VENDOR_MANIFEST.json
```

业务开发不要修改 vendor 标准库源码。

## 构建

从仓库根目录执行：

```bash
cmake -S firmware -B firmware/build/Debug -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_TOOLCHAIN_FILE=firmware/cmake/gcc-arm-none-eabi.cmake

cmake --build firmware/build/Debug
```

当前 CMake 目标名仍为：

```text
stm32f103_std_template
```

因此构建产物位于：

```text
firmware/build/Debug/
├── stm32f103_std_template.elf
├── stm32f103_std_template.hex
├── stm32f103_std_template.bin
└── stm32f103_std_template.map
```

GitHub Actions 同时验证 Debug / Release，并检查 CMSIS 兼容、源码目录结构以及 BIN/HEX/MAP 产物。

## 当前原则

- 先让每个 STM32 外设单独可验证，再组合整车逻辑。
- 当前项目主线是 STM32 外设能力，不提前堆 RS485、CAN、IMU、复杂通信或多环控制。
- PWM 先验证频率和占空比，再接 H 桥和电机。
- 编码器先验证 CNT，再计算速度，再做反馈。
- 黑线用于位置校正，不作为持续方向基准。
- 不依赖侧边挡板进行方向控制。
- PID 不是当前前置条件；只有简单反馈不足时再引入。
- 项目代码为主线；标准库源码用于理解封装；RM0008 用于确认硬件依据。
- 寄存器追到“能解释当前代码”为止，不继续无收益深挖。
