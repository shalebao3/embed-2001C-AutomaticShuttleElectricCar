# Start：启动与内核支持

本目录只负责 STM32F103C8T6 的启动、CMSIS 兼容和链接布局，不放业务外设初始化。

- `startup.cmake`：统一选择 CMSIS、system 和 GNU 启动文件。
- `cmsis-compat.cmake`：针对固定旧版 CMSIS 的 GNU 构建兼容处理，只生成构建目录副本。
- `STM32F103xx_FLASH.ld`：64KB Flash / 20KB RAM 链接脚本。

第三方标准库仍固定在 `Libraries/STM32F10x_StdPeriph_Lib` 子模块中，不复制源码到本目录。
