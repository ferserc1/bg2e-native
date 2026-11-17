#!/usr/bin/env python3
"""Exercise an already-built log_probe; this script never invokes a compiler."""
import argparse
from pathlib import Path
import subprocess
import tempfile


def run(probe, mode, directory):
    return subprocess.run([str(probe), mode, str(directory)], capture_output=True, text=True, timeout=90)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("probe", type=Path, help="Path to an already-built log_probe executable")
    probe = parser.parse_args().probe.resolve()
    with tempfile.TemporaryDirectory(prefix="bg2e-deep-debug-") as temporary:
        root = Path(temporary)
        assert run(probe, "disabled", root).returncode == 0
        assert not list(root.iterdir()), "Disabled mode created diagnostic artifacts"
        stress = run(probe, "stress", root)
        assert stress.returncode == 0, stress.stderr
        log = root / "bg2e-deep-debug.log"
        data = log.read_bytes()
        assert len(data) <= 12 * 1024 * 1024
        assert data.endswith(b"\n")
        lines = data.decode().splitlines()
        assert lines[0].startswith("# bg2e deep debug session ")
        sequences = [int(line.split(" ", 1)[0]) for line in lines[1:]]
        assert all(b > a for a, b in zip(sequences, sequences[1:])), "Records interleaved or reordered"
        assert "event=oldest " not in data.decode()
        assert "event=newest " in data.decode()
        assert any("event=huge " in line and line.endswith("[truncated]") for line in lines)
        concurrent = [line for line in lines if "event=concurrent " in line]
        assert concurrent and all(line.endswith("end=complete") for line in concurrent)
        assert "warning-capture-42" in data.decode() and "warning-capture-42" in stress.stderr
        assert "error-capture-43" in data.decode() and "error-capture-43" in stress.stderr
        assert "same warning occurrences=64" in data.decode()
        assert "distinct warning occurrences=1" in data.decode()
        assert not (root / "bg2e-deep-debug.log.tmp").exists()
        # A process owns the fixed path without truncating it on competing startup.
        owner = subprocess.Popen([str(probe), "hold", str(root)], stdin=subprocess.PIPE,
                                 stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
        try:
            assert owner.stdout.readline().strip() == "READY"
            before = log.read_bytes()
            competing = run(probe, "open", root)
            assert competing.returncode == 2 and "another process" in competing.stderr
            assert log.read_bytes() == before, "Competing initialization modified the owner's log"
        finally:
            owner.communicate("\n", timeout=10)
        assert owner.returncode == 0
        # A completed write survives process abort and the OS releases ownership.
        aborted = run(probe, "abort", root)
        assert aborted.returncode != 0
        assert b"event=abort.last " in log.read_bytes() and log.read_bytes().endswith(b"\n")
        assert run(probe, "open", root).returncode == 0
        failure = root / "failure"
        failure.mkdir()
        failed_output = run(probe, "write-failure", failure)
        assert failed_output.returncode == 0, failed_output.stderr
        assert "persistent logging failed" in failed_output.stderr
        assert "warning-capture-42" in failed_output.stderr
        assert (failure / "bg2e-deep-debug.log").stat().st_size <= 12 * 1024 * 1024
        # A disabled failed sink must release ownership during explicit shutdown.
        assert run(probe, "open", failure).returncode == 0
        # Missing/non-directory paths must fail explicitly; no silent redirection.
        assert run(probe, "open", root / "missing").returncode == 2
        assert run(probe, "open", log).returncode == 2
    print("Deep debug logger checks passed")


if __name__ == "__main__":
    main()
