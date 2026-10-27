#!/usr/bin/env python3
"""tools/build_corpus_index.py - Builds SQLite FTS5 Index and SHA-256 Manifest for Taryaq Corpus
Dubai Create AI Agents Championship 2026-2027 | Best Societal AI Agent
"""

import sys
import os
import re
import json
import sqlite3
import hashlib

ROOT_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
CORPUS_DIR = os.path.join(ROOT_DIR, "corpus")
DB_PATH = os.path.join(CORPUS_DIR, "taryaq_corpus.db")
MANIFEST_PATH = os.path.join(CORPUS_DIR, "manifest.json")
MANIFEST_SHA_PATH = os.path.join(CORPUS_DIR, "manifest.sha256")

CORPUS_FILES = [
    "dha_sn_03_2019.txt",
    "drug_monographs.txt",
    "uae_controlled_substances.txt",
]


def sha256_str(text: str) -> str:
    return hashlib.sha256(text.encode("utf-8")).hexdigest()


def sha256_file(path: str) -> str:
    h = hashlib.sha256()
    with open(path, "rb") as f:
        while chunk := f.read(65536):
            h.update(chunk)
    return h.hexdigest()


def parse_corpus_file(filepath: str):
    with open(filepath, "r", encoding="utf-8") as f:
        content = f.read()

    doc_id = None
    doc_title = None
    doc_auth = None

    for line in content.splitlines():
        if line.startswith("# DOCUMENT:"):
            doc_id = line.split(":", 1)[1].strip()
        elif line.startswith("# TITLE:"):
            doc_title = line.split(":", 1)[1].strip()
        elif line.startswith("# AUTHORITY:"):
            doc_auth = line.split(":", 1)[1].strip()

    if not doc_id:
        doc_id = os.path.splitext(os.path.basename(filepath))[0].upper()

    # Split into sections
    section_blocks = re.split(r"(?=\[SECTION:\s*[^\]]+\])", content)
    chunks = []

    for block in section_blocks:
        block = block.strip()
        if not block.startswith("[SECTION:"):
            continue

        sec_match = re.search(r"\[SECTION:\s*([^\]]+)\]", block)
        title_match = re.search(r"\[TITLE:\s*([^\]]+)\]", block)

        section = sec_match.group(1).strip() if sec_match else "UNKNOWN"
        title = title_match.group(1).strip() if title_match else section

        # Passage text is everything after the tags
        lines = []
        for line in block.splitlines():
            line_str = line.strip()
            if line_str.startswith("[SECTION:") or line_str.startswith("[TITLE:"):
                continue
            if line_str.startswith("#"):
                continue
            if line_str:
                lines.append(line_str)

        passage = " ".join(lines)
        passage_hash = sha256_str(passage)

        clean_sec = re.sub(r"[^A-Za-z0-9_]+", "_", section).strip("_")
        chunk_id = f"{doc_id}_{clean_sec}"

        chunks.append({
            "chunk_id": chunk_id,
            "doc_id": doc_id,
            "doc_title": doc_title or doc_id,
            "authority": doc_auth or "UAE Health Authority",
            "section": section,
            "title": title,
            "passage": passage,
            "passage_sha256": passage_hash,
        })

    return doc_id, doc_title, chunks


def build_index():
    os.makedirs(CORPUS_DIR, exist_ok=True)
    manifest = {
        "version": "1.0",
        "description": "Taryaq Sovereign Clinical & DHA Statutory Corpus Manifest",
        "documents": {},
        "chunks": {},
    }

    # Reset SQLite DB
    if os.path.exists(DB_PATH):
        os.remove(DB_PATH)

    conn = sqlite3.connect(DB_PATH)
    cur = conn.cursor()

    cur.execute("""
        CREATE TABLE corpus_chunks (
            chunk_id TEXT PRIMARY KEY,
            doc_id TEXT NOT NULL,
            doc_title TEXT NOT NULL,
            section TEXT NOT NULL,
            title TEXT NOT NULL,
            passage TEXT NOT NULL,
            passage_sha256 TEXT NOT NULL
        );
    """)

    cur.execute("""
        CREATE VIRTUAL TABLE corpus_fts USING fts5(
            doc_id UNINDEXED,
            section UNINDEXED,
            title,
            passage,
            tokenize='porter'
        );
    """)

    all_chunks = []

    for fname in CORPUS_FILES:
        fpath = os.path.join(CORPUS_DIR, fname)
        if not os.path.exists(fpath):
            sys.stderr.write(f"[-] Warning: Corpus file not found: {fpath}\n")
            continue

        fhash = sha256_file(fpath)
        doc_id, doc_title, chunks = parse_corpus_file(fpath)

        manifest["documents"][doc_id] = {
            "file": fname,
            "title": doc_title,
            "sha256": fhash,
            "chunk_count": len(chunks),
        }

        for chunk in chunks:
            cur.execute("""
                INSERT INTO corpus_chunks (chunk_id, doc_id, doc_title, section, title, passage, passage_sha256)
                VALUES (?, ?, ?, ?, ?, ?, ?);
            """, (
                chunk["chunk_id"],
                chunk["doc_id"],
                chunk["doc_title"],
                chunk["section"],
                chunk["title"],
                chunk["passage"],
                chunk["passage_sha256"],
            ))

            cur.execute("""
                INSERT INTO corpus_fts (doc_id, section, title, passage)
                VALUES (?, ?, ?, ?);
            """, (
                chunk["doc_id"],
                chunk["section"],
                chunk["title"],
                chunk["passage"],
            ))

            manifest["chunks"][chunk["chunk_id"]] = {
                "doc_id": chunk["doc_id"],
                "section": chunk["section"],
                "title": chunk["title"],
                "passage_sha256": chunk["passage_sha256"],
                "passage_preview": chunk["passage"][:120] + ("..." if len(chunk["passage"]) > 120 else ""),
            }
            all_chunks.append(chunk)

    conn.commit()
    conn.close()

    # Write manifest.json
    with open(MANIFEST_PATH, "w", encoding="utf-8") as f:
        json.dump(manifest, f, indent=2, ensure_ascii=False)

    # Write manifest.sha256
    with open(MANIFEST_SHA_PATH, "w", encoding="utf-8") as f:
        for fname in CORPUS_FILES:
            fp = os.path.join(CORPUS_DIR, fname)
            if os.path.exists(fp):
                f.write(f"{sha256_file(fp)}  {fname}\n")
        f.write(f"{sha256_file(MANIFEST_PATH)}  manifest.json\n")
        f.write(f"{sha256_file(DB_PATH)}  taryaq_corpus.db\n")

    print(f"[SUCCESS] Built Taryaq Corpus Index: {len(all_chunks)} chunks indexed into {DB_PATH}")
    print(f"[SUCCESS] Generated manifest: {MANIFEST_PATH}")
    return 0


if __name__ == "__main__":
    sys.exit(build_index())
