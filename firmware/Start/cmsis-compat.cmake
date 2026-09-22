# CMSIS V1.30 构建兼容：只写构建目录，不修改固定版本 vendor 标准库。
# 调用前：CMSIS_CORE 指向已初始化的 CoreSupport 目录。
# 输出：CMSIS_CORE_INCLUDE、CMSIS_CORE_SOURCE。
set(CMSIS_COMPAT_DIR "${CMAKE_CURRENT_BINARY_DIR}/cmsis-compat")
set(CMSIS_CORE_INCLUDE "${CMSIS_CORE}")
set(CMSIS_CORE_SOURCE "${CMSIS_CORE}/core_cm3.c")

if(NOT EXISTS "${CMSIS_CORE_SOURCE}")
    message(FATAL_ERROR "CMSIS 缺少 core_cm3.c，请检查固定版本 vendor 标准库目录。")
endif()

if(NOT EXISTS "${CMSIS_CORE}/core_cm3.h")
    if(NOT EXISTS "${CMSIS_CORE}/core_cm3.h.old")
        message(FATAL_ERROR "CMSIS 缺少 core_cm3.h 和 core_cm3.h.old，请检查固定版本 vendor 标准库目录。")
    endif()
    file(MAKE_DIRECTORY "${CMSIS_COMPAT_DIR}")
    configure_file("${CMSIS_CORE}/core_cm3.h.old"
                   "${CMSIS_COMPAT_DIR}/core_cm3.h" COPYONLY)
    set(CMSIS_CORE_INCLUDE "${CMSIS_COMPAT_DIR}")
    message(STATUS "Restored CMSIS V1.30 core_cm3.h in build directory")
endif()

# 旧版 CMSIS 的 GNU STREXB/H/W 实现缺少 early-clobber 约束。
# 只在构建目录创建兼容副本，绝不修改固定版本 vendor 标准库。
if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
    file(READ "${CMSIS_CORE_SOURCE}" _cmsis_source)
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
                 "${CMSIS_CORE_SOURCE}")
    foreach(_instruction IN ITEMS strexb strexh strex)
        set(_old "__ASM volatile (\"${_instruction} %0, %2, [%1]\" : \"=r\" (result) : \"r\" (addr), \"r\" (value) );")
        set(_new "__ASM volatile (\"${_instruction} %0, %2, [%1]\" : \"=&r\" (result) : \"r\" (addr), \"r\" (value) );")
        string(FIND "${_cmsis_source}" "${_old}" _old_position)
        if(_old_position EQUAL -1)
            string(FIND "${_cmsis_source}" "${_new}" _new_position)
            if(_new_position EQUAL -1)
                message(FATAL_ERROR "CMSIS ${_instruction} 实现与 V1.30 预期不符，停止自动适配，请人工核对。")
            endif()
        else()
            string(REPLACE "${_old}" "${_new}" _cmsis_source "${_cmsis_source}")
        endif()
    endforeach()
    file(MAKE_DIRECTORY "${CMSIS_COMPAT_DIR}")
    file(CONFIGURE OUTPUT "${CMSIS_COMPAT_DIR}/core_cm3.c"
         CONTENT "${_cmsis_source}" @ONLY)
    set(CMSIS_CORE_SOURCE "${CMSIS_COMPAT_DIR}/core_cm3.c")
    message(STATUS "Applied CMSIS V1.30 GNU STREX constraints in build copy")
endif()
