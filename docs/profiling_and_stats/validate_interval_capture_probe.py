"""Exercise schema validation with synthetic captures, without compiling native code."""
import copy
import json
from pathlib import Path
import tempfile
from validate_cpu_capture import validate


def fixture():
    return [
        dict(type="session", version=3, session=1, begin_ns=10, gpu_submission_timing=True),
        dict(type="gpu_submission", session=1, token=1, thread=1, requested_ns=11,
             submitted_ns=12, resolved_ns=20, status="available", backend="Metal", workload=1,
             device=1, queue=1, command=1, submission=1, gpu_duration_ms=0.5,
             pass_gpu=True, detailed_gpu=False, interval_capability=2, intervals_dropped=2,
             intervals=[dict(id=1, parent=0, context=42, label="parent", status="available",
                             granularity=2, gpu_duration_ms=0.4),
                        dict(id=2, parent=1, context=42, label="child", status="unsupported",
                             granularity=0, gpu_duration_ms=None)]),
        dict(type="summary", session=1, end_ns=21, dropped=0, writer_dropped=0, partial=0,
             unresolved_gpu=1, gpu_available=1, gpu_failed=0, gpu_unsupported=0, gpu_dropped=0),
    ]


def main():
    with tempfile.TemporaryDirectory() as directory:
        path = Path(directory) / "capture.jsonl"

        def check(records, valid):
            path.write_text("".join(json.dumps(record) + "\n" for record in records))
            try:
                validate(path)
            except AssertionError:
                assert not valid
            else:
                assert valid

        check(fixture(), True)
        for version in (1, 2):
            records = fixture()
            records[0]["version"] = version
            if version == 1:
                records.pop(1)
            check(records, True)
        invalid = [
            ("id", 2), ("parent", 1), ("context", -1), ("label", "x" * 96),
            ("granularity", 3), ("granularity", 0), ("status", "pending"),
            ("gpu_duration_ms", None), ("gpu_duration_ms", float("nan")),
            ("gpu_duration_ms", -1),
        ]
        for key, value in invalid:
            records = fixture()
            records[1]["intervals"][0][key] = value
            check(records, False)
        records = fixture()
        records[1]["intervals"][1]["gpu_duration_ms"] = 0
        check(records, False)
        records = fixture()
        records[1]["intervals"] = [copy.deepcopy(records[1]["intervals"][0])] * 33
        check(records, False)
        records = fixture()
        records[1]["pass_gpu"] = False
        check(records, False)
    print("Schema probes: 3 valid captures accepted; 13 invalid interval captures rejected")


if __name__ == "__main__":
    main()
