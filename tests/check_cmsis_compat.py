#!/usr/bin/env python3
"""验证模板目录、固定 CMSIS 兼容处理和固件构建产物。"""

import argparse
import hashlib
import json
from pathlib import Path


def require(condition: bool, message: str) -> None:
    if not condition:
        raise RuntimeError(message)


def git_blob_sha(data: bytes) -> str:
    return hashlib.sha1(
        b"blob " + str(len(data)).encode("ascii") + b"\0" + data
    ).hexdigest()


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("build_dir", type=Path)
    args = parser.parse_args()

    root = Path(__file__).resolve().parents[1]
    build = args.build_dir.resolve()
    firmware = root / "firmware"
    library = firmware / "Libraries/STM32F10x_StdPeriph_Lib"
    core = library / "Libraries/CMSIS/CM3/CoreSupport"
    device = library / "Libraries/CMSIS/CM3/DeviceSupport/ST/STM32F10x"
    compat = build / "cmsis-compat"
    project_source = firmware / "src"

    manifest_path = library / "VENDOR_MANIFEST.json"
    require(manifest_path.is_file(), "缺少标准库 VENDOR_MANIFEST.json")
    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    require(
        manifest.get("upstream_commit")
        == "afa743577f2784e95be2d5003380fdb84a702519",
        "标准库上游提交与模板审核版本不符",
    )
    expected_files = manifest.get("files", {})
    require(expected_files, "标准库 vendor manifest 没有文件记录")

    actual_files = {
        str(path.relative_to(library)).replace("\\", "/")
        for path in (library / "Libraries").rglob("*")
        if path.is_file()
    }
    require(
        actual_files == set(expected_files),
        "标准库 vendor 文件集合与 manifest 不一致",
    )
    for relative_path, metadata in expected_files.items():
        vendored_sha = metadata.get("vendored_blob_sha")
        require(vendored_sha, "manifest 缺少 vendored_blob_sha：" + relative_path)
        require(
            git_blob_sha((library / relative_path).read_bytes()) == vendored_sha,
            "标准库 vendor 文件被修改：" + relative_path,
        )

    header = (core / "core_cm3.h.old").read_bytes()
    require(
        git_blob_sha(header) == "7ab7b4b43685d3ab2facb53d326c48eeb2bdfda1",
        "CMSIS 头文件与审核版本不一致",
    )
    require(
        (compat / "core_cm3.h").read_bytes() == header,
        "生成 core_cm3.h 不是原样恢复",
    )

    source_bytes = (core / "core_cm3.c").read_bytes()
    require(
        git_blob_sha(source_bytes) == "fcff0d133ca83a837ea4a2076d4fc629e14d75b9",
        "CMSIS core_cm3.c 与审核版本不一致",
    )
    expected = source_bytes.decode("utf-8").replace("\r\n", "\n")
    for instruction in ("strexb", "strexh", "strex"):
        old = (
            '__ASM volatile ("'
            + instruction
            + ' %0, %2, [%1]" : "=r" (result) : "r" (addr), "r" (value) );'
        )
        new = old.replace('"=r"', '"=&r"')
        require(expected.count(old) == 1, "无法唯一定位原始 " + instruction + " 语句")
        expected = expected.replace(old, new)
    require(
        (compat / "core_cm3.c").read_text(encoding="utf-8") == expected,
        "core_cm3.c 构建副本改动超出三处 STREX 约束",
    )

    require(not (firmware / "Core").exists(), "仍遗留旧 Core 目录")
    require(
        not (firmware / "STM32F103xx_FLASH.ld").exists(),
        "仍遗留 firmware 根目录链接脚本",
    )
    for directory in ("User", "App", "Bsp", "Common"):
        require((project_source / directory).is_dir(), "src 缺少 " + directory)

    for filename in (
        "main.c",
        "main.h",
        "stm32f10x_it.c",
        "stm32f10x_it.h",
        "stm32f10x_conf.h",
    ):
        require((project_source / "User" / filename).is_file(), "User 缺少 " + filename)

    for filename in ("Com_Time.c", "Com_Time.h"):
        require((project_source / "Common" / filename).is_file(), "Common 缺少 " + filename)

    for filename in (
        "startup.cmake",
        "cmsis-compat.cmake",
        "STM32F103xx_FLASH.ld",
    ):
        require((firmware / "Start" / filename).is_file(), "Start 缺少 " + filename)

    commands = json.loads(
        (build / "compile_commands.json").read_text(encoding="utf-8")
    )

    expected_units = {
        "main.c": project_source / "User/main.c",
        "stm32f10x_it.c": project_source / "User/stm32f10x_it.c",
        "Com_Time.c": project_source / "Common/Com_Time.c",
        "system_stm32f10x.c": device / "system_stm32f10x.c",
        "startup_stm32f10x_md.s":
            device / "startup/TrueSTUDIO/startup_stm32f10x_md.s",
        "core_cm3.c": compat / "core_cm3.c",
    }

    for filename, expected_path in expected_units.items():
        units = [entry for entry in commands if Path(entry["file"]).name == filename]
        require(len(units) == 1, filename + " 必须且只能编译一份")
        unit = Path(units[0]["file"])
        if not unit.is_absolute():
            unit = Path(units[0]["directory"]) / unit
        require(unit.resolve() == expected_path.resolve(), filename + " 编译来源不正确")

    for entry in commands:
        command = entry.get("command", " ".join(entry.get("arguments", [])))
        require(
            "/Core/Inc" not in command and "/Core/Src" not in command,
            "编译命令仍引用旧 Core 路径",
        )

    project = "stm32f103_std_template"
    for suffix in (".bin", ".hex", ".map"):
        artifact = build / (project + suffix)
        require(
            artifact.is_file() and artifact.stat().st_size > 0,
            "缺少构建产物：" + str(artifact),
        )

    print(
        "PASS: template layout, vendored pinned StdPeriph, CMSIS compatibility, "
        "unique startup units and BIN/HEX/MAP artifacts"
    )


if __name__ == "__main__":
    try:
        main()
    except (
        RuntimeError,
        OSError,
        ValueError,
    ) as exc:
        raise SystemExit(
            "Template verification failed: " + str(exc)
        ) from exc
