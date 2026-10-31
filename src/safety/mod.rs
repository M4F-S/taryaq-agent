//! Safe, idiomatic Rust API wrapping the inviolable C99 Taryaq Safety Kernel.

pub mod ffi;

use ffi::{
    TaryaqPatientCompanionResult, TaryaqPharmacistResult, TaryaqResult, TaryaqRxContext,
    TaryaqSoapNote, TaryaqTriageResult,
};
pub use ffi::{TaryaqTriageLevel, TaryaqVerdict};
use serde::{Deserialize, Serialize};
use std::ffi::CString;
use std::os::raw::c_char;

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct RxContext {
    pub primary_drug: String,
    pub concurrent_drug: Option<String>,
    pub patient_egfr_ml_min: f64,
    pub is_controlled_substance: bool,
    pub has_controlled_id_verified: bool,
    pub icd10_code: Option<String>,
    pub submission_delay_days: u32,
    pub has_ejadah_lab_justification: bool,
    pub has_clinician_override: bool,
}

impl Default for RxContext {
    fn default() -> Self {
        Self {
            primary_drug: String::new(),
            concurrent_drug: None,
            patient_egfr_ml_min: 90.0,
            is_controlled_substance: false,
            has_controlled_id_verified: false,
            icd10_code: None,
            submission_delay_days: 0,
            has_ejadah_lab_justification: true,
            has_clinician_override: false,
        }
    }
}

#[derive(Debug, Clone, Serialize, Deserialize, PartialEq, Eq)]
pub struct ClinicalResult {
    pub verdict: TaryaqVerdict,
    pub alert_code: String,
    pub clinical_rationale: String,
    pub doc_id: String,
    pub section: String,
    pub passage_sha256: String,
}

impl From<TaryaqResult> for ClinicalResult {
    fn from(r: TaryaqResult) -> Self {
        Self {
            verdict: r.verdict,
            alert_code: c_chars_to_string(&r.alert_code),
            clinical_rationale: c_chars_to_string(&r.clinical_rationale),
            doc_id: c_chars_to_string(&r.doc_id),
            section: c_chars_to_string(&r.section),
            passage_sha256: c_chars_to_string(&r.passage_sha256),
        }
    }
}

#[derive(Debug, Clone, Serialize, Deserialize, PartialEq, Eq)]
pub struct TriageResult {
    pub level: TaryaqTriageLevel,
    pub is_red_flag: bool,
    pub triage_category: String,
    pub matched_trigger: String,
    pub action_protocol: String,
    pub clinical_guidance: String,
}

impl From<TaryaqTriageResult> for TriageResult {
    fn from(t: TaryaqTriageResult) -> Self {
        Self {
            level: t.level,
            is_red_flag: t.is_red_flag,
            triage_category: c_chars_to_string(&t.triage_category),
            matched_trigger: c_chars_to_string(&t.matched_trigger),
            action_protocol: c_chars_to_string(&t.action_protocol),
            clinical_guidance: c_chars_to_string(&t.clinical_guidance),
        }
    }
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct SoapNote {
    pub subjective: String,
    pub objective: String,
    pub assessment: String,
    pub plan: String,
    pub ranked_ddx_json: String,
    pub cds_interrupt_triggered: bool,
    pub cds_interrupt_code: String,
    pub cds_interrupt_rationale: String,
    pub evidence_doc_id: String,
    pub evidence_sha256: String,
}

impl From<TaryaqSoapNote> for SoapNote {
    fn from(s: TaryaqSoapNote) -> Self {
        Self {
            subjective: c_chars_to_string(&s.subjective),
            objective: c_chars_to_string(&s.objective),
            assessment: c_chars_to_string(&s.assessment),
            plan: c_chars_to_string(&s.plan),
            ranked_ddx_json: c_chars_to_string(&s.ranked_ddx_json),
            cds_interrupt_triggered: s.cds_interrupt_triggered,
            cds_interrupt_code: c_chars_to_string(&s.cds_interrupt_code),
            cds_interrupt_rationale: c_chars_to_string(&s.cds_interrupt_rationale),
            evidence_doc_id: c_chars_to_string(&s.evidence_doc_id),
            evidence_sha256: c_chars_to_string(&s.evidence_sha256),
        }
    }
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct PharmacistResult {
    pub total_drugs_screened: usize,
    pub interaction_count: usize,
    pub has_contraindication_block: bool,
    pub primary_block_code: String,
    pub primary_block_rationale: String,
    pub is_controlled_gate: bool,
    pub controlled_drug_name: String,
    pub net_claim_aed: f64,
    pub delay_days: u32,
    pub overdue_days: u32,
    pub penalty_rate_pct: f64,
    pub penalty_aed: f64,
    pub adjusted_net_aed: f64,
    pub claim_cycle_warn: bool,
    pub claim_alert_code: String,
    pub evidence_doc_id: String,
    pub evidence_section: String,
    pub evidence_sha256: String,
    pub polypharmacy_matrix_json: String,
}

impl From<TaryaqPharmacistResult> for PharmacistResult {
    fn from(p: TaryaqPharmacistResult) -> Self {
        Self {
            total_drugs_screened: p.total_drugs_screened,
            interaction_count: p.interaction_count,
            has_contraindication_block: p.has_contraindication_block,
            primary_block_code: c_chars_to_string(&p.primary_block_code),
            primary_block_rationale: c_chars_to_string(&p.primary_block_rationale),
            is_controlled_gate: p.is_controlled_gate,
            controlled_drug_name: c_chars_to_string(&p.controlled_drug_name),
            net_claim_aed: p.net_claim_aed,
            delay_days: p.delay_days,
            overdue_days: p.overdue_days,
            penalty_rate_pct: p.penalty_rate_pct,
            penalty_aed: p.penalty_aed,
            adjusted_net_aed: p.adjusted_net_aed,
            claim_cycle_warn: p.claim_cycle_warn,
            claim_alert_code: c_chars_to_string(&p.claim_alert_code),
            evidence_doc_id: c_chars_to_string(&p.evidence_doc_id),
            evidence_section: c_chars_to_string(&p.evidence_section),
            evidence_sha256: c_chars_to_string(&p.evidence_sha256),
            polypharmacy_matrix_json: c_chars_to_string(&p.polypharmacy_matrix_json),
        }
    }
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct PatientCompanionResult {
    pub triage: TriageResult,
    pub lay_instructions: String,
    pub dietary_precautions: String,
    pub red_flag_warning: String,
    pub requires_immediate_998: bool,
}

impl From<TaryaqPatientCompanionResult> for PatientCompanionResult {
    fn from(c: TaryaqPatientCompanionResult) -> Self {
        Self {
            triage: TriageResult::from(c.triage),
            lay_instructions: c_chars_to_string(&c.lay_instructions),
            dietary_precautions: c_chars_to_string(&c.dietary_precautions),
            red_flag_warning: c_chars_to_string(&c.red_flag_warning),
            requires_immediate_998: c.requires_immediate_998,
        }
    }
}

/// Helper converting fixed-size C char buffers to Rust String
pub(crate) fn c_chars_to_string(buf: &[c_char]) -> String {
    let u8_slice = unsafe { std::slice::from_raw_parts(buf.as_ptr() as *const u8, buf.len()) };
    let len = u8_slice.iter().position(|&b| b == 0).unwrap_or(buf.len());
    String::from_utf8_lossy(&u8_slice[..len]).trim().to_string()
}

/// Verify a prescription context against the C99 safety engine.
/// Handles CString allocations and passes pointers to the C99 governor.
pub fn verify_prescription_and_claim(rx: &RxContext) -> ClinicalResult {
    let c_primary = match CString::new(rx.primary_drug.as_str()) {
        Ok(s) => s,
        Err(_) => {
            return ClinicalResult {
                verdict: TaryaqVerdict::ClinicalContraindicationBlock,
                alert_code: "INVALID_RX_CONTEXT".to_string(),
                clinical_rationale: "Null byte in primary drug name".to_string(),
                doc_id: String::new(),
                section: String::new(),
                passage_sha256: String::new(),
            };
        }
    };

    let c_concurrent = rx.concurrent_drug.as_ref().and_then(|d| CString::new(d.as_str()).ok());
    let c_icd10 = rx.icd10_code.as_ref().and_then(|c| CString::new(c.as_str()).ok());

    let c_rx = TaryaqRxContext {
        primary_drug: c_primary.as_ptr(),
        concurrent_drug: c_concurrent.as_ref().map_or(std::ptr::null(), |s| s.as_ptr()),
        patient_egfr_ml_min: rx.patient_egfr_ml_min,
        is_controlled_substance: rx.is_controlled_substance,
        has_controlled_id_verified: rx.has_controlled_id_verified,
        icd10_code: c_icd10.as_ref().map_or(std::ptr::null(), |s| s.as_ptr()),
        submission_delay_days: rx.submission_delay_days,
        has_ejadah_lab_justification: rx.has_ejadah_lab_justification,
        has_clinician_override: rx.has_clinician_override,
    };

    let mut out_result = TaryaqResult::default();
    unsafe {
        ffi::taryaq_verify_prescription_and_claim_ptr(&c_rx, &mut out_result);
    }
    ClinicalResult::from(out_result)
}

/// Direct raw C99 FFI invocation supporting raw pointer passes (including NULL context for F-01 testing)
pub fn verify_prescription_raw(raw_ptr: *const TaryaqRxContext) -> ClinicalResult {
    let mut out_result = TaryaqResult::default();
    unsafe {
        ffi::taryaq_verify_prescription_and_claim_ptr(raw_ptr, &mut out_result);
    }
    ClinicalResult::from(out_result)
}

/// Triage patient symptom narrative in dialectal Arabic or English
pub fn triage_patient_symptoms(narrative: &str) -> TriageResult {
    let c_narrative = match CString::new(narrative) {
        Ok(s) => s,
        Err(_) => {
            return TriageResult {
                level: TaryaqTriageLevel::TriageEmergency,
                is_red_flag: true,
                triage_category: "MALFORMED_INPUT".to_string(),
                matched_trigger: "NULL_BYTE".to_string(),
                action_protocol: "Immediate emergency evaluation required.".to_string(),
                clinical_guidance: "Input string contained interior null bytes.".to_string(),
            };
        }
    };

    let raw_res = unsafe { ffi::taryaq_triage_patient_symptoms(c_narrative.as_ptr()) };
    TriageResult::from(raw_res)
}

/// Initialize the sovereign clinical database
pub fn clinical_db_init(db_path: Option<&str>) -> bool {
    let c_path = db_path.and_then(|p| CString::new(p).ok());
    let ptr = c_path.as_ref().map_or(std::ptr::null(), |s| s.as_ptr());
    unsafe { ffi::taryaq_clinical_db_init(ptr) }
}

/// Close the sovereign clinical database connection
pub fn clinical_db_close() {
    unsafe { ffi::taryaq_clinical_db_close() }
}

/// Query active formulary drug count from sovereign database
pub fn clinical_db_count_drugs() -> i32 {
    unsafe { ffi::taryaq_clinical_db_count_drugs() }
}

/// Tri-Modal Interface 1: Clinician Ambient Copilot
pub fn clinician_ambient_copilot(
    dialogue: &str,
    egfr: f64,
    proposed_drugs_csv: &str,
) -> Option<SoapNote> {
    let c_dialogue = CString::new(dialogue).ok()?;
    let c_drugs = CString::new(proposed_drugs_csv).ok()?;
    let mut soap = TaryaqSoapNote::default();

    let success = unsafe {
        ffi::taryaq_clinician_ambient_copilot(
            c_dialogue.as_ptr(),
            egfr,
            c_drugs.as_ptr(),
            &mut soap,
        )
    };

    if success {
        Some(SoapNote::from(soap))
    } else {
        None
    }
}

/// Tri-Modal Interface 2: Pharmacist Verification Console
pub fn pharmacist_verification(
    meds_csv: &str,
    egfr: f64,
    net_claim_aed: f64,
    submission_delay_days: u32,
    icd10_code: Option<&str>,
    id_verified: bool,
) -> Option<PharmacistResult> {
    let c_meds = CString::new(meds_csv).ok()?;
    let c_icd10 = icd10_code.and_then(|c| CString::new(c).ok());
    let mut out_res = TaryaqPharmacistResult::default();

    let success = unsafe {
        ffi::taryaq_pharmacist_verification(
            c_meds.as_ptr(),
            egfr,
            net_claim_aed,
            submission_delay_days,
            c_icd10.as_ref().map_or(std::ptr::null(), |s| s.as_ptr()),
            id_verified,
            &mut out_res,
        )
    };

    if success {
        Some(PharmacistResult::from(out_res))
    } else {
        None
    }
}

/// Tri-Modal Interface 3: Patient Health Companion
pub fn patient_companion_consult(
    patient_message: &str,
    active_meds_csv: &str,
    egfr: f64,
) -> Option<PatientCompanionResult> {
    let c_msg = CString::new(patient_message).ok()?;
    let c_meds = CString::new(active_meds_csv).ok()?;
    let mut out_res = TaryaqPatientCompanionResult::default();

    let success = unsafe {
        ffi::taryaq_patient_companion_consult(
            c_msg.as_ptr(),
            c_meds.as_ptr(),
            egfr,
            &mut out_res,
        )
    };

    if success {
        Some(PatientCompanionResult::from(out_res))
    } else {
        None
    }
}
