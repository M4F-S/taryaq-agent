//! Sovereign Clinical Database Engine for TARYAQ (ترياق)
//! Dubai Create AI Agents Championship 2026–2027 | Best Societal AI Agent
//! Implements Canonical Master Specification Section 2.1 & 2.2

use rusqlite::{params, Connection, OptionalExtension};
use serde::{Deserialize, Serialize};
use std::collections::HashSet;
use std::path::Path;
use std::sync::Mutex;
use thiserror::Error;

/// Statutory alert code for unmodeled drugs or unverified combinations
pub const UNVERIFIED_DRUG_REQUIRES_PHARMACIST_REVIEW: &str =
    "UNVERIFIED_DRUG_REQUIRES_PHARMACIST_REVIEW";

/// Default alert code when a profile or co-prescription is verified safe
pub const DHA_NABIDH_SAFE: &str = "DHA_NABIDH_SAFE";

#[derive(Debug, Error)]
pub enum ClinicalError {
    #[error("SQLite database error: {0}")]
    Sqlite(#[from] rusqlite::Error),
    #[error("JSON serialization error: {0}")]
    Json(#[from] serde_json::Error),
    #[error("Clinical database not found at path: {0}")]
    NotFound(String),
    #[error("Thread lock acquisition error: {0}")]
    LockError(String),
}

/// Clinical Severity Tier for Drug-Drug Interactions
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, Serialize, Deserialize)]
pub enum InteractionSeverity {
    Contraindicated,
    Major,
    Moderate,
}

impl InteractionSeverity {
    pub fn as_str(&self) -> &'static str {
        match self {
            InteractionSeverity::Contraindicated => "CONTRAINDICATED",
            InteractionSeverity::Major => "MAJOR",
            InteractionSeverity::Moderate => "MODERATE",
        }
    }

    pub fn from_str_lossy(s: &str) -> Self {
        match s.to_ascii_uppercase().as_str() {
            "CONTRAINDICATED" => InteractionSeverity::Contraindicated,
            "MAJOR" => InteractionSeverity::Major,
            _ => InteractionSeverity::Moderate,
        }
    }
}

/// Generalized Drug Formulary record (Master Spec §2.1)
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct FormularyDrug {
    pub atc_code: String,
    pub canonical_name: String,
    pub arabic_name: String,
    pub brand_names_json: String,
    pub drug_class_id: i32,
    pub renal_egfr_cutoff: f64,
    pub hepatic_metabolism_pct: f64,
    pub max_daily_dose_mg: f64,
    pub is_controlled_uae: bool,
    pub schedule_law_30_2021: Option<String>,
    pub narrow_therapeutic_index: bool,
    pub cyp3a4_profile: String,
    pub cyp2d6_profile: String,
    pub cyp2c9_profile: String,
    pub cyp2c19_profile: String,
    pub p_gp_profile: String,
    pub qt_risk: String,
    pub hyperkalemia_risk: bool,
    pub serotonin_risk: bool,
    pub indications_icd10_json: String,
    pub clinical_pearls: Option<String>,
}

impl FormularyDrug {
    pub fn brand_names(&self) -> Vec<String> {
        serde_json::from_str(&self.brand_names_json).unwrap_or_default()
    }

    pub fn indications(&self) -> Vec<String> {
        serde_json::from_str(&self.indications_icd10_json).unwrap_or_default()
    }
}

/// Pairwise & Class Clinical Interaction record (Master Spec §2.1 & §2.2)
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct DrugInteraction {
    pub interaction_id: i64,
    pub drug_a_atc: String,
    pub drug_b_atc: String,
    pub alert_code: String,
    pub severity_level: String,
    pub mechanism_text: String,
    pub clinical_action: String,
    pub evidence_citation: String,
}

impl DrugInteraction {
    pub fn severity(&self) -> InteractionSeverity {
        InteractionSeverity::from_str_lossy(&self.severity_level)
    }

    pub fn is_contraindicated(&self) -> bool {
        self.severity() == InteractionSeverity::Contraindicated
    }
}

/// Generalized Disease / Indication Taxonomy record (Master Spec §2.1)
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct MedicalCondition {
    pub icd10_code: String,
    pub icd11_code: Option<String>,
    pub condition_name_en: String,
    pub condition_name_ar: String,
    pub category: String,
    pub red_flag_priority: bool,
}

/// Diagnostic Laboratory Panel record (LOINC, Master Spec §2.1)
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct LabDiagnosticPanel {
    pub loinc_code: String,
    pub test_name: String,
    pub specimen: String,
    pub standard_unit: String,
    pub reference_low: Option<f64>,
    pub reference_high: Option<f64>,
    pub critical_panic_low: Option<f64>,
    pub critical_panic_high: Option<f64>,
}

/// Staging Buffer for Human-in-the-Loop Web Updates (Master Spec §2.1)
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct HitlQuarantineRecord {
    pub staging_id: i64,
    pub source_url: String,
    pub extracted_finding: String,
    pub target_atc_code: Option<String>,
    pub proposed_interaction_json: Option<String>,
    pub evidence_pmid: Option<String>,
    pub discovered_at: Option<String>,
    pub review_status: String,
    pub reviewer_notes: Option<String>,
    pub reviewed_at: Option<String>,
}

/// Overall outcome verdict for medication screening
#[derive(Debug, Clone, Copy, PartialEq, Eq, Serialize, Deserialize)]
pub enum InteractionCheckVerdict {
    Safe,
    ContraindicatedBlock,
    MajorWarning,
    ModerateWarning,
    UnverifiedReview,
}

impl InteractionCheckVerdict {
    pub fn as_str(&self) -> &'static str {
        match self {
            InteractionCheckVerdict::Safe => "SAFE",
            InteractionCheckVerdict::ContraindicatedBlock => "CONTRAINDICATED_BLOCK",
            InteractionCheckVerdict::MajorWarning => "MAJOR_WARNING",
            InteractionCheckVerdict::ModerateWarning => "MODERATE_WARNING",
            InteractionCheckVerdict::UnverifiedReview => "UNVERIFIED_REVIEW",
        }
    }
}

/// Outcome of screening an individual pairwise combination
#[derive(Debug, Clone, Serialize, Deserialize)]
pub enum InteractionCheckResult {
    VerifiedSafe,
    InteractionFound(DrugInteraction),
    UnverifiedDrug { drug: String, reason: String },
    UnverifiedCombination { drug_a: String, drug_b: String, reason: String },
}

/// Comprehensive outcome of multi-drug patient profile screening
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct ProfileScreeningResult {
    pub is_safe: bool,
    pub verdict: InteractionCheckVerdict,
    pub primary_alert_code: String,
    pub primary_rationale: String,
    pub interactions_detected: Vec<DrugInteraction>,
    pub unverified_drugs: Vec<String>,
    pub unverified_combinations: Vec<(String, String)>,
    pub resolved_drugs: Vec<FormularyDrug>,
}

/// Thread-safe Sovereign Clinical Database Engine
pub struct ClinicalDatabase {
    conn: Mutex<Connection>,
}

impl ClinicalDatabase {
    /// Open SQLite database from specified file path
    pub fn open(path: impl AsRef<Path>) -> Result<Self, ClinicalError> {
        let p = path.as_ref();
        if !p.exists() {
            return Err(ClinicalError::NotFound(p.to_string_lossy().to_string()));
        }
        let conn = Connection::open_with_flags(
            p,
            rusqlite::OpenFlags::SQLITE_OPEN_READ_WRITE
                | rusqlite::OpenFlags::SQLITE_OPEN_CREATE
                | rusqlite::OpenFlags::SQLITE_OPEN_NO_MUTEX,
        )?;
        conn.execute_batch("PRAGMA foreign_keys = ON;")?;
        Ok(Self {
            conn: Mutex::new(conn),
        })
    }

    /// Open in-memory SQLite database and initialize complete canonical schema
    pub fn open_in_memory() -> Result<Self, ClinicalError> {
        let conn = Connection::open_in_memory()?;
        conn.execute_batch("PRAGMA foreign_keys = ON;")?;
        let db = Self {
            conn: Mutex::new(conn),
        };
        db.init_schema()?;
        Ok(db)
    }

    /// Locate and open default database using standard environment and relative paths
    pub fn open_default() -> Result<Self, ClinicalError> {
        if let Ok(env_path) = std::env::var("TARYAQ_CLINICAL_DB") {
            if !env_path.trim().is_empty() && Path::new(&env_path).exists() {
                return Self::open(&env_path);
            }
        }

        let candidates = [
            "corpus/taryaq_clinical.db",
            "../corpus/taryaq_clinical.db",
            "../../corpus/taryaq_clinical.db",
        ];

        for cand in &candidates {
            if Path::new(cand).exists() {
                return Self::open(cand);
            }
        }

        // Check relative to current executable
        if let Ok(exe) = std::env::current_exe() {
            if let Some(parent) = exe.parent() {
                let candidate = parent.join("corpus").join("taryaq_clinical.db");
                if candidate.exists() {
                    return Self::open(candidate);
                }
            }
        }

        Err(ClinicalError::NotFound(
            "Could not locate corpus/taryaq_clinical.db in standard paths".to_string(),
        ))
    }

    /// Initialize the complete Canonical Master Specification §2.1 DDL
    pub fn init_schema(&self) -> Result<(), ClinicalError> {
        let conn = self.conn.lock().map_err(|e| ClinicalError::LockError(e.to_string()))?;
        conn.execute_batch(
            r#"
            CREATE TABLE IF NOT EXISTS drug_classes (
                class_id INTEGER PRIMARY KEY AUTOINCREMENT,
                class_name TEXT NOT NULL UNIQUE COLLATE NOCASE,
                category TEXT NOT NULL,
                default_cyp_profile TEXT DEFAULT 'NONE'
            );

            CREATE TABLE IF NOT EXISTS formulary_drugs (
                atc_code VARCHAR(10) PRIMARY KEY,
                canonical_name VARCHAR(128) NOT NULL COLLATE NOCASE,
                arabic_name VARCHAR(128) NOT NULL COLLATE NOCASE,
                brand_names_json TEXT NOT NULL,
                drug_class_id INTEGER NOT NULL,
                renal_egfr_cutoff REAL DEFAULT 0.0,
                hepatic_metabolism_pct REAL DEFAULT 0.0,
                max_daily_dose_mg REAL NOT NULL,
                is_controlled_uae BOOLEAN DEFAULT 0,
                schedule_law_30_2021 TEXT,
                narrow_therapeutic_index BOOLEAN DEFAULT 0,
                cyp3a4_profile TEXT DEFAULT 'NONE',
                cyp2d6_profile TEXT DEFAULT 'NONE',
                cyp2c9_profile TEXT DEFAULT 'NONE',
                cyp2c19_profile TEXT DEFAULT 'NONE',
                p_gp_profile TEXT DEFAULT 'NONE',
                qt_risk TEXT DEFAULT 'NONE',
                hyperkalemia_risk BOOLEAN DEFAULT 0,
                serotonin_risk BOOLEAN DEFAULT 0,
                indications_icd10_json TEXT NOT NULL,
                clinical_pearls TEXT,
                FOREIGN KEY(drug_class_id) REFERENCES drug_classes(class_id)
            );
            CREATE INDEX IF NOT EXISTS idx_formulary_canonical ON formulary_drugs(canonical_name);
            CREATE INDEX IF NOT EXISTS idx_formulary_arabic ON formulary_drugs(arabic_name);

            CREATE TABLE IF NOT EXISTS drug_interactions (
                interaction_id INTEGER PRIMARY KEY AUTOINCREMENT,
                drug_a_atc VARCHAR(10) NOT NULL,
                drug_b_atc VARCHAR(10) NOT NULL,
                alert_code VARCHAR(64) UNIQUE NOT NULL,
                severity_level VARCHAR(16) CHECK(severity_level IN ('CONTRAINDICATED', 'MAJOR', 'MODERATE')),
                mechanism_text TEXT NOT NULL,
                clinical_action TEXT NOT NULL,
                evidence_citation TEXT NOT NULL,
                drug_a_id INTEGER,
                drug_b_id INTEGER,
                class_a_id INTEGER,
                class_b_id INTEGER,
                severity TEXT,
                clinical_rationale TEXT,
                action_directive TEXT,
                evidence_doc_id TEXT,
                evidence_section TEXT,
                evidence_sha256 TEXT,
                mechanism TEXT,
                FOREIGN KEY(drug_a_atc) REFERENCES formulary_drugs(atc_code),
                FOREIGN KEY(drug_b_atc) REFERENCES formulary_drugs(atc_code)
            );
            CREATE INDEX IF NOT EXISTS idx_interactions_atc_pair ON drug_interactions(drug_a_atc, drug_b_atc);
            CREATE INDEX IF NOT EXISTS idx_interactions_alert ON drug_interactions(alert_code);

            CREATE TABLE IF NOT EXISTS medical_conditions (
                icd10_code VARCHAR(10) PRIMARY KEY,
                icd11_code VARCHAR(10),
                condition_name_en VARCHAR(256) NOT NULL,
                condition_name_ar VARCHAR(256) NOT NULL,
                category VARCHAR(64) NOT NULL,
                red_flag_priority BOOLEAN DEFAULT 0
            );
            CREATE INDEX IF NOT EXISTS idx_med_conditions_cat ON medical_conditions(category);
            CREATE INDEX IF NOT EXISTS idx_med_conditions_redflag ON medical_conditions(red_flag_priority);

            CREATE TABLE IF NOT EXISTS lab_diagnostic_panels (
                loinc_code VARCHAR(16) PRIMARY KEY,
                test_name VARCHAR(128) NOT NULL,
                specimen VARCHAR(32) NOT NULL,
                standard_unit VARCHAR(32) NOT NULL,
                reference_low REAL,
                reference_high REAL,
                critical_panic_low REAL,
                critical_panic_high REAL
            );
            CREATE INDEX IF NOT EXISTS idx_lab_panels_name ON lab_diagnostic_panels(test_name);

            CREATE TABLE IF NOT EXISTS hitl_clinical_quarantine (
                staging_id INTEGER PRIMARY KEY AUTOINCREMENT,
                source_url TEXT NOT NULL,
                extracted_finding TEXT NOT NULL,
                target_atc_code VARCHAR(10),
                proposed_interaction_json TEXT,
                evidence_pmid VARCHAR(16),
                discovered_at DATETIME DEFAULT CURRENT_TIMESTAMP,
                review_status TEXT DEFAULT 'PENDING',
                reviewer_notes TEXT,
                reviewed_at DATETIME
            );
            CREATE INDEX IF NOT EXISTS idx_quarantine_status ON hitl_clinical_quarantine(review_status);
            "#,
        )?;
        Ok(())
    }

    /// Fast indexed lookup by primary ATC code (<0.1 ms)
    pub fn get_drug_by_atc(&self, atc_code: &str) -> Result<Option<FormularyDrug>, ClinicalError> {
        let clean_atc = atc_code.trim();
        let conn = self.conn.lock().map_err(|e| ClinicalError::LockError(e.to_string()))?;
        let mut stmt = conn.prepare_cached(
            r#"SELECT atc_code, canonical_name, arabic_name, brand_names_json, drug_class_id,
                      renal_egfr_cutoff, hepatic_metabolism_pct, max_daily_dose_mg,
                      is_controlled_uae, schedule_law_30_2021, narrow_therapeutic_index,
                      cyp3a4_profile, cyp2d6_profile, cyp2c9_profile, cyp2c19_profile,
                      p_gp_profile, qt_risk, hyperkalemia_risk, serotonin_risk,
                      indications_icd10_json, clinical_pearls
               FROM formulary_drugs WHERE atc_code = ? LIMIT 1;"#,
        )?;

        let drug = stmt
            .query_row(params![clean_atc], |row| {
                Ok(FormularyDrug {
                    atc_code: row.get(0)?,
                    canonical_name: row.get(1)?,
                    arabic_name: row.get(2)?,
                    brand_names_json: row.get(3)?,
                    drug_class_id: row.get(4)?,
                    renal_egfr_cutoff: row.get(5)?,
                    hepatic_metabolism_pct: row.get(6)?,
                    max_daily_dose_mg: row.get(7)?,
                    is_controlled_uae: row.get(8)?,
                    schedule_law_30_2021: row.get(9)?,
                    narrow_therapeutic_index: row.get(10)?,
                    cyp3a4_profile: row.get(11)?,
                    cyp2d6_profile: row.get(12)?,
                    cyp2c9_profile: row.get(13)?,
                    cyp2c19_profile: row.get(14)?,
                    p_gp_profile: row.get(15)?,
                    qt_risk: row.get(16)?,
                    hyperkalemia_risk: row.get(17)?,
                    serotonin_risk: row.get(18)?,
                    indications_icd10_json: row.get(19)?,
                    clinical_pearls: row.get(20)?,
                })
            })
            .optional()?;

        Ok(drug)
    }

    /// Flexible multi-channel drug lookup: ATC code, canonical name, Arabic name, or brand name (<0.1 ms)
    pub fn lookup_drug(&self, query: &str) -> Result<Option<FormularyDrug>, ClinicalError> {
        let q = query.trim();
        if q.is_empty() {
            return Ok(None);
        }

        // 1. Direct ATC code match
        if let Some(drug) = self.get_drug_by_atc(q)? {
            return Ok(Some(drug));
        }

        // 2. Canonical or Arabic exact match
        let conn = self.conn.lock().map_err(|e| ClinicalError::LockError(e.to_string()))?;
        let mut stmt = conn.prepare_cached(
            r#"SELECT atc_code, canonical_name, arabic_name, brand_names_json, drug_class_id,
                      renal_egfr_cutoff, hepatic_metabolism_pct, max_daily_dose_mg,
                      is_controlled_uae, schedule_law_30_2021, narrow_therapeutic_index,
                      cyp3a4_profile, cyp2d6_profile, cyp2c9_profile, cyp2c19_profile,
                      p_gp_profile, qt_risk, hyperkalemia_risk, serotonin_risk,
                      indications_icd10_json, clinical_pearls
               FROM formulary_drugs
               WHERE canonical_name = ? COLLATE NOCASE
                  OR arabic_name = ? COLLATE NOCASE
               LIMIT 1;"#,
        )?;

        let drug = stmt
            .query_row(params![q, q], |row| {
                Ok(FormularyDrug {
                    atc_code: row.get(0)?,
                    canonical_name: row.get(1)?,
                    arabic_name: row.get(2)?,
                    brand_names_json: row.get(3)?,
                    drug_class_id: row.get(4)?,
                    renal_egfr_cutoff: row.get(5)?,
                    hepatic_metabolism_pct: row.get(6)?,
                    max_daily_dose_mg: row.get(7)?,
                    is_controlled_uae: row.get(8)?,
                    schedule_law_30_2021: row.get(9)?,
                    narrow_therapeutic_index: row.get(10)?,
                    cyp3a4_profile: row.get(11)?,
                    cyp2d6_profile: row.get(12)?,
                    cyp2c9_profile: row.get(13)?,
                    cyp2c19_profile: row.get(14)?,
                    p_gp_profile: row.get(15)?,
                    qt_risk: row.get(16)?,
                    hyperkalemia_risk: row.get(17)?,
                    serotonin_risk: row.get(18)?,
                    indications_icd10_json: row.get(19)?,
                    clinical_pearls: row.get(20)?,
                })
            })
            .optional()?;

        if drug.is_some() {
            return Ok(drug);
        }

        // 3. Normalized / stripped dosage query (e.g. "Ibuprofen 400mg" -> "Ibuprofen")
        let first_word = q.split_whitespace().next().unwrap_or(q);
        if first_word.len() >= 3 && first_word != q {
            let mut stmt2 = conn.prepare_cached(
                r#"SELECT atc_code, canonical_name, arabic_name, brand_names_json, drug_class_id,
                          renal_egfr_cutoff, hepatic_metabolism_pct, max_daily_dose_mg,
                          is_controlled_uae, schedule_law_30_2021, narrow_therapeutic_index,
                          cyp3a4_profile, cyp2d6_profile, cyp2c9_profile, cyp2c19_profile,
                          p_gp_profile, qt_risk, hyperkalemia_risk, serotonin_risk,
                          indications_icd10_json, clinical_pearls
                   FROM formulary_drugs
                   WHERE canonical_name = ? COLLATE NOCASE
                      OR arabic_name = ? COLLATE NOCASE
                   LIMIT 1;"#,
            )?;
            let drug_prefix = stmt2
                .query_row(params![first_word, first_word], |row| {
                    Ok(FormularyDrug {
                        atc_code: row.get(0)?,
                        canonical_name: row.get(1)?,
                        arabic_name: row.get(2)?,
                        brand_names_json: row.get(3)?,
                        drug_class_id: row.get(4)?,
                        renal_egfr_cutoff: row.get(5)?,
                        hepatic_metabolism_pct: row.get(6)?,
                        max_daily_dose_mg: row.get(7)?,
                        is_controlled_uae: row.get(8)?,
                        schedule_law_30_2021: row.get(9)?,
                        narrow_therapeutic_index: row.get(10)?,
                        cyp3a4_profile: row.get(11)?,
                        cyp2d6_profile: row.get(12)?,
                        cyp2c9_profile: row.get(13)?,
                        cyp2c19_profile: row.get(14)?,
                        p_gp_profile: row.get(15)?,
                        qt_risk: row.get(16)?,
                        hyperkalemia_risk: row.get(17)?,
                        serotonin_risk: row.get(18)?,
                        indications_icd10_json: row.get(19)?,
                        clinical_pearls: row.get(20)?,
                    })
                })
                .optional()?;

            if drug_prefix.is_some() {
                return Ok(drug_prefix);
            }
        }

        // 4. Search in brand_names_json (e.g. "Glucophage" -> Metformin, "Plavix" -> Clopidogrel)
        let pattern = format!("%\"{}\"%", q);
        let mut stmt3 = conn.prepare_cached(
            r#"SELECT atc_code, canonical_name, arabic_name, brand_names_json, drug_class_id,
                      renal_egfr_cutoff, hepatic_metabolism_pct, max_daily_dose_mg,
                      is_controlled_uae, schedule_law_30_2021, narrow_therapeutic_index,
                      cyp3a4_profile, cyp2d6_profile, cyp2c9_profile, cyp2c19_profile,
                      p_gp_profile, qt_risk, hyperkalemia_risk, serotonin_risk,
                      indications_icd10_json, clinical_pearls
               FROM formulary_drugs
               WHERE brand_names_json LIKE ? COLLATE NOCASE
               LIMIT 1;"#,
        )?;
        let brand_drug = stmt3
            .query_row(params![pattern], |row| {
                Ok(FormularyDrug {
                    atc_code: row.get(0)?,
                    canonical_name: row.get(1)?,
                    arabic_name: row.get(2)?,
                    brand_names_json: row.get(3)?,
                    drug_class_id: row.get(4)?,
                    renal_egfr_cutoff: row.get(5)?,
                    hepatic_metabolism_pct: row.get(6)?,
                    max_daily_dose_mg: row.get(7)?,
                    is_controlled_uae: row.get(8)?,
                    schedule_law_30_2021: row.get(9)?,
                    narrow_therapeutic_index: row.get(10)?,
                    cyp3a4_profile: row.get(11)?,
                    cyp2d6_profile: row.get(12)?,
                    cyp2c9_profile: row.get(13)?,
                    cyp2c19_profile: row.get(14)?,
                    p_gp_profile: row.get(15)?,
                    qt_risk: row.get(16)?,
                    hyperkalemia_risk: row.get(17)?,
                    serotonin_risk: row.get(18)?,
                    indications_icd10_json: row.get(19)?,
                    clinical_pearls: row.get(20)?,
                })
            })
            .optional()?;

        Ok(brand_drug)
    }

    /// Fast commutative lookup between two ATC codes (<0.1 ms)
    pub fn get_interaction(
        &self,
        drug_a_atc: &str,
        drug_b_atc: &str,
    ) -> Result<Option<DrugInteraction>, ClinicalError> {
        let atc_a = drug_a_atc.trim();
        let atc_b = drug_b_atc.trim();

        let conn = self.conn.lock().map_err(|e| ClinicalError::LockError(e.to_string()))?;
        let mut stmt = conn.prepare_cached(
            r#"SELECT interaction_id, drug_a_atc, drug_b_atc, alert_code,
                      severity_level, mechanism_text, clinical_action, evidence_citation
               FROM drug_interactions
               WHERE (drug_a_atc = ? AND drug_b_atc = ?)
                  OR (drug_a_atc = ? AND drug_b_atc = ?)
               LIMIT 1;"#,
        )?;

        let inter = stmt
            .query_row(params![atc_a, atc_b, atc_b, atc_a], |row| {
                Ok(DrugInteraction {
                    interaction_id: row.get(0)?,
                    drug_a_atc: row.get(1)?,
                    drug_b_atc: row.get(2)?,
                    alert_code: row.get(3)?,
                    severity_level: row.get(4)?,
                    mechanism_text: row.get(5)?,
                    clinical_action: row.get(6)?,
                    evidence_citation: row.get(7)?,
                })
            })
            .optional()?;

        Ok(inter)
    }

    /// Retrieve interaction rule directly by alert code
    pub fn get_interaction_by_alert_code(
        &self,
        alert_code: &str,
    ) -> Result<Option<DrugInteraction>, ClinicalError> {
        let code = alert_code.trim();
        let conn = self.conn.lock().map_err(|e| ClinicalError::LockError(e.to_string()))?;
        let mut stmt = conn.prepare_cached(
            r#"SELECT interaction_id, drug_a_atc, drug_b_atc, alert_code,
                      severity_level, mechanism_text, clinical_action, evidence_citation
               FROM drug_interactions
               WHERE alert_code = ? LIMIT 1;"#,
        )?;

        let inter = stmt
            .query_row(params![code], |row| {
                Ok(DrugInteraction {
                    interaction_id: row.get(0)?,
                    drug_a_atc: row.get(1)?,
                    drug_b_atc: row.get(2)?,
                    alert_code: row.get(3)?,
                    severity_level: row.get(4)?,
                    mechanism_text: row.get(5)?,
                    clinical_action: row.get(6)?,
                    evidence_citation: row.get(7)?,
                })
            })
            .optional()?;

        Ok(inter)
    }

    /// List all modeled interactions in the matrix
    pub fn list_all_interactions(&self) -> Result<Vec<DrugInteraction>, ClinicalError> {
        let conn = self.conn.lock().map_err(|e| ClinicalError::LockError(e.to_string()))?;
        let mut stmt = conn.prepare(
            r#"SELECT interaction_id, drug_a_atc, drug_b_atc, alert_code,
                      severity_level, mechanism_text, clinical_action, evidence_citation
               FROM drug_interactions
               ORDER BY interaction_id ASC;"#,
        )?;

        let rows = stmt.query_map([], |row| {
            Ok(DrugInteraction {
                interaction_id: row.get(0)?,
                drug_a_atc: row.get(1)?,
                drug_b_atc: row.get(2)?,
                alert_code: row.get(3)?,
                severity_level: row.get(4)?,
                mechanism_text: row.get(5)?,
                clinical_action: row.get(6)?,
                evidence_citation: row.get(7)?,
            })
        })?;

        let mut results = Vec::new();
        for r in rows {
            results.push(r?);
        }
        Ok(results)
    }

    /// Total count of modeled interaction rules
    pub fn count_interactions(&self) -> Result<usize, ClinicalError> {
        let conn = self.conn.lock().map_err(|e| ClinicalError::LockError(e.to_string()))?;
        let count: usize = conn.query_row("SELECT COUNT(*) FROM drug_interactions;", [], |r| r.get(0))?;
        Ok(count)
    }

    /// Total count of drugs in generalized formulary
    pub fn count_formulary_drugs(&self) -> Result<usize, ClinicalError> {
        let conn = self.conn.lock().map_err(|e| ClinicalError::LockError(e.to_string()))?;
        let count: usize = conn.query_row("SELECT COUNT(*) FROM formulary_drugs;", [], |r| r.get(0))?;
        Ok(count)
    }

    /// Retrieve medical condition by ICD-10 code
    pub fn get_medical_condition(&self, icd10: &str) -> Result<Option<MedicalCondition>, ClinicalError> {
        let code = icd10.trim();
        let conn = self.conn.lock().map_err(|e| ClinicalError::LockError(e.to_string()))?;
        let mut stmt = conn.prepare_cached(
            r#"SELECT icd10_code, icd11_code, condition_name_en, condition_name_ar,
                      category, red_flag_priority
               FROM medical_conditions WHERE icd10_code = ? LIMIT 1;"#,
        )?;

        let cond = stmt
            .query_row(params![code], |row| {
                Ok(MedicalCondition {
                    icd10_code: row.get(0)?,
                    icd11_code: row.get(1)?,
                    condition_name_en: row.get(2)?,
                    condition_name_ar: row.get(3)?,
                    category: row.get(4)?,
                    red_flag_priority: row.get(5)?,
                })
            })
            .optional()?;

        Ok(cond)
    }

    /// Retrieve diagnostic lab panel by LOINC code
    pub fn get_lab_panel(&self, loinc: &str) -> Result<Option<LabDiagnosticPanel>, ClinicalError> {
        let code = loinc.trim();
        let conn = self.conn.lock().map_err(|e| ClinicalError::LockError(e.to_string()))?;
        let mut stmt = conn.prepare_cached(
            r#"SELECT loinc_code, test_name, specimen, standard_unit,
                      reference_low, reference_high, critical_panic_low, critical_panic_high
               FROM lab_diagnostic_panels WHERE loinc_code = ? LIMIT 1;"#,
        )?;

        let panel = stmt
            .query_row(params![code], |row| {
                Ok(LabDiagnosticPanel {
                    loinc_code: row.get(0)?,
                    test_name: row.get(1)?,
                    specimen: row.get(2)?,
                    standard_unit: row.get(3)?,
                    reference_low: row.get(4)?,
                    reference_high: row.get(5)?,
                    critical_panic_low: row.get(6)?,
                    critical_panic_high: row.get(7)?,
                })
            })
            .optional()?;

        Ok(panel)
    }

    /// Insert new finding into Human-in-the-Loop quarantine buffer
    pub fn insert_hitl_finding(
        &self,
        source_url: &str,
        extracted_finding: &str,
        target_atc: Option<&str>,
        proposed_interaction_json: Option<&str>,
        evidence_pmid: Option<&str>,
    ) -> Result<i64, ClinicalError> {
        let conn = self.conn.lock().map_err(|e| ClinicalError::LockError(e.to_string()))?;
        conn.execute(
            r#"INSERT INTO hitl_clinical_quarantine (
                   source_url, extracted_finding, target_atc_code,
                   proposed_interaction_json, evidence_pmid, review_status
               ) VALUES (?, ?, ?, ?, ?, 'PENDING');"#,
            params![
                source_url,
                extracted_finding,
                target_atc,
                proposed_interaction_json,
                evidence_pmid
            ],
        )?;
        Ok(conn.last_insert_rowid())
    }

    /// Query HITL quarantine record
    pub fn get_hitl_record(&self, staging_id: i64) -> Result<Option<HitlQuarantineRecord>, ClinicalError> {
        let conn = self.conn.lock().map_err(|e| ClinicalError::LockError(e.to_string()))?;
        let mut stmt = conn.prepare_cached(
            r#"SELECT staging_id, source_url, extracted_finding, target_atc_code,
                      proposed_interaction_json, evidence_pmid, discovered_at,
                      review_status, reviewer_notes, reviewed_at
               FROM hitl_clinical_quarantine WHERE staging_id = ? LIMIT 1;"#,
        )?;

        let rec = stmt
            .query_row(params![staging_id], |row| {
                Ok(HitlQuarantineRecord {
                    staging_id: row.get(0)?,
                    source_url: row.get(1)?,
                    extracted_finding: row.get(2)?,
                    target_atc_code: row.get(3)?,
                    proposed_interaction_json: row.get(4)?,
                    evidence_pmid: row.get(5)?,
                    discovered_at: row.get(6)?,
                    review_status: row.get(7)?,
                    reviewer_notes: row.get(8)?,
                    reviewed_at: row.get(9)?,
                })
            })
            .optional()?;

        Ok(rec)
    }

    /// Helper to determine if a combination is an explicitly verified safe co-prescription
    fn is_explicitly_safe_pair(&self, atc_a: &str, atc_b: &str) -> bool {
        let safe_pairs: [(&str, &str); 4] = [
            ("A10BA02", "C10AA01"), // Metformin + Simvastatin
            ("N02BE01", "J01CA04"), // Paracetamol + Amoxicillin
            ("N02BE01", "C09AA03"), // Paracetamol + Lisinopril
            ("A10BA02", "A10BH01"), // Metformin + Sitagliptin
        ];

        for (sa, sb) in &safe_pairs {
            if (atc_a == *sa && atc_b == *sb) || (atc_a == *sb && atc_b == *sa) {
                return true;
            }
        }
        false
    }

    /// Screen an individual pair of drugs with fail-closed safety gate
    pub fn check_interaction_pair(
        &self,
        drug_a_query: &str,
        drug_b_query: &str,
    ) -> Result<InteractionCheckResult, ClinicalError> {
        // Step 1: Resolve Drug A
        let drug_a = match self.lookup_drug(drug_a_query)? {
            Some(d) => d,
            None => {
                return Ok(InteractionCheckResult::UnverifiedDrug {
                    drug: drug_a_query.to_string(),
                    reason: format!(
                        "Medication '{}' is not modeled in the verified formulary; requires clinical pharmacist review",
                        drug_a_query
                    ),
                });
            }
        };

        // Step 2: Resolve Drug B
        let drug_b = match self.lookup_drug(drug_b_query)? {
            Some(d) => d,
            None => {
                return Ok(InteractionCheckResult::UnverifiedDrug {
                    drug: drug_b_query.to_string(),
                    reason: format!(
                        "Medication '{}' is not modeled in the verified formulary; requires clinical pharmacist review",
                        drug_b_query
                    ),
                });
            }
        };

        // Step 3: Query Interaction Matrix
        if let Some(interaction) = self.get_interaction(&drug_a.atc_code, &drug_b.atc_code)? {
            return Ok(InteractionCheckResult::InteractionFound(interaction));
        }

        // Step 4: Check if explicitly verified safe combination
        if self.is_explicitly_safe_pair(&drug_a.atc_code, &drug_b.atc_code) {
            return Ok(InteractionCheckResult::VerifiedSafe);
        }

        // Step 5: Fail-closed gate: unmodeled combination
        let reason = format!(
            "Drug combination ('{}' + '{}') is not explicitly modeled in the sovereign interaction matrix; requires clinical pharmacist review",
            drug_a.canonical_name, drug_b.canonical_name
        );
        Ok(InteractionCheckResult::UnverifiedCombination {
            drug_a: drug_a.canonical_name,
            drug_b: drug_b.canonical_name,
            reason,
        })
    }

    /// Dynamic pair interaction checker across active multi-drug patient profiles
    /// Evaluates all unordered pairs across patient medications with fail-closed verification.
    pub fn screen_patient_profile(
        &self,
        drugs: &[&str],
    ) -> Result<ProfileScreeningResult, ClinicalError> {
        let mut resolved_drugs = Vec::new();
        let mut unverified_drugs = Vec::new();
        let mut seen_atcs = HashSet::new();

        // 1. Resolve all input drugs
        for &dq in drugs {
            let trimmed = dq.trim();
            if trimmed.is_empty() {
                continue;
            }
            match self.lookup_drug(trimmed)? {
                Some(drug) => {
                    if seen_atcs.insert(drug.atc_code.clone()) {
                        resolved_drugs.push(drug);
                    }
                }
                None => {
                    unverified_drugs.push(trimmed.to_string());
                }
            }
        }

        // Fail-closed gate: any unmodeled drug immediately flags UNVERIFIED_DRUG_REQUIRES_PHARMACIST_REVIEW
        if !unverified_drugs.is_empty() {
            let primary_unverified = unverified_drugs[0].clone();
            return Ok(ProfileScreeningResult {
                is_safe: false,
                verdict: InteractionCheckVerdict::UnverifiedReview,
                primary_alert_code: UNVERIFIED_DRUG_REQUIRES_PHARMACIST_REVIEW.to_string(),
                primary_rationale: format!(
                    "Fail-Closed Gate: Medication '{}' is not modeled in the verified sovereign clinical database; requires clinical pharmacist review.",
                    primary_unverified
                ),
                interactions_detected: Vec::new(),
                unverified_drugs,
                unverified_combinations: Vec::new(),
                resolved_drugs,
            });
        }

        // Single drug regimen (or empty) has no pairwise interactions
        if resolved_drugs.len() <= 1 {
            return Ok(ProfileScreeningResult {
                is_safe: true,
                verdict: InteractionCheckVerdict::Safe,
                primary_alert_code: DHA_NABIDH_SAFE.to_string(),
                primary_rationale: "Medication regimen verified safe; no pairwise interactions detected.".to_string(),
                interactions_detected: Vec::new(),
                unverified_drugs: Vec::new(),
                unverified_combinations: Vec::new(),
                resolved_drugs,
            });
        }

        // 2. Screen all unique pairwise combinations
        let mut interactions_detected = Vec::new();
        let mut unverified_combinations = Vec::new();

        let n = resolved_drugs.len();
        for i in 0..n {
            for j in (i + 1)..n {
                let da = &resolved_drugs[i];
                let db = &resolved_drugs[j];

                if let Some(inter) = self.get_interaction(&da.atc_code, &db.atc_code)? {
                    interactions_detected.push(inter);
                } else if !self.is_explicitly_safe_pair(&da.atc_code, &db.atc_code) {
                    unverified_combinations.push((da.canonical_name.clone(), db.canonical_name.clone()));
                }
            }
        }

        // 3. Compute hierarchical verdict
        if !unverified_combinations.is_empty() {
            let (first_a, first_b) = &unverified_combinations[0];
            return Ok(ProfileScreeningResult {
                is_safe: false,
                verdict: InteractionCheckVerdict::UnverifiedReview,
                primary_alert_code: UNVERIFIED_DRUG_REQUIRES_PHARMACIST_REVIEW.to_string(),
                primary_rationale: format!(
                    "Fail-Closed Gate: Combination ('{}' + '{}') is not explicitly verified in the sovereign interaction matrix; requires clinical pharmacist review.",
                    first_a, first_b
                ),
                interactions_detected,
                unverified_drugs: Vec::new(),
                unverified_combinations,
                resolved_drugs,
            });
        }

        if let Some(contra) = interactions_detected.iter().find(|i| i.is_contraindicated()) {
            return Ok(ProfileScreeningResult {
                is_safe: false,
                verdict: InteractionCheckVerdict::ContraindicatedBlock,
                primary_alert_code: contra.alert_code.clone(),
                primary_rationale: contra.clinical_action.clone(),
                interactions_detected,
                unverified_drugs: Vec::new(),
                unverified_combinations: Vec::new(),
                resolved_drugs,
            });
        }

        if let Some(major) = interactions_detected.iter().find(|i| i.severity() == InteractionSeverity::Major) {
            return Ok(ProfileScreeningResult {
                is_safe: false,
                verdict: InteractionCheckVerdict::MajorWarning,
                primary_alert_code: major.alert_code.clone(),
                primary_rationale: major.clinical_action.clone(),
                interactions_detected,
                unverified_drugs: Vec::new(),
                unverified_combinations: Vec::new(),
                resolved_drugs,
            });
        }

        if let Some(mod_inter) = interactions_detected.first() {
            return Ok(ProfileScreeningResult {
                is_safe: true,
                verdict: InteractionCheckVerdict::ModerateWarning,
                primary_alert_code: mod_inter.alert_code.clone(),
                primary_rationale: mod_inter.clinical_action.clone(),
                interactions_detected,
                unverified_drugs: Vec::new(),
                unverified_combinations: Vec::new(),
                resolved_drugs,
            });
        }

        Ok(ProfileScreeningResult {
            is_safe: true,
            verdict: InteractionCheckVerdict::Safe,
            primary_alert_code: DHA_NABIDH_SAFE.to_string(),
            primary_rationale: "All pairwise medication combinations verified safe and compatible.".to_string(),
            interactions_detected: Vec::new(),
            unverified_drugs: Vec::new(),
            unverified_combinations: Vec::new(),
            resolved_drugs,
        })
    }
}
