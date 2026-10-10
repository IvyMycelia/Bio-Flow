#!/usr/bin/env python3
import csv
import io
import json
import math
from pathlib import Path
import random
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "scripts"))
import reference

RUNNER = ROOT / "scripts/run.py"
FIXTURES = ROOT / "data/synthetic"
RTOL = 1e-12
ATOL = 1e-12
cases = 0


def read_tsv(path):
    with path.open(encoding="utf-8", newline="") as stream:
        reader = csv.DictReader(stream, delimiter="\t", quoting=csv.QUOTE_NONE)
        return reader.fieldnames, list(reader)


def parse(text):
    return [list(row) for row in csv.reader(io.StringIO(text), delimiter="\t", quoting=csv.QUOTE_NONE)]


def encode(rows):
    return "".join("\t".join(row) + "\n" for row in rows)


def compare_rows(actual, expected):
    assert len(actual) == len(expected), (actual, expected)
    for got, wanted in zip(actual, expected):
        assert set(got) == set(wanted)
        for key, value in wanted.items():
            if key.endswith("_per_1000") and value != "NA":
                number = float(got[key])
                assert math.isfinite(number)
                assert math.isclose(number, float(value), rel_tol=RTOL, abs_tol=ATOL), (key, got[key], value)
            else:
                assert got[key] == str(value), (key, got[key], value)


def run_case(species, annotations, success=True, allow=True):
    global cases
    cases += 1

    with tempfile.TemporaryDirectory(prefix="bioflow-verification-") as name:
        base = Path(name)
        sp = base / "species.tsv"
        ap = base / "annotations.tsv"
        sp.write_bytes(species.encode("utf-8"))
        ap.write_bytes(annotations.encode("utf-8"))
        output = base / "results"

        command = [
            sys.executable, str(RUNNER),
            "--species", str(sp),
            "--annotations", str(ap),
            "--output", str(output)
        ]
        if allow:
            command.append("--allow-rejected")
        completed = subprocess.run(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, check=False)
        assert (completed.returncode == 0) == success, (
            command, completed.returncode, completed.stdout, completed.stderr,
            (output / "execution.log").read_text(errors="replace")
            if (output / "execution.log").exists() else ""
        )
        if not success:
            assert json.loads((output / "qc.json").read_text())["processing_status"] in {"failed", "invalid_input"}
            return None

        expected = reference.analyze_text(species, annotations)
        expected_counts, expected_norm, expected_assignments, expected_qc = expected

        count_header, counts = read_tsv(output / "gene_family_counts.tsv")
        norm_header, normalized = read_tsv(output / "normalized_features.tsv")
        assignment_header, assignments = read_tsv(output / "protein_assignments.tsv")

        count_columns = [f + "_count" for f in reference.FAMILIES]
        norm_columns = [f + "_per_1000" for f in reference.FAMILIES]
        assert count_header == reference.SPECIES + count_columns
        assert norm_header == (reference.SPECIES + count_columns + norm_columns + ["normalization_status"])
        assert assignment_header == [
            "species_id", "protein_id", "status", "assigned_family",
            "candidate_families", "distinct_evidence_rows", "duplicate_evidence_rows"
        ]
        compare_rows(counts, expected_counts)
        compare_rows(normalized, expected_norm)
        compare_rows(assignments, expected_assignments)

        provenance = json.loads((output / "qc.json").read_text())
        assert provenance["counters"] == expected_qc

        _, evidence = read_tsv(output / "annotation_evidence.tsv")
        _, rejected = read_tsv(output / "diagnostics.tsv")
        assert len(evidence) == expected_qc["accepted_annotation_records"]
        assert sum(int(r["duplicate"]) for r in evidence) == (expected_qc["duplicate_annotation_rows"])
        assert len(rejected) == (expected_qc["rejected_metadata_rows"] + expected_qc["rejected_annotation_rows"])

        source_rows = {
            "species_metadata.tsv": parse(species),
            "protein_annotations.tsv": parse(annotations)
        }
        for row in evidence:
            original = source_rows["protein_annotations.tsv"][int(row["input_row"]) - 1]
            assert json.loads(row["record_json"]) == original
        for row in rejected:
            original = source_rows[row["input_file"]][int(row["input_row"]) - 1]
            assert json.loads(row["record_json"]) == original

        return counts, normalized, assignments, expected_qc


def main():
    species = (FIXTURES / "species_metadata.tsv").read_text()
    annotations = (FIXTURES / "protein_annotations.tsv").read_text()
    baseline = run_case(species, annotations)

    counts, normalized, assignments, qc = baseline
    expected_counts = [[1, 0, 0, 1, 1], [0, 1, 0, 0, 0], [1, 0, 0, 0, 0]]
    for row, expected in zip(counts, expected_counts):
        assert [int(row[f + "_count"]) for f in reference.FAMILIES] == expected
    assert [r["normalization_status"] for r in normalized] == ["ok", "missing_proteome_size", "zero_proteome_size"]
    assert qc["annotation_rows"] == 15
    assert qc["accepted_annotation_records"] == 11
    assert qc["rejected_annotation_rows"] == 4
    assert qc["distinct_annotation_records"] == 10
    assert qc["duplicate_annotation_rows"] == 1
    assert qc["unique_proteins"] == 8
    assert qc["classified_proteins"] == 5
    assert qc["conflicting_classifications"] == 1
    assert qc["ambiguous_classifications"] == 2
    assert qc["missing_fields"] == 3
    assert qc["normalization_failed_cells"] == 10

    assert run_case(species, annotations) == baseline
    rng = random.Random(773)
    for _ in range(8):
        srows, arows = parse(species), parse(annotations)
        sdata, adata = srows[1:], arows[1:]
        rng.shuffle(sdata)
        rng.shuffle(adata)
        assert run_case(encode([srows[0]] + sdata), encode([arows[0]] + adata)) == baseline

    run_case(species, annotations, success=False, allow=False)

    # A header-only annotation file yields zero counts for every species.
    annotation_header = encode([parse(annotations)[0]])
    run_case(species, annotation_header)

    # Extra columns are kept and used in exact duplicate identity.
    srows, arows = parse(species), parse(annotations)
    run_case(
        encode([srows[0] + ["quality_method"]] +
               [row + ["synthetic_method"] for row in srows[1:]]),
        encode([arows[0] + ["evidence_details"]] +
               [row + [str(i)] for i, row in enumerate(arows[1:])])
    )

    # Required columns can be in any order.
    run_case(
        encode([list(reversed(row)) for row in parse(species)]),
        encode([list(reversed(row)) for row in parse(annotations)])
    )

    # Exact metadata duplicates collapse; conflicting metadata is fatal.
    run_case(encode(srows + [srows[1]]), annotations)
    conflicting = srows[1].copy()
    conflicting[4] = "2000"
    run_case(encode(srows + [conflicting]), annotations, success=False)

    # Integer parsing never accepts signs, fractions, partial values or NA.
    for value in ["12x", "-1", "+1", "1.5", "NaN", "NA", " 1000", "2147483648"]:
        changed = [row.copy() for row in srows]
        changed[1][4] = value
        run_case(encode(changed), annotations)

    # Completeness must be finite and lie within 0.100.
    for value in ["high", "NaN", "Inf", "-0.1", "100.1", "1e-400"]:
        changed = [row.copy() for row in srows]
        changed[1][5] = value
        run_case(encode(changed), annotations)

    # Largest supported denominator and inconsistent smaller denominator.
    changed = [row.copy() for row in srows]
    changed[1][4] = "2147483647"
    run_case(encode(changed), annotations)
    changed[1][4] = "1"
    result = run_case(encode(changed), annotations)
    assert result[1][0]["normalization_status"] == ("candidate_count_exceeds_proteome_size")

    # Missing required values, unknown vocabulary, and ragged records.
    for column, value in [(0, ""), (1, ""), (4, "unknown"), (5, "unknown")]:
        changed = [row.copy() for row in arows]
        changed[1][column] = value
        run_case(species, encode(changed))
    run_case(species, encode(arows + [["A", "too_short"]]))

    # Identifiers are kept literally, including Unicode and "NA".
    changed = [row.copy() for row in arows]
    for row in changed[1:]:
        if row[0] == "A" and row[1] == "p1":
            row[1] = 'NA α "protein"'
    run_case(species, encode(changed))

    # Fatal file and header errors.
    run_case("", annotations, success=False)
    run_case(species, "", success=False)
    run_case(encode([srows[0]]), annotations, success=False)
    run_case(encode([srows[0][1:]] + [r[1:] for r in srows[1:]]), annotations, success=False)
    duplicate_header = [row.copy() for row in srows]
    duplicate_header[0][1] = duplicate_header[0][0]
    run_case(encode(duplicate_header), annotations, success=False)
    run_case(species, annotations + "\0", success=False)

    print(f"PASS: {cases} executed cases; exact counts, structure and QC; floating tolerance rtol={RTOL}, atol={ATOL}")

if __name__ == "__main__":
    main()