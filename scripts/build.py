#!/usr/bin/env python3
import argparse
import csv
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess

ROOT = Path(__file__).resolve().parents[2]
FAMILIES = [
    "laccase_associated", "manganese_peroxidase", "lignin_peroxidase", "versatile_peroxidase", "cytochrome_p450"
]


def sha256(path):
    digest = hashlib.sha256()
    with Path(path).open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def git(*arguments):
    return subprocess.check_output(["git", "-C", str(ROOT), *arguments], text=True).strip()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--species", type=Path, required=True)
    parser.add_argument("--annotations", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--binary", type=Path, default=ROOT / "bin/")
    parser.add_argument("--allow-rejected", action="store_true")
    args = parser.parse_args()

    binary = args.binary.resolve()
    manifest_path = binary.parent / "build.json"
    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    if sha256(binary) != manifest["binary_sha256"]:
        raise SystemExit("Executable checksum differs from build.json")
    for name, expected in manifest["source_sha256"].items():
        if sha256(ROOT / name) != expected:
            raise SystemExit(f"Source changed size build: {name}")

    sources = {
        "species_metadata.tsv": args.species.resolve(),
        "protein_annotations.tsv": args.annotations.resolve()
    }
    for path in sources.values():
        if not path.is_file():
            raise SystemExit(f"Input is not a regular file: {path}")

    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    snapshots = output / "inputs"
    snapshots.mkdir()

    inputs = {}
    for name, original in sources.items():
        snapshot = snapshots / name
        shutil.copyfile(original, snapshot)
        inputs[name] = {
            "original_path": str(original),
            "snapshot": "inputs/" + name,
            "sha256": sha256(snapshot),
            "bytes": snapshot.stat().st_size
        }

    completed = subprocess.run(
        [str(binary), str(snapshots / "species_metadata.tsv"), str(snapshots / "protein_annotations.tsv")],
        cwd=output,
        env=dict(os.environ, LC_ALL="C"),
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        check=False
    )
    (output / "execution.log").write_bytes(completed.stdout + completed.stderr)

    counters = {}
    qc_path = output / "qc.tsv"
    if qc_path.exists():
        with qc_path.open(encoding="utf-8", newline="") as stream:
            for row in csv.DictReader(stream, delimiter="\t", quoting=csv.QUOTE_NONE):
                counters[row["key"]] = int(row["value"])

    rejected = (counters.get("rejected_metadata_rows", 0) + counters.get("rejected_annotation_rows", 0))
    if completed.returncode != 0:
        status = "failed"
    elif rejected and not args.allow_rejected:
        status = "invalid_input"
    elif rejected:
        status = "accepted_with_rejections"
    else:
        status = "ok"

    code_files = sorted((ROOT / "src").glob("*"))
    code_files += sorted((ROOT / "scripts").glob("*.py"))
    provenance = {
        "processing_status": status,
        "core_exit_code": completed.returncode,
        "inputs": inputs,
        "counters": counters,
        "build": manifest,
        "bioflow_revision": git("rev-parse", "HEAD"),
        "bioflow_worktree_dirty": bool(git("status", "--porcelain", "--untracked-files=normal")),
        "analysis_source_sha256": {
            str(path.relative_to(ROOT)): sha256(path)
            for path in code_files if path.is_file()
        },
        "parameters": {
            "classification_policy": "conservative_family_specific_v1",
            "families": FAMILIES,
            "supported_evidence": [
                "family_specific_curated_homology",
                "family_specific_experimental"
            ],
            "ambiguous_evidence": ["domain_only", "keyword_only"],
            "confidence_levels": ["low", "medium", "high"],
            "confidence_filter": None,
            "normalization_scale": 1_000,
            "completeness_units": "percent",
            "input_missing_value": "empty_cell",
            "normalized_missing_value": "NA",
            "allow_rejected": args.allow_rejected,
            "max_input_bytes": 128 * 1024 * 1024,
            "max_line_bytes": 1024 * 1024,
            "max_records_per_input": 1_000_000,
            "max_columns": 256,
            "maximum_proteome_size": 2147483647
        },
        "output_sha256": {
            path.name: sha256(path)
            for path in sorted(output.glob("*.tsv"))
        }
    }
    (output / "qc.json").write_text(json.dumps(provenance, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(f"{status}: {output}")
    return 0 if status in {"ok", "accepted_with_rejections"} else 2


if __name__ == "__main__":
    raise SystemExit(main())