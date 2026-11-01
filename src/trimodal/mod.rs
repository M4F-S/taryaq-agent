//! Tri-Modal Clinical Interfaces for TARYAQ (ترياق)
//! Dubai Create AI Agents Championship 2026–2027

pub mod audit;
pub mod copilot;
pub mod pharmacist;
pub mod companion;

pub use audit::{AuditEntryInput, AuditError, AuditLedger, AuditRecord, DEFAULT_AUDIT_LOG};
pub use copilot::{
    CdsInterrupt, ClinicianAmbientCopilot, ClinicianEncounterInput, DialogueTurn,
    DifferentialDiagnosis, StructuredSoapNote, TraceLinkedEvidence, VitalsObservation,
};
pub use pharmacist::{
    ClinicianOverrideSubmission, DhaPenaltyBreakdown, PharmacistError,
    PharmacistVerificationConsole, PharmacistVerificationInput, PharmacistVerificationReport,
    TokenRegistry,
};
pub use companion::{
    DialectCategory, EmergencyDispatchDirective, PatientCompanionConsultation,
    PatientHealthCompanion, PosologyGuidance, RedFlagCategory,
};
