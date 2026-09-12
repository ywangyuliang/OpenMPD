#!/usr/bin/env python3

import argparse
import csv
import os
import re
import shlex
import shutil
import statistics
import subprocess
import sys
import tempfile
from dataclasses import dataclass
from datetime import datetime
from pathlib import Path


HERE = Path(__file__).resolve().parent
OPENMPD = HERE.parent.parent
REPO = OPENMPD.parent

REPEATS = 5
TIMEOUT = 300
OPTIMIZATION = "-O2"
PHYSICAL_CORES = 12
SOCKETS = 2
VARIANT_ORDER = ("seq", "omp", "mpi", "mpi_omp", "ompd", "ompd_omp")
MPI_REFERENCE_VARIANTS = frozenset(("mpi", "mpi_omp"))
HYBRID_VARIANTS = frozenset(("mpi_omp", "ompd_omp"))
PROCESS_COUNTS = (2, 4, 8, 12, 24)
THREAD_COUNTS = (2, 4, 8, 12, 24)
# The hybrid campaign includes physical-core points at 4, 8 and 12 resources,
# plus the existing 24-resource configurations. Intermediate 16-resource
# points are intentionally omitted.
HYBRID_POINTS = ((2, 2), (2, 4), (4, 2), (4, 3), (12, 2))
HYBRID_THREAD_COUNTS = frozenset(threads for _, threads in HYBRID_POINTS)


@dataclass
class Case:
    name: str
    family: str
    variants: tuple[str, ...]
    ompd: Path
    sequential: Path
    openmp: Path | None
    mpi: Path | None
    args: list[str]
    libraries: list[str]
    uses_runtime: bool


CSV_FIELDS = (
    "benchmark",
    "family",
    "comparison_group",
    "variant",
    "processes",
    "threads",
    "total_resources",
    "repetitions",
    "mean_seconds",
    "calculation_mean_seconds",
)

CALCULATION_TIME_PATTERN = re.compile(
    r"^OMPD_CALC_TIME_SECONDS="
    r"([0-9]+(?:\.[0-9]+)?(?:[eE][+-]?[0-9]+)?)\s*$",
    re.MULTILINE,
)


def words(value):
    return [] if not value or value == "-" else shlex.split(value)


def source_path(value):
    if value == "-":
        return None
    value = value.replace("{repo}", str(REPO)).replace("{root}", str(OPENMPD))
    path = Path(value)
    return path if path.is_absolute() else OPENMPD / path


def load_cases(path):
    cases = []
    names = set()
    with path.open(newline="", encoding="utf-8") as stream:
        rows = csv.reader(stream, delimiter="\t")
        for line, row in enumerate(rows, 1):
            if not row or row[0].startswith("#"):
                continue
            if len(row) != 10:
                raise ValueError(f"{path}:{line}: expected 10 columns, got {len(row)}")

            (
                name, family, variant_values, ompd, seq, omp, mpi,
                args, libs, runtime,
            ) = row
            if name in names:
                raise ValueError(f"{path}:{line}: duplicate case: {name}")
            names.add(name)
            variants = tuple(variant_values.split(","))
            unknown = [variant for variant in variants if variant not in VARIANT_ORDER]
            if unknown:
                raise ValueError(f"{path}:{line}: unknown variant(s): {', '.join(unknown)}")
            if len(variants) != len(set(variants)):
                raise ValueError(f"{path}:{line}: variants must not be repeated")
            expected_order = tuple(variant for variant in VARIANT_ORDER if variant in variants)
            if variants != expected_order:
                raise ValueError(f"{path}:{line}: variants must follow this order: {','.join(VARIANT_ORDER)}")
            if "seq" not in variants or "ompd" not in variants:
                raise ValueError(f"{path}:{line}: every case must include seq and ompd")
            if "omp" not in variants:
                raise ValueError(f"{path}:{line}: comparable performance cases must include omp")
            if ompd == "-" or seq == "-":
                raise ValueError(f"{path}:{line}: every case needs ompd_source and sequential_source")
            if runtime not in ("yes", "no"):
                raise ValueError(f"{path}:{line}: uses_runtime must be yes or no")
            if "omp" in variants and omp == "-":
                raise ValueError(f"{path}:{line}: omp needs an openmp_source")
            if "omp" not in variants and omp != "-":
                raise ValueError(f"{path}:{line}: openmp_source is set but omp is not enabled")
            if any(variant in variants for variant in MPI_REFERENCE_VARIANTS) and mpi == "-":
                raise ValueError(f"{path}:{line}: MPI variants need an mpi_source")
            if not any(variant in variants for variant in MPI_REFERENCE_VARIANTS) and mpi != "-":
                raise ValueError(f"{path}:{line}: mpi_source is set but no MPI variant is enabled")
            if "mpi_omp" in variants and "mpi" not in variants:
                raise ValueError(f"{path}:{line}: mpi_omp also requires the mpi reference variant")
            cases.append(Case(
                name=name,
                family=family,
                variants=variants,
                ompd=source_path(ompd),
                sequential=source_path(seq),
                openmp=source_path(omp),
                mpi=source_path(mpi),
                args=words(args),
                libraries=words(libs),
                uses_runtime=runtime == "yes",
            ))
    return cases


def run(command, cwd, stdout, stderr, env=None, timeout=TIMEOUT):
    try:
        with stdout.open("w", encoding="utf-8") as out, stderr.open("w", encoding="utf-8") as err:
            return subprocess.run(
                command,
                cwd=cwd,
                env=env,
                stdin=subprocess.DEVNULL,
                stdout=out,
                stderr=err,
                timeout=timeout,
                check=False,
            ).returncode
    except subprocess.TimeoutExpired:
        stderr.write_text(f"timeout after {timeout}s: {shlex.join(command)}\n", encoding="utf-8")
        return 124


def run_and_log(command, cwd, log, env=None):
    try:
        with log.open("w", encoding="utf-8") as output:
            returncode = subprocess.run(
                command,
                cwd=cwd,
                env=env,
                stdin=subprocess.DEVNULL,
                stdout=output,
                stderr=subprocess.STDOUT,
                timeout=TIMEOUT * REPEATS,
                check=False,
            ).returncode
        if returncode and log.stat().st_size == 0:
            with log.open("a", encoding="utf-8") as output:
                output.write(f"command: {shlex.join(command)}\n")
                output.write(f"exit code: {returncode}\n")
        return returncode
    except subprocess.TimeoutExpired:
        with log.open("a", encoding="utf-8") as output:
            output.write(f"\ntimeout after {TIMEOUT * REPEATS}s\n")
        return 124


def relative_path(path, directory):
    return os.path.relpath(path, directory)


def program_args(case, run_dir):
    repo = relative_path(REPO, run_dir)
    root = relative_path(OPENMPD, run_dir)
    return [
        value.replace("{repo}", repo)
        .replace("{root}", root)
        .replace("{output}", "result.out")
        for value in case.args
    ]


def perf_mean(log):
    text = log.read_text(encoding="utf-8", errors="replace")
    match = re.search(
        r"^\s*([0-9]+(?:[.,][0-9]+)?)"
        r"(?:\s+\+-\s+[0-9]+(?:[.,][0-9]+)?)?"
        r"\s+seconds time elapsed(?:\s+\(.*\))?\s*$",
        text,
        re.MULTILINE,
    )
    if not match:
        raise ValueError("elapsed time was not found in perf output")
    return float(match.group(1).replace(",", "."))


def calculation_mean(log, repetitions=REPEATS):
    text = log.read_text(encoding="utf-8", errors="replace")
    times = [float(value) for value in CALCULATION_TIME_PATTERN.findall(text)]
    if len(times) != repetitions:
        raise ValueError(
            f"expected {repetitions} calculation times, found {len(times)}"
        )
    return statistics.fmean(times)


def safe_name(value):
    return re.sub(r"[^A-Za-z0-9_.-]", "_", value)


class Runner:
    def __init__(self, args, cases, work_dir, logs_dir, warmup_logs_dir):
        self.args = args
        self.cases = cases
        self.work_dir = work_dir
        self.logs_dir = logs_dir
        self.warmup_logs_dir = warmup_logs_dir
        self.rows = []
        self.failures = 0

    def enabled_variants(self, case):
        include_mpi = getattr(self.args, "include_mpi_reference", False)
        include_hybrid = getattr(self.args, "include_hybrid", False)
        return tuple(
            variant for variant in case.variants
            if include_mpi or variant not in MPI_REFERENCE_VARIANTS
            if include_hybrid or variant not in HYBRID_VARIANTS
        )

    def fail(self, case, stage, message):
        self.failures += 1
        message = message.strip().replace("\n", " ")[-600:]
        message = message.replace(str(OPENMPD), ".").replace(str(REPO), "..")
        error_log = self.logs_dir / f"{safe_name(case.name)}--{safe_name(stage)}.error.log"
        error_log.write_text(
            f"case: {case.name}\nstage: {stage}\nerror: {message}\n",
            encoding="utf-8",
        )
        print(
            f"fail - {case.name} [{stage}]: {message} (see {error_log})",
            file=sys.stderr,
            flush=True,
        )

    def compile(self, case, case_dir):
        variants = self.enabled_variants(case)
        sources = [case.sequential, case.ompd]
        if "omp" in variants:
            sources.append(case.openmp)
        if any(variant in variants for variant in MPI_REFERENCE_VARIANTS):
            sources.append(case.mpi)
        missing = [source for source in sources if not source.is_file()]
        if missing:
            self.fail(case, "sources", "missing: " + ", ".join(map(str, missing)))
            return None

        compile_dir = case_dir / "compile"
        compile_dir.mkdir()
        include = ["-I", str(OPENMPD / "include")]
        executables = {"seq": case_dir / "sequential.out"}

        def compile_one(name, command):
            stdout = compile_dir / f"{name}.stdout"
            stderr = compile_dir / f"{name}.stderr"
            if run(command, OPENMPD, stdout, stderr):
                self.fail(case, f"compile-{name}", stderr.read_text(errors="replace"))
                return False
            return True

        commands = {
            "seq": [
                "gcc", OPTIMIZATION, str(case.sequential), *include,
                *case.libraries, "-o", str(executables["seq"]),
            ],
        }
        if "omp" in variants:
            executables["omp"] = case_dir / "openmp.out"
            commands["omp"] = [
                "gcc", OPTIMIZATION, "-fopenmp", str(case.openmp), *include,
                *case.libraries, "-o", str(executables["omp"]),
            ]
        if "mpi" in variants or "mpi_omp" in variants:
            executables["mpi"] = case_dir / "mpi.out"
            commands["mpi"] = [
                "mpicc", OPTIMIZATION, str(case.mpi), *include,
                *case.libraries, "-o", str(executables["mpi"]),
            ]
            if "mpi_omp" in variants:
                executables["mpi_omp"] = case_dir / "mpi_openmp.out"
                commands["mpi_omp"] = [
                    "mpicc", OPTIMIZATION, "-fopenmp", str(case.mpi), *include,
                    *case.libraries, "-o", str(executables["mpi_omp"]),
                ]

        for name, command in commands.items():
            if not compile_one(name, command):
                return None

        generated = case_dir / "generated.c"
        fparse_error = compile_dir / "fparse.stderr"
        fparse = [
            str(OPENMPD / "fparse"),
            str(case.ompd),
            str(compile_dir / "fparse.log"),
            str(compile_dir / "fparse.error"),
            str(generated),
        ]
        if run(fparse, OPENMPD, compile_dir / "fparse.stdout", fparse_error):
            self.fail(case, "fparse", fparse_error.read_text(errors="replace"))
            return None

        generated_sources = [str(generated)]
        if case.uses_runtime:
            generated_sources += [
                str(OPENMPD / "src/ompd_runtime.c"),
                str(OPENMPD / "src/hash_map.c"),
            ]

        ompd_flags = ["mpicc", OPTIMIZATION]
        executables["ompd"] = case_dir / "ompd.out"
        ompd_command = [
            *ompd_flags, *generated_sources, *include, *case.libraries,
            "-o", str(executables["ompd"]),
        ]
        if not compile_one("ompd", ompd_command):
            return None

        if "ompd_omp" in variants:
            executables["ompd_omp"] = case_dir / "ompd_openmp.out"
            ompd_openmp_command = [
                *ompd_flags, "-fopenmp", *generated_sources, *include, *case.libraries,
                "-o", str(executables["ompd_omp"]),
            ]
            if not compile_one("ompd_omp", ompd_openmp_command):
                return None

        return executables

    def configurations(self, case):
        requested_processes = getattr(self.args, "processes", None)
        requested_threads = getattr(self.args, "threads", None)
        max_resources = getattr(self.args, "max_resources", None)
        processes = tuple(
            value for value in PROCESS_COUNTS
            if not requested_processes or value in requested_processes
        )
        threads = tuple(
            value for value in THREAD_COUNTS
            if not requested_threads or value in requested_threads
        )
        hybrid = tuple(
            (process_count, thread_count)
            for process_count, thread_count in HYBRID_POINTS
            if (not requested_processes or process_count in requested_processes)
            and (not requested_threads or thread_count in requested_threads)
            and (not max_resources or process_count * thread_count <= max_resources)
        )

        points = {
            "seq": ((1, 1),),
            "omp": tuple((1, thread_count) for thread_count in threads),
            "mpi": tuple((process_count, 1) for process_count in processes),
            "mpi_omp": hybrid,
            "ompd": tuple((process_count, 1) for process_count in processes),
            "ompd_omp": hybrid,
        }
        configs = []
        for variant in self.enabled_variants(case):
            configs.extend(
                (variant, process_count, thread_count)
                for process_count, thread_count in points[variant]
            )
        return configs

    def command(self, case, executable, variant, processes, threads, run_dir):
        executable = relative_path(executable, run_dir)
        command = [executable, *program_args(case, run_dir)]
        if variant in ("mpi", "mpi_omp", "ompd", "ompd_omp"):
            if variant in HYBRID_VARIANTS:
                hybrid_map_by = (
                    getattr(self.args, "hybrid_map_by", None)
                    or f"ppr:{processes // SOCKETS}:socket:PE={threads}"
                )
                if processes * threads <= PHYSICAL_CORES:
                    # Reserve a distinct physical core for every OpenMP
                    # thread of every rank.
                    mpi_options = [
                        "--map-by", hybrid_map_by,
                        "--bind-to", "core",
                    ]
                else:
                    # This configuration needs logical CPU binding because
                    # it exceeds the physical-core limit.
                    mpi_options = [
                        "--use-hwthread-cpus",
                        "--map-by", hybrid_map_by,
                        "--bind-to", "hwthread",
                    ]
            else:
                mpi_options = ["--map-by", "core", "--bind-to", "core"]
            if variant not in HYBRID_VARIANTS and processes > PHYSICAL_CORES:
                mpi_options = [
                    "--use-hwthread-cpus",
                    "--map-by", "hwthread",
                    "--bind-to", "hwthread",
                ]
            if getattr(self.args, "report_bindings", False):
                mpi_options.append("--report-bindings")
            command = ["mpirun", *mpi_options, "-np", str(processes), *command]
        return command

    def measure(self, case, case_dir, executable, variant, processes, threads):
        label = f"{variant}-{processes}p-{threads}t"
        run_dir = case_dir / label
        run_dir.mkdir()
        log = self.logs_dir / f"{safe_name(case.name)}--{label}.log"
        env = os.environ.copy()
        env["OMP_NUM_THREADS"] = str(threads)
        env.pop("DISPLAY", None)
        env.pop("XAUTHORITY", None)
        command = self.command(case, executable, variant, processes, threads, run_dir)

        warmup_stdout = self.warmup_logs_dir / f"{safe_name(case.name)}--{label}.stdout.log"
        warmup_stderr = self.warmup_logs_dir / f"{safe_name(case.name)}--{label}.stderr.log"
        warmup_returncode = run(command, run_dir, warmup_stdout, warmup_stderr, env)
        if warmup_returncode:
            stdout = warmup_stdout.read_text(encoding="utf-8", errors="replace").strip()
            stderr = warmup_stderr.read_text(encoding="utf-8", errors="replace").strip()
            diagnostics = " ".join(
                part for part in (f"stderr: {stderr}" if stderr else "", f"stdout: {stdout}" if stdout else "")
                if part
            )
            if diagnostics:
                diagnostics = f"; {diagnostics}"
            self.fail(
                case,
                f"{label}-warmup",
                f"warm-up failed with exit code {warmup_returncode}{diagnostics}",
            )
            return

        perf = ["perf", "stat", "-r", str(REPEATS), "--", *command]
        returncode = run_and_log(perf, run_dir, log, env)
        contents = log.read_text(encoding="utf-8", errors="replace")
        print(f"\n--- {log} ---")
        print(contents, end="" if contents.endswith("\n") else "\n", flush=True)

        if returncode:
            self.fail(case, label, f"perf stat failed; see {log}")
            return
        try:
            mean = perf_mean(log)
            calculation = calculation_mean(log)
        except ValueError as error:
            self.fail(case, label, f"{error}; see {log}")
            return

        self.rows.append({
            "benchmark": case.name,
            "family": case.family,
            "comparison_group": (
                "mpi_reference" if variant in MPI_REFERENCE_VARIANTS
                else "hybrid" if variant in HYBRID_VARIANTS
                else "core"
            ),
            "variant": variant,
            "processes": processes,
            "threads": threads,
            "total_resources": processes * threads,
            "repetitions": REPEATS,
            "mean_seconds": f"{mean:.9f}",
            "calculation_mean_seconds": f"{calculation:.9f}",
        })
        print(
            f"means recorded in CSV: perf={mean:.9f} s, "
            f"calculation={calculation:.9f} s\n",
            flush=True,
        )

    def run_case(self, case):
        print(f"case - {case.name} ({case.family})", flush=True)
        case_dir = self.work_dir / safe_name(case.name)
        case_dir.mkdir()

        executables = self.compile(case, case_dir)
        if not executables:
            return

        for variant, processes, threads in self.configurations(case):
            self.measure(
                case,
                case_dir,
                executables[variant],
                variant,
                processes,
                threads,
            )

    def run_all(self):
        for case in self.cases:
            self.run_case(case)


def positive_list(value):
    try:
        numbers = [int(item) for item in value.split(",")]
    except ValueError as error:
        raise argparse.ArgumentTypeError("expected comma-separated integers") from error
    if not numbers or any(number < 1 for number in numbers):
        raise argparse.ArgumentTypeError("values must be positive")
    return numbers


def parse_args():
    timestamp = datetime.now().strftime("%Y%m%d%H%M%S")
    parser = argparse.ArgumentParser(
        description="Run the OpenMPD examples five times with perf and save their mean times."
    )
    parser.add_argument("--cases", type=Path, default=Path("tests/performance/cases.tsv"))
    parser.add_argument("--case", action="append", dest="case_names")
    parser.add_argument("--family", choices=("base", "tasking", "halo"))
    parser.add_argument("--processes", type=positive_list)
    parser.add_argument("--threads", type=positive_list)
    parser.add_argument("--max-resources", type=int)
    parser.add_argument(
        "--include-mpi",
        "--include-mpi-reference",
        dest="include_mpi_reference",
        action="store_true",
        help="also measure handwritten MPI/MPI+OpenMP reference implementations",
    )
    parser.add_argument(
        "--include-ompd-openmp",
        "--include-hybrid",
        dest="include_hybrid",
        action="store_true",
        help="also measure available OpenMPD+OpenMP and MPI+OpenMP variants",
    )
    parser.add_argument(
        "--report-bindings",
        action="store_true",
        help="ask mpirun to record the CPU binding of each MPI rank in the logs",
    )
    parser.add_argument("--hybrid-map-by")
    parser.add_argument(
        "--plan",
        action="store_true",
        help="print the selected configuration plan without compiling or running it",
    )
    parser.add_argument(
        "--output",
        type=Path,
        help="output CSV (default: a timestamped name, including the selected family)",
    )
    args = parser.parse_args()
    if args.max_resources is not None and args.max_resources < 1:
        parser.error("--max-resources must be positive")
    if args.output is None:
        family = f"_{args.family}" if args.family else ""
        args.output = (
            Path("tests/performance/results")
            / f"performance{family}_{timestamp}.csv"
        )
    return args


def select_cases(cases, args):
    if args.family:
        cases = [case for case in cases if case.family == args.family]
    if args.case_names:
        wanted = set(args.case_names)
        cases = [case for case in cases if case.name in wanted]
        missing = wanted - {case.name for case in cases}
        if missing:
            raise ValueError("unknown case(s): " + ", ".join(sorted(missing)))
    if args.processes:
        invalid = [
            case.name
            for case in cases
            if not set(args.processes).intersection(PROCESS_COUNTS)
        ]
        if invalid:
            raise ValueError("requested process counts are not valid for: " + ", ".join(invalid))
    if args.threads:
        invalid = [
            case.name
            for case in cases
            if any(variant in case.variants for variant in ("omp", "mpi_omp", "ompd_omp"))
            and not set(args.threads).intersection(
                set(THREAD_COUNTS).union(HYBRID_THREAD_COUNTS)
            )
        ]
        if invalid:
            raise ValueError("requested thread counts are not valid for: " + ", ".join(invalid))
    return cases


def format_points(configurations, case, variant):
    points = [
        f"{processes}p{x_threads}t"
        for name, processes, x_threads in configurations
        if name == variant
    ]
    if points:
        return ",".join(points)
    if variant in MPI_REFERENCE_VARIANTS.union(HYBRID_VARIANTS) and variant in case.variants:
        return "optional"
    return "n/a"


def print_plan(args, cases):
    runner = Runner(args, cases, None, None, None)
    totals = {"base": 0, "tasking": 0, "halo": 0}
    core_total = 0
    mpi_total = 0
    hybrid_total = 0
    print("Performance configuration plan")
    for case in cases:
        configurations = runner.configurations(case)
        core = sum(
            variant not in MPI_REFERENCE_VARIANTS.union(HYBRID_VARIANTS)
            for variant, _, _ in configurations
        )
        mpi_reference = sum(
            variant in MPI_REFERENCE_VARIANTS
            for variant, _, _ in configurations
        )
        hybrid = len(configurations) - core - mpi_reference
        core_total += core
        mpi_total += mpi_reference
        hybrid_total += hybrid
        totals[case.family] = totals.get(case.family, 0) + len(configurations)
        variants = "; ".join(
            f"{variant}={format_points(configurations, case, variant)}"
            for variant in VARIANT_ORDER
        )
        print(
            f"{case.name} [{case.family}]: "
            f"core={core}, hybrid={hybrid}, mpi-reference={mpi_reference}, "
            f"total={len(configurations)}"
        )
        print(f"  {variants}")
    family_summary = ", ".join(
        f"{family}={total}" for family, total in totals.items() if total
    )
    print(
        f"summary: core={core_total}, hybrid={hybrid_total}, mpi-reference={mpi_total}, "
        f"total={core_total + hybrid_total + mpi_total} ({family_summary})"
    )


def check_tools():
    tools = ("perf", "make", "gcc", "mpicc", "mpirun")
    missing = [tool for tool in tools if shutil.which(tool) is None]
    if missing:
        raise RuntimeError("required command not found: " + ", ".join(missing))


def build_translator():
    result = subprocess.run(
        ["make", "fparse"],
        cwd=OPENMPD,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        check=False,
    )
    print(result.stdout, end="")
    if result.returncode:
        raise RuntimeError("could not build fparse")


def write_csv(path, rows):
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", newline="", encoding="utf-8") as stream:
        writer = csv.DictWriter(stream, fieldnames=CSV_FIELDS, lineterminator="\n")
        writer.writeheader()
        writer.writerows(rows)


def main():
    os.chdir(OPENMPD)
    args = parse_args()
    try:
        cases = select_cases(load_cases(args.cases), args)
        if args.plan:
            print_plan(args, cases)
            return 0
        check_tools()
        build_translator()
    except (OSError, ValueError, RuntimeError) as error:
        print(error, file=sys.stderr)
        return 2

    logs_dir = args.output.with_name(f"{args.output.stem}.logs")
    warmup_logs_dir = args.output.with_name(f"{args.output.stem}.warmup.logs")
    logs_dir.mkdir(parents=True, exist_ok=True)
    warmup_logs_dir.mkdir(parents=True, exist_ok=True)
    (OPENMPD / "tmp").mkdir(exist_ok=True)

    with tempfile.TemporaryDirectory(prefix="ompd-performance-", dir=OPENMPD / "tmp") as work:
        runner = Runner(args, cases, Path(work), logs_dir, warmup_logs_dir)
        runner.run_all()

    write_csv(args.output, runner.rows)
    print(f"\ncsv: {args.output}")
    print(f"logs: {logs_dir}")
    print(f"warm-up logs: {warmup_logs_dir}")
    print(f"rows: {len(runner.rows)}")
    print(f"failures: {runner.failures}")
    return 1 if runner.failures else 0


if __name__ == "__main__":
    raise SystemExit(main())
