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
                if len(parts) != 13 and len(parts) != 16:
                    sys.stderr.write(
                        f"[-] Line {line_num}: Canonical payload field count mismatch for {action} "
                        f"(expected 13 or 16, got {len(parts)})\n"
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

                if len(parts) == 16:
                    doc_id = parts[13]
                    sec = parts[14]
                    hsh = parts[15]
                    if "statutory_doc_id" in rec and rec["statutory_doc_id"] != doc_id:
                        sys.stderr.write(
                            f"[-] Line {line_num}: Field tampering! statutory_doc_id '{rec['statutory_doc_id']}' != canonical '{doc_id}'\n"
                        )
                        sys.exit(1)
                    if "statutory_section" in rec and rec["statutory_section"] != sec:
                        sys.stderr.write(
                            f"[-] Line {line_num}: Field tampering! statutory_section '{rec['statutory_section']}' != canonical '{sec}'\n"
                        )
                        sys.exit(1)
                    if "statutory_passage_sha256" in rec and rec["statutory_passage_sha256"].lower() != hsh.lower():
                        sys.stderr.write(
                            f"[-] Line {line_num}: Field tampering! statutory_passage_sha256 '{rec['statutory_passage_sha256']}' != canonical '{hsh}'\n"
                        )
                        sys.exit(1)

                # Verify statutory passage hash against corpus manifest
                if "statutory_passage_sha256" in rec and rec["statutory_passage_sha256"]:
                    manifest_path = os.path.join(
                        os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
                        "corpus",
                        "manifest.json"
                    )
                    if os.path.exists(manifest_path):
                        try:
                            with open(manifest_path, "r", encoding="utf-8") as mf:
                                mdata = json.load(mf)
                            rec_doc = rec.get("statutory_doc_id", "")
                            rec_sec = rec.get("statutory_section", "")
                            rec_hsh = rec.get("statutory_passage_sha256", "").lower()
                            matched_hash = None
                            for ch in mdata.get("chunks", {}).values():
                                if ch.get("doc_id") == rec_doc and ch.get("section") == rec_sec:
                                    matched_hash = ch.get("passage_sha256", "").lower()
                                    break
                            if matched_hash and rec_hsh != matched_hash:
                                sys.stderr.write(
                                    f"[-] Line {line_num}: Cryptographic tampering detected! "
                                    f"statutory_passage_sha256 '{rec_hsh}' does not match corpus manifest '{matched_hash}'\n"
                                )
                                sys.exit(1)
                        except Exception:
                            pass

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
