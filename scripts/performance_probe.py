#!/usr/bin/env python3
import concurrent.futures
import json
import os
import statistics
import time
import urllib.error
import urllib.request
from pathlib import Path


ROOT_DIR = Path(__file__).resolve().parents[1]
BASE_URL = os.environ.get("AUCTION_PERF_BASE_URL", "http://127.0.0.1:18080").rstrip("/")
OUTPUT_JSON = ROOT_DIR / "build" / "performance_report.json"
OUTPUT_MD = ROOT_DIR / "build" / "performance_report.md"


def request(path, method="GET", body=None, token=None):
    data = None
    headers = {"Content-Type": "application/json"}
    if token:
        headers["Authorization"] = f"Bearer {token}"
    if body is not None:
        data = json.dumps(body).encode("utf-8")
    req = urllib.request.Request(f"{BASE_URL}{path}", data=data, method=method, headers=headers)
    started = time.perf_counter()
    try:
        with urllib.request.urlopen(req, timeout=10) as response:
            response.read()
            status = response.status
    except urllib.error.HTTPError as error:
        error.read()
        status = error.code
    except Exception:
        status = 0
    elapsed_ms = (time.perf_counter() - started) * 1000
    return {"status": status, "elapsed_ms": elapsed_ms}


def run_case(name, path, concurrency, total, method="GET", body_factory=None, token=None):
    def one(index):
        body = body_factory(index) if body_factory else None
        return request(path, method=method, body=body, token=token)

    with concurrent.futures.ThreadPoolExecutor(max_workers=concurrency) as pool:
        results = list(pool.map(one, range(total)))

    latencies = [row["elapsed_ms"] for row in results]
    success = sum(1 for row in results if 200 <= row["status"] < 300)
    sorted_latencies = sorted(latencies)
    p95_index = max(0, min(len(sorted_latencies) - 1, int(len(sorted_latencies) * 0.95) - 1))
    return {
        "name": name,
        "path": path,
        "concurrency": concurrency,
        "total": total,
        "success": success,
        "failed": total - success,
        "average_ms": round(statistics.fmean(latencies), 2),
        "p95_ms": round(sorted_latencies[p95_index], 2),
        "max_ms": round(max(latencies), 2),
        "status_counts": {str(status): sum(1 for row in results if row["status"] == status) for status in sorted({row["status"] for row in results})},
    }


def main():
    OUTPUT_JSON.parent.mkdir(parents=True, exist_ok=True)
    cases = [
        run_case("healthz", "/healthz", concurrency=20, total=100),
        run_case("auction_list_200_concurrent", "/api/auctions", concurrency=200, total=200),
    ]

    auction_id = os.environ.get("AUCTION_PERF_BID_AUCTION_ID")
    token = os.environ.get("AUCTION_PERF_AUTH_TOKEN")
    bid_amount = float(os.environ.get("AUCTION_PERF_BID_AMOUNT", "0") or "0")
    if auction_id and token and bid_amount > 0:
        cases.append(
            run_case(
                "bid_api",
                f"/api/auctions/{auction_id}/bids",
                concurrency=int(os.environ.get("AUCTION_PERF_BID_CONCURRENCY", "10")),
                total=int(os.environ.get("AUCTION_PERF_BID_TOTAL", "10")),
                method="POST",
                token=token,
                body_factory=lambda index: {
                    "bid_amount": bid_amount + index,
                    "request_id": f"perf-{int(time.time())}-{index}",
                },
            )
        )

    report = {
        "base_url": BASE_URL,
        "generated_at": time.strftime("%Y-%m-%dT%H:%M:%S%z"),
        "cases": cases,
    }
    OUTPUT_JSON.write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding="utf-8")

    lines = [
        "# 本地性能验证报告",
        "",
        f"- Base URL: `{BASE_URL}`",
        f"- 生成时间: `{report['generated_at']}`",
        "",
        "| 场景 | 并发 | 请求数 | 成功 | 平均 ms | P95 ms | 最大 ms | 状态码 |",
        "|---|---:|---:|---:|---:|---:|---:|---|",
    ]
    for case in cases:
        lines.append(
            f"| {case['name']} | {case['concurrency']} | {case['total']} | {case['success']} | "
            f"{case['average_ms']} | {case['p95_ms']} | {case['max_ms']} | {case['status_counts']} |"
        )
    OUTPUT_MD.write_text("\n".join(lines) + "\n", encoding="utf-8")
    print(f"wrote {OUTPUT_JSON}")
    print(f"wrote {OUTPUT_MD}")
    if cases[0]["success"] == 0:
        raise SystemExit("performance probe failed: backend health endpoint is not reachable")


if __name__ == "__main__":
    main()
