# STM32F103C8T6 启动支持的统一构建入口。
# 调用前：STDPERIPH_ROOT 已指向仓库内固定版本的 vendor 标准库。
# 输出：STM32_STARTUP_SOURCES、STM32_STARTUP_INCLUDE_DIRS。
# 厂商源码由 firmware/Libraries 中的 vendor 目录统一管理，本目录不维护第二套副本。
set(CMSIS_CORE "${STDPERIPH_ROOT}/Libraries/CMSIS/CM3/CoreSupport")
set(CMSIS_DEVICE "${STDPERIPH_ROOT}/Libraries/CMSIS/CM3/DeviceSupport/ST/STM32F10x")
set(STARTUP_FILE "${CMSIS_DEVICE}/startup/TrueSTUDIO/startup_stm32f10x_md.s")

include("${CMAKE_CURRENT_LIST_DIR}/cmsis-compat.cmake")

set(STM32_STARTUP_SOURCES
    "${CMSIS_CORE_SOURCE}"
    "${CMSIS_DEVICE}/system_stm32f10x.c"
    "${STARTUP_FILE}"
)
set(STM32_STARTUP_INCLUDE_DIRS
    "${CMSIS_CORE_INCLUDE}"
    "${CMSIS_CORE}"
    "${CMSIS_DEVICE}"
)

foreach(_startup_source IN LISTS STM32_STARTUP_SOURCES)
    if(NOT EXISTS "${_startup_source}")
        message(FATAL_ERROR "启动支持文件不存在：${_startup_source}，请检查固定版本 vendor 标准库目录。")
    endif()
endforeach()
unset(_startup_source)
