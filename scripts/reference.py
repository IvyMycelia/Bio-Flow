#!/usr/bin/env python3
import math
from pathlib import Path
import re
import sys

FAMILIES = (
    "laccase_associated", "manganese_peroxidase", "lignin_peroxidase", "versatile_peroxidase", "cytochrome_p450"
)
SPECIES = (
    "species_id scientific_name strain_id assembly_accession proteome_size assembly_completeness data_source"
).split()
ANNOTATIONS = (
    "species_id protein_id functional_family annotation_source evidence_type confidence_level"
).split()
SUPPORTED = {
    "family_specific_curated_homology", "family_specific_experimental"
}
EVIDENCE = SUPPORTED | {"domain_only", "keyword_only"}
DECIMAL = re.compile(
    r"[+-]?(?:[0-9]+(?:\.[0-9]*)?|\.[0-9]+)(?:[eE][+-]?[0-9]+)?\Z"
)
COUNTERS = (
    "metadata_rows annotation_rows duplicate_metadata_rows "
    "duplicate_annotation_rows rejected_metadata_rows rejected_annotation_rows "
    "missing_fields invalid_numeric_fields unique_proteins classified_proteins "
    "ambiguous_classifications conflicting_classifications normalization_failures "
    "normalization_failed_cells distinct_annotation_records "
    "accepted_annotation_records species_processed"
).split()

def table(text, required):
    if "\0" in text or text.startswith("\ufeff"):
        raise ValueError("NUL or BOM in TSV")
    lines = text.split("\n")
    if lines[-1] == "":
        lines.pop()
    if not lines:
        raise ValueError("empty TSV")
    lines = [line.removesuffix("\r") for line in lines]
    for line in lines:
        if any((ord(c) < 32 and c != "\t") or ord(c) == 127 for c in line):
            raise ValueError("embedded control character")
    rows = [tuple(line.split("\t")) for line in lines]
    header = rows.pop(0)
    if any(not name for name in header) or len(set(header)) != len(header):
        raise ValueError("empty or duplicate header")
    if not set(required) <= set(header):
        raise ValueError("missing required header")
    return header, rows


def invalid_completeness(text):
    if not text:
        return False
    if not DECIMAL.fullmatch(text):
        return True
    value = float(text)
    significand = re.split("[eE]", text)[0]
    nonzero_literal = any(c in "123456789" for c in significand)
    return (
        not math.isfinite(value)
        or not 0 <= value <= 100
        or 0 < abs(value) < sys.float_info.min
        or (value == 0 and nonzero_literal)
    )


def analyze_text(species_text, annotation_text):
    sh, sr = table(species_text, SPECIES)
    ah, ar = table(annotation_text, ANNOTATIONS)
    q = dict.fromkeys(COUNTERS, 0)
    q.update(metadata_rows=len(sr), annotation_rows=len(ar))
    species, sizes, seen = {}, {}, set()

    for raw in sr:
        if len(raw) != len(sh):
            q["rejected_metadata_rows"] += 1
            continue
        r = dict(zip(sh, raw))
        q["missing_fields"] += sum(r[k] == "" for k in SPECIES)
        missing = any(
            r[k] == "" for k in SPECIES
            if k not in {"proteome_size", "assembly_completeness"}
        )
        size = r["proteome_size"]
        digits = size.lstrip("0") or "0"
        bad_size = bool(size) and (
            not re.fullmatch(r"[0-9]+", size)
            or len(digits) > 10
            or (len(digits) == 10 and digits > "2147483647")
        )
        bad_complete = invalid_completeness(r["assembly_completeness"])
        q["invalid_numeric_fields"] += int(bad_size) + int(bad_complete)
        if missing or bad_size or bad_complete:
            q["rejected_metadata_rows"] += 1
            continue
        if raw in seen:
            q["duplicate_metadata_rows"] += 1
            continue
        sid = r["species_id"]
        if sid in species:
            raise ValueError("conflicting valid species_id" + sid)
        seen.add(raw)
        species[sid] = {k: r[k] for k in SPECIES}
        sizes[sid] = int(digits) if size else None

    if not species:
        raise ValueError("no valid species metadata")

    groups, seen = {}, set()
    for raw in ar:
        if len(raw) != len(ah):
            q["rejected_annotation_rows"] += 1
            continue
        r = dict(zip(ah, raw))
        q["missing_fields"] += sum(r[k] == "" for k in ANNOTATIONS)
        if (
            any(r[k] == "" for k in ANNOTATIONS)
            or r["species_id"] not in species
            or r["functional_family"] not in FAMILIES
            or r["evidence_type"] not in EVIDENCE
            or r["confidence_level"] not in {"high", "medium", "low"}
        ):
            q["rejected_annotation_rows"] += 1
            continue
        q["accepted_annotation_records"] += 1
        key = r["species_id"], r["protein_id"]
        group = groups.setdefault(key, [[], 0])
        if raw in seen:
            q["duplicate_annotation_rows"] += 1
            group[1] += 1
            continue
        seen.add(raw)
        group[0].append(r)

    counts = {sid: dict.fromkeys(FAMILIES, 0) for sid in species}
    assignments = []
    for (sid, pid), (rows, duplicates) in sorted(groups.items()):
        families = [
            f for f in FAMILIES
            if any(r["functional_family"] == f for r in rows)
        ]
        supported = any(r["evidence_type"] in SUPPORTED for r in rows)
        status = (
            "conflicting" if len(families) > 1
            else "classified" if supported
            else "ambiguous"
        )
        family = families[0] if status == "classified" else ""
        if family:
            counts[sid][family] += 1
        counter = {
            "classified": "classified_proteins",
            "ambiguous": "ambiguous_classifications",
            "conflicting": "conflicting_classifications"
        }[status]
        q[counter] += 1
        assignments.append({
            "species_id": sid, "protein_id": pid, "status": status,
            "assigned_family": family,
            "candidate_families": ";".join(families),
            "distinct_evidence_rows": len(rows),
            "duplicate_evidence_rows": duplicates
        })

    count_rows, normalized = [], []
    for sid in sorted(species):
        size = sizes[sid]
        status = (
            "missing_proteome_size" if size is None
            else "zero_proteome_size" if size == 0
            else "candidate_count_exceeds_proteome_size"
            if sum(counts[sid].values()) > size
            else "ok"
        )
        row = dict(
            species[sid],
            **{f + "_count": counts[sid][f] for f in FAMILIES}
        )
        count_rows.append(row)
        normalized.append(dict(
            row,
            **{
                f + "_per_1000":
                format(1_000.0 * counts[sid][f] / size, ".17g")
                if status == "ok" else "NA"
                for f in FAMILIES
            },
            normalization_status=status
        ))
        q["normalization_failures"] += status != "ok"

    q.update(
        species_processed=len(species),
        unique_proteins=len(groups),
        distinct_annotation_records=len(seen)
    )
    q["normalization_failed_cells"] = 5 * q["normalization_failures"]
    return count_rows, normalized, assignments, q


def analyze(species_path, annotation_path):
    return analyze_text(
        Path(species_path).read_bytes().decode("utf-8"),
        Path(annotation_path).read_bytes().decode("utf-8")
    )