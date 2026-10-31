//! Deterministic Acceptance Suite for Taryaq C99 Inviolable Safety Kernel FFI Bridge
//! Verifies all 20 C99 unit checks codified in `adapters/taryaq_clinical_adapter.c`

use std::ffi::CString;
use taryaq::safety::ffi::*;
use taryaq::safety::*;

const DHA_SN03_2019_SEC_3_7_1_SHA256: &str =
    "b0a422ad0e2edc66290007caaa5508d4ec4ac4229453b170881fb482449218f3";
const DHA_SN03_2019_SEC_3_7_2_SHA256: &str =
    "35bd39f8756297aaf921e2e2720f30c7e9ceac5f524646d40fd0601298089a5a";

#[test]
fn test_01_fatal_cyp3a4_renal_interaction() {
    let rx = RxContext {
        primary_drug: "Colchicine".to_string(),
        concurrent_drug: Some("Clarithromycin".to_string()),
        patient_egfr_ml_min: 22.0,
        is_controlled_substance: false,
        has_controlled_id_verified: false,
        icd10_code: Some("M10.0".to_string()),
        submission_delay_days: 0,
        has_ejadah_lab_justification: true,
        has_clinician_override: false,
    };
    let r = verify_prescription_and_claim(&rx);
    assert_eq!(r.verdict, TaryaqVerdict::ClinicalContraindicationBlock);
    assert_eq!(r.alert_code, "FATAL_CYP3A4_RENAL_CONTRAINDICATION");
}

#[test]
fn test_02_controlled_drug_gate_unverified_id() {
    let rx = RxContext {
        primary_drug: "Morphine".to_string(),
        concurrent_drug: None,
        patient_egfr_ml_min: 90.0,
        is_controlled_substance: true,
        has_controlled_id_verified: false,
        icd10_code: Some("R52".to_string()),
        submission_delay_days: 0,
        has_ejadah_lab_justification: true,
        has_clinician_override: false,
    };
    let r = verify_prescription_and_claim(&rx);
    assert_eq!(r.verdict, TaryaqVerdict::ClinicalControlledDrugGate);
    assert_eq!(r.alert_code, "UAE_CONTROLLED_RX_VERIFICATION_REQUIRED");
}

#[test]
fn test_03_dha_late_claim_submission() {
    let rx = RxContext {
        primary_drug: "Metformin".to_string(),
        concurrent_drug: None,
        patient_egfr_ml_min: 75.0,
        is_controlled_substance: false,
        has_controlled_id_verified: false,
        icd10_code: Some("E11.9".to_string()),
        submission_delay_days: 42,
        has_ejadah_lab_justification: true,
        has_clinician_override: false,
    };
    let r = verify_prescription_and_claim(&rx);
    assert_eq!(r.verdict, TaryaqVerdict::ClinicalClaimCycleWarn);
    assert_eq!(r.alert_code, "DHA_SN03_2019_DELAY_PENALTY_EXPOSURE");
    assert_eq!(r.doc_id, "DHA_SN_03_2019");
    assert_eq!(r.section, "§3.7.2");
    assert_eq!(r.passage_sha256, DHA_SN03_2019_SEC_3_7_2_SHA256);
}

#[test]
fn test_04_dha_on_time_claim_submission() {
    let rx = RxContext {
        primary_drug: "Metformin".to_string(),
        concurrent_drug: None,
        patient_egfr_ml_min: 75.0,
        is_controlled_substance: false,
        has_controlled_id_verified: false,
        icd10_code: Some("E11.9".to_string()),
        submission_delay_days: 20,
        has_ejadah_lab_justification: true,
        has_clinician_override: false,
    };
    let r = verify_prescription_and_claim(&rx);
    assert_eq!(r.verdict, TaryaqVerdict::ClinicalSafe);
    assert_eq!(r.alert_code, "DHA_NABIDH_SAFE");
    assert_eq!(r.doc_id, "DHA_SN_03_2019");
    assert_eq!(r.section, "§3.7.1");
    assert_eq!(r.passage_sha256, DHA_SN03_2019_SEC_3_7_1_SHA256);
}

#[test]
fn test_05_metformin_severe_renal_impairment() {
    let rx = RxContext {
        primary_drug: "Metformin".to_string(),
        concurrent_drug: None,
        patient_egfr_ml_min: 26.0,
        is_controlled_substance: false,
        has_controlled_id_verified: false,
        icd10_code: Some("E11.22".to_string()),
        submission_delay_days: 0,
        has_ejadah_lab_justification: true,
        has_clinician_override: false,
    };
    let r = verify_prescription_and_claim(&rx);
    assert_eq!(r.verdict, TaryaqVerdict::ClinicalContraindicationBlock);
    assert_eq!(r.alert_code, "METFORMIN_SEVERE_RENAL_CUTOFF_EGFR_LT_30");
}

#[test]
fn test_06_commutative_order_invariance() {
    let rx = RxContext {
        primary_drug: "clarithromycin".to_string(),
        concurrent_drug: Some("colchicine 0.5mg".to_string()),
        patient_egfr_ml_min: 22.0,
        is_controlled_substance: false,
        has_controlled_id_verified: false,
        icd10_code: Some("M10.0".to_string()),
        submission_delay_days: 0,
        has_ejadah_lab_justification: true,
        has_clinician_override: false,
    };
    let r = verify_prescription_and_claim(&rx);
    assert_eq!(r.verdict, TaryaqVerdict::ClinicalContraindicationBlock);
    assert_eq!(r.alert_code, "FATAL_CYP3A4_RENAL_CONTRAINDICATION");
}

#[test]
fn test_07_null_context_fails_closed() {
    let r = verify_prescription_raw(std::ptr::null());
    assert_eq!(r.verdict, TaryaqVerdict::ClinicalContraindicationBlock);
    assert_eq!(r.alert_code, "INVALID_RX_CONTEXT");
}

#[test]
fn test_08_nan_egfr_fails_closed() {
    let rx = RxContext {
        primary_drug: "Paracetamol".to_string(),
        patient_egfr_ml_min: f64::NAN,
        ..Default::default()
    };
    let r = verify_prescription_and_claim(&rx);
    assert_eq!(r.verdict, TaryaqVerdict::ClinicalContraindicationBlock);
    assert_eq!(r.alert_code, "INVALID_RENAL_LAB_VALUE");
}

#[test]
fn test_09_auto_detect_narcotic_tramadol() {
    let rx = RxContext {
        primary_drug: "tramadol 50mg".to_string(),
        patient_egfr_ml_min: 90.0,
        is_controlled_substance: false, // Caller omitted flag
        has_controlled_id_verified: false,
        ..Default::default()
    };
    let r = verify_prescription_and_claim(&rx);
    assert_eq!(r.verdict, TaryaqVerdict::ClinicalControlledDrugGate);
    assert_eq!(r.alert_code, "UAE_CONTROLLED_RX_VERIFICATION_REQUIRED");
}

#[test]
fn test_10_simvastatin_clarithromycin_rhabdo() {
    let rx = RxContext {
        primary_drug: "Simvastatin".to_string(),
        concurrent_drug: Some("Clarithromycin".to_string()),
        patient_egfr_ml_min: 90.0,
        is_controlled_substance: false,
        has_controlled_id_verified: false,
        icd10_code: Some("E78.0".to_string()),
        submission_delay_days: 0,
        has_ejadah_lab_justification: true,
        has_clinician_override: false,
    };
    let r = verify_prescription_and_claim(&rx);
    assert_eq!(r.verdict, TaryaqVerdict::ClinicalContraindicationBlock);
    assert_eq!(
        r.alert_code,
        "FATAL_CYP3A4_RHABDOMYOLYSIS_CONTRAINDICATION"
    );
}

#[test]
fn test_11_warfarin_amiodarone_hemorrhage() {
    let rx = RxContext {
        primary_drug: "Warfarin".to_string(),
        concurrent_drug: Some("Amiodarone".to_string()),
        patient_egfr_ml_min: 85.0,
        is_controlled_substance: false,
        has_controlled_id_verified: false,
        icd10_code: Some("I48.9".to_string()),
        submission_delay_days: 0,
        has_ejadah_lab_justification: true,
        has_clinician_override: false,
    };
    let r = verify_prescription_and_claim(&rx);
    assert_eq!(r.verdict, TaryaqVerdict::ClinicalContraindicationBlock);
    assert_eq!(r.alert_code, "WARFARIN_AMIODARONE_HEMORRHAGE_RISK");
}

#[test]
fn test_12_unmodeled_drug_fails_closed() {
    let rx = RxContext {
        primary_drug: "ExperimentalCompound-X 400mg".to_string(),
        concurrent_drug: None,
        patient_egfr_ml_min: 95.0,
        is_controlled_substance: false,
        has_controlled_id_verified: false,
        icd10_code: Some("R51".to_string()),
        submission_delay_days: 0,
        has_ejadah_lab_justification: true,
        has_clinician_override: false,
    };
    let r = verify_prescription_and_claim(&rx);
    assert_eq!(r.verdict, TaryaqVerdict::ClinicalUnverifiedReview);
    assert_eq!(
        r.alert_code,
        "UNVERIFIED_DRUG_REQUIRES_PHARMACIST_REVIEW"
    );
}

#[test]
fn test_13_verified_modeled_drug_ibuprofen() {
    let _ = clinical_db_init(None);
    let rx = RxContext {
        primary_drug: "Ibuprofen 400mg".to_string(),
        concurrent_drug: None,
        patient_egfr_ml_min: 95.0,
        is_controlled_substance: false,
        has_controlled_id_verified: false,
        icd10_code: Some("M79.1".to_string()),
        submission_delay_days: 0,
        has_ejadah_lab_justification: true,
        has_clinician_override: false,
    };
    let r = verify_prescription_and_claim(&rx);
    assert_eq!(r.verdict, TaryaqVerdict::ClinicalSafe);
    assert_eq!(r.alert_code, "DHA_NABIDH_SAFE");
}

#[test]
fn test_14_unmodeled_combination_fails_closed() {
    let rx = RxContext {
        primary_drug: "Colchicine".to_string(),
        concurrent_drug: Some("Warfarin".to_string()),
        patient_egfr_ml_min: 90.0,
        is_controlled_substance: false,
        has_controlled_id_verified: false,
        icd10_code: Some("M10.0".to_string()),
        submission_delay_days: 0,
        has_ejadah_lab_justification: true,
        has_clinician_override: false,
    };
    let r = verify_prescription_and_claim(&rx);
    assert_eq!(r.verdict, TaryaqVerdict::ClinicalUnverifiedReview);
    assert_eq!(
        r.alert_code,
        "UNVERIFIED_DRUG_REQUIRES_PHARMACIST_REVIEW"
    );
}

#[test]
fn test_15_emergency_triage_arabic_acute_chest_pain() {
    let tr = triage_patient_symptoms("أشعر بألم في الصدر وضيق تنفس مستمر");
    assert!(tr.is_red_flag);
    assert_eq!(tr.level, TaryaqTriageLevel::TriageEmergency);
    assert_eq!(tr.triage_category, "CARDIOVASCULAR_ACUTE");
}

#[test]
fn test_16_emergency_triage_english_acute_stroke() {
    let tr = triage_patient_symptoms("Patient woke up with facial drooping and slurred speech");
    assert!(tr.is_red_flag);
    assert_eq!(tr.level, TaryaqTriageLevel::TriageEmergency);
    assert_eq!(tr.triage_category, "STROKE_NEUROLOGICAL");
}

#[test]
fn test_17_routine_care_triage() {
    let tr = triage_patient_symptoms("أريد تجديد وصفة دواء السكر ميتفورمين 500");
    assert!(!tr.is_red_flag);
    assert_eq!(tr.level, TaryaqTriageLevel::TriageRoutine);
    assert_eq!(tr.triage_category, "ROUTINE_CARE");
}

#[test]
fn test_18_tri_modal_clinician_copilot_soap_cds() {
    let soap = clinician_ambient_copilot(
        "Patient presents with severe acute right big toe pain (podagra) and swelling for 2 days. eGFR: 22.0 mL/min.",
        22.0,
        "Colchicine, Clarithromycin",
    );
    assert!(soap.is_some());
    let s = soap.unwrap();
    assert!(s.cds_interrupt_triggered);
    assert_eq!(s.cds_interrupt_code, "FATAL_CYP3A4_RENAL_CONTRAINDICATION");
    assert!(s.assessment.contains("M10.0"));
}

#[test]
fn test_19_tri_modal_pharmacist_verification() {
    let pharm = pharmacist_verification(
        "Metformin, Simvastatin, Clarithromycin",
        45.0,
        1000.0,
        42,
        Some("E11.9"),
        true,
    );
    assert!(pharm.is_some());
    let p = pharm.unwrap();
    assert!(p.has_contraindication_block);
    assert_eq!(p.overdue_days, 12);
    assert!((p.penalty_aed - 3.60).abs() < 0.01);
    assert!(p.claim_cycle_warn);
}

#[test]
fn test_20_tri_modal_patient_companion() {
    // Red-flag acute consultation
    let comp_emergency = patient_companion_consult(
        "عندي وجع وضغط بصدري ونازل لإيدي اليسار ومش قادر اتنفس",
        "Metformin",
        55.0,
    );
    assert!(comp_emergency.is_some());
    let ce = comp_emergency.unwrap();
    assert!(ce.requires_immediate_998);
    assert_eq!(ce.triage.level, TaryaqTriageLevel::TriageEmergency);

    // Routine medication counseling
    let comp_routine = patient_companion_consult(
        "كيف آخذ علاج الميتفورمين والسيبروفلوكساسين؟",
        "Metformin, Ciprofloxacin",
        55.0,
    );
    assert!(comp_routine.is_some());
    let cr = comp_routine.unwrap();
    assert!(!cr.requires_immediate_998);
    assert!(cr.lay_instructions.contains("ميتفورمين"));
    assert!(cr.dietary_precautions.contains("الحليب"));
}

#[test]
fn test_direct_c99_extern_c_ffi_bindings() {
    // 1. Direct call to taryaq_verify_prescription_and_claim
    let primary = CString::new("Colchicine").unwrap();
    let concurrent = CString::new("Clarithromycin").unwrap();
    let icd10 = CString::new("M10.0").unwrap();

    let raw_rx = TaryaqRxContext {
        primary_drug: primary.as_ptr(),
        concurrent_drug: concurrent.as_ptr(),
        patient_egfr_ml_min: 22.0,
        is_controlled_substance: false,
        has_controlled_id_verified: false,
        icd10_code: icd10.as_ptr(),
        submission_delay_days: 0,
        has_ejadah_lab_justification: true,
        has_clinician_override: false,
    };

    let res = unsafe { taryaq_verify_prescription_and_claim(&raw_rx) };
    assert_eq!(res.verdict, TaryaqVerdict::ClinicalContraindicationBlock);

    let mut out_res = TaryaqResult::default();
    let verdict = unsafe { taryaq_verify_prescription_and_claim_ptr(&raw_rx, &mut out_res) };
    assert_eq!(verdict, TaryaqVerdict::ClinicalContraindicationBlock);
    assert_eq!(out_res.verdict, TaryaqVerdict::ClinicalContraindicationBlock);

    // 2. Direct call to taryaq_triage_patient_symptoms
    let narrative = CString::new("أشعر بألم في الصدر وضيق تنفس مستمر").unwrap();
    let triage_res = unsafe { taryaq_triage_patient_symptoms(narrative.as_ptr()) };
    assert!(triage_res.is_red_flag);
    assert_eq!(triage_res.level, TaryaqTriageLevel::TriageEmergency);
}
