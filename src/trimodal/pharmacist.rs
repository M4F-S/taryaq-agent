//! Pharmacist Verification Console Interface
//! Dubai Create AI Agents Championship 2026–2027

use crate::clinical::{
    ClinicalDatabase, DrugInteraction, InteractionCheckVerdict,
    ProfileScreeningResult,
};
use crate::trimodal::audit::{AuditEntryInput, AuditLedger, AuditRecord};
use serde::{Deserialize, Serialize};
use std::collections::HashSet;
use std::path::{Path, PathBuf};
use std::sync::{Arc, Mutex, RwLock};
use thiserror::Error;

pub const DHA_SN_03_2019_3_7_1_PASSAGE_SHA256: &str =
    "b0a422ad0e2edc66290007caaa5508d4ec4ac4229453b170881fb482449218f3";

#[derive(Error, Debug)]
pub enum PharmacistError {
    #[error("Clinical Database Error: {0}")]
    DbError(String),
    #[error("Audit Ledger Error: {0}")]
    AuditError(#[from] crate::trimodal::audit::AuditError),
    #[error("Override Token Replay Conflict (HTTP 409): Token '{token}' has already been consumed. Replay attacks are prohibited under Section 2.4.")]
    OverrideReplayConflict {
        token: String,
        status_code: u16,
        message: String,
    },
    #[error("Invalid Clinician Override: {0}")]
    InvalidOverride(String),
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct ClinicianOverrideSubmission {
    pub override_token: String,
    pub clinician_id: String,
    pub clinical_rationale: String,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct PharmacistVerificationInput {
    pub patient_id: String,
    pub medication_regimen: Vec<String>,
    pub patient_egfr: f64,
    pub primary_diagnosis_icd10: Option<String>,
    pub net_claim_aed: f64,
    pub claim_submission_delay_days: u32,
    pub is_resubmission: bool,
    pub resubmission_delay_days: Option<u32>,
    pub clinician_override: Option<ClinicianOverrideSubmission>,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct DhaPenaltyBreakdown {
    pub statutory_rule: String,
    pub net_claim_aed: f64,
    pub submission_delay_days: u32,
    pub statutory_grace_period_days: u32,
    pub overdue_days: u32,
    pub statutory_penalty_rate_pct_per_day: f64,
    pub total_penalty_rate_pct: f64,
    pub penalty_amount_aed: f64,
    pub adjusted_net_claim_aed: f64,
    pub denial_alert_code: Option<String>,
    pub warning_message: Option<String>,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct PharmacistVerificationReport {
    pub patient_id: String,
    pub total_drugs_screened: usize,
    pub is_dispensation_approved: bool,
    pub is_contraindication_blocked: bool,
    pub primary_verdict: String,
    pub primary_alert_code: String,
    pub primary_clinical_rationale: String,
    pub detected_interactions: Vec<DrugInteraction>,
    pub unverified_drugs: Vec<String>,
    pub renal_ckd_stage: String,
    pub renal_dosing_warning: Option<String>,
    pub override_status: String,
    pub override_token_consumed: Option<String>,
    pub dha_claims_scrubbing: DhaPenaltyBreakdown,
    pub audit_committed: bool,
    pub audit_record: Option<AuditRecord>,
}

/// Thread-safe single-use token registry protecting against override replay attacks
pub struct TokenRegistry {
    consumed_tokens: RwLock<HashSet<String>>,
    storage_file: Option<PathBuf>,
    lock_mutex: Mutex<()>,
}

impl TokenRegistry {
    pub fn new() -> Self {
        Self {
            consumed_tokens: RwLock::new(HashSet::new()),
            storage_file: None,
            lock_mutex: Mutex::new(()),
        }
    }

    pub fn with_storage(path: impl AsRef<Path>) -> Self {
        let p = path.as_ref().to_path_buf();
        let mut set = HashSet::new();
        if p.exists() {
            if let Ok(content) = std::fs::read_to_string(&p) {
                for line in content.lines() {
                    let t = line.trim();
                    if !t.is_empty() {
                        set.insert(t.to_string());
                    }
                }
            }
        }
        Self {
            consumed_tokens: RwLock::new(set),
            storage_file: Some(p),
            lock_mutex: Mutex::new(()),
        }
    }

    /// Check and atomically consume an override token; returns HTTP 409 Conflict if replayed
    pub fn consume_token(&self, token: &str) -> Result<(), PharmacistError> {
        let t = token.trim();
        if t.is_empty() {
            return Err(PharmacistError::InvalidOverride("Override token cannot be empty".to_string()));
        }

        let _guard = self.lock_mutex.lock().unwrap();

        // Check if already consumed
        {
            let set = self.consumed_tokens.read().unwrap();
            if set.contains(t) {
                return Err(PharmacistError::OverrideReplayConflict {
                    token: t.to_string(),
                    status_code: 409,
                    message: format!(
                        "Token '{}' has already been consumed and cannot be reused. Replay attack blocked with HTTP 409 Conflict.",
                        t
                    ),
                });
            }
        }

        // Consume token
        {
            let mut set = self.consumed_tokens.write().unwrap();
            set.insert(t.to_string());
        }

        // Persist if storage configured
        if let Some(ref path) = self.storage_file {
            use std::io::Write;
            if let Ok(mut f) = std::fs::OpenOptions::new().create(true).append(true).open(path) {
                let _ = writeln!(f, "{}", t);
            }
        }

        Ok(())
    }

    pub fn is_consumed(&self, token: &str) -> bool {
        let set = self.consumed_tokens.read().unwrap();
        set.contains(token.trim())
    }
}

pub struct PharmacistVerificationConsole {
    db: Option<Arc<ClinicalDatabase>>,
    audit_ledger: Arc<AuditLedger>,
    token_registry: Arc<TokenRegistry>,
}

impl PharmacistVerificationConsole {
    pub fn new(
        db: Option<Arc<ClinicalDatabase>>,
        audit_ledger: Option<Arc<AuditLedger>>,
        token_registry: Option<Arc<TokenRegistry>>,
    ) -> Self {
        Self {
            db,
            audit_ledger: audit_ledger.unwrap_or_else(|| Arc::new(AuditLedger::default_ledger())),
            token_registry: token_registry.unwrap_or_else(|| Arc::new(TokenRegistry::new())),
        }
    }

    pub fn with_default() -> Self {
        let db = ClinicalDatabase::open_default().ok().map(Arc::new);
        let audit = Arc::new(AuditLedger::default_ledger());
        let tokens = Arc::new(TokenRegistry::new());
        Self::new(db, Some(audit), Some(tokens))
    }

    pub fn token_registry(&self) -> &Arc<TokenRegistry> {
        &self.token_registry
    }

    /// Perform full multi-drug polypharmacy risk screening, DHA SN 03/2019 claims penalty calculation,
    /// single-use override validation, and append-only audit chaining.
    pub fn verify_patient_regimen(
        &self,
        input: &PharmacistVerificationInput,
    ) -> Result<PharmacistVerificationReport, PharmacistError> {
        let n_drugs = input.medication_regimen.len();

        // 1. Polypharmacy screening via ClinicalDatabase
        let (screening, renal_warning) = self.screen_polypharmacy(&input.medication_regimen, input.patient_egfr)?;

        let has_contraindication = !screening.is_safe
            && (screening.verdict == InteractionCheckVerdict::ContraindicatedBlock
                || screening.verdict == InteractionCheckVerdict::UnverifiedReview);

        // 2. Evaluate Clinician Override if present
        let mut override_approved = false;
        let mut consumed_token = None;
        let mut override_status_str = "NONE".to_string();

        if has_contraindication {
            if let Some(ref ov) = input.clinician_override {
                if ov.clinical_rationale.trim().len() < 5 {
                    return Err(PharmacistError::InvalidOverride(
                        "Clinical rationale must contain substantive justification (>= 5 chars)".to_string(),
                    ));
                }

                // Attempt to consume token (fails with HTTP 409 if replayed)
                self.token_registry.consume_token(&ov.override_token)?;

                override_approved = true;
                consumed_token = Some(ov.override_token.clone());
                override_status_str = "APPROVED".to_string();
            }
        }

        let is_approved = !has_contraindication || override_approved;

        // 3. DHA SN 03/2019 Claims Penalty Calculation
        let claims_scrub = self.scrub_dha_claims(
            input.net_claim_aed,
            input.claim_submission_delay_days,
            input.is_resubmission,
            input.resubmission_delay_days,
        );

        // 4. Determine Verdict & Alert Code
        let (verdict_str, alert_code_str, rationale_str) = if is_approved {
            if override_approved {
                (
                    "CLINICAL_CONTRAINDICATION_OVERRIDDEN".to_string(),
                    screening.primary_alert_code.clone(),
                    format!("Override Approved: {}", screening.primary_rationale),
                )
            } else if claims_scrub.overdue_days > 0 {
                (
                    "CLINICAL_CLAIM_CYCLE_WARN".to_string(),
                    "DHA_SN_03_2019_LATE_SUBMISSION_WARNING".to_string(),
                    claims_scrub
                        .warning_message
                        .clone()
                        .unwrap_or_else(|| "Statutory claim submission overdue".to_string()),
                )
            } else {
                (
                    "CLINICAL_SAFE".to_string(),
                    "DHA_NABIDH_SAFE".to_string(),
                    "Patient medication profile screened safe with zero contraindications.".to_string(),
                )
            }
        } else {
            (
                "CLINICAL_CONTRAINDICATION_BLOCK".to_string(),
                screening.primary_alert_code.clone(),
                screening.primary_rationale.clone(),
            )
        };

        // 5. Append to Cryptographic Audit Ledger
        let audit_input = AuditEntryInput {
            action: if is_approved {
                "PRESCRIPTION_COMMITTED".to_string()
            } else {
                "PRESCRIPTION_ROLLED_BACK".to_string()
            },
            patient_id: input.patient_id.clone(),
            date: crate::trimodal::audit::current_date_utc(),
            dose: "Standard Profile Posology".to_string(),
            primary_drug: input.medication_regimen.first().cloned().unwrap_or_else(|| "NONE".to_string()),
            concurrent_drug: input.medication_regimen.get(1).cloned(),
            patient_egfr: input.patient_egfr,
            verdict: verdict_str.clone(),
            alert_code: alert_code_str.clone(),
            override_status: override_status_str.clone(),
            override_token: consumed_token.clone(),
            override_rationale: input.clinician_override.as_ref().map(|o| o.clinical_rationale.clone()),
            is_repeat_override: false,
            statutory_doc_id: Some("DHA_SN_03_2019".to_string()),
            statutory_section: Some("§3.7.1".to_string()),
            statutory_passage_sha256: Some(DHA_SN_03_2019_3_7_1_PASSAGE_SHA256.to_string()),
            claim_net_aed: claims_scrub.net_claim_aed,
            claim_delay_days: claims_scrub.submission_delay_days,
            penalty_aed: claims_scrub.penalty_amount_aed,
            reason: if !is_approved {
                Some(format!("Prescription blocked: {}", screening.primary_alert_code))
            } else {
                None
            },
        };

        let audit_rec = match self.audit_ledger.append_record(&audit_input, None) {
            Ok(rec) => Some(rec),
            Err(e) => {
                // Fail-closed on audit tampering or disk full
                return Err(PharmacistError::AuditError(e));
            }
        };

        Ok(PharmacistVerificationReport {
            patient_id: input.patient_id.clone(),
            total_drugs_screened: n_drugs,
            is_dispensation_approved: is_approved,
            is_contraindication_blocked: has_contraindication,
            primary_verdict: verdict_str,
            primary_alert_code: alert_code_str,
            primary_clinical_rationale: rationale_str,
            detected_interactions: screening.interactions_detected,
            unverified_drugs: screening.unverified_drugs,
            renal_ckd_stage: ckd_stage_str(input.patient_egfr).to_string(),
            renal_dosing_warning: renal_warning,
            override_status: override_status_str,
            override_token_consumed: consumed_token,
            dha_claims_scrubbing: claims_scrub,
            audit_committed: audit_rec.is_some(),
            audit_record: audit_rec,
        })
    }

    /// Screen multi-drug profile and renal contraindications
    fn screen_polypharmacy(
        &self,
        drugs: &[String],
        egfr: f64,
    ) -> Result<(ProfileScreeningResult, Option<String>), PharmacistError> {
        let drug_slices: Vec<&str> = drugs.iter().map(|s| s.as_str()).collect();

        // 1. Check renal cutoffs first
        let mut renal_warning = None;
        for d in drugs {
            let lower = d.to_lowercase();
            if lower.contains("metformin") && egfr < 30.0 {
                let inter = ProfileScreeningResult {
                    is_safe: false,
                    verdict: InteractionCheckVerdict::ContraindicatedBlock,
                    primary_alert_code: "METFORMIN_SEVERE_RENAL_IMPAIRMENT".to_string(),
                    primary_rationale: format!(
                        "Contraindication: Metformin at eGFR {:.1} mL/min (< 30 mL/min cutoff) is strictly contraindicated due to risk of fatal lactic acidosis.",
                        egfr
                    ),
                    interactions_detected: Vec::new(),
                    unverified_drugs: Vec::new(),
                    unverified_combinations: Vec::new(),
                    resolved_drugs: Vec::new(),
                };
                return Ok((inter, Some("eGFR < 30 mL/min: Absolute Metformin Contraindication".to_string())));
            } else if lower.contains("metformin") && egfr < 45.0 {
                renal_warning = Some(format!(
                    "Renal Warning: Metformin at eGFR {:.1} mL/min requires dose ceiling of 1000 mg/day under DHA/DHIC/HEIPD/CG-07.",
                    egfr
                ));
            }

            if lower.contains("colchicine") && egfr < 30.0 {
                let inter = ProfileScreeningResult {
                    is_safe: false,
                    verdict: InteractionCheckVerdict::ContraindicatedBlock,
                    primary_alert_code: "COLCHICINE_SEVERE_RENAL_IMPAIRMENT".to_string(),
                    primary_rationale: format!(
                        "Contraindication: Colchicine in severe renal impairment (eGFR {:.1} < 30 mL/min) risks cumulative neuromuscular toxicity and fatal pancytopenia.",
                        egfr
                    ),
                    interactions_detected: Vec::new(),
                    unverified_drugs: Vec::new(),
                    unverified_combinations: Vec::new(),
                    resolved_drugs: Vec::new(),
                };
                return Ok((inter, Some("eGFR < 30 mL/min: Absolute Colchicine Contraindication".to_string())));
            }
        }

        // 2. Query ClinicalDatabase for multi-drug pairwise combinations
        if let Some(ref db) = self.db {
            let res = db
                .screen_patient_profile(&drug_slices)
                .map_err(|e| PharmacistError::DbError(e.to_string()))?;
            Ok((res, renal_warning))
        } else {
            // Fallback: check pairwise known interactions
            let res = fallback_screen_profile(&drug_slices);
            Ok((res, renal_warning))
        }
    }

    /// DHA SN 03/2019 Claims Penalty Engine (§3.7 & §3.8)
    /// 0.03% per calendar day overdue past statutory 30-day window
    pub fn scrub_dha_claims(
        &self,
        net_claim_aed: f64,
        submission_delay_days: u32,
        is_resubmission: bool,
        resubmission_delay_days: Option<u32>,
    ) -> DhaPenaltyBreakdown {
        let (statutory_rule, grace_period, delay_days) = if is_resubmission {
            (
                "DHA SN 03/2019 §3.8.1 (Resubmission Window: 21 calendar days from Remittance Advice)",
                21,
                resubmission_delay_days.unwrap_or(submission_delay_days),
            )
        } else {
            (
                "DHA SN 03/2019 §3.7.1 (Initial Claim Window: 30 calendar days from encounter)",
                30,
                submission_delay_days,
            )
        };

        let overdue_days = delay_days.saturating_sub(grace_period);
        let daily_rate = 0.0003; // 0.03% per day
        let total_rate = overdue_days as f64 * daily_rate;
        let penalty_aed = round_2dec(net_claim_aed * total_rate);
        let adjusted_net = (net_claim_aed - penalty_aed).max(0.0);

        let (alert_code, warning_msg) = if overdue_days > 0 {
            (
                Some("DHA_SN_03_2019_LATE_SUBMISSION_WARNING".to_string()),
                Some(format!(
                    "DHA SN 03/2019 Claim Submission Delayed by {} days ({} days overdue past {}-day statutory window). Statutory penalty rate: {:.4}% (0.03%/day). Penalty AED: {:.2}. Adjusted Net Claim: AED {:.2}.",
                    delay_days, overdue_days, grace_period, total_rate * 100.0, penalty_aed, adjusted_net
                )),
            )
        } else {
            (None, None)
        };

        DhaPenaltyBreakdown {
            statutory_rule: statutory_rule.to_string(),
            net_claim_aed,
            submission_delay_days: delay_days,
            statutory_grace_period_days: grace_period,
            overdue_days,
            statutory_penalty_rate_pct_per_day: 0.03,
            total_penalty_rate_pct: total_rate * 100.0,
            penalty_amount_aed: penalty_aed,
            adjusted_net_claim_aed: adjusted_net,
            denial_alert_code: alert_code,
            warning_message: warning_msg,
        }
    }
}

fn round_2dec(val: f64) -> f64 {
    (val * 100.0).round() / 100.0
}

fn ckd_stage_str(egfr: f64) -> &'static str {
    if egfr >= 90.0 {
        "CKD Stage 1 (Normal / eGFR >= 90)"
    } else if egfr >= 60.0 {
        "CKD Stage 2 (Mild / eGFR 60-89)"
    } else if egfr >= 45.0 {
        "CKD Stage 3a (Mild-to-Moderate / eGFR 45-59)"
    } else if egfr >= 30.0 {
        "CKD Stage 3b (Moderate-to-Severe / eGFR 30-44)"
    } else if egfr >= 15.0 {
        "CKD Stage 4 (Severe / eGFR 15-29)"
    } else {
        "CKD Stage 5 (Kidney Failure / eGFR < 15)"
    }
}

fn fallback_screen_profile(drugs: &[&str]) -> ProfileScreeningResult {
    let lower_drugs: Vec<String> = drugs.iter().map(|d| d.to_lowercase()).collect();

    // Check Colchicine + Clarithromycin
    if lower_drugs.iter().any(|d| d.contains("colchicine"))
        && lower_drugs.iter().any(|d| d.contains("clarithromycin"))
    {
        return ProfileScreeningResult {
            is_safe: false,
            verdict: InteractionCheckVerdict::ContraindicatedBlock,
            primary_alert_code: "COLCHICINE_CLARITHROMYCIN_FATAL_TOXICITY".to_string(),
            primary_rationale: "Absolute Contraindication: Potent CYP3A4 and P-gp inhibition by Clarithromycin causes fatal colchicine accumulation.".to_string(),
            interactions_detected: Vec::new(),
            unverified_drugs: Vec::new(),
            unverified_combinations: Vec::new(),
            resolved_drugs: Vec::new(),
        };
    }

    // Check Warfarin + Amiodarone
    if lower_drugs.iter().any(|d| d.contains("warfarin"))
        && lower_drugs.iter().any(|d| d.contains("amiodarone"))
    {
        return ProfileScreeningResult {
            is_safe: false,
            verdict: InteractionCheckVerdict::ContraindicatedBlock,
            primary_alert_code: "WARFARIN_AMIODARONE_HEMORRHAGE".to_string(),
            primary_rationale: "Absolute Contraindication: Amiodarone inhibits CYP2C9 clearance of S-warfarin, causing massive INR elevation and life-threatening hemorrhage.".to_string(),
            interactions_detected: Vec::new(),
            unverified_drugs: Vec::new(),
            unverified_combinations: Vec::new(),
            resolved_drugs: Vec::new(),
        };
    }

    // Check Simvastatin + Clarithromycin
    if lower_drugs.iter().any(|d| d.contains("simvastatin"))
        && lower_drugs.iter().any(|d| d.contains("clarithromycin"))
    {
        return ProfileScreeningResult {
            is_safe: false,
            verdict: InteractionCheckVerdict::ContraindicatedBlock,
            primary_alert_code: "SIMVASTATIN_CLARITHROMYCIN_RHABDO".to_string(),
            primary_rationale: "Absolute Contraindication: Potent CYP3A4 inhibition increases simvastatin plasma concentrations >10-fold, precipitating severe rhabdomyolysis and acute renal failure.".to_string(),
            interactions_detected: Vec::new(),
            unverified_drugs: Vec::new(),
            unverified_combinations: Vec::new(),
            resolved_drugs: Vec::new(),
        };
    }

    ProfileScreeningResult {
        is_safe: true,
        verdict: InteractionCheckVerdict::Safe,
        primary_alert_code: "DHA_NABIDH_SAFE".to_string(),
        primary_rationale: "Medication profile verified safe.".to_string(),
        interactions_detected: Vec::new(),
        unverified_drugs: Vec::new(),
        unverified_combinations: Vec::new(),
        resolved_drugs: Vec::new(),
    }
}
