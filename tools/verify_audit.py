#!/usr/bin/env python3
"""tools/verify_audit.py - Cryptographic HMAC Audit Chain Verifier for Taryaq
Dubai Create AI Agents Championship 2026-2027 | Best Societal AI Agent

Verifies HMAC-SHA256 chained audit ledgers. Fails closed on any tampering,
key violation, missing field, chain break, or field mismatch.
"""

import sys
import os
import argparse
import json
import hmac
import hashlib


def parse_args():
    parser = argparse.ArgumentParser(
        description="Verify cryptographic HMAC-SHA256 audit ledger chain."
    )
    parser.add_argument(
        "--ledger",
        default=os.environ.get("AUDIT_FILE", "taryaq_audit.jsonl"),
        help="Path to JSONL audit ledger (default: taryaq_audit.jsonl)",
    )
    parser.add_argument(
        "--key",
        default=os.environ.get("TARYAQ_AUDIT_KEY"),
        help="HMAC secret key (min 32 bytes, default: $TARYAQ_AUDIT_KEY)",
    )
    return parser.parse_args()


def verify_ledger(ledger_path: str, key_str: str) -> None:
    if not key_str or len(key_str.encode("utf-8")) < 32:
        sys.stderr.write(
            "[-] Audit key missing or < 32 bytes: export TARYAQ_AUDIT_KEY=<32+ bytes>\n"
        )
        sys.exit(1)

    if not os.path.exists(ledger_path):
        sys.stderr.write(f"[-] Ledger file '{ledger_path}' not found\n")
        sys.exit(1)

    key_bytes = key_str.encode("utf-8")
    expected_prev_mac = "0" * 64
    record_count = 0

    with open(ledger_path, "r", encoding="utf-8") as f:
        for line_num, line in enumerate(f, 1):
            raw = line.strip()
            if not raw:
                continue

            record_count += 1
            try:
                rec = json.loads(raw)
            except Exception as e:
                sys.stderr.write(
                    f"[-] Line {line_num}: Malformed JSON record: {e}\n"
                )
                sys.exit(1)

            # 1. Enforce presence of required fields
            for req in ("canonical", "prev_sha256", "mac"):
                if req not in rec:
                    sys.stderr.write(
                        f"[-] Line {line_num}: Missing required cryptographic field '{req}'\n"
                    )
                    sys.exit(1)

            canonical = rec["canonical"]
            prev_sha256 = rec["prev_sha256"]
            mac = rec["mac"]

            # 2. Validate hex string formatting (64 chars)
            if len(prev_sha256) != 64 or not all(
                c in "0123456789abcdefABCDEF" for c in prev_sha256
            ):
                sys.stderr.write(
                    f"[-] Line {line_num}: prev_sha256 is not 64 hex characters: '{prev_sha256}'\n"
                )
                sys.exit(1)

            if len(mac) != 64 or not all(
                c in "0123456789abcdefABCDEF" for c in mac
            ):
                sys.stderr.write(
                    f"[-] Line {line_num}: mac is not 64 hex characters: '{mac}'\n"
                )
                sys.exit(1)

            # 3. Verify chaining invariant: prev_sha256 must match expected
            if prev_sha256.lower() != expected_prev_mac.lower():
                sys.stderr.write(
                    f"[-] Line {line_num}: Audit chain break! prev_sha256 '{prev_sha256}' does not match expected '{expected_prev_mac}'\n"
                )
                sys.exit(1)

            # 4. Verify HMAC-SHA256 signature
            hmac_msg = f"{prev_sha256}\n{canonical}".encode("utf-8")
            calc_mac = hmac.new(key_bytes, hmac_msg, hashlib.sha256).hexdigest()
            if not hmac.compare_digest(calc_mac.lower(), mac.lower()):
                sys.stderr.write(
                    f"[-] Line {line_num}: Cryptographic tampering detected! HMAC signature mismatch "
                    f"(computed '{calc_mac}', record contains '{mac}')\n"
                )
                sys.exit(1)

            # 5. Cross-verify record data fields against canonical payload
            parts = canonical.split("|")
            action = rec.get("action")
            if action == "PRESCRIPTION_COMMITTED":
                if len(parts) != 13:
                    sys.stderr.write(
                        f"[-] Line {line_num}: Canonical payload field count mismatch for {action} "
                        f"(expected 13, got {len(parts)})\n"
                    )
                    sys.exit(1)

                if "patient_egfr" in rec:
                    try:
                        rec_egfr = float(rec["patient_egfr"])
                        canon_egfr = float(parts[6])
                        if abs(rec_egfr - canon_egfr) > 0.001:
                            sys.stderr.write(
                                f"[-] Line {line_num}: Record field tampering! patient_egfr {rec_egfr} "
                                f"does not match canonical {canon_egfr}\n"
                            )
                            sys.exit(1)
                    except ValueError:
                        sys.stderr.write(
                            f"[-] Line {line_num}: Malformed numeric value in patient_egfr\n"
                        )
                        sys.exit(1)

                if "primary_drug" in rec and rec["primary_drug"] != parts[4]:
                    sys.stderr.write(
                        f"[-] Line {line_num}: Field tampering! primary_drug '{rec['primary_drug']}' != canonical '{parts[4]}'\n"
                    )
                    sys.exit(1)

                if "concurrent_drug" in rec and rec["concurrent_drug"] != parts[5]:
                    sys.stderr.write(
                        f"[-] Line {line_num}: Field tampering! concurrent_drug '{rec['concurrent_drug']}' != canonical '{parts[5]}'\n"
                    )
                    sys.exit(1)

                if "alert_code" in rec and rec["alert_code"] != parts[7]:
                    sys.stderr.write(
                        f"[-] Line {line_num}: Field tampering! alert_code '{rec['alert_code']}' != canonical '{parts[7]}'\n"
                    )
                    sys.exit(1)

            elif action == "PRESCRIPTION_ROLLED_BACK":
                if len(parts) != 8:
                    sys.stderr.write(
                        f"[-] Line {line_num}: Canonical payload field count mismatch for {action} "
                        f"(expected 8, got {len(parts)})\n"
                    )
                    sys.exit(1)

                if "patient_egfr" in rec:
                    try:
                        rec_egfr = float(rec["patient_egfr"])
                        canon_egfr = float(parts[6])
                        if abs(rec_egfr - canon_egfr) > 0.001:
                            sys.stderr.write(
                                f"[-] Line {line_num}: Record field tampering! patient_egfr {rec_egfr} "
                                f"does not match canonical {canon_egfr}\n"
                            )
                            sys.exit(1)
                    except ValueError:
                        sys.stderr.write(
                            f"[-] Line {line_num}: Malformed numeric value in patient_egfr\n"
                        )
                        sys.exit(1)

                if "reason" in rec and rec["reason"] != parts[7]:
                    sys.stderr.write(
                        f"[-] Line {line_num}: Field tampering! reason '{rec['reason']}' != canonical '{parts[7]}'\n"
                    )
                    sys.exit(1)

            # Forward the chain
            expected_prev_mac = mac.lower()

    if record_count == 0:
        sys.stderr.write("[-] Ledger is empty (0 records found)\n")
        sys.exit(1)

    print(f"OK {record_count} records verified")
    sys.exit(0)


def main():
    args = parse_args()
    verify_ledger(args.ledger, args.key)


if __name__ == "__main__":
    main()
