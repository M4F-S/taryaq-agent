//! The Pharmacist Gate: Human-in-the-Loop Review & Promotion Workflow
//! Air-Gapped Verification Engine for Mohamed Fathy, BPharm
//! Dubai Create AI Agents Championship 2026–2027

use crate::clinical::{ClinicalDatabase, DrugInteraction, HitlQuarantineRecord};
use crate::quarantine::scanner::{HitlError, ProposedInteractionPayload};
use serde::{Deserialize, Serialize};
use std::sync::Arc;

pub const DEFAULT_PHARMACIST_REVIEWER: &str =
    "Mohamed Fathy, BPharm (Licensed Clinical Pharmacist & Systems Architect)";

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct PromotionResult {
    pub staging_id: i64,
    pub alert_code: String,
    pub severity: String,
    pub drug_a_atc: String,
    pub drug_b_atc: String,
    pub reviewer_credentials: String,
    pub reviewer_notes: String,
    pub reviewed_at_utc: String,
    pub evidence_sha256: String,
    pub promoted_interaction: DrugInteraction,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct RejectionResult {
    pub staging_id: i64,
    pub reviewer_credentials: String,
    pub reviewer_notes: String,
    pub rejected_at_utc: String,
}

pub struct ThePharmacistGate {
    db: Arc<ClinicalDatabase>,
}

impl ThePharmacistGate {
    pub fn new(db: Arc<ClinicalDatabase>) -> Self {
        Self { db }
    }

    pub fn with_default() -> Result<Self, HitlError> {
        let db = ClinicalDatabase::open_default()?;
        Ok(Self::new(Arc::new(db)))
    }

    /// List all quarantined findings awaiting pharmacist review
    pub fn list_pending_findings(&self) -> Result<Vec<HitlQuarantineRecord>, HitlError> {
        Ok(self.db.list_hitl_records(Some("PENDING"))?)
    }

    /// List quarantined findings with optional status filter ('PENDING', 'APPROVED', 'REJECTED')
    pub fn list_findings(&self, status: Option<&str>) -> Result<Vec<HitlQuarantineRecord>, HitlError> {
        Ok(self.db.list_hitl_records(status)?)
    }

    /// Retrieve full details of a quarantined finding
    pub fn get_finding(&self, staging_id: i64) -> Result<Option<HitlQuarantineRecord>, HitlError> {
        Ok(self.db.get_hitl_record(staging_id)?)
    }

    /// Inspect a quarantine record with structured parsed payload
    pub fn inspect_finding(
        &self,
        staging_id: i64,
    ) -> Result<Option<(HitlQuarantineRecord, Option<ProposedInteractionPayload>)>, HitlError> {
        let record = match self.db.get_hitl_record(staging_id)? {
            Some(r) => r,
            None => return Ok(None),
        };

        let parsed_payload: Option<ProposedInteractionPayload> = record
            .proposed_interaction_json
            .as_deref()
            .and_then(|json| serde_json::from_str(json).ok());

        Ok(Some((record, parsed_payload)))
    }

    /// Licensed Pharmacist Gate Approval: Validates clinical evidence and atomically promotes rule to production
    pub fn approve_finding(
        &self,
        staging_id: i64,
        reviewer_notes: &str,
        reviewer_credentials: Option<&str>,
    ) -> Result<PromotionResult, HitlError> {
        let rec = self
            .db
            .get_hitl_record(staging_id)?
            .ok_or(HitlError::RecordNotFound(staging_id))?;

        if rec.review_status != "PENDING" {
            return Err(HitlError::InvalidTransition(staging_id, rec.review_status));
        }

        let reviewer = reviewer_credentials
            .unwrap_or(DEFAULT_PHARMACIST_REVIEWER)
            .trim();

        if reviewer.is_empty() {
            return Err(HitlError::UnauthorizedReviewer(
                "Reviewer credentials cannot be empty for HITL approval".to_string(),
            ));
        }

        // Parse proposed interaction JSON
        let json_str = rec.proposed_interaction_json.as_deref().ok_or_else(|| {
            HitlError::MalformedProposal("Missing proposed_interaction_json in quarantine record".to_string())
        })?;

        let payload: ProposedInteractionPayload = serde_json::from_str(json_str)
            .map_err(|e| HitlError::MalformedProposal(format!("JSON parsing failed: {}", e)))?;

        let formatted_notes = format!("APPROVED by {}: {}", reviewer, reviewer_notes.trim());

        // Atomically promote into live production table: drug_interactions
        let promoted_interaction = self.db.promote_quarantine_record(
            staging_id,
            &payload.drug_a_name,
            &payload.drug_a_atc,
            &payload.drug_b_name,
            &payload.drug_b_atc,
            &payload.alert_code,
            &payload.severity,
            &payload.mechanism,
            &payload.clinical_action,
            &payload.evidence_citation,
            Some(&payload.evidence_sha256),
            &formatted_notes,
        )?;

        let now_utc = crate::trimodal::audit::current_date_utc();

        Ok(PromotionResult {
            staging_id,
            alert_code: payload.alert_code,
            severity: payload.severity,
            drug_a_atc: payload.drug_a_atc,
            drug_b_atc: payload.drug_b_atc,
            reviewer_credentials: reviewer.to_string(),
            reviewer_notes: formatted_notes,
            reviewed_at_utc: now_utc,
            evidence_sha256: payload.evidence_sha256,
            promoted_interaction,
        })
    }

    /// Licensed Pharmacist Gate Rejection: Discards finding without modifying live clinical tables
    pub fn reject_finding(
        &self,
        staging_id: i64,
        reviewer_notes: &str,
        reviewer_credentials: Option<&str>,
    ) -> Result<RejectionResult, HitlError> {
        let rec = self
            .db
            .get_hitl_record(staging_id)?
            .ok_or(HitlError::RecordNotFound(staging_id))?;

        if rec.review_status != "PENDING" {
            return Err(HitlError::InvalidTransition(staging_id, rec.review_status));
        }

        let reviewer = reviewer_credentials
            .unwrap_or(DEFAULT_PHARMACIST_REVIEWER)
            .trim();

        if reviewer.is_empty() {
            return Err(HitlError::UnauthorizedReviewer(
                "Reviewer credentials cannot be empty for HITL rejection".to_string(),
            ));
        }

        let formatted_notes = format!("REJECTED by {}: {}", reviewer, reviewer_notes.trim());
        let updated = self.db.reject_quarantine_record(staging_id, &formatted_notes)?;

        if !updated {
            return Err(HitlError::InvalidTransition(
                staging_id,
                "Concurrent state modification occurred".to_string(),
            ));
        }

        let now_utc = crate::trimodal::audit::current_date_utc();

        Ok(RejectionResult {
            staging_id,
            reviewer_credentials: reviewer.to_string(),
            reviewer_notes: formatted_notes,
            rejected_at_utc: now_utc,
        })
    }
}
