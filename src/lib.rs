//! TARYAQ (ترياق) Sovereign Clinical AI Agent Runtime
//! Dubai Create AI Agents Championship 2026–2027

pub mod safety;

pub use safety::{
    clinical_db_close, clinical_db_count_drugs, clinical_db_init, clinician_ambient_copilot,
    patient_companion_consult, pharmacist_verification, triage_patient_symptoms,
    verify_prescription_and_claim, verify_prescription_raw, ClinicalResult,
    PatientCompanionResult, PharmacistResult, RxContext, SoapNote, TriageResult,
};
pub use safety::{TaryaqTriageLevel, TaryaqVerdict};
