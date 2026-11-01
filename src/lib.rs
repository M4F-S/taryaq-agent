//! TARYAQ (ترياق) Sovereign Clinical AI Agent Runtime
//! Dubai Create AI Agents Championship 2026–2027

pub mod clinical;
pub mod safety;
pub mod trimodal;
pub mod voice;

pub use clinical::{
    ClinicalDatabase, ClinicalError, DrugInteraction, FormularyDrug,
    HitlQuarantineRecord, InteractionCheckResult, InteractionCheckVerdict,
    InteractionSeverity, LabDiagnosticPanel, MedicalCondition, ProfileScreeningResult,
    DHA_NABIDH_SAFE, UNVERIFIED_DRUG_REQUIRES_PHARMACIST_REVIEW,
};
pub use trimodal::{
    AuditEntryInput, AuditError, AuditLedger, AuditRecord, CdsInterrupt,
    ClinicianAmbientCopilot, ClinicianEncounterInput, ClinicianOverrideSubmission,
    DhaPenaltyBreakdown, DialectCategory, DialogueTurn, DifferentialDiagnosis,
    EmergencyDispatchDirective, PatientCompanionConsultation, PatientHealthCompanion,
    PharmacistError, PharmacistVerificationConsole, PharmacistVerificationInput,
    PharmacistVerificationReport, PosologyGuidance, RedFlagCategory,
    StructuredSoapNote, TokenRegistry, TraceLinkedEvidence, VitalsObservation,
    DEFAULT_AUDIT_LOG,
};
pub use voice::{
    AudioFormat, AudioSegment, TranscriptionResult, VoiceError, VoicePipeline,
};
pub use safety::{
    clinical_db_close, clinical_db_count_drugs, clinical_db_init, clinician_ambient_copilot,
    patient_companion_consult, pharmacist_verification, triage_patient_symptoms,
    verify_prescription_and_claim, verify_prescription_raw, ClinicalResult,
    PatientCompanionResult, PharmacistResult, RxContext, SoapNote, TriageResult,
};
pub use safety::{TaryaqTriageLevel, TaryaqVerdict};
