# STM32F10x Standard Peripheral Library (vendored)

此目录是模板仓库直接跟踪的第三方固定版本源码，不是 Git Submodule。

- 版本：STM32F10x Standard Peripheral Library V3.5.0
- 上游：https://github.com/wajatimur/stm32f10x-stdperiph-lib
- 上游提交：`afa743577f2784e95be2d5003380fdb84a702519`
- 引入范围：标准外设驱动 `inc/src`，以及 STM32F103C8T6 构建所需的最小 CMSIS / startup 文件
- 文本编码：仓库统一存储为 UTF-8；`VENDOR_MANIFEST.json` 同时记录上游与本仓库 blob SHA

## 约束

- 业务开发不要修改本目录中的第三方源码。
- 如需升级标准库，应从上游重新导入并同步更新 `VENDOR_MANIFEST.json`。
- CI 会依据 manifest 中的 vendored blob SHA 校验全部 vendor 文件，防止模板项目误改标准库。
