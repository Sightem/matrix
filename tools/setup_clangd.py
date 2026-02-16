from __future__ import annotations

import argparse
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys


def _write_text(path: Path, contents: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(contents, encoding="utf-8", newline="\n")


def _write_json(path: Path, payload: object) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(payload, indent=2) + "\n", encoding="utf-8", newline="\n")


def _ensure_native_compile_db(repo_root: Path, native_build_dir: Path) -> Path:
    compile_db = native_build_dir / "compile_commands.json"
    if compile_db.exists():
        return compile_db

    print(f"[clangd] Missing {compile_db}, running `cmake --preset native`...")
    subprocess.run(["cmake", "--preset", "native"], cwd=repo_root, check=True)

    if not compile_db.exists():
        raise RuntimeError(
            f"Expected {compile_db} after configuring, but it was not created."
        )
    return compile_db


def _symlink_or_copy(dest: Path, target: Path) -> None:
    if dest.exists() or dest.is_symlink():
        dest.unlink()

    try:
        rel_target = os.path.relpath(target, start=dest.parent)
        dest.symlink_to(rel_target)
    except OSError:
        shutil.copyfile(target, dest)


def _generate_shell_compile_db(
    repo_root: Path, cedev_include: Path, out_path: Path
) -> None:
    src_dir = repo_root / "shell" / "src"
    sources = sorted(src_dir.rglob("*.cpp"))
    if not sources:
        raise RuntimeError(f"No shell sources found under {src_dir}")

    shim = repo_root / "tools" / "clangd" / "ce_shim.hpp"

    base_args = [
        "clang++",
        "-std=gnu++20",
        "-fno-exceptions",
        "-fno-rtti",
        f"-include{shim}",
        f"-I{repo_root / 'core' / 'include'}",
        f"-I{repo_root / 'shell' / 'include'}",
        f"-I{repo_root / 'third_party' / 'libtexce' / 'src'}",
        f"-I{repo_root / 'third_party' / 'libtexce' / 'include'}",
        # Must be AFTER host system headers; CEdev provides sys/types.h etc.
        f"-idirafter{cedev_include}",
        # Match CE build feature flags used in CMakeLists.txt.
        "-DTEX_USE_FONTLIB",
        "-DTEX_DIRECT_RENDER",
    ]

    entries: list[dict[str, object]] = []
    for src in sources:
        entries.append(
            {
                "directory": str(repo_root),
                "file": str(src),
                "arguments": base_args + ["-c", str(src)],
            }
        )

    _write_json(out_path, entries)


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Set up clangd (and optional VSCode settings) for this repo."
    )
    parser.add_argument(
        "--cedev",
        default=str(Path.home() / "CEdev"),
        help="Path to CEdev root (default: ~/CEdev).",
    )
    parser.add_argument(
        "--native-build-dir",
        default="build/native",
        help="CMake native build directory (default: build/native).",
    )
    parser.add_argument(
        "--no-vscode",
        action="store_true",
        help="Do not write .vscode settings/extensions/shim.",
    )
    args = parser.parse_args()

    repo_root = Path(__file__).resolve().parents[1]
    cedev_root = Path(os.path.expanduser(args.cedev)).resolve()
    cedev_include = cedev_root / "include"
    if not cedev_include.is_dir():
        print(
            f"error: CEdev include dir not found at {cedev_include}. "
            f"Pass --cedev /path/to/CEdev.",
            file=sys.stderr,
        )
        return 2

    libtexce_src = repo_root / "third_party" / "libtexce" / "src"
    if not libtexce_src.is_dir():
        print(
            f"warning: libtexce sources not found at {libtexce_src}. "
            "If this is a fresh clone, run `git submodule update --init --recursive`.",
            file=sys.stderr,
        )

    native_build_dir = (repo_root / args.native_build_dir).resolve()
    native_compile_db = _ensure_native_compile_db(repo_root, native_build_dir)

    if not args.no_vscode:
        _write_text(
            repo_root / ".vscode" / "extensions.json",
            json.dumps(
                {
                    "recommendations": [
                        "llvm-vs-code-extensions.vscode-clangd",
                        "ms-vscode.cmake-tools",
                        "twxs.cmake",
                    ]
                },
                indent=2,
            )
            + "\n",
        )

        _write_text(
            repo_root / ".vscode" / "settings.json",
            "{\n"
            '  "clangd.arguments": [\n'
            '    "--background-index",\n'
            '    "--clang-tidy",\n'
            '    "--completion-style=detailed",\n'
            '    "--header-insertion=never"\n'
            "  ],\n"
            '  "clangd.fallbackFlags": [\n'
            '    "-std=c++20",\n'
            '    "-include${workspaceFolder}/tools/clangd/ce_shim.hpp",\n'
            '    "-I${workspaceFolder}/core/include",\n'
            '    "-I${workspaceFolder}/shell/include",\n'
            '    "-I${workspaceFolder}/third_party/libtexce/src",\n'
            '    "-I${workspaceFolder}/third_party/libtexce/include",\n'
            '    "-idirafter${env:HOME}/CEdev/include"\n'
            "  ],\n"
            '  "C_Cpp.intelliSenseEngine": "disabled",\n'
            '  "C_Cpp.errorSquiggles": "disabled",\n'
            '  "cmake.useCMakePresets": "always",\n'
            '  "cmake.configureOnOpen": false\n'
            "}\n",
        )

    _symlink_or_copy(
        repo_root / "core" / "compile_commands.json",
        native_compile_db,
    )

    _generate_shell_compile_db(
        repo_root,
        cedev_include,
        repo_root / "shell" / "compile_commands.json",
    )

    print("[clangd] Done.")
    print("  - core/compile_commands.json -> build/native/compile_commands.json")
    print("  - shell/compile_commands.json generated for host clang++ parsing")
    if not args.no_vscode:
        print("  - .vscode/{settings,extensions}.json written")
        print("Next: restart clangd in VSCode (Command Palette: 'Clangd: Restart language server').")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
