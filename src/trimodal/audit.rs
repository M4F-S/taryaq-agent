//! Cryptographic HMAC-SHA256 Append-Only Audit Ledger
//! Dubai Create AI Agents Championship 2026–2027

use hmac::{Hmac, KeyInit, Mac};
use serde::{Deserialize, Serialize};
use sha2::{Digest, Sha256};
use std::fs::{File, OpenOptions};
use std::io::{BufRead, BufReader, Write};
use std::os::unix::fs::OpenOptionsExt;
use std::os::unix::io::AsRawFd;
use std::path::{Path, PathBuf};
use std::time::{SystemTime, UNIX_EPOCH};
use thiserror::Error;

type HmacSha256 = Hmac<Sha256>;

pub const DEFAULT_AUDIT_LOG: &str = "taryaq_audit.jsonl";
pub const GENESIS_PREV_MAC: &str = "0000000000000000000000000000000000000000000000000000000000000000";

#[derive(Error, Debug)]
pub enum AuditError {
    #[error("I/O error: {0}")]
    Io(#[from] std::io::Error),
    #[error("Audit secret key error: {0}")]
    KeyError(String),
    #[error("Audit chain corrupted: {0}")]
    ChainCorruption(String),
    #[error("Serialization error: {0}")]
    Serialization(#[from] serde_json::Error),
    #[error("Lock error: {0}")]
    LockError(String),
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct AuditEntryInput {
    pub action: String, // "PRESCRIPTION_COMMITTED" or "PRESCRIPTION_ROLLED_BACK"
    pub patient_id: String,
    pub date: String,
    pub dose: String,
    pub primary_drug: String,
    pub concurrent_drug: Option<String>,
    pub patient_egfr: f64,
    pub verdict: String,
    pub alert_code: String,
    pub override_status: String, // "APPROVED" or "NONE"
    pub override_token: Option<String>,
    pub override_rationale: Option<String>,
    pub is_repeat_override: bool,
    pub statutory_doc_id: Option<String>,
    pub statutory_section: Option<String>,
    pub statutory_passage_sha256: Option<String>,
    pub claim_net_aed: f64,
    pub claim_delay_days: u32,
    pub penalty_aed: f64,
    pub reason: Option<String>, // For PRESCRIPTION_ROLLED_BACK
}

impl Default for AuditEntryInput {
    fn default() -> Self {
        Self {
            action: "PRESCRIPTION_COMMITTED".to_string(),
            patient_id: "DXB-PAT-001".to_string(),
            date: current_date_utc(),
            dose: "Standard Dose".to_string(),
            primary_drug: String::new(),
            concurrent_drug: None,
            patient_egfr: 90.0,
            verdict: "CLINICAL_SAFE".to_string(),
            alert_code: "DHA_NABIDH_SAFE".to_string(),
            override_status: "NONE".to_string(),
            override_token: None,
            override_rationale: None,
            is_repeat_override: false,
            statutory_doc_id: Some("DHA_SN_03_2019".to_string()),
            statutory_section: Some("§3.7.1".to_string()),
            statutory_passage_sha256: None,
            claim_net_aed: 0.0,
            claim_delay_days: 0,
            penalty_aed: 0.0,
            reason: None,
        }
    }
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct AuditRecord {
    pub timestamp_utc: u64,
    pub reporting_tz: String,
    pub agent: String,
    pub action: String,
    pub patient_id: String,
    pub date: String,
    pub dose: String,
    pub primary_drug: String,
    pub concurrent_drug: String,
    pub patient_egfr: f64,
    pub verdict: String,
    pub alert_code: String,
    pub override_status: String,
    pub override_token: String,
    pub override_rationale: String,
    pub is_repeat_override: bool,
    pub statutory_doc_id: String,
    pub statutory_section: String,
    pub statutory_passage_sha256: String,
    pub claim_net_aed: f64,
    pub claim_delay_days: u32,
    pub penalty_aed: f64,
    pub canonical: String,
    pub prev_sha256: String,
    pub mac: String,
    pub sha256_signature: String,
    pub state: String,
}

pub struct AuditLedger {
    file_path: PathBuf,
}

impl AuditLedger {
    pub fn new(path: impl AsRef<Path>) -> Self {
        Self {
            file_path: path.as_ref().to_path_buf(),
        }
    }

    pub fn default_ledger() -> Self {
        let p = std::env::var("AUDIT_FILE").unwrap_or_else(|_| DEFAULT_AUDIT_LOG.to_string());
        Self::new(p)
    }

    /// Read the previous record's MAC from file under lock
    fn get_last_mac(&self, file: &File) -> Result<String, AuditError> {
        let reader = BufReader::new(file);
        let mut last_mac = None;

        for line in reader.lines() {
            let l = line?;
            let trimmed = l.trim();
            if trimmed.is_empty() {
                continue;
            }
            if let Ok(val) = serde_json::from_str::<serde_json::Value>(trimmed) {
                if let Some(m) = val.get("mac").and_then(|v| v.as_str()) {
                    if m.len() == 64 {
                        last_mac = Some(m.to_string());
                    }
                }
            }
        }

        Ok(last_mac.unwrap_or_else(|| GENESIS_PREV_MAC.to_string()))
    }

    /// Append an audit entry ensuring strict HMAC-SHA256 chaining and posix flock
    pub fn append_record(&self, input: &AuditEntryInput, audit_key: Option<&str>) -> Result<AuditRecord, AuditError> {
        let key_str = match audit_key {
            Some(k) => k.to_string(),
            None => std::env::var("TARYAQ_AUDIT_KEY")
                .unwrap_or_else(|_| "taryaq-default-audit-key-32-bytes-min!!".to_string()),
        };

        if key_str.len() < 32 {
            return Err(AuditError::KeyError(
                "TARYAQ_AUDIT_KEY must be at least 32 bytes for HMAC-SHA256".to_string(),
            ));
        }

        let mut file = OpenOptions::new()
            .read(true)
            .write(true)
            .create(true)
            .mode(0o600)
            .open(&self.file_path)?;

        let fd = file.as_raw_fd();
        let lock_res = unsafe { libc::flock(fd, libc::LOCK_EX) };
        if lock_res != 0 {
            return Err(AuditError::LockError("Failed to acquire flock on audit ledger".to_string()));
        }

        // Defer unlock
        struct FlockGuard(i32);
        impl Drop for FlockGuard {
            fn drop(&mut self) {
                unsafe {
                    libc::flock(self.0, libc::LOCK_UN);
                }
            }
        }
        let _guard = FlockGuard(fd);

        let prev_mac = self.get_last_mac(&file)?;

        let now_utc = SystemTime::now()
            .duration_since(UNIX_EPOCH)
            .unwrap_or_default()
            .as_secs();

        let primary_drug = if input.primary_drug.trim().is_empty() {
            "NONE".to_string()
        } else {
            input.primary_drug.trim().to_string()
        };

        let concurrent_drug = input
            .concurrent_drug
            .as_deref()
            .filter(|s| !s.trim().is_empty())
            .unwrap_or("NONE")
            .to_string();

        let override_tok = input
            .override_token
            .as_deref()
            .filter(|s| !s.trim().is_empty())
            .unwrap_or("NONE")
            .to_string();

        let override_rat = input
            .override_rationale
            .as_deref()
            .filter(|s| !s.trim().is_empty())
            .unwrap_or("NONE")
            .to_string();

        let doc_id = input
            .statutory_doc_id
            .as_deref()
            .filter(|s| !s.trim().is_empty())
            .unwrap_or("DHA_SN_03_2019")
            .to_string();

        let section = input
            .statutory_section
            .as_deref()
            .filter(|s| !s.trim().is_empty())
            .unwrap_or("§3.7.1")
            .to_string();

        let passage_sha = input
            .statutory_passage_sha256
            .as_deref()
            .filter(|s| !s.trim().is_empty())
            .map(|s| s.to_string());

        // Construct canonical string
        let canonical = if input.action == "PRESCRIPTION_ROLLED_BACK" {
            let reason = input.reason.as_deref().unwrap_or("Operator abort or unapproved block");
            format!(
                "{}|Asia/Dubai|Taryaq v0|PRESCRIPTION_ROLLED_BACK|{}|{}|{:.1}|{}",
                now_utc, primary_drug, concurrent_drug, input.patient_egfr, reason
            )
        } else if let Some(ref hsh) = passage_sha {
            format!(
                "{}|Asia/Dubai|Taryaq v0|PRESCRIPTION_COMMITTED|{}|{}|{:.1}|{}|{}|{}|{:.2}|{}|{:.2}|{}|{}|{}",
                now_utc,
                primary_drug,
                concurrent_drug,
                input.patient_egfr,
                input.alert_code,
                override_tok,
                override_rat,
                input.claim_net_aed,
                input.claim_delay_days,
                input.penalty_aed,
                doc_id,
                section,
                hsh
            )
        } else {
            format!(
                "{}|Asia/Dubai|Taryaq v0|PRESCRIPTION_COMMITTED|{}|{}|{:.1}|{}|{}|{}|{:.2}|{}|{:.2}",
                now_utc,
                primary_drug,
                concurrent_drug,
                input.patient_egfr,
                input.alert_code,
                override_tok,
                override_rat,
                input.claim_net_aed,
                input.claim_delay_days,
                input.penalty_aed
            )
        };

        // Compute legacy SHA-256
        let mut hasher = Sha256::new();
        hasher.update(canonical.as_bytes());
        let sha256_signature = hex::encode(hasher.finalize());

        // Compute HMAC-SHA256 signature
        let hmac_msg = format!("{}\n{}", prev_mac, canonical);
        let mut mac_hasher = HmacSha256::new_from_slice(key_str.as_bytes())
            .map_err(|e| AuditError::KeyError(e.to_string()))?;
        mac_hasher.update(hmac_msg.as_bytes());
        let mac = hex::encode(mac_hasher.finalize().into_bytes());

        let record = AuditRecord {
            timestamp_utc: now_utc,
            reporting_tz: "Asia/Dubai".to_string(),
            agent: "Taryaq v0".to_string(),
            action: input.action.clone(),
            patient_id: input.patient_id.clone(),
            date: input.date.clone(),
            dose: input.dose.clone(),
            primary_drug,
            concurrent_drug,
            patient_egfr: input.patient_egfr,
            verdict: input.verdict.clone(),
            alert_code: input.alert_code.clone(),
            override_status: input.override_status.clone(),
            override_token: override_tok,
            override_rationale: override_rat,
            is_repeat_override: input.is_repeat_override,
            statutory_doc_id: doc_id,
            statutory_section: section,
            statutory_passage_sha256: passage_sha.unwrap_or_default(),
            claim_net_aed: input.claim_net_aed,
            claim_delay_days: input.claim_delay_days,
            penalty_aed: input.penalty_aed,
            canonical,
            prev_sha256: prev_mac,
            mac,
            sha256_signature,
            state: if input.action == "PRESCRIPTION_ROLLED_BACK" {
                "ROLLED_BACK".to_string()
            } else {
                "COMMITTED".to_string()
            },
        };

        // Write line
        let line = serde_json::to_string(&record)? + "\n";
        use std::io::Seek;
        file.seek(std::io::SeekFrom::End(0))?;
        file.write_all(line.as_bytes())?;
        file.flush()?;

        Ok(record)
    }
}

pub fn current_date_utc() -> String {
    let now = SystemTime::now()
        .duration_since(UNIX_EPOCH)
        .unwrap_or_default()
        .as_secs();
    let days = now / 86400;
    // Approximating date calculation from epoch days:
    // 1970-01-01 + days
    let mut y = 1970;
    let mut d = days as i64;
    loop {
        let leap = if y % 4 == 0 && (y % 100 != 0 || y % 400 == 0) { 1 } else { 0 };
        let days_in_year = 365 + leap;
        if d < days_in_year {
            let mut m = 1;
            let month_days = [31, 28 + leap, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31];
            for &md in &month_days {
                if d < md {
                    return format!("{:04}-{:02}-{:02}", y, m, d + 1);
                }
                d -= md;
                m += 1;
            }
            return format!("{:04}-12-31", y);
        }
        d -= days_in_year;
        y += 1;
    }
}
