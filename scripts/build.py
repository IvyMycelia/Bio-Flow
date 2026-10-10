#!/usr/bin/env python3
import argparse
import hashlib
import json
import os
from pathlib import Path
import shlex
import shutil
import subprocess

ROOT = Path(__file__).resolve().parents[1]
PIN = "3fba262bf367d7d3ef290ee1b63f56c1ec3ed0b4"
SOURCES = [
    "src/native.h",
    "src/model.flo",
    "src/classify.flo",
    "src/main.flo"
]


def sha256(path):
    digest = hashlib.sha256()
    with Path(path).open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def git(directory, *arguments):
    return subprocess.check_output(["git", "-C", str(directory), *arguments], text=True).strip()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--flower", type=Path, required=True)
    parser.add_argument("--sanitize", action="store_true")
    args = parser.parse_args()

    flower = args.flower.resolve()
    revision = git(flower, "rev-parse", "HEAD")
    if revision != PIN:
        raise SystemExit(f"Expected Flower {PIN}; found {revision}."
                         f"Check out the pinned revision and rebuild Flower.")

    unchanged = subprocess.run(
        [
            "git", "-C", str(flower),
            "diff", "--quiet", "HEAD", "--",
            "src", "include", "scripts", "Makefile"
        ],
        check=False
    )
    if unchanged.returncode != 0:
        raise SystemExit("Flower compiler sources differ from the pinned revision")

    compiler = flower / "bin" / "Flower"
    if not compiler.is_file():
        raise SystemExit("Build and bootstrap Flower first")

    cc = shutil.which(os.environ.get("CC", "cc"))
    if cc is None:
        raise SystemExit("C compiler not found")

    output = ROOT / "bin"
    output.mkdir(parents=True, exist_ok=True)

    flags = [
        "-std=c99",
        "-O2",
        "-include",
        str(ROOT / "src" / "native.h")
    ]
    if args.sanitize:
        flags = [
            "-std=c99",
            "-O1",
            "-g",
            "-fsanitize=address,undefined",
            "-fno-omit-frame-pointer",
            "-include",
            str(ROOT / "src" / "native.h")
        ]

    environment = dict(os.environ, LC_ALL="C")
    environment["CC"] = shlex.quote(cc)
    environment["CFLAGS"] = shlex.join(flags)

    subprocess.run(
        [str(compiler), "src/main.flo", "bin/bioflow"],
        cwd=ROOT,
        env=environment,
        check=True
    )

    provenance = {
        "compiler_repository": "https://github.com/IvyMycelia/flower",
        "compiler_revision": revision,
        "compiler_binary_sha256": sha256(compiler),
        "bioflow_revision": git(ROOT, "rev-parse", "HEAD"),
        "source_sha256": { name: sha256(ROOT / name) for name in SOURCES },
        "binary_sha256": sha256(output / "bioflow"),
        "generated_c_sha256": sha256(output / "bioflow.c"),
        "c_compiler": subprocess.check_output([cc, "--version"], text=True).splitlines()[0],
        "cflags": flags
    }

    (output / "build.json").write_text(json.dumps(provenance, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(f"Built {output / 'bioflow'}")

if __name__ == "__main__":
    main()