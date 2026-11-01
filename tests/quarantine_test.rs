//! Integration Tests for SPRINT-04: HITL Web Ingestion & Quarantine Buffer
//! Dubai Create AI Agents Championship 2026–2027 | Best Societal AI Agent
//! Verifies Web Alert Scanner, Quarantine Staging Buffer, The Pharmacist Gate
//! Approval/Rejection Workflows, and Strict Live Engine Isolation.

use std::fs;
use std::path::{Path, PathBuf};
use std::sync::atomic::{AtomicUsize, Ordering};
use std::sync::Arc;
use std::time::{SystemTime, UNIX_EPOCH};
use taryaq::clinical::{ClinicalDatabase, InteractionCheckVerdict};
use taryaq::quarantine::{
    AlertSource, HitlError, ThePharmacistGate, WebAlertScanner, DEFAULT_PHARMACIST_REVIEWER,
};

static DB_COUNTER: AtomicUsize = AtomicUsize::new(1);

fn create_temp_clinical_db() -> (PathBuf, Arc<ClinicalDatabase>) {
    let canonical = Path::new("corpus/taryaq_clinical.db");
    assert!(
        canonical.exists(),
        "Canonical database corpus/taryaq_clinical.db must exist"
    );

    let count = DB_COUNTER.fetch_add(1, Ordering::SeqCst);
    let pid = std::process::id();
    let nanos = SystemTime::now()
        .duration_since(UNIX_EPOCH)
        .unwrap()
        .as_nanos();
    let temp_path = std::env::temp_dir().join(format!("taryaq_quarantine_{}_{}_{}.db", pid, nanos, count));
    fs::copy(canonical, &temp_path).expect("Failed to create temporary test database copy");

    let db = ClinicalDatabase::open(&temp_path).expect("Failed to open temporary test database");
    (temp_path, Arc::new(db))
}

fn cleanup_temp_db(path: PathBuf) {
    let _ = fs::remove_file(path);
}

// =========================================================================
// 1. QUARANTINE BUFFER INITIAL STATE & TABLE CHECKS
// =========================================================================

#[test]
fn test_01_quarantine_table_active_and_has_initial_records() {
    let (path, db) = create_temp_clinical_db();
    let gate = ThePharmacistGate::new(db.clone());

    let all_records = gate.list_findings(None).expect("List all findings");
    assert!(
        all_records.len() >= 2,
        "Expected at least 2 baseline quarantine records, found {}",
        all_records.len()
    );

    let pending = gate
        .list_pending_findings()
        .expect("List pending findings");
    assert!(
        !pending.is_empty(),
        "Expected at least 1 pending quarantine record"
    );

    for rec in &pending {
        assert_eq!(rec.review_status, "PENDING");
        assert!(rec.staging_id > 0);
        assert!(!rec.source_url.is_empty());
        assert!(!rec.extracted_finding.is_empty());
    }

    cleanup_temp_db(path);
}

// =========================================================================
// 2. WEB ALERT SCANNER TESTS
// =========================================================================

#[test]
fn test_02_web_alert_scanner_source_filtering() {
    let (path, db) = create_temp_clinical_db();
    let scanner = WebAlertScanner::new(db);

    // Scan OpenFDA
    let fda_findings = scanner.scan_sources(AlertSource::OpenFda);
    assert_eq!(fda_findings.len(), 1);
    assert_eq!(fda_findings[0].source_name, "OpenFDA");
    assert_eq!(fda_findings[0].candidate_drug_a_name, "Ritonavir");
    assert_eq!(fda_findings[0].candidate_drug_b_name, "Tacrolimus");
    assert_eq!(fda_findings[0].proposed_severity, "CONTRAINDICATED");

    // Scan PubMed
    let pubmed_findings = scanner.scan_sources(AlertSource::PubMed);
    assert_eq!(pubmed_findings.len(), 1);
    assert_eq!(pubmed_findings[0].source_name, "PubMed");
    assert_eq!(pubmed_findings[0].candidate_drug_a_name, "Linezolid");
    assert_eq!(pubmed_findings[0].candidate_drug_b_name, "Methylene Blue");
    assert_eq!(pubmed_findings[0].evidence_pmid.as_deref(), Some("36912345"));

    // Scan WHO
    let who_findings = scanner.scan_sources(AlertSource::Who);
    assert_eq!(who_findings.len(), 1);
    assert_eq!(who_findings[0].source_name, "WHO");
    assert_eq!(who_findings[0].candidate_drug_a_name, "Semaglutide");
    assert_eq!(who_findings[0].candidate_drug_b_name, "Propofol");

    // Scan DHA
    let dha_findings = scanner.scan_sources(AlertSource::Dha);
    assert_eq!(dha_findings.len(), 1);
    assert_eq!(dha_findings[0].source_name, "DHA");
    assert_eq!(dha_findings[0].candidate_drug_a_name, "Metformin");
    assert_eq!(dha_findings[0].candidate_drug_b_name, "Iodinated Radiocontrast");

    // Scan MOHAP
    let mohap_findings = scanner.scan_sources(AlertSource::Mohap);
    assert_eq!(mohap_findings.len(), 1);
    assert_eq!(mohap_findings[0].source_name, "MOHAP");
    assert_eq!(mohap_findings[0].candidate_drug_a_name, "Colchicine");

    // Scan All
    let all_findings = scanner.scan_sources(AlertSource::All);
    assert_eq!(all_findings.len(), 5);

    cleanup_temp_db(path);
}

#[test]
fn test_03_web_alert_scanner_ingests_into_quarantine_buffer() {
    let (path, db) = create_temp_clinical_db();
    let scanner = WebAlertScanner::new(db.clone());
    let gate = ThePharmacistGate::new(db.clone());

    let initial_count = gate.list_findings(None).unwrap().len();

    // Ingest OpenFDA finding
    let staged_ids = scanner
        .scan_and_stage(AlertSource::OpenFda)
        .expect("Scan and stage");
    assert_eq!(staged_ids.len(), 1);

    let new_count = gate.list_findings(None).unwrap().len();
    assert_eq!(new_count, initial_count + 1);

    let rec = gate
        .get_finding(staged_ids[0])
        .unwrap()
        .expect("Finding exists");
    assert_eq!(rec.review_status, "PENDING");
    assert!(rec.source_url.contains("fda.gov"));
    assert!(rec.extracted_finding.contains("Ritonavir"));
    assert_eq!(rec.evidence_pmid.as_deref(), Some("37592014"));

    cleanup_temp_db(path);
}

// =========================================================================
// 3. STRICT ISOLATION OF UNAPPROVED QUARANTINE RECORDS
// =========================================================================

#[test]
fn test_04_unapproved_alerts_strictly_isolated_from_live_clinical_engine() {
    let (path, db) = create_temp_clinical_db();
    let scanner = WebAlertScanner::new(db.clone());

    // Ingest the novel Linezolid + Methylene Blue alert
    let findings = scanner.scan_sources(AlertSource::PubMed);
    let staged_ids = scanner.ingest_findings(&findings).expect("Ingest finding");
    assert_eq!(staged_ids.len(), 1);

    // Verify finding is strictly quarantined and NOT in live production drug_interactions
    let live_interaction = db
        .get_interaction("J01XX08", "V03AB17")
        .expect("Query live table");
    assert!(
        live_interaction.is_none(),
        "Quarantined alert MUST NOT be present in live drug_interactions table before pharmacist approval"
    );

    // Verify reverse order also returns None
    let live_reverse = db
        .get_interaction("V03AB17", "J01XX08")
        .expect("Query live table reverse");
    assert!(
        live_reverse.is_none(),
        "Quarantined alert MUST NOT be discoverable in reverse order"
    );

    cleanup_temp_db(path);
}

// =========================================================================
// 4. THE PHARMACIST GATE: APPROVAL WORKFLOW & PROMOTION
// =========================================================================

#[test]
fn test_05_pharmacist_gate_approval_promotes_to_production() {
    let (path, db) = create_temp_clinical_db();
    let scanner = WebAlertScanner::new(db.clone());
    let gate = ThePharmacistGate::new(db.clone());

    // Ingest PubMed alert: Linezolid (J01XX08) + Methylene Blue (V03AB17)
    let findings = scanner.scan_sources(AlertSource::PubMed);
    let staged_ids = scanner.ingest_findings(&findings).expect("Ingest finding");
    let staging_id = staged_ids[0];

    // Verify initial state is PENDING
    let pending_rec = gate.get_finding(staging_id).unwrap().unwrap();
    assert_eq!(pending_rec.review_status, "PENDING");

    // Mohamed Fathy, BPharm conducts clinical review and approves finding
    let reviewer = "Mohamed Fathy, BPharm (DHA-PHARM-8821)";
    let clinical_notes = "Validated with Crit Care Med 2023; dual MAO-A inhibition triggers lethal hyperthermic serotonin toxicity. Promoted to CONTRAINDICATED tier.";

    let receipt = gate
        .approve_finding(staging_id, clinical_notes, Some(reviewer))
        .expect("Pharmacist Gate Approval");

    // Verify approval receipt
    assert_eq!(receipt.staging_id, staging_id);
    assert_eq!(receipt.alert_code, "LINEZOLID_METHYLENE_BLUE_FATAL_SEROTONIN");
    assert_eq!(receipt.severity, "CONTRAINDICATED");
    assert_eq!(receipt.drug_a_atc, "J01XX08");
    assert_eq!(receipt.drug_b_atc, "V03AB17");
    assert_eq!(receipt.reviewer_credentials, reviewer);
    assert!(receipt.reviewer_notes.contains(clinical_notes));

    // Verify quarantine table record status updated to APPROVED
    let updated_rec = gate.get_finding(staging_id).unwrap().unwrap();
    assert_eq!(updated_rec.review_status, "APPROVED");
    assert!(updated_rec.reviewer_notes.unwrap().contains(reviewer));
    assert!(updated_rec.reviewed_at.is_some());

    // Verify the rule is now live in production drug_interactions table
    let promoted_inter = db
        .get_interaction("J01XX08", "V03AB17")
        .expect("Query live table")
        .expect("Promoted interaction must exist in production table");
    assert_eq!(
        promoted_inter.alert_code,
        "LINEZOLID_METHYLENE_BLUE_FATAL_SEROTONIN"
    );
    assert_eq!(
        promoted_inter.severity(),
        taryaq::clinical::InteractionSeverity::Contraindicated
    );
    assert!(promoted_inter.evidence_citation.contains("36912345"));

    // Verify commutative order invariance on promoted interaction
    let reverse_inter = db
        .get_interaction("V03AB17", "J01XX08")
        .expect("Query reverse")
        .expect("Promoted interaction must be commutatively queryable");
    assert_eq!(
        reverse_inter.alert_code,
        "LINEZOLID_METHYLENE_BLUE_FATAL_SEROTONIN"
    );

    // Verify active patient profile screening immediately flags this newly promoted contraindication
    let screening = db
        .screen_patient_profile(&["J01XX08", "V03AB17"])
        .expect("Screening patient profile");

    assert_eq!(
        screening.verdict,
        InteractionCheckVerdict::ContraindicatedBlock
    );
    assert!(!screening.interactions_detected.is_empty());
    assert_eq!(
        screening.interactions_detected[0].alert_code,
        "LINEZOLID_METHYLENE_BLUE_FATAL_SEROTONIN"
    );

    cleanup_temp_db(path);
}

// =========================================================================
// 5. THE PHARMACIST GATE: REJECTION WORKFLOW
// =========================================================================

#[test]
fn test_06_pharmacist_gate_rejection_leaves_production_untouched() {
    let (path, db) = create_temp_clinical_db();
    let scanner = WebAlertScanner::new(db.clone());
    let gate = ThePharmacistGate::new(db.clone());

    // Ingest WHO alert: Semaglutide + Propofol
    let findings = scanner.scan_sources(AlertSource::Who);
    let staged_ids = scanner.ingest_findings(&findings).expect("Ingest finding");
    let staging_id = staged_ids[0];

    // Mohamed Fathy, BPharm reviews and rejects for outpatient formulary
    let rejection_notes =
        "Inpatient surgical induction risk only; not applicable to outpatient formulary co-dispensing.";
    let receipt = gate
        .reject_finding(
            staging_id,
            rejection_notes,
            Some(DEFAULT_PHARMACIST_REVIEWER),
        )
        .expect("Pharmacist Gate Rejection");

    assert_eq!(receipt.staging_id, staging_id);
    assert_eq!(receipt.reviewer_credentials, DEFAULT_PHARMACIST_REVIEWER);
    assert!(receipt.reviewer_notes.contains(rejection_notes));

    // Verify quarantine table record status updated to REJECTED
    let updated_rec = gate.get_finding(staging_id).unwrap().unwrap();
    assert_eq!(updated_rec.review_status, "REJECTED");
    assert!(updated_rec.reviewer_notes.unwrap().contains("REJECTED by"));

    // Verify the interaction is NOT in production tables
    let live = db
        .get_interaction("A10BJ06", "N01AX10")
        .expect("Query live table");
    assert!(
        live.is_none(),
        "Rejected finding must NEVER be promoted to production"
    );

    cleanup_temp_db(path);
}

// =========================================================================
// 6. SAFETY BOUNDARIES, PERMISSION & ERROR HANDLING
// =========================================================================

#[test]
fn test_07_invalid_transitions_prevented() {
    let (path, db) = create_temp_clinical_db();
    let scanner = WebAlertScanner::new(db.clone());
    let gate = ThePharmacistGate::new(db.clone());

    let staged_ids = scanner
        .scan_and_stage(AlertSource::PubMed)
        .expect("Scan and stage");
    let staging_id = staged_ids[0];

    // Approve the record
    gate.approve_finding(staging_id, "Initial approval", None)
        .expect("Approve record");

    // Attempting to approve an already approved record must fail
    let err_approve = gate
        .approve_finding(staging_id, "Second approval", None)
        .unwrap_err();
    match err_approve {
        HitlError::InvalidTransition(id, status) => {
            assert_eq!(id, staging_id);
            assert_eq!(status, "APPROVED");
        }
        other => panic!("Expected InvalidTransition, got {:?}", other),
    }

    // Attempting to reject an already approved record must fail
    let err_reject = gate
        .reject_finding(staging_id, "Reject approved record", None)
        .unwrap_err();
    match err_reject {
        HitlError::InvalidTransition(id, status) => {
            assert_eq!(id, staging_id);
            assert_eq!(status, "APPROVED");
        }
        other => panic!("Expected InvalidTransition, got {:?}", other),
    }

    cleanup_temp_db(path);
}

#[test]
fn test_08_unauthorized_empty_credentials_rejected() {
    let (path, db) = create_temp_clinical_db();
    let scanner = WebAlertScanner::new(db.clone());
    let gate = ThePharmacistGate::new(db.clone());

    let staged_ids = scanner
        .scan_and_stage(AlertSource::PubMed)
        .expect("Scan and stage");
    let staging_id = staged_ids[0];

    // Empty reviewer credentials rejected
    let err = gate
        .approve_finding(staging_id, "Valid notes", Some("   "))
        .unwrap_err();
    match err {
        HitlError::UnauthorizedReviewer(msg) => {
            assert!(msg.contains("cannot be empty"));
        }
        other => panic!("Expected UnauthorizedReviewer, got {:?}", other),
    }

    cleanup_temp_db(path);
}

#[test]
fn test_09_quarantine_record_not_found() {
    let (path, db) = create_temp_clinical_db();
    let gate = ThePharmacistGate::new(db);

    let err = gate
        .approve_finding(999_999, "Notes", None)
        .unwrap_err();
    match err {
        HitlError::RecordNotFound(id) => {
            assert_eq!(id, 999_999);
        }
        other => panic!("Expected RecordNotFound, got {:?}", other),
    }

    cleanup_temp_db(path);
}

#[test]
fn test_10_end_to_end_pharmacovigilance_discovery_and_screening_workflow() {
    let (path, db) = create_temp_clinical_db();
    let scanner = WebAlertScanner::new(db.clone());
    let gate = ThePharmacistGate::new(db.clone());

    // 1. Initial State: Ritonavir (J05AE03) and Tacrolimus (L04AD02) interaction does not exist in live matrix
    assert!(db.get_interaction("J05AE03", "L04AD02").unwrap().is_none());

    // 2. Automated web alert ingestion stages finding
    let staged_ids = scanner
        .scan_and_stage(AlertSource::OpenFda)
        .expect("Scan OpenFDA");
    assert_eq!(staged_ids.len(), 1);
    let staging_id = staged_ids[0];

    // 3. Mohamed Fathy, BPharm approves with DHA registration reference
    let receipt = gate
        .approve_finding(
            staging_id,
            "Urgent FDA Boxed Warning Alert; CYP3A4/P-gp 100-fold AUC elevation",
            Some("Mohamed Fathy, BPharm (DHA-PHARM-8821)"),
        )
        .expect("Approve finding");

    assert_eq!(receipt.alert_code, "RITONAVIR_TACROLIMUS_CALCINEURIN_TOXICITY");

    // 4. Clinical engine now identifies the interaction as Contraindicated
    let live = db
        .get_interaction("J05AE03", "L04AD02")
        .unwrap()
        .expect("Live interaction must now exist");
    assert_eq!(
        live.severity(),
        taryaq::clinical::InteractionSeverity::Contraindicated
    );

    // 5. Patient profile screening immediately stops dispensing
    let screening = db
        .screen_patient_profile(&["J05AE03", "L04AD02"])
        .unwrap();
    assert_eq!(
        screening.verdict,
        InteractionCheckVerdict::ContraindicatedBlock
    );

    cleanup_temp_db(path);
}

#[test]
fn test_11_sha256_provenance_verification() {
    let (path, db) = create_temp_clinical_db();
    let scanner = WebAlertScanner::new(db.clone());

    let all_findings = scanner.scan_sources(AlertSource::All);
    assert_eq!(all_findings.len(), 5);

    for f in &all_findings {
        // SHA-256 must be exactly 64 hex characters
        assert_eq!(f.evidence_sha256.len(), 64);
        assert!(f.evidence_sha256.chars().all(|c| c.is_ascii_hexdigit()));

        // Provenance hash recalculation check
        let hash_input = format!("{}:{}:{}:{}", f.source_name, f.source_url, f.evidence_citation, f.extracted_text);
        let recalculated = taryaq::quarantine::compute_evidence_sha256(&hash_input);
        assert_eq!(f.evidence_sha256, recalculated);
    }

    cleanup_temp_db(path);
}

#[test]
fn test_12_inspect_quarantine_record_payload() {
    let (path, db) = create_temp_clinical_db();
    let scanner = WebAlertScanner::new(db.clone());
    let gate = ThePharmacistGate::new(db.clone());

    let staged_ids = scanner
        .scan_and_stage(AlertSource::PubMed)
        .expect("Scan PubMed");
    assert_eq!(staged_ids.len(), 1);
    let staging_id = staged_ids[0];

    let inspected = gate
        .inspect_finding(staging_id)
        .expect("Inspect finding")
        .expect("Record must exist");

    let (record, payload_opt) = inspected;
    assert_eq!(record.staging_id, staging_id);
    assert_eq!(record.review_status, "PENDING");
    assert_eq!(record.evidence_pmid.as_deref(), Some("36912345"));

    let payload = payload_opt.expect("Payload must be valid JSON");
    assert_eq!(payload.drug_a_name, "Linezolid");
    assert_eq!(payload.drug_b_name, "Methylene Blue");
    assert_eq!(payload.severity, "CONTRAINDICATED");
    assert_eq!(payload.alert_code, "LINEZOLID_METHYLENE_BLUE_FATAL_SEROTONIN");
    assert_eq!(payload.evidence_sha256.len(), 64);

    cleanup_temp_db(path);
}
