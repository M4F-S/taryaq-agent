//! Clinical Module for TARYAQ (ترياق)
//! Sovereign Healthcare AI Agent - Dubai Create AI Agents Championship 2026–2027

pub mod db;

pub use db::{
    ClinicalDatabase, ClinicalError, DrugInteraction, FormularyDrug,
    HitlQuarantineRecord, InteractionCheckResult, InteractionCheckVerdict,
    InteractionSeverity, LabDiagnosticPanel, MedicalCondition, ProfileScreeningResult,
    DHA_NABIDH_SAFE, UNVERIFIED_DRUG_REQUIRES_PHARMACIST_REVIEW,
};
