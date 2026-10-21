"""Validate version 1/2/3 captures; optionally verify the standalone probe outputs."""
import argparse
import json
import math
from pathlib import Path


def validate(path):
    with path.open(encoding="utf-8") as source:
        records = [json.loads(line) for line in source]
    assert records and records[0]["type"] == "session", path
    version = records[0]["version"]
    assert version in (1, 2, 3), path
    assert records[-1]["type"] == "summary", path
    session = records[0]["session"]
    begin, end = records[0]["begin_ns"], records[-1]["end_ns"]
    assert begin <= end, path
    for record in records:
        assert record["session"] == session, path
    for record in records[1:-1]:
        if record["type"] == "gpu_submission":
            assert version >= 2, path
            assert record["token"] > 0 and record["thread"] > 0, record
            assert begin <= record["requested_ns"] <= record["resolved_ns"] <= end, record
            assert record["status"] in ("available", "unsupported", "dropped", "failed", "aborted", "unavailable"), record
            assert record["backend"] in ("Unknown", "Vulkan", "Metal"), record
            assert 0 <= record["workload"] <= 4, record
            if record["status"] == "available":
                assert record["requested_ns"] <= record["submitted_ns"] <= record["resolved_ns"], record
                assert record["submitted_ns"] > 0, record
                assert all(record[key] > 0 for key in ("device", "queue", "command", "submission")), record
                assert isinstance(record["gpu_duration_ms"], (int, float)), record
                assert math.isfinite(record["gpu_duration_ms"]) and record["gpu_duration_ms"] >= 0, record
            else:
                assert record["gpu_duration_ms"] is None, record
            if version >= 3:
                assert isinstance(record["pass_gpu"], bool) and isinstance(record["detailed_gpu"], bool), record
                assert isinstance(record["intervals_dropped"], int) and record["intervals_dropped"] >= 0, record
                assert record["interval_capability"] in (0, 1, 2), record
                intervals = record["intervals"]
                assert len(intervals) <= 32, record
                assert record["pass_gpu"] or not intervals, record
                for index, interval in enumerate(intervals, 1):
                    assert interval["id"] == index and 0 <= interval["parent"] < index, interval
                    assert isinstance(interval["context"], int) and interval["context"] >= 0, interval
                    assert isinstance(interval["label"], str) and len(interval["label"]) <= 95, interval
                    assert interval["granularity"] in (0, 1, 2), interval
                    assert interval["status"] in ("available", "unsupported", "dropped", "failed", "aborted", "unavailable"), interval
                    duration = interval["gpu_duration_ms"]
                    if interval["status"] == "available":
                        assert record["status"] == "available" and interval["granularity"] != 0, interval
                        assert isinstance(duration, (int, float)) and math.isfinite(duration) and duration >= 0, interval
                    else:
                        assert duration is None, interval
            continue
        assert record["type"] in ("scope", "counter", "frame"), path
        assert begin <= record["begin_ns"] <= record["end_ns"] <= end, record
        assert record["thread"] > 0, record
        if record["type"] == "scope":
            assert math.isfinite(record["inclusive_ms"]) and math.isfinite(record["exclusive_ms"]), record
            assert 0 <= record["exclusive_ms"] <= record["inclusive_ms"], record
            assert record["calls"] == 1, record
    for key in ("dropped", "writer_dropped", "partial", "unresolved_gpu"):
        assert records[-1][key] >= 0, path
    if version >= 2:
        assert isinstance(records[0]["gpu_submission_timing"], bool), path
        for key in ("gpu_available", "gpu_failed", "gpu_unsupported", "gpu_dropped"):
            assert records[-1][key] >= 0, path
        gpu = [r for r in records if r["type"] == "gpu_submission"]
        assert len({r["token"] for r in gpu}) == len(gpu), path
        for key, statuses in (("gpu_available", {"available"}),
                              ("gpu_failed", {"failed", "aborted", "unavailable"}),
                              ("gpu_unsupported", {"unsupported"}), ("gpu_dropped", {"dropped"})):
            assert records[-1][key] >= sum(r["status"] in statuses for r in gpu), path
    return records


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("captures", nargs="+", type=Path)
    parser.add_argument("--probe", action="store_true")
    parser.add_argument("--gpu-probe", action="store_true")
    args = parser.parse_args()
    if args.probe and args.gpu_probe:
        parser.error("choose either --probe or --gpu-probe")
    sessions = []
    for path in args.captures:
        records = validate(path)
        sessions.append(records[0]["session"])
        if args.probe:
            assert all(r.get("label") != "probe.crossing" for r in records)
            assert sum(r["type"] == "frame" for r in records) == 1
            if path.name == "first.jsonl":
                assert records[-1]["partial"] >= 1
                assert any(r.get("label") == "probe.final" and r["value"] == 7 for r in records)
        if args.gpu_probe:
            gpu = [r for r in records if r["type"] == "gpu_submission"]
            assert len(gpu) == 1 and gpu[0]["status"] == "available", path
            assert gpu[0]["epoch"] != 9999 and gpu[0]["session"] != 9999, path
            if path.name == "gpu_first.jsonl":
                assert gpu[0]["presentation_number"] == 2 and records[-1]["unresolved_gpu"] == 1, path
            elif path.name == "gpu_second.jsonl":
                assert gpu[0]["presentation_number"] == 4 and records[-1]["unresolved_gpu"] == 0, path
            else:
                raise AssertionError(f"Unexpected GPU probe file: {path}")
        print(f"{path}: valid, {len(records)} records")
    if args.probe or args.gpu_probe:
        assert len(sessions) == len(set(sessions))


if __name__ == "__main__":
    main()
