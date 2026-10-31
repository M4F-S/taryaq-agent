//! Low-level C99 FFI bindings for Taryaq Safety & Claims Engine
//! Directly links `libdubai_adapters` C99 kernel.

use serde::{Deserialize, Serialize};
use std::os::raw::{c_char, c_double, c_int};

#[repr(u32)]
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, Serialize, Deserialize)]
pub enum TaryaqVerdict {
    ClinicalSafe = 0,
    ClinicalClaimCycleWarn = 1,
    ClinicalControlledDrugGate = 2,
    ClinicalContraindicationBlock = 3,
    ClinicalUnverifiedReview = 4,
}

#[repr(C)]
#[derive(Debug, Clone)]
pub struct TaryaqRxContext {
    pub primary_drug: *const c_char,
    pub concurrent_drug: *const c_char,
    pub patient_egfr_ml_min: c_double,
    pub is_controlled_substance: bool,
    pub has_controlled_id_verified: bool,
    pub icd10_code: *const c_char,
    pub submission_delay_days: u32,
    pub has_ejadah_lab_justification: bool,
    pub has_clinician_override: bool,
}

impl Default for TaryaqRxContext {
    fn default() -> Self {
        Self {
            primary_drug: std::ptr::null(),
            concurrent_drug: std::ptr::null(),
            patient_egfr_ml_min: 0.0,
            is_controlled_substance: false,
            has_controlled_id_verified: false,
            icd10_code: std::ptr::null(),
            submission_delay_days: 0,
            has_ejadah_lab_justification: true,
            has_clinician_override: false,
        }
    }
}

#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct TaryaqResult {
    pub verdict: TaryaqVerdict,
    pub alert_code: [c_char; 64],
    pub clinical_rationale: [c_char; 256],
    pub doc_id: [c_char; 64],
    pub section: [c_char; 64],
    pub passage_sha256: [c_char; 65],
}

impl Default for TaryaqResult {
    fn default() -> Self {
        Self {
            verdict: TaryaqVerdict::ClinicalSafe,
            alert_code: [0; 64],
            clinical_rationale: [0; 256],
            doc_id: [0; 64],
            section: [0; 64],
            passage_sha256: [0; 65],
        }
    }
}

#[repr(u32)]
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, Serialize, Deserialize)]
pub enum TaryaqTriageLevel {
    TriageRoutine = 0,
    TriageUrgent = 1,
    TriageEmergency = 2,
}

#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct TaryaqTriageResult {
    pub level: TaryaqTriageLevel,
    pub is_red_flag: bool,
    pub triage_category: [c_char; 64],
    pub matched_trigger: [c_char; 64],
    pub action_protocol: [c_char; 256],
    pub clinical_guidance: [c_char; 256],
}

impl Default for TaryaqTriageResult {
    fn default() -> Self {
        Self {
            level: TaryaqTriageLevel::TriageRoutine,
            is_red_flag: false,
            triage_category: [0; 64],
            matched_trigger: [0; 64],
            action_protocol: [0; 256],
            clinical_guidance: [0; 256],
        }
    }
}

#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct TaryaqSoapNote {
    pub subjective: [c_char; 1024],
    pub objective: [c_char; 1024],
    pub assessment: [c_char; 1024],
    pub plan: [c_char; 1024],
    pub ranked_ddx_json: [c_char; 512],
    pub cds_interrupt_triggered: bool,
    pub cds_interrupt_code: [c_char; 64],
    pub cds_interrupt_rationale: [c_char; 256],
    pub evidence_doc_id: [c_char; 64],
    pub evidence_sha256: [c_char; 65],
}

impl Default for TaryaqSoapNote {
    fn default() -> Self {
        Self {
            subjective: [0; 1024],
            objective: [0; 1024],
            assessment: [0; 1024],
            plan: [0; 1024],
            ranked_ddx_json: [0; 512],
            cds_interrupt_triggered: false,
            cds_interrupt_code: [0; 64],
            cds_interrupt_rationale: [0; 256],
            evidence_doc_id: [0; 64],
            evidence_sha256: [0; 65],
        }
    }
}

#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct TaryaqPharmacistResult {
    pub total_drugs_screened: usize,
    pub interaction_count: usize,
    pub has_contraindication_block: bool,
    pub primary_block_code: [c_char; 64],
    pub primary_block_rationale: [c_char; 256],
    pub is_controlled_gate: bool,
    pub controlled_drug_name: [c_char; 64],
    pub net_claim_aed: c_double,
    pub delay_days: u32,
    pub overdue_days: u32,
    pub penalty_rate_pct: c_double,
    pub penalty_aed: c_double,
    pub adjusted_net_aed: c_double,
    pub claim_cycle_warn: bool,
    pub claim_alert_code: [c_char; 64],
    pub evidence_doc_id: [c_char; 64],
    pub evidence_section: [c_char; 64],
    pub evidence_sha256: [c_char; 65],
    pub polypharmacy_matrix_json: [c_char; 2048],
}

impl Default for TaryaqPharmacistResult {
    fn default() -> Self {
        Self {
            total_drugs_screened: 0,
            interaction_count: 0,
            has_contraindication_block: false,
            primary_block_code: [0; 64],
            primary_block_rationale: [0; 256],
            is_controlled_gate: false,
            controlled_drug_name: [0; 64],
            net_claim_aed: 0.0,
            delay_days: 0,
            overdue_days: 0,
            penalty_rate_pct: 0.0,
            penalty_aed: 0.0,
            adjusted_net_aed: 0.0,
            claim_cycle_warn: false,
            claim_alert_code: [0; 64],
            evidence_doc_id: [0; 64],
            evidence_section: [0; 64],
            evidence_sha256: [0; 65],
            polypharmacy_matrix_json: [0; 2048],
        }
    }
}

#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct TaryaqPatientCompanionResult {
    pub triage: TaryaqTriageResult,
    pub lay_instructions: [c_char; 1024],
    pub dietary_precautions: [c_char; 512],
    pub red_flag_warning: [c_char; 1024],
    pub requires_immediate_998: bool,
}

impl Default for TaryaqPatientCompanionResult {
    fn default() -> Self {
        Self {
            triage: TaryaqTriageResult::default(),
            lay_instructions: [0; 1024],
            dietary_precautions: [0; 512],
            red_flag_warning: [0; 1024],
            requires_immediate_998: false,
        }
    }
}

extern "C" {
    pub fn taryaq_verify_prescription_and_claim(rx: *const TaryaqRxContext) -> TaryaqResult;
    pub fn taryaq_verify_prescription_and_claim_ptr(
        rx: *const TaryaqRxContext,
        out_result: *mut TaryaqResult,
    ) -> TaryaqVerdict;
    pub fn taryaq_triage_patient_symptoms(symptom_narrative: *const c_char) -> TaryaqTriageResult;

    pub fn taryaq_clinical_db_init(db_path: *const c_char) -> bool;
    pub fn taryaq_clinical_db_close();
    pub fn taryaq_clinical_db_count_drugs() -> c_int;

    pub fn taryaq_clinician_ambient_copilot(
        dialogue_transcript: *const c_char,
        patient_egfr: c_double,
        proposed_drugs_csv: *const c_char,
        out_soap: *mut TaryaqSoapNote,
    ) -> bool;

    pub fn taryaq_pharmacist_verification(
        medications_csv: *const c_char,
        patient_egfr: c_double,
        net_claim_aed: c_double,
        submission_delay_days: u32,
        icd10_code: *const c_char,
        id_verified: bool,
        out_result: *mut TaryaqPharmacistResult,
    ) -> bool;

    pub fn taryaq_patient_companion_consult(
        patient_message: *const c_char,
        active_meds_csv: *const c_char,
        patient_egfr: c_double,
        out_result: *mut TaryaqPatientCompanionResult,
    ) -> bool;
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::mem::size_of;

    #[test]
    fn test_c99_struct_sizes() {
        assert_eq!(size_of::<TaryaqRxContext>(), 48);
        assert_eq!(size_of::<TaryaqResult>(), 520);
        assert_eq!(size_of::<TaryaqTriageResult>(), 648);
        assert_eq!(size_of::<TaryaqSoapNote>(), 5058);
        assert_eq!(size_of::<TaryaqPharmacistResult>(), 2760);
        assert_eq!(size_of::<TaryaqPatientCompanionResult>(), 3212);
    }
}
