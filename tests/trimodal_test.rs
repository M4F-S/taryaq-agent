//! Integration Tests for TARYAQ Tri-Modal Interfaces & Local Voice Pipeline
//! Dubai Create AI Agents Championship 2026–2027

use std::fs::File;
use std::process::Command;
use std::sync::Arc;
use taryaq::clinical::ClinicalDatabase;
use taryaq::safety::TaryaqTriageLevel;
use taryaq::trimodal::audit::AuditLedger;
use taryaq::trimodal::copilot::{
    ClinicianAmbientCopilot, ClinicianEncounterInput,
};
use taryaq::trimodal::pharmacist::{
    ClinicianOverrideSubmission, PharmacistError, PharmacistVerificationConsole,
    PharmacistVerificationInput, TokenRegistry,
};
use taryaq::trimodal::companion::{
    DialectCategory, PatientHealthCompanion, RedFlagCategory,
};
use taryaq::voice::{AudioFormat, VoiceError, VoicePipeline};

fn get_clinical_db() -> Arc<ClinicalDatabase> {
    Arc::new(ClinicalDatabase::open_default().expect("Failed to open clinical database"))
}

#[test]
fn test_01_clinician_ambient_copilot_soap_notes_and_ranked_ddx() {
    let db = get_clinical_db();
    let copilot = ClinicianAmbientCopilot::new(Some(db));

    let dialogue = r#"
Doctor: Good morning Mr. Al-Mansoor. What brings you to the clinic today?
Patient: Doctor, عندي عوار ونغز قوي بالصدر وضيق تنفس من الصباح.
Doctor: I see. Let me examine you. Blood pressure is 155/95 mmHg, heart rate is 92 bpm.
Doctor: Your routine labs indicate eGFR is 45 ml/min, which is CKD Stage 3a.
Patient: هل الضغط مرتفع وايد يا دكتور؟
Doctor: Yes, we need to monitor your hypertension and renal function closely.
"#;

    let input = ClinicianEncounterInput {
        dialogue: dialogue.to_string(),
        turns: Vec::new(),
        patient_id: Some("DXB-PAT-7721".to_string()),
        patient_egfr: Some(45.0),
        proposed_prescriptions: vec!["Amlodipine".to_string()],
        active_medications: vec!["Atorvastatin".to_string()],
        documented_vitals: None,
    };

    let soap = copilot.process_encounter(&input);

    // Assert SOAP components
    assert!(!soap.encounter_id.is_empty());
    assert_eq!(soap.patient_id, "DXB-PAT-7721");
    assert!(soap.subjective.contains("CHIEF COMPLAINT"));
    assert!(soap.objective.contains("Blood Pressure: 155/95"));
    assert!(soap.objective.contains("Estimated GFR"));
    assert!(soap.assessment.contains("RANKED DIFFERENTIAL DIAGNOSES"));
    assert!(soap.plan.contains("RECOMMENDED MANAGEMENT"));

    // Assert trace-linked evidence hashes
    assert!(!soap.trace_evidence.is_empty());
    for ev in &soap.trace_evidence {
        assert_eq!(ev.evidence_sha256.len(), 64);
        assert!(!ev.snippet.is_empty());
    }

    // Assert ranked DDx contains ICD-10 codes
    assert!(!soap.ranked_ddx.is_empty());
    let icd_codes: Vec<&str> = soap.ranked_ddx.iter().map(|d| d.icd10_code.as_str()).collect();
    assert!(icd_codes.contains(&"I10") || icd_codes.contains(&"N18.3") || icd_codes.contains(&"I21.9"));

    // Assert Markdown note contains alert / table
    assert!(soap.markdown_note.contains("## S - Subjective"));
    assert!(soap.markdown_note.contains("## O - Objective"));
    assert!(soap.markdown_note.contains("## A - Assessment"));
    assert!(soap.markdown_note.contains("## P - Plan"));
    assert!(soap.markdown_note.contains("ICD-10 Differential Diagnoses Coding Table"));
}

#[test]
fn test_02_clinician_pre_order_cds_interrupt_metformin_renal_cutoff() {
    let db = get_clinical_db();
    let copilot = ClinicianAmbientCopilot::new(Some(db));

    let input = ClinicianEncounterInput {
        dialogue: "Doctor: Patient diagnosed with Type 2 diabetes. Ordering Metformin 1000mg BID.".to_string(),
        turns: Vec::new(),
        patient_id: Some("DXB-PAT-RENAL".to_string()),
        patient_egfr: Some(24.5), // < 30 mL/min cutoff!
        proposed_prescriptions: vec!["Metformin".to_string()],
        active_medications: Vec::new(),
        documented_vitals: None,
    };

    let soap = copilot.process_encounter(&input);

    assert!(soap.cds_interrupt.is_some());
    let interrupt = soap.cds_interrupt.unwrap();
    assert!(interrupt.triggered);
    assert_eq!(interrupt.alert_code, "METFORMIN_SEVERE_RENAL_IMPAIRMENT");
    assert_eq!(interrupt.severity, "CONTRAINDICATED");
    assert!(interrupt.clinical_rationale.contains("< 30 mL/min cutoff"));
    assert_eq!(interrupt.statutory_doc_id, "DHA_DHIC_HEIPD_CG_07");

    // Plan must show order drafting halted
    assert!(soap.plan.contains("ORDER DRAFT HALTED"));
    assert!(soap.markdown_note.contains("PRE-ORDER CDS INTERRUPT TRIGGERED"));
}

#[test]
fn test_03_clinician_pre_order_cds_interrupt_colchicine_clarithromycin() {
    let db = get_clinical_db();
    let copilot = ClinicianAmbientCopilot::new(Some(db));

    let input = ClinicianEncounterInput {
        dialogue: "Doctor evaluates patient with acute gout. Prescribing Colchicine. Patient currently taking Clarithromycin for chest infection.".to_string(),
        turns: Vec::new(),
        patient_id: Some("DXB-PAT-TOXIC".to_string()),
        patient_egfr: Some(75.0),
        proposed_prescriptions: vec!["Colchicine".to_string()],
        active_medications: vec!["Clarithromycin".to_string()],
        documented_vitals: None,
    };

    let soap = copilot.process_encounter(&input);

    assert!(soap.cds_interrupt.is_some());
    let interrupt = soap.cds_interrupt.unwrap();
    assert!(interrupt.triggered);
    assert_eq!(interrupt.severity, "CONTRAINDICATED");
    assert!(soap.plan.contains("ORDER DRAFT HALTED"));
}

#[test]
fn test_04_pharmacist_polypharmacy_screening_safe_regimen() {
    let db = get_clinical_db();
    let console = PharmacistVerificationConsole::new(Some(db), None, None);

    let input = PharmacistVerificationInput {
        patient_id: "DXB-PAT-SAFE".to_string(),
        medication_regimen: vec!["Lisinopril".to_string(), "Amlodipine".to_string()],
        patient_egfr: 85.0,
        primary_diagnosis_icd10: Some("I10".to_string()),
        net_claim_aed: 350.0,
        claim_submission_delay_days: 12,
        is_resubmission: false,
        resubmission_delay_days: None,
        clinician_override: None,
    };

    let report = console.verify_patient_regimen(&input).expect("Verification failed");

    assert!(report.is_dispensation_approved);
    assert!(!report.is_contraindication_blocked);
    assert_eq!(report.primary_verdict, "CLINICAL_SAFE");
    assert_eq!(report.primary_alert_code, "DHA_NABIDH_SAFE");
    assert_eq!(report.total_drugs_screened, 2);
    assert_eq!(report.dha_claims_scrubbing.overdue_days, 0);
    assert_eq!(report.dha_claims_scrubbing.penalty_amount_aed, 0.0);
    assert!(report.audit_committed);
}

#[test]
fn test_05_pharmacist_contraindication_block_simvastatin_clarithromycin() {
    let db = get_clinical_db();
    let console = PharmacistVerificationConsole::new(Some(db), None, None);

    let input = PharmacistVerificationInput {
        patient_id: "DXB-PAT-BLOCK".to_string(),
        medication_regimen: vec!["Simvastatin".to_string(), "Clarithromycin".to_string()],
        patient_egfr: 70.0,
        primary_diagnosis_icd10: Some("E78.5".to_string()),
        net_claim_aed: 1200.0,
        claim_submission_delay_days: 10,
        is_resubmission: false,
        resubmission_delay_days: None,
        clinician_override: None,
    };

    let report = console.verify_patient_regimen(&input).expect("Verification failed");

    assert!(!report.is_dispensation_approved);
    assert!(report.is_contraindication_blocked);
    assert_eq!(report.primary_verdict, "CLINICAL_CONTRAINDICATION_BLOCK");
    assert!(report.primary_alert_code.contains("RHABDO"));
    assert!(report.primary_clinical_rationale.contains("rhabdomyolysis") || report.primary_clinical_rationale.contains("renal failure"));
}

#[test]
fn test_06_pharmacist_single_use_override_and_replay_attack_409() {
    let db = get_clinical_db();
    let token_registry = Arc::new(TokenRegistry::new());
    let console = PharmacistVerificationConsole::new(Some(db), None, Some(token_registry));

    let token_id = "TARYAQ-OVERRIDE-8842-CLIN";

    let input = PharmacistVerificationInput {
        patient_id: "DXB-PAT-OVERRIDE".to_string(),
        medication_regimen: vec!["Colchicine".to_string(), "Clarithromycin".to_string()],
        patient_egfr: 55.0,
        primary_diagnosis_icd10: Some("M10.0".to_string()),
        net_claim_aed: 800.0,
        claim_submission_delay_days: 5,
        is_resubmission: false,
        resubmission_delay_days: None,
        clinician_override: Some(ClinicianOverrideSubmission {
            override_token: token_id.to_string(),
            clinician_id: "DHA-PHYS-9912".to_string(),
            clinical_rationale: "Patient in intensive monitoring unit with adjusted reduced dosing".to_string(),
        }),
    };

    // First attempt: should succeed and consume token
    let report1 = console.verify_patient_regimen(&input).expect("First override should succeed");
    assert!(report1.is_dispensation_approved);
    assert_eq!(report1.override_status, "APPROVED");
    assert_eq!(report1.override_token_consumed, Some(token_id.to_string()));
    assert_eq!(report1.primary_verdict, "CLINICAL_CONTRAINDICATION_OVERRIDDEN");

    // Second attempt with the EXACT SAME TOKEN: Replay attack!
    let res2 = console.verify_patient_regimen(&input);
    assert!(res2.is_err(), "Replay token must be rejected!");

    match res2 {
        Err(PharmacistError::OverrideReplayConflict { token, status_code, message }) => {
            assert_eq!(token, token_id);
            assert_eq!(status_code, 409, "Must return HTTP 409 Conflict status");
            assert!(message.contains("already been consumed"));
        }
        other => panic!("Expected OverrideReplayConflict (HTTP 409), got: {:?}", other),
    }
}

#[test]
fn test_07_pharmacist_dha_sn_03_2019_claims_penalty_calculation() {
    let console = PharmacistVerificationConsole::with_default();

    // 1. Initial claim: 45 days delay (15 days overdue past 30-day grace period)
    let scrub1 = console.scrub_dha_claims(10_000.0, 45, false, None);
    assert_eq!(scrub1.statutory_grace_period_days, 30);
    assert_eq!(scrub1.overdue_days, 15);
    assert_eq!(scrub1.statutory_penalty_rate_pct_per_day, 0.03);
    // 15 days * 0.03% = 0.45%
    assert!((scrub1.total_penalty_rate_pct - 0.45).abs() < 0.0001);
    // 10,000 * 0.0045 = 45.00 AED
    assert_eq!(scrub1.penalty_amount_aed, 45.0);
    assert_eq!(scrub1.adjusted_net_claim_aed, 9955.0);
    assert_eq!(scrub1.denial_alert_code, Some("DHA_SN_03_2019_LATE_SUBMISSION_WARNING".to_string()));

    // 2. On-time claim: 20 days delay (<= 30 days)
    let scrub2 = console.scrub_dha_claims(5_000.0, 20, false, None);
    assert_eq!(scrub2.overdue_days, 0);
    assert_eq!(scrub2.penalty_amount_aed, 0.0);
    assert_eq!(scrub2.adjusted_net_claim_aed, 5_000.0);
    assert!(scrub2.denial_alert_code.is_none());

    // 3. Resubmission claim: 31 days delay (10 days overdue past 21-day grace period under §3.8.1)
    let scrub3 = console.scrub_dha_claims(4_000.0, 0, true, Some(31));
    assert_eq!(scrub3.statutory_grace_period_days, 21);
    assert_eq!(scrub3.overdue_days, 10);
    // 10 days * 0.03% = 0.30%
    assert!((scrub3.total_penalty_rate_pct - 0.30).abs() < 0.0001);
    // 4000 * 0.0030 = 12.00 AED
    assert_eq!(scrub3.penalty_amount_aed, 12.0);
    assert_eq!(scrub3.adjusted_net_claim_aed, 3988.0);
}

#[test]
fn test_08_pharmacist_hmac_sha256_audit_chain_verified_by_python_verifier() {
    let test_audit_file = "target/test_taryaq_audit.jsonl";
    if std::path::Path::new(test_audit_file).exists() {
        let _ = std::fs::remove_file(test_audit_file);
    }

    let audit_key = "test-key-32-bytes-minimum-000000";
    let ledger = Arc::new(AuditLedger::new(test_audit_file));
    let db = get_clinical_db();
    let console = PharmacistVerificationConsole::new(Some(db), Some(ledger), None);

    // Commit Record 1: Clean safe prescription
    let in1 = PharmacistVerificationInput {
        patient_id: "DXB-AUDIT-1".to_string(),
        medication_regimen: vec!["Lisinopril".to_string(), "Amlodipine".to_string()],
        patient_egfr: 90.0,
        primary_diagnosis_icd10: Some("I10".to_string()),
        net_claim_aed: 300.0,
        claim_submission_delay_days: 10,
        is_resubmission: false,
        resubmission_delay_days: None,
        clinician_override: None,
    };
    std::env::set_var("TARYAQ_AUDIT_KEY", audit_key);
    let rep1 = console.verify_patient_regimen(&in1).expect("Record 1");
    assert!(rep1.audit_committed);

    // Commit Record 2: Blocked prescription
    let in2 = PharmacistVerificationInput {
        patient_id: "DXB-AUDIT-2".to_string(),
        medication_regimen: vec!["Warfarin".to_string(), "Amiodarone".to_string()],
        patient_egfr: 75.0,
        primary_diagnosis_icd10: Some("I48.91".to_string()),
        net_claim_aed: 500.0,
        claim_submission_delay_days: 15,
        is_resubmission: false,
        resubmission_delay_days: None,
        clinician_override: None,
    };
    let rep2 = console.verify_patient_regimen(&in2).expect("Record 2");
    assert!(rep2.audit_committed);

    // Commit Record 3: Overridden prescription
    let in3 = PharmacistVerificationInput {
        patient_id: "DXB-AUDIT-3".to_string(),
        medication_regimen: vec!["Colchicine".to_string(), "Clarithromycin".to_string()],
        patient_egfr: 60.0,
        primary_diagnosis_icd10: Some("M10.0".to_string()),
        net_claim_aed: 1500.0,
        claim_submission_delay_days: 40,
        is_resubmission: false,
        resubmission_delay_days: None,
        clinician_override: Some(ClinicianOverrideSubmission {
            override_token: "TARYAQ-AUDIT-TOK-001".to_string(),
            clinician_id: "DHA-DOC-1234".to_string(),
            clinical_rationale: "Approved in specialized rheumatology unit".to_string(),
        }),
    };
    let rep3 = console.verify_patient_regimen(&in3).expect("Record 3");
    assert!(rep3.audit_committed);

    // Verify chain with tools/verify_audit.py
    let output = Command::new("python3")
        .arg("tools/verify_audit.py")
        .arg("--ledger")
        .arg(test_audit_file)
        .arg("--key")
        .arg(audit_key)
        .output()
        .expect("Failed to execute tools/verify_audit.py");

    let stdout = String::from_utf8_lossy(&output.stdout);
    let stderr = String::from_utf8_lossy(&output.stderr);

    assert!(
        output.status.success(),
        "Audit verifier failed with exit code {:?}.\nStdout: {}\nStderr: {}",
        output.status.code(),
        stdout,
        stderr
    );
    assert!(stdout.contains("3 records verified"), "Verifier output: {}", stdout);
}

#[test]
fn test_09_patient_companion_dialectal_gulf_acute_chest_pain_998_dispatch() {
    let companion = PatientHealthCompanion::new();
    let message = "يا دكتور أحس بعوار ونغز قوي بصدري وضيق تنفس من الصبح ومو قادر أتحمل.";

    let consult = companion.consult(message, &[], None);

    assert_eq!(consult.detected_dialect, DialectCategory::Gulf);
    assert_eq!(consult.triage_level, TaryaqTriageLevel::TriageEmergency);

    assert!(consult.emergency_directive.is_some());
    let em = consult.emergency_directive.unwrap();
    assert!(em.requires_immediate_dispatch);
    assert_eq!(em.red_flag_category, RedFlagCategory::CardiovascularAcute);
    assert!(em.emergency_dispatch_numbers.contains(&"998".to_string()));
    assert!(em.arabic_emergency_alert.contains("998"));
    assert!(em.english_emergency_alert.contains("998"));
}

#[test]
fn test_10_patient_companion_dialectal_egyptian_stroke_fast_998_dispatch() {
    let companion = PatientHealthCompanion::new();
    let message = "الحقني يا دكتور والدي فجأة وشه معوج ومش عارف يتكلم وعنده ثقل في اللسان.";

    let consult = companion.consult(message, &[], None);

    assert_eq!(consult.detected_dialect, DialectCategory::Egyptian);
    assert_eq!(consult.triage_level, TaryaqTriageLevel::TriageEmergency);

    assert!(consult.emergency_directive.is_some());
    let em = consult.emergency_directive.unwrap();
    assert!(em.requires_immediate_dispatch);
    assert_eq!(em.red_flag_category, RedFlagCategory::NeurologicalStrokeFast);
    assert!(em.emergency_dispatch_numbers.contains(&"998".to_string()));
    assert!(em.arabic_emergency_alert.contains("سكتة دماغية"));
}

#[test]
fn test_11_patient_companion_dialectal_levantine_severe_hemorrhage_998_dispatch() {
    let companion = PatientHealthCompanion::new();
    let message = "عم بستفرغ دم كتير وحاسس بدوخة قوية وخنقة شو أعمل؟";

    let consult = companion.consult(message, &[], None);

    assert_eq!(consult.detected_dialect, DialectCategory::Levantine);
    assert_eq!(consult.triage_level, TaryaqTriageLevel::TriageEmergency);

    assert!(consult.emergency_directive.is_some());
    let em = consult.emergency_directive.unwrap();
    assert!(em.requires_immediate_dispatch);
    assert_eq!(em.red_flag_category, RedFlagCategory::SevereHemorrhage);
    assert!(em.arabic_emergency_alert.contains("998"));
}

#[test]
fn test_12_patient_companion_plain_language_medication_counseling() {
    let companion = PatientHealthCompanion::new();
    let message = "مرحبا دكتور، أريد معرفة الطريقة الصحيحة لتناول علاج السكر والضغط والدهون.";
    let active_meds = vec![
        "Metformin".to_string(),
        "Lisinopril".to_string(),
        "Atorvastatin".to_string(),
    ];

    let consult = companion.consult(message, &active_meds, Some(85.0));

    assert_eq!(consult.triage_level, TaryaqTriageLevel::TriageRoutine);
    assert!(consult.emergency_directive.is_none());

    assert_eq!(consult.medication_guidance.len(), 3);

    // Metformin checks: food relation
    let met = consult.medication_guidance.iter().find(|g| g.drug_name == "Metformin").unwrap();
    assert!(met.food_relation_ar.contains("وسط الطعام") || met.food_relation_ar.contains("وجبة"));
    assert!(met.food_relation_en.contains("with meals") || met.food_relation_en.contains("food"));

    // Statin checks: avoid grapefruit
    let statin = consult.medication_guidance.iter().find(|g| g.drug_name == "Atorvastatin").unwrap();
    assert!(statin.what_to_avoid_ar.contains("الجريب فروت"));
    assert!(statin.what_to_avoid_en.contains("grapefruit"));

    // Lifestyle guidance
    assert!(consult.lifestyle_counseling_en.contains("hydration"));
}

#[test]
fn test_13_voice_pipeline_wav_audio_ingestion_and_tri_modal_routing() {
    let fixture_path = "tests/fixtures/test_ambient_dialogue.wav";
    assert!(std::path::Path::new(fixture_path).exists(), "Fixture must exist");

    let pipeline = VoicePipeline::new();
    let trans = pipeline.transcribe_audio_file(fixture_path).expect("Transcription failed");

    assert_eq!(trans.format, AudioFormat::Wav);
    assert!(!trans.text.is_empty());
    assert!(trans.duration_sec > 0.0);
    assert!(!trans.segments.is_empty());

    // Route to Clinician Ambient Copilot
    let soap = pipeline.route_to_copilot(&trans, Some(65.0), vec!["Amlodipine".to_string()], Vec::new());
    assert!(!soap.encounter_id.is_empty());
    assert!(soap.markdown_note.contains("## S - Subjective"));

    // Route to Pharmacist Console
    let pharm = pipeline.route_to_pharmacist(&trans, 60.0, 800.0, 5);
    assert!(pharm.is_ok());

    // Route to Patient Companion
    let comp = pipeline.route_to_companion(&trans, &["Metformin".to_string()], Some(60.0));
    assert!(!comp.conversational_response_en.is_empty());
}

#[test]
fn test_14_voice_pipeline_unsupported_format_and_corrupt_file_fail_closed() {
    let pipeline = VoicePipeline::new();

    // 1. Non-existent file
    let res_missing = pipeline.transcribe_audio_file("tests/fixtures/does_not_exist.wav");
    assert!(matches!(res_missing, Err(VoiceError::FileNotFound(_))));

    // 2. Unsupported format (.txt)
    let res_unsupported = pipeline.transcribe_audio_file("Cargo.toml");
    assert!(matches!(res_unsupported, Err(VoiceError::UnsupportedFormat(_))));

    // 3. Corrupt 0-byte file
    let corrupt_path = "target/corrupt_zero_byte.wav";
    {
        let _ = File::create(corrupt_path).unwrap();
    }
    let res_corrupt = pipeline.transcribe_audio_file(corrupt_path);
    assert!(matches!(res_corrupt, Err(VoiceError::CorruptAudio(_, _))));
}
