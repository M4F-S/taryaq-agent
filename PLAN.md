# Taryaq — implementation plan (repo-local)

**Owner:** the implementation agent assigned to `taryaq-agent`. This file is the complete brief; you do not need any external document.
**Category:** Best Societal AI Agent.
**Rounds:** **1A now** · **1B blocked** (until Qala and Nibras merge their build fixes) · 2 memory · 3 RAG.

---

## Ground rules

- **You own this repo only.** Never edit `qala-agent`, `nibras-agent`, or the umbrella.
- **You must not edit these vendored files** — they belong to other repos and are replaced wholesale in Round 1B:
  `adapters/qala_mcp_policy_adapter.c`, `adapters/qala_adapters.h`, `adapters/nibras_arabic_fademem_adapter.c`, `adapters/nibras_adapters.h`, `adapters/grounding_shield.c/.h`.
  If one of them needs a change, **stop and report** — do not patch it here. That is how this repo ended up running a one-revision-stale policy gate.
- **Your own files:** `src/taryaq_agent.c`, `adapters/taryaq_clinical_adapter.c`, `adapters/taryaq_adapters.h`, `adapters/taryaq_gomaa_bridge.py`, `tests/`, `Makefile`, `README.md`, `LICENSE`, `NOTICE`, `sync_mirror.sh`.
- **Green on macOS is not evidence.** Every acceptance run must include:
  `docker run --rm -v "$PWD":/w -w /w gcc:13 sh -c "apt-get update -qq && apt-get install -y -qq libcurl4-openssl-dev >/dev/null && make clean && make test"`
- **Definition of done** (each round): `make clean && make test` exits 0 **twice consecutively**, container run passes, no `build/`, `*.o`, `*.so`, `*.jsonl`, `__pycache__` tracked.
- **Claims policy.** Banned until proven: `tamper-evident`, `cryptographic chaining`, `< 0.02 ms`, `AF_UNIX`, `Laya 322M`, `Jais 3`, `100 % on-device`.

---

# Round 1A — make it honest (start now, ~5 h)

## T-1 · gomaa guard (30 min)

`adapters/taryaq_gomaa_bridge.py` searches `~/.gemini/antigravity/scratch/mnemosyne`. Replace with `GOMAA_HOME` (default `~/gomaa`). Decorate `tests/test_taryaq_gomaa.py::TestTaryaqGomaaIntegration` with `@unittest.skipUnless(GOMAA_AVAILABLE, "optional: gomaa Memory OS not installed (set GOMAA_HOME)")`. Add an "Optional integration: gomaa" README section.

**Accept:** `GOMAA_HOME=/nonexistent make test` exits 0 → `3 passed, 5 skipped`.

## T-2 · Make the audit chain real (4 h) — the headline

Today `src/taryaq_agent.c` hashes a pipe payload with **unkeyed** SHA-256, writes **no** `prev_sha256`, and the acceptance test only asserts the substring `sha256_signature` (`:949`). Anyone with write access can edit a clinical record and recompute it. Measured repro: change eGFR 22 → 90, leave the signature stale, and the verifier still reports `chain_intact: true`.

**C writer (`taryaq_commit_audit`, `src/taryaq_agent.c`)**
1. Keep building the existing pipe payload; name it `canonical`.
2. Read the last non-empty line of the ledger (seek to end, read the last ~4 KB) and extract `"mac":"<64 hex>"` → `prev`. No previous record → 64 zeros. Validate 64 hex chars; if malformed, **refuse to append** (fail closed).
3. `mac = HMAC-SHA256(key, prev || "\n" || canonical)`, key from `TARYAQ_AUDIT_KEY`.
   - Unset or < 32 bytes → **do not write**; print `[-] Audit key missing: export TARYAQ_AUDIT_KEY=<32+ bytes>`; return false.
   - Never fall back to an unkeyed hash. A silent fallback is how this debt was created.
   - HMAC with the existing primitives, no new dependency:
     `HMAC(K,m) = H((K0 ^ opad) || H((K0 ^ ipad) || m))`, `K0` = key padded to 64 bytes (hash first if longer), `ipad = 0x36`, `opad = 0x5c`. Add `hmac_sha256_hex()` next to `sha256_hex()` and **unit-test it against a Python `hmac` vector** — a wrong HMAC looks like "tamper detected" forever, which is worse than no chain.
4. Emit one JSONL record with, in addition to today's fields:
   `"canonical":"<pipe payload>","prev_sha256":"<prev>","mac":"<mac>"`.
   Keep `sha256_signature` as `sha256(canonical)` for compatibility; comment it as legacy.
5. Serialise read-prev + append: `open(path, O_WRONLY|O_CREAT|O_APPEND, 0600)` + `flock(fd, LOCK_EX)` + `fsync`. Two concurrent writers must not fork the chain.

**Verifier (`tools/verify_audit.py`, new)**
- Reads a ledger; requires `canonical`, `prev_sha256`, `mac` on every record.
- Recomputes HMAC and compares with `hmac.compare_digest`; checks `prev_sha256` equals the previous record's `mac` (first = 64 zeros).
- Exit 0 with `OK <n> records`, or exit 1 naming the line and reason. Any missing field is a failure — **never** "skip unsigned".

**Make targets**
```make
audit-verify:
	@TARYAQ_AUDIT_KEY="$${TARYAQ_AUDIT_KEY:?set TARYAQ_AUDIT_KEY}" python3 tools/verify_audit.py --ledger $${AUDIT_FILE:-taryaq_audit.jsonl}
```
and have `test` export a fixed test key (e.g. `test-key-32-bytes-minimum-000000`).

**Tamper test (`tests/test_audit_tamper.py`, new)**
1. Generate a 2-record ledger with `./build/taryaq_agent --test --audit-file tmp.jsonl`.
2. `make audit-verify` → exit 0.
3. Mutate `patient_egfr` in record 2, leave `mac` → exit 1.
4. Recompute `mac` correctly but corrupt `prev_sha256` → exit 1 (chain break).
5. Delete record 1 → exit 1.

**Accept:** 2–5 behave as stated; the old repro (edit eGFR, stale signature) now fails verification.

> If the owner decides to retract instead of implement: delete every "tamper-evident"/"cryptographic" phrase and rename the endpoint status to "append-only JSONL with checksums". Do not leave the current wording.

## T-3 · Claims (45 min)

`README.md:12` claims `< 0.02 ms` and "tamper-evident SHA-256 cryptographic audit chaining"; your own test prints `Cold Start: 0.040 ms`, and real process start is ~10 ms. `README.md:16-35` describes clinician/pharmacist/patient interfaces that do not exist in code.

- Replace with scoped, measured statements: "deterministic clinical gate, X µs per check (measured on <host>)"; after T-2, "HMAC-chained audit ledger, verifiable with `make audit-verify`".
- Move the tri-modal section under "Roadmap — not shipped".
- Keep the real differentiator: fail-closed on unmodelled drugs (verified) and the interaction rules.

## T-4 · Licence + attribution + CI + mirror (~1 h)

`LICENSE` (Apache-2.0), `NOTICE`:

```
This product includes software derived from:
  - Belya (C99 autonomous SE core)    — © Mohamed Fathy — <repo URL>
  - gomaa (agent memory OS, optional) — © Mohamed Fathy — <repo URL>
Components: adapters/taryaq_clinical_adapter.c, src/taryaq_agent.c,
adapters/taryaq_gomaa_bridge.py
```
README "Built on": new = clinical rules, claim scrubber, triage gate, audit chain; reused = Belya engine; optional = gomaa.
Plus `.github/workflows/ci.yml` (`ubuntu-latest`, gcc, libcurl, `make clean && make test`), `make verify`, and the mirror-script changes (`GDRIVE_ROOT` env, explicit `git add`, abort on artifacts, no `rsync --delete` by default, remove the Gmail address from `AGENT_INSTRUCTIONS.md`).

---

# Round 1B — blocked on Qala + Nibras merging (~1.5 h)

**Do not start until both have merged their build fixes.** Copying "close enough" files is what produced the current drift.

## T-5 · Resync five vendored files

1. `qala-agent/adapters/qala_mcp_policy_adapter.c` → `adapters/`
2. `qala-agent/adapters/qala_adapters.h` → `adapters/` ← **required** (declares the token-store/sandbox API; without it you get `implicit declaration of function 'qala_set_token_store_path'`)
3. `nibras-agent/adapters/nibras_adapters.h` → `adapters/` ← **required** (struct members; without it `nibras_memory_node_t has no member named 'is_pinned'`)
4. `nibras-agent/adapters/nibras_arabic_fademem_adapter.c` → `adapters/`
5. `grounding_shield.c/.h` → `adapters/` (must be byte-identical to `qala-agent`'s)

Rebuild. `src/taryaq_agent.c` needs no API changes.

## T-6 · Token ledger isolation (the resync makes the self-test non-idempotent)

Measured on a resynced copy: run 1 exit 0, **run 2 exit 1** (`Human override failed … already consumed`), and `qala-tokens.jsonl` lands in the repo root.

In `run_test_scenario()` (`src/taryaq_agent.c`), immediately after the opening brace:
```c
    /* Self-test only: isolate the single-use ledger so reruns are idempotent
     * and the production ledger is never touched. */
    qala_set_token_store_path("build/taryaq_test_tokens.jsonl");
    (void)remove("build/taryaq_test_tokens.jsonl");
```
And narrow your `Makefile` clean target to `build/` + `taryaq_audit.jsonl` + `qala-audit.jsonl` — **never** `qala-tokens.jsonl` or `*.jsonl`.

**Accept:** `make all` exit 0; 8/8 milestones; `Ran 3 tests … OK`; **three consecutive `./build/taryaq_agent --test` runs all exit 0**; no `qala-tokens.jsonl` in the repo root; clinical probes unchanged — Colchicine+Ketoconazole → UNVERIFIED, Simvastatin+Clarithromycin → BLOCKED, Warfarin+Amiodarone → BLOCKED, Unobtainium → UNVERIFIED, Colchicine+Clarithromycin → BLOCKED.

---

# Round 2 — patient timeline (4–6 days, after 1B)

The README promises "longitudinal polypharmacy trajectory"; there is no code for it. This is the most clinically meaningful thing you can add, and it needs no model.

- [ ] SQLite table (or JSONL if you prefer zero deps): `patient_id`, `date`, `drug`, `dose`, `egfr`, `verdict`, `alert_code`, `override_token`, `rationale`.
- [ ] `timeline <patient_id>` → chronological view with eGFR trend and every prior block/override.
- [ ] Re-check on each new prescription: does this pair already have an override history? Escalate if a prior override was for the same interaction.
- [ ] `make timeline-demo` seeding 6 realistic encounters (one CKD progression, one repeat override attempt).
- [ ] Test: the second override attempt for the same patient+interaction is flagged as a repeat, not treated as fresh.

**Do not** add embeddings. Deterministic queries are the feature.

---

# Round 3 — clinical RAG (5–8 days, after Round 2)

- [ ] Corpus: DHA SN 03/2019 (you have the PDF), the drug labels for the nine drugs in your table, UAE controlled-substance schedules. Store as versioned text + a SHA-256 manifest.
- [ ] Chunk by section; index in SQLite FTS5 (BM25); top-k retrieval, no network, no model.
- [ ] **Use retrieval to fix the 30-day rule** (`adapters/taryaq_clinical_adapter.c:232` currently fires on `submission_delay_days > 0` and computes the penalty from day 1, while the message says "past the statutory 30-day window"): retrieve the clause, apply `max(0, days - 30)`, and cite `doc_id` + `section` + `passage_sha256` in the verdict.
- [ ] Extend the grounding call site so the citation travels into the audit record — a verdict citing a clause whose hash no longer matches the corpus is a hard block.
- [ ] Demo: "how is the late-submission penalty calculated?" → retrieved clause + quote + citation + verifier verdict.

**Do not** claim RAG over Jais or Laya — neither model exists in this repo.

---

## Report back

**Round 1A:** exit codes (`make clean && make test` ×2, container), `GOMAA_HOME=/nonexistent make test`, the four tamper-test outcomes, `git diff --stat`.
**Round 1B:** three consecutive `--test` runs, `git diff --stat`, the five clinical probes, and `diff -q` proof that vendored files are byte-identical to their owners.
**Every round:** confirmation that no file outside `taryaq-agent/` was touched.

**Do not mix rounds in one PR.**
