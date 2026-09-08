#!/usr/bin/env python3

import csv
import math
import os
import re
import shlex
import shutil
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path

SCRIPT_DIR = Path(__file__).resolve().parent
ROOT_DIR = SCRIPT_DIR.parent.parent
REPO_DIR = ROOT_DIR.parent
INCLUDE_DIR = ROOT_DIR / "include"
SRC_DIR = ROOT_DIR / "src"
NUMBER_PATTERN = r"[-+]?(?:\d+(?:\.\d*)?|\.\d+)(?:[eE][-+]?\d+)?"
ABS_TOLERANCE = 1e-9
REL_TOLERANCE = 1e-7

# Runtime options for case selection, log location, slow cases, family filters and per-command timeouts.
@dataclass
class Config:
    cases_file: Path = Path(os.environ.get("CASES_FILE", SCRIPT_DIR / "cases.tsv"))
    work_root: Path = Path(os.environ.get("WORK_ROOT", "/tmp/ompd-regression"))
    run_slow: bool = os.environ.get("RUN_SLOW", "0") == "1"
    filter_family: str = os.environ.get("FILTER_FAMILY", "")
    case_timeout: int = int(os.environ.get("CASE_TIMEOUT", "30"))


@dataclass
class Case:
    name: str
    family: str
    ompd_source: str
    sequential_source: str
    np: str
    args: list[str]
    libs: list[str]
    uses_runtime: bool
    profile: str
    slow: bool


@dataclass
class Counts:
    passed: int = 0
    skipped: int = 0
    failed: int = 0


@dataclass
class Comparison:
    matches: bool
    message: str = ""


# cases.tsv loading helpers.
def log(message: str = "") -> None:
    print(message, flush=True)


def split_words(value: str) -> list[str]:
    if not value or value == "-":
        return []
    return shlex.split(value)


def load_cases(path: Path) -> list[Case]:
    cases = []
    with path.open(newline="", encoding="utf-8") as fh:
        for row in csv.reader(fh, delimiter="\t"):
            if not row or row[0].startswith("#"):
                continue
            if len(row) != 10:
                raise ValueError(f"{path}: expected 10 columns, got {len(row)}: {row}")
            name, family, ompd_source, sequential_source, np, args, libs, uses_runtime, profile, slow = row
            cases.append(
                Case(
                    name=name,
                    family=family,
                    ompd_source=ompd_source,
                    sequential_source=sequential_source,
                    np=np,
                    args=split_words(args),
                    libs=split_words(libs),
                    uses_runtime=uses_runtime == "yes",
                    profile=profile,
                    slow=slow == "yes",
                )
            )
    return cases


def sanitize_name(name: str) -> str:
    return re.sub(r"[^A-Za-z0-9_.-]", "_", name)


def replace_placeholders(value: str, case_dir: Path) -> str:
    return (
        value.replace("{case_dir}", str(case_dir))
        .replace("{root}", str(ROOT_DIR))
        .replace("{repo}", str(REPO_DIR))
    )


def resolve_source(value: str, case_dir: Path) -> Path:
    path = Path(replace_placeholders(value, case_dir))
    return path if path.is_absolute() else ROOT_DIR / path


def sequential_libraries(libraries: list[str]) -> list[str]:
    return [library for library in libraries if library != "-fopenmp"]


# Result comparison helpers. Each profile knows how to compare one output shape.
def close_enough(expected: float, actual: float) -> bool:
    return math.isclose(expected, actual, rel_tol=REL_TOLERANCE, abs_tol=ABS_TOLERANCE)


def compare_numbers(expected: list[float], actual: list[float], context: str) -> Comparison:
    if len(expected) != len(actual):
        return Comparison(False, f"{context}: sequential count={len(expected)}, ompd count={len(actual)}")
    for index, (expected_value, actual_value) in enumerate(zip(expected, actual)):
        if not close_enough(expected_value, actual_value):
            return Comparison(
                False,
                f"{context}[{index}]: sequential={expected_value}, ompd={actual_value}, "
                f"abs_tol={ABS_TOLERANCE}, rel_tol={REL_TOLERANCE}",
            )
    return Comparison(True)


class Profile:
    output_name = ""

    def compare_text(self, sequential: str, ompd: str) -> Comparison:
        return Comparison(False, "profile does not compare standard output")

    def compare_files(self, sequential: Path, ompd: Path) -> Comparison:
        return Comparison(False, "profile does not compare files")


# Reads lines such as "pi = 3.14" or "Pi 3.14" and ignores the rest.
class LabeledNumberProfile(Profile):
    def __init__(self, label: str):
        self.label = label
        self.pattern = re.compile(
            rf"^\s*{re.escape(label)}\s*=?\s*({NUMBER_PATTERN})",
            re.MULTILINE | re.IGNORECASE,
        )

    def extract(self, text: str) -> float | None:
        match = self.pattern.search(text)
        return float(match.group(1)) if match else None

    def compare_text(self, sequential: str, ompd: str) -> Comparison:
        expected = self.extract(sequential)
        actual = self.extract(ompd)
        if expected is None or actual is None:
            return Comparison(False, f"missing labeled result: {self.label}")
        if close_enough(expected, actual):
            return Comparison(True)
        return Comparison(
            False,
            f"{self.label}: sequential={expected}, ompd={actual}, "
            f"abs_tol={ABS_TOLERANCE}, rel_tol={REL_TOLERANCE}",
        )


class LabeledIntegerProfile(LabeledNumberProfile):
    def compare_text(self, sequential: str, ompd: str) -> Comparison:
        expected = self.extract(sequential)
        actual = self.extract(ompd)
        if expected is None or actual is None:
            return Comparison(False, f"missing integer result: {self.label}")
        expected_integer = int(expected)
        actual_integer = int(actual)
        if expected == expected_integer and actual == actual_integer and expected_integer == actual_integer:
            return Comparison(True)
        return Comparison(False, f"{self.label}: sequential={expected_integer}, ompd={actual_integer}")


# Numeric tables, for example the MD energy table.
class NumericRowsProfile(Profile):
    def __init__(self, width: int, columns: list[int]):
        self.width = width
        self.columns = columns

    def extract(self, text: str) -> list[list[float]]:
        rows = []
        for line in text.splitlines():
            tokens = line.split()
            if len(tokens) != self.width:
                continue
            try:
                values = [float(token) for token in tokens]
            except ValueError:
                continue
            rows.append([values[index] for index in self.columns])
        return rows

    def compare_text(self, sequential: str, ompd: str) -> Comparison:
        expected = self.extract(sequential)
        actual = self.extract(ompd)
        if len(expected) != len(actual):
            return Comparison(False, f"rows: sequential count={len(expected)}, ompd count={len(actual)}")
        for row_index, (expected_row, actual_row) in enumerate(zip(expected, actual)):
            comparison = compare_numbers(expected_row, actual_row, f"row {row_index}")
            if not comparison.matches:
                return comparison
        return Comparison(True)


class NumericSectionProfile(Profile):
    def __init__(self, marker: str):
        self.marker = marker

    def extract(self, text: str) -> list[float]:
        marker_index = text.find(self.marker)
        if marker_index < 0:
            return []
        values = []
        started = False
        for line in text[marker_index + len(self.marker):].splitlines():
            tokens = line.split()
            if not tokens:
                if started:
                    break
                continue
            try:
                row = [float(token) for token in tokens]
            except ValueError:
                if started:
                    break
                continue
            values.extend(row)
            started = True
        return values

    def compare_text(self, sequential: str, ompd: str) -> Comparison:
        expected = self.extract(sequential)
        actual = self.extract(ompd)
        if not expected or not actual:
            return Comparison(False, f"missing numeric section: {self.marker}")
        return compare_numbers(expected, actual, self.marker)


# Final board output, for example Game of Life.
class BoardProfile(Profile):
    def __init__(self, marker: str):
        self.marker = marker

    def extract(self, text: str) -> list[str]:
        lines = text.splitlines()
        try:
            start = lines.index(self.marker) + 1
        except ValueError:
            return []
        board = []
        for line in lines[start:]:
            tokens = line.split()
            if tokens and all(token in ("O", ".") for token in tokens):
                board.append(" ".join(tokens))
            elif board:
                break
        return board

    def compare_text(self, sequential: str, ompd: str) -> Comparison:
        expected = self.extract(sequential)
        actual = self.extract(ompd)
        if not expected or not actual:
            return Comparison(False, f"missing board: {self.marker}")
        if expected == actual:
            return Comparison(True)
        limit = min(len(expected), len(actual))
        for index in range(limit):
            if expected[index] != actual[index]:
                return Comparison(False, f"board row {index}: sequential={expected[index]!r}, ompd={actual[index]!r}")
        return Comparison(False, f"board rows: sequential={len(expected)}, ompd={len(actual)}")


class MatrixRowsProfile(Profile):
    def __init__(self, marker: str):
        self.marker = marker

    def extract(self, text: str) -> list[list[int]]:
        marker_index = text.find(self.marker)
        if marker_index < 0:
            return []
        rows = []
        for line in text[marker_index + len(self.marker):].splitlines():
            match = re.match(r"\s*Fila\s+\d+\s*:\s*(.*)", line)
            if match:
                try:
                    rows.append([int(value) for value in match.group(1).split()])
                except ValueError:
                    return []
            elif rows:
                break
        return rows

    def compare_text(self, sequential: str, ompd: str) -> Comparison:
        expected = self.extract(sequential)
        actual = self.extract(ompd)
        if expected == actual and expected:
            return Comparison(True)
        limit = min(len(expected), len(actual))
        for row_index in range(limit):
            if expected[row_index] != actual[row_index]:
                return Comparison(
                    False,
                    f"matrix row {row_index}: sequential={expected[row_index]}, ompd={actual[row_index]}",
                )
        return Comparison(False, f"matrix rows: sequential={len(expected)}, ompd={len(actual)}")


# Pipeline output: each index produces an a,b,c,d,e tuple.
class IndexedValuesProfile(Profile):
    pattern = re.compile(
        r"a\[(\d+)\]:\s*(-?\d+)\s+"
        r"b\[\d+\]:\s*(-?\d+)\s+"
        r"c\[\d+\]:\s*(-?\d+)\s+"
        r"d\[\d+\]:\s*(-?\d+)\s+"
        r"e\[\d+\]:\s*(-?\d+)"
    )

    def extract(self, text: str) -> dict[int, tuple[int, ...]]:
        return {
            int(match.group(1)): tuple(int(value) for value in match.groups()[1:])
            for match in self.pattern.finditer(text)
        }

    def compare_text(self, sequential: str, ompd: str) -> Comparison:
        expected = self.extract(sequential)
        actual = self.extract(ompd)
        if expected == actual and expected:
            return Comparison(True)
        keys = sorted(set(expected) | set(actual))
        for key in keys:
            if expected.get(key) != actual.get(key):
                return Comparison(False, f"index {key}: sequential={expected.get(key)}, ompd={actual.get(key)}")
        return Comparison(False, "missing indexed results")


class NumericGridProfile(Profile):
    output_name = "result.out"

    @staticmethod
    def extract(path: Path) -> tuple[int, int, list[float]] | None:
        try:
            tokens = path.read_text(encoding="utf-8", errors="replace").split()
            rows = int(tokens[0])
            columns = int(tokens[1])
            values = [float(value) for value in tokens[2:]]
        except (OSError, ValueError, IndexError):
            return None
        if len(values) != rows * columns:
            return None
        return rows, columns, values

    def compare_files(self, sequential: Path, ompd: Path) -> Comparison:
        expected = self.extract(sequential)
        actual = self.extract(ompd)
        if expected is None or actual is None:
            return Comparison(False, "invalid numeric grid")
        if expected[:2] != actual[:2]:
            return Comparison(False, f"grid dimensions: sequential={expected[:2]}, ompd={actual[:2]}")
        return compare_numbers(expected[2], actual[2], "grid")


# Images and other files where the result must match byte by byte.
class BinaryProfile(Profile):
    def __init__(self, output_name: str = "result.bin"):
        self.output_name = output_name

    def compare_files(self, sequential: Path, ompd: Path) -> Comparison:
        try:
            expected = sequential.read_bytes()
            actual = ompd.read_bytes()
        except OSError as error:
            return Comparison(False, str(error))
        if expected == actual:
            return Comparison(True)
        limit = min(len(expected), len(actual))
        for index in range(limit):
            if expected[index] != actual[index]:
                return Comparison(False, f"byte {index}: sequential={expected[index]}, ompd={actual[index]}")
        return Comparison(False, f"file size: sequential={len(expected)}, ompd={len(actual)}")


def create_profile(value: str) -> Profile:
    kind, separator, configuration = value.partition(":")
    if kind == "labeled" and separator:
        return LabeledNumberProfile(configuration)
    if kind == "integer" and separator:
        return LabeledIntegerProfile(configuration)
    if kind == "rows" and separator:
        width, columns = configuration.split(":", 1)
        return NumericRowsProfile(int(width), [int(column) for column in columns.split(",")])
    if kind == "section" and separator:
        return NumericSectionProfile(configuration)
    if kind == "board" and separator:
        return BoardProfile(configuration)
    if kind == "matrix-rows" and separator:
        return MatrixRowsProfile(configuration)
    if kind == "indexed-values" and not separator:
        return IndexedValuesProfile()
    if kind == "numeric-grid" and not separator:
        return NumericGridProfile()
    if kind == "binary":
        return BinaryProfile(configuration if separator else "result.bin")
    raise ValueError(f"unknown result profile: {value}")


# Running test cases
class RegressionRunner:
    def __init__(self, config: Config):
        self.config = config
        self.counts = Counts()

    def run_process(self, cmd: list[str], cwd: Path, stdout: Path, stderr: Path) -> int:
        try:
            with stdout.open("w", encoding="utf-8") as out, stderr.open("w", encoding="utf-8") as err:
                completed = subprocess.run(
                    cmd,
                    cwd=cwd,
                    stdin=subprocess.DEVNULL,
                    stdout=out,
                    stderr=err,
                    timeout=self.config.case_timeout,
                    check=False,
                )
            return completed.returncode
        except subprocess.TimeoutExpired:
            stderr.write_text(f"timeout after {self.config.case_timeout}s: {' '.join(cmd)}\n", encoding="utf-8")
            return 124

    def record_failure(self, case: Case, case_dir: Path, reason: str) -> None:
        log(f"not ok - {case.name} ({reason})")
        log(f"       logs: {case_dir}")
        self.counts.failed += 1

    def compile_sequential(self, case: Case, case_dir: Path, executable: Path) -> bool:
        source = resolve_source(case.sequential_source, case_dir)
        command = ["gcc", "-O2", str(source), "-I", str(INCLUDE_DIR), *sequential_libraries(case.libs), "-o", str(executable)]
        return self.run_process(command, ROOT_DIR, case_dir / "sequential.compile.stdout", case_dir / "sequential.compile.stderr") == 0

    def generate_ompd(self, case: Case, case_dir: Path, generated: Path) -> bool:
        source = resolve_source(case.ompd_source, case_dir)
        command = ["./fparse", str(source), str(case_dir / "fparse.log"), str(case_dir / "fparse.error"), str(generated)]
        return self.run_process(command, ROOT_DIR, case_dir / "fparse.stdout", case_dir / "fparse.stderr") == 0

    def compile_ompd(self, case: Case, case_dir: Path, generated: Path, executable: Path) -> bool:
        command = ["mpicc", "-O2", str(generated)]
        if case.uses_runtime:
            command.extend([str(SRC_DIR / "ompd_runtime.c"), str(SRC_DIR / "hash_map.c")])
        command.extend(["-I", str(INCLUDE_DIR), *case.libs, "-o", str(executable)])
        return self.run_process(command, ROOT_DIR, case_dir / "ompd.compile.stdout", case_dir / "ompd.compile.stderr") == 0

    def arguments(self, case: Case, run_dir: Path, profile: Profile) -> list[str]:
        output = run_dir / profile.output_name if profile.output_name else run_dir / "result.out"
        values = []
        for argument in case.args:
            value = replace_placeholders(argument, run_dir)
            values.append(value.replace("{output}", str(output)))
        return values

    def execute(self, executable: Path, args: list[str], run_dir: Path, prefix: str, np: str = "") -> bool:
        command = [str(executable), *args]
        if np not in ("", "-", "0"):
            command = ["mpirun", "-np", np, str(executable), *args]
        return self.run_process(command, run_dir, run_dir.parent / f"{prefix}.stdout", run_dir.parent / f"{prefix}.stderr") == 0

    def compare(self, profile: Profile, case_dir: Path, sequential_dir: Path, ompd_dir: Path) -> Comparison:
        if profile.output_name:
            return profile.compare_files(sequential_dir / profile.output_name, ompd_dir / profile.output_name)
        sequential = (case_dir / "sequential.stdout").read_text(encoding="utf-8", errors="replace")
        ompd = (case_dir / "ompd.stdout").read_text(encoding="utf-8", errors="replace")
        return profile.compare_text(sequential, ompd)

    def run_case(self, case: Case) -> None:
        if self.config.filter_family and case.family != self.config.filter_family:
            self.counts.skipped += 1
            log(f"skip - {case.name} (family filter: {self.config.filter_family})")
            return
        if case.slow and not self.config.run_slow:
            self.counts.skipped += 1
            log(f"skip - {case.name} (set RUN_SLOW=1)")
            return

        case_dir = self.config.work_root / sanitize_name(case.name)
        if not self.is_safe_case_dir(case_dir):
            self.record_failure(case, case_dir, f"unsafe work directory: {case_dir}")
            return

        shutil.rmtree(case_dir, ignore_errors=True)
        sequential_dir = case_dir / "sequential"
        ompd_dir = case_dir / "ompd"
        sequential_dir.mkdir(parents=True)
        ompd_dir.mkdir()

        log(f"case - {case.name}")
        profile = create_profile(case.profile)
        sequential_executable = case_dir / "sequential.out"
        generated = case_dir / "generated.c"
        ompd_executable = case_dir / "ompd.out"

        if not self.compile_sequential(case, case_dir, sequential_executable):
            self.record_failure(case, case_dir, "sequential compile failed")
            return
        if not self.generate_ompd(case, case_dir, generated):
            self.record_failure(case, case_dir, "fparse failed")
            return
        if not self.compile_ompd(case, case_dir, generated, ompd_executable):
            self.record_failure(case, case_dir, "ompd compile failed")
            return

        if not self.execute(sequential_executable, self.arguments(case, sequential_dir, profile), sequential_dir, "sequential"):
            self.record_failure(case, case_dir, "sequential run failed")
            return
        if not self.execute(ompd_executable, self.arguments(case, ompd_dir, profile), ompd_dir, "ompd", case.np):
            self.record_failure(case, case_dir, "ompd run failed")
            return

        comparison = self.compare(profile, case_dir, sequential_dir, ompd_dir)
        if comparison.matches:
            self.counts.passed += 1
            log(f"ok - {case.name}")
        else:
            self.record_failure(case, case_dir, comparison.message)

    def is_safe_case_dir(self, case_dir: Path) -> bool:
        work_root = self.config.work_root.resolve()
        candidate = case_dir.resolve()
        return os.path.commonpath([work_root, candidate]) == str(work_root)

    def run(self) -> int:
        if not (ROOT_DIR / "fparse").is_file():
            log(f"fparse not found at {ROOT_DIR / 'fparse'}")
            return 1
        self.config.work_root.mkdir(parents=True, exist_ok=True)
        for case in load_cases(self.config.cases_file):
            self.run_case(case)
        log()
        log(f"summary: pass={self.counts.passed} skip={self.counts.skipped} fail={self.counts.failed}")
        return 1 if self.counts.failed else 0


def main() -> int:
    return RegressionRunner(Config()).run()

if __name__ == "__main__":
    sys.exit(main())
