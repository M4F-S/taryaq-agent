# Makefile - Taryaq Sovereign Clinical AI Agent
# Track: Best Societal AI Agent | Dubai AI Agents Championship 2026-2027

CC ?= cc
CFLAGS ?= -std=c99 -Wall -Wextra -Werror -O2 -Iadapters
LDFLAGS ?= -lm -lcurl -lsqlite3

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
    LIB_EXT = dylib
    SHARED_FLAGS = -dynamiclib
else
    LIB_EXT = so
    SHARED_FLAGS = -shared
endif

BUILD_DIR = build
BIN = $(BUILD_DIR)/taryaq_agent
LIB = $(BUILD_DIR)/libtaryaq.$(LIB_EXT)

ADAPTER_SRCS = adapters/taryaq_clinical_adapter.c \
               adapters/taryaq_rag_engine.c \
               adapters/grounding_shield.c \
               adapters/nibras_arabic_fademem_adapter.c \
               adapters/qala_mcp_policy_adapter.c

export TARYAQ_AUDIT_KEY ?= test-key-32-bytes-minimum-000000

all: corpus $(BIN) $(LIB)

corpus:
	python3 tools/build_corpus_index.py

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BIN): src/taryaq_agent.c $(ADAPTER_SRCS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) src/taryaq_agent.c $(ADAPTER_SRCS) $(LDFLAGS) -o $(BIN)

$(LIB): $(ADAPTER_SRCS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -fPIC $(SHARED_FLAGS) $(ADAPTER_SRCS) -lm -lsqlite3 -o $(LIB)

test: corpus $(BIN) $(LIB)
	@echo "=================================================================="
	@echo " RUNNING TARYAQ STANDALONE C99 ADAPTER ACCEPTANCE TEST"
	@echo "=================================================================="
	$(CC) $(CFLAGS) -DTARYAQ_STANDALONE_TEST adapters/taryaq_clinical_adapter.c adapters/taryaq_rag_engine.c -lm -lsqlite3 -o $(BUILD_DIR)/test_taryaq_adapter
	./$(BUILD_DIR)/test_taryaq_adapter
	@echo "[PASS] Taryaq clinical adapter tests passed."
	@echo "=================================================================="
	@echo " RUNNING TARYAQ NATIVE CLI ACCEPTANCE BATTERY"
	@echo "=================================================================="
	./$(BIN) --test
	@echo "=================================================================="
	@echo " RUNNING TARYAQ PYTHON ACCEPTANCE SUITE"
	@echo "=================================================================="
	python3 tests/test_taryaq_acceptance.py
	@echo "=================================================================="
	@echo " RUNNING TARYAQ CRYPTOGRAPHIC AUDIT TAMPER SUITE"
	@echo "=================================================================="
	python3 tests/test_audit_tamper.py
	@echo "=================================================================="
	@echo " RUNNING TARYAQ LONGITUDINAL PATIENT TIMELINE SUITE"
	@echo "=================================================================="
	python3 tests/test_patient_timeline.py
	@echo "=================================================================="
	@echo " RUNNING TARYAQ CLINICAL RAG & DHA GROUNDING SUITE"
	@echo "=================================================================="
	python3 tests/test_clinical_rag.py
	@echo "=================================================================="
	@echo " RUNNING TARYAQ GOMAA LONGITUDINAL PATIENT MEMORY SUITE"
	@echo "=================================================================="
	python3 tests/test_taryaq_gomaa.py
	@echo "=================================================================="
	@echo " RUNNING TARYAQ HMAC AUDIT CHAIN VERIFICATION"
	@echo "=================================================================="
	$(MAKE) audit-verify AUDIT_FILE=taryaq_audit.jsonl
	@echo "[SUCCESS] All Taryaq Agent verification suites passed deterministically!"

rag-demo: $(BIN)
	@TARYAQ_AUDIT_KEY="$${TARYAQ_AUDIT_KEY:-test-key-32-bytes-minimum-000000}" ./$(BIN) --rag-demo

timeline-demo: $(BIN)
	@TARYAQ_AUDIT_KEY="$${TARYAQ_AUDIT_KEY:-test-key-32-bytes-minimum-000000}" ./$(BIN) --timeline-demo

audit-verify:
	@TARYAQ_AUDIT_KEY="$${TARYAQ_AUDIT_KEY:?set TARYAQ_AUDIT_KEY}" python3 tools/verify_audit.py --ledger $${AUDIT_FILE:-taryaq_audit.jsonl}

verify: test audit-verify

clean:
	rm -rf $(BUILD_DIR)
	rm -f taryaq_audit.jsonl qala-audit.jsonl taryaq_encounters.jsonl
	find . -type d -name "__pycache__" -exec rm -rf {} + 2>/dev/null || true
	find . -type f -name "*.pyc" -delete 2>/dev/null || true

.PHONY: all corpus test verify audit-verify rag-demo timeline-demo clean
