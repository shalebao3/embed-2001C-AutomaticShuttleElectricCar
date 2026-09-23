cmake_minimum_required(VERSION 3.22)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

set(CMAKE_PROJECT_NAME stm32f103_std_template)
project(${CMAKE_PROJECT_NAME} LANGUAGES C ASM)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(MCU_FLAGS
    -mcpu=cortex-m3
    -mthumb
)

set(STDPERIPH_ROOT "${CMAKE_SOURCE_DIR}/Libraries/STM32F10x_StdPeriph_Lib")
set(STDPERIPH_DRIVER "${STDPERIPH_ROOT}/Libraries/STM32F10x_StdPeriph_Driver")

if(NOT EXISTS "${STDPERIPH_DRIVER}/src/stm32f10x_gpio.c")
    message(FATAL_ERROR
        "STM32F10x 固定 vendor 标准库不完整：缺少 stm32f10x_gpio.c。"
        "请从模板仓库恢复 firmware/Libraries/STM32F10x_StdPeriph_Lib。"
    )
endif()

include("${CMAKE_CURRENT_SOURCE_DIR}/Start/startup.cmake")

file(GLOB STDPERIPH_SOURCES CONFIGURE_DEPENDS
    "${STDPERIPH_DRIVER}/src/*.c"
)

# 模板中的业务目录全部纳入自动发现。后续新增 .c 文件时，
# CMake 会因 CONFIGURE_DEPENDS 自动重新配置，不需要逐个维护源码列表。
file(GLOB_RECURSE PROJECT_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/src/User/*.c"
    "${CMAKE_SOURCE_DIR}/src/App/*.c"
    "${CMAKE_SOURCE_DIR}/src/Driver/*.c"
    "${CMAKE_SOURCE_DIR}/src/Bsp/*.c"
    "${CMAKE_SOURCE_DIR}/src/Common/*.c"
)

add_library(stm32_stdperiph STATIC
    ${STDPERIPH_SOURCES}
)

target_include_directories(stm32_stdperiph PUBLIC
    "${CMAKE_SOURCE_DIR}/src/User"
    "${STDPERIPH_DRIVER}/inc"
    ${STM32_STARTUP_INCLUDE_DIRS}
)

target_compile_definitions(stm32_stdperiph PUBLIC
    STM32F10X_MD
    USE_STDPERIPH_DRIVER
)

target_compile_options(stm32_stdperiph PRIVATE
    ${MCU_FLAGS}
    -Wall
    -ffunction-sections
    -fdata-sections
)

add_executable(${CMAKE_PROJECT_NAME}
    ${PROJECT_SOURCES}
    ${STM32_STARTUP_SOURCES}
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    src/User
    src/App
    src/Driver
    src/Bsp
    src/Common
)

target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    STM32F10X_MD
    USE_STDPERIPH_DRIVER
)

target_compile_options(${CMAKE_PROJECT_NAME} PRIVATE
    ${MCU_FLAGS}
    -Wall
    -Wextra
    -ffunction-sections
    -fdata-sections
)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(${CMAKE_PROJECT_NAME} PRIVATE -O0 -g3)
    target_compile_options(stm32_stdperiph PRIVATE -O0 -g3)
else()
    target_compile_options(${CMAKE_PROJECT_NAME} PRIVATE -Os)
    target_compile_options(stm32_stdperiph PRIVATE -Os)
endif()

target_link_options(${CMAKE_PROJECT_NAME} PRIVATE
    ${MCU_FLAGS}
    "-T${CMAKE_SOURCE_DIR}/Start/STM32F103xx_FLASH.ld"
    --specs=nano.specs
    --specs=nosys.specs
    -Wl,--gc-sections
    "-Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_PROJECT_NAME}.map"
    -Wl,--print-memory-usage
)

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE
    stm32_stdperiph
    m
)

add_custom_command(TARGET ${CMAKE_PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O ihex
        $<TARGET_FILE:${CMAKE_PROJECT_NAME}>
        "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_PROJECT_NAME}.hex"
    COMMAND ${CMAKE_OBJCOPY} -O binary
        $<TARGET_FILE:${CMAKE_PROJECT_NAME}>
        "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_PROJECT_NAME}.bin"
    COMMAND ${CMAKE_SIZE}
        $<TARGET_FILE:${CMAKE_PROJECT_NAME}>
    COMMENT "Generating HEX/BIN and printing firmware size"
)
