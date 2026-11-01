//! Human-in-the-Loop (HITL) Web Ingestion & Quarantine Module
//! Air-Gapped Clinical Buffer & Pharmacist Gate
//! Dubai Create AI Agents Championship 2026–2027

pub mod reviewer;
pub mod scanner;

pub use reviewer::{
    PromotionResult, RejectionResult, ThePharmacistGate, DEFAULT_PHARMACIST_REVIEWER,
};
pub use scanner::{
    compute_evidence_sha256, AlertSource, HitlError, ProposedInteractionPayload, RawAlertFinding,
    WebAlertScanner,
};
