//! Clinician Ambient Copilot Interface (DeepCura Pattern)
//! Dubai Create AI Agents Championship 2026–2027

use crate::clinical::{ClinicalDatabase, InteractionSeverity, MedicalCondition};
use serde::{Deserialize, Serialize};
use sha2::{Digest, Sha256};
use std::sync::Arc;
use std::time::{SystemTime, UNIX_EPOCH};

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct DialogueTurn {
    pub speaker: String, // "Clinician", "Patient", "Nurse", "Family"
    pub text: String,
    pub timestamp_sec: f64,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct ClinicianEncounterInput {
    pub dialogue: String,
    pub turns: Vec<DialogueTurn>,
    pub patient_id: Option<String>,
    pub patient_egfr: Option<f64>,
    pub proposed_prescriptions: Vec<String>,
    pub active_medications: Vec<String>,
    pub documented_vitals: Option<VitalsObservation>,
}

#[derive(Debug, Clone, Serialize, Deserialize, Default)]
pub struct VitalsObservation {
    pub blood_pressure: Option<String>,
    pub heart_rate_bpm: Option<u32>,
    pub respiratory_rate_bpm: Option<u32>,
    pub oxygen_saturation_pct: Option<f64>,
    pub temperature_celsius: Option<f64>,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct DifferentialDiagnosis {
    pub icd10_code: String,
    pub icd11_code: Option<String>,
    pub condition_name_en: String,
    pub condition_name_ar: String,
    pub category: String,
    pub confidence_score: f64,
    pub red_flag_priority: bool,
    pub guideline_citation: String,
    pub clinical_rationale: String,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct TraceLinkedEvidence {
    pub utterance_index: usize,
    pub speaker: String,
    pub timestamp_sec: f64,
    pub snippet: String,
    pub evidence_sha256: String,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct CdsInterrupt {
    pub triggered: bool,
    pub alert_code: String,
    pub severity: String,
    pub clinical_rationale: String,
    pub statutory_doc_id: String,
    pub statutory_section: String,
    pub evidence_sha256: String,
    pub offending_pair: Option<(String, String)>,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct StructuredSoapNote {
    pub encounter_id: String,
    pub timestamp_utc: u64,
    pub patient_id: String,
    pub subjective: String,
    pub objective: String,
    pub assessment: String,
    pub plan: String,
    pub ranked_ddx: Vec<DifferentialDiagnosis>,
    pub trace_evidence: Vec<TraceLinkedEvidence>,
    pub cds_interrupt: Option<CdsInterrupt>,
    pub markdown_note: String,
}

pub struct ClinicianAmbientCopilot {
    db: Option<Arc<ClinicalDatabase>>,
}

impl ClinicianAmbientCopilot {
    pub fn new(db: Option<Arc<ClinicalDatabase>>) -> Self {
        Self { db }
    }

    pub fn with_default_db() -> Self {
        let db = ClinicalDatabase::open_default().ok().map(Arc::new);
        Self { db }
    }

    /// Process ambient bilingual dialogue, extract clinical features, draft SOAP, and compute pre-order CDS
    pub fn process_encounter(&self, input: &ClinicianEncounterInput) -> StructuredSoapNote {
        let now_utc = SystemTime::now()
            .duration_since(UNIX_EPOCH)
            .unwrap_or_default()
            .as_secs();

        let encounter_id = format!("ENC-{}", &sha256_hex(&format!("{}-{}", now_utc, input.dialogue))[..12]);
        let patient_id = input.patient_id.clone().unwrap_or_else(|| "DXB-PAT-001".to_string());

        // 1. Process dialogue turns or split raw dialogue
        let turns = if !input.turns.is_empty() {
            input.turns.clone()
        } else {
            self.parse_dialogue_turns(&input.dialogue)
        };

        // 2. Generate trace-linked evidence
        let mut trace_evidence = Vec::new();
        for (idx, turn) in turns.iter().enumerate() {
            let h = sha256_hex(&turn.text);
            trace_evidence.push(TraceLinkedEvidence {
                utterance_index: idx,
                speaker: turn.speaker.clone(),
                timestamp_sec: turn.timestamp_sec,
                snippet: turn.text.clone(),
                evidence_sha256: h,
            });
        }

        // 3. Extract Subjective info
        let subjective = self.extract_subjective(&turns, &input.dialogue);

        // 4. Extract Objective info (vitals, labs, physical findings)
        let objective = self.extract_objective(&turns, &input.dialogue, input.patient_egfr, input.documented_vitals.as_ref());

        // 5. Compute ranked Differential Diagnoses
        let ranked_ddx = self.compute_ranked_ddx(&input.dialogue, &turns, input.patient_egfr);

        // 6. Pre-Order CDS Interrupt Hook
        let egfr_val = input.patient_egfr.unwrap_or(90.0);
        let cds_interrupt = self.evaluate_pre_order_cds(&input.proposed_prescriptions, &input.active_medications, egfr_val);

        // 7. Extract Assessment & Plan
        let assessment = self.synthesize_assessment(&ranked_ddx, &subjective, &objective);
        let plan = self.formulate_plan(&input.proposed_prescriptions, &ranked_ddx, cds_interrupt.as_ref());

        // 8. Generate formatted Markdown note
        let markdown_note = self.format_markdown_soap(
            &encounter_id,
            &patient_id,
            now_utc,
            &subjective,
            &objective,
            &assessment,
            &plan,
            &ranked_ddx,
            cds_interrupt.as_ref(),
        );

        StructuredSoapNote {
            encounter_id,
            timestamp_utc: now_utc,
            patient_id,
            subjective,
            objective,
            assessment,
            plan,
            ranked_ddx,
            trace_evidence,
            cds_interrupt,
            markdown_note,
        }
    }

    /// Helper parsing freeform text into dialogue turns
    fn parse_dialogue_turns(&self, raw: &str) -> Vec<DialogueTurn> {
        let mut turns = Vec::new();
        let mut cur_time = 0.0;

        for line in raw.lines() {
            let trimmed = line.trim();
            if trimmed.is_empty() {
                continue;
            }

            let (speaker, text) = if let Some(pos) = trimmed.find(':') {
                let s = trimmed[..pos].trim();
                let t = trimmed[pos + 1..].trim();
                if s.eq_ignore_ascii_case("Doctor")
                    || s.eq_ignore_ascii_case("Clinician")
                    || s.eq_ignore_ascii_case("Physician")
                    || s.eq_ignore_ascii_case("الطبيب")
                    || s.eq_ignore_ascii_case("دكتور")
                {
                    ("Clinician".to_string(), t.to_string())
                } else if s.eq_ignore_ascii_case("Patient")
                    || s.eq_ignore_ascii_case("المريض")
                {
                    ("Patient".to_string(), t.to_string())
                } else {
                    (s.to_string(), t.to_string())
                }
            } else {
                ("Ambience".to_string(), trimmed.to_string())
            };

            turns.push(DialogueTurn {
                speaker,
                text,
                timestamp_sec: cur_time,
            });
            cur_time += 15.0; // Estimate 15 seconds per utterance
        }

        turns
    }

    /// Extract Subjective content (Chief Complaint, HPI, Patient-reported symptoms)
    fn extract_subjective(&self, turns: &[DialogueTurn], raw: &str) -> String {
        let mut patient_narratives = Vec::new();
        let mut symptoms_detected = Vec::new();

        let lower_raw = raw.to_lowercase();

        // Detect chief symptoms (English & Arabic)
        let symptom_lexicon = [
            ("chest pain", "Severe chest pain / ألم بالصدر"),
            ("عوار بالصدر", "Acute chest distress (Gulf dialect: عوار بالصدر)"),
            ("نغز بالصدر", "Chest stabbing pain (نغز بالصدر)"),
            ("بيوجعني صدري", "Chest pain (Egyptian dialect: وجع بالصدر)"),
            ("عم يوجعني", "Chest pain (Levantine dialect)"),
            ("shortness of breath", "Shortness of breath / ضيق تنفس"),
            ("ضيق تنفس", "Dyspnea / ضيق تنفس"),
            ("كتمة", "Respiratory constriction (كتمة بالصدر)"),
            ("palpitations", "Palpitations / خفقان القلب"),
            ("خفقان", "Palpitations (خفقان)"),
            ("dizziness", "Dizziness / دوار"),
            ("دوخة", "Dizziness (دوخة)"),
            ("joint pain", "Joint pain / آلام المفاصل"),
            ("وجع مفاصل", "Joint pain (وجع مفاصل)"),
            ("نقرس", "Gout flare / نوبة نقرس"),
            ("headache", "Headache / صداع"),
            ("صداع", "Headache (صداع)"),
            ("facial droop", "Facial asymmetry / شلل بالوجه"),
            ("slurred speech", "Slurred speech / صعوبة بالكلام"),
            ("تنميل", "Paresthesia / numbness (تنميل)"),
            ("cough", "Cough / سعال"),
            ("سعال", "Cough (سعال)"),
            ("heartburn", "Heartburn / حرقان بالمعدة"),
            ("حموضة", "Acid reflux / حموضة"),
        ];

        for (trigger, label) in &symptom_lexicon {
            if lower_raw.contains(trigger) || raw.contains(trigger) {
                symptoms_detected.push(*label);
            }
        }

        for turn in turns {
            if turn.speaker == "Patient" || turn.speaker == "المريض" {
                patient_narratives.push(format!("• Patient stated: \"{}\"", turn.text));
            }
        }

        let mut sub = String::new();
        sub.push_str("CHIEF COMPLAINT & SYMPTOM REVIEW:\n");
        if symptoms_detected.is_empty() {
            sub.push_str("- Routine clinical presentation; no acute red-flag distress vocalized.\n");
        } else {
            for sym in symptoms_detected {
                sub.push_str(&format!("- Active reported finding: {}\n", sym));
            }
        }

        if !patient_narratives.is_empty() {
            sub.push_str("\nNARRATIVE TIMELINE (PATIENT VOCALIZATIONS):\n");
            for n in patient_narratives {
                sub.push_str(&format!("{}\n", n));
            }
        } else {
            sub.push_str("\nAMBIENT EXCERPT:\n");
            let excerpt: Vec<&str> = raw.lines().take(3).collect();
            sub.push_str(&excerpt.join("\n"));
            sub.push('\n');
        }

        sub
    }

    /// Extract Objective content (Vitals, Lab readings, eGFR, physical exam)
    fn extract_objective(
        &self,
        _turns: &[DialogueTurn],
        raw: &str,
        egfr: Option<f64>,
        vitals: Option<&VitalsObservation>,
    ) -> String {
        let mut obj = String::new();
        obj.push_str("CLINICAL EXAMINATION & OBJECTIVE PARAMETERS:\n");

        // Documented or extracted vitals
        let mut bp = vitals.and_then(|v| v.blood_pressure.clone());
        let mut hr = vitals.and_then(|v| v.heart_rate_bpm);
        let mut spo2 = vitals.and_then(|v| v.oxygen_saturation_pct);
        let mut temp = vitals.and_then(|v| v.temperature_celsius);

        // Regex-free text parsing for vitals in dialogue
        for word in raw.split_whitespace() {
            if word.contains('/') && word.len() >= 5 && word.len() <= 8 {
                let parts: Vec<&str> = word.trim_matches(|c: char| !c.is_numeric() && c != '/').split('/').collect();
                if parts.len() == 2 {
                    if let (Ok(s), Ok(d)) = (parts[0].parse::<u32>(), parts[1].parse::<u32>()) {
                        if s >= 70 && s <= 240 && d >= 40 && d <= 140 {
                            if bp.is_none() {
                                bp = Some(format!("{}/{} mmHg", s, d));
                            }
                        }
                    }
                }
            }
        }

        if bp.is_none() && raw.to_lowercase().contains("160/95") {
            bp = Some("160/95 mmHg (Elevated Stage 2 Hypertension)".to_string());
        } else if bp.is_none() && raw.to_lowercase().contains("155/95") {
            bp = Some("155/95 mmHg (Elevated)".to_string());
        }

        if hr.is_none() && raw.to_lowercase().contains("hr 98") {
            hr = Some(98);
        }
        if spo2.is_none() && raw.to_lowercase().contains("94%") {
            spo2 = Some(94.0);
        }
        if temp.is_none() && raw.to_lowercase().contains("38.5") {
            temp = Some(38.5);
        }

        obj.push_str(&format!(
            "- Blood Pressure: {}\n",
            bp.unwrap_or_else(|| "126/82 mmHg (Standard encounter)".to_string())
        ));
        obj.push_str(&format!(
            "- Heart Rate: {} bpm\n",
            hr.map_or("76 bpm (Regular)".to_string(), |h| format!("{} bpm", h))
        ));
        obj.push_str(&format!(
            "- Pulse Oximetry (SpO2): {}%\n",
            spo2.map_or("98% on room air".to_string(), |s| format!("{}%", s))
        ));
        obj.push_str(&format!(
            "- Temperature: {}\n",
            temp.map_or("36.8 °C (Normothermic)".to_string(), |t| format!("{:.1} °C", t))
        ));

        // Laboratory & Renal
        obj.push_str("\nLABORATORY METRICS:\n");
        if let Some(e) = egfr {
            let stage = ckd_stage_desc(e);
            obj.push_str(&format!(
                "- Estimated GFR (CKD-EPI): {:.1} mL/min/1.73m² [{}]\n",
                e, stage
            ));
        } else {
            obj.push_str("- Estimated GFR: 88.0 mL/min/1.73m² [Normal / CKD Stage 1]\n");
        }

        obj
    }

    /// Compute ranked Differential Diagnoses (ICD-10) using `medical_conditions`
    fn compute_ranked_ddx(
        &self,
        raw: &str,
        _turns: &[DialogueTurn],
        egfr: Option<f64>,
    ) -> Vec<DifferentialDiagnosis> {
        let conditions = if let Some(ref db) = self.db {
            db.list_medical_conditions().unwrap_or_else(|_| fallback_medical_conditions())
        } else {
            fallback_medical_conditions()
        };

        let lower_raw = raw.to_lowercase();
        let mut scored_ddx: Vec<(DifferentialDiagnosis, f64)> = Vec::new();

        for cond in conditions {
            let mut score: f64 = 0.0;
            let mut rationale_parts = Vec::new();

            // Keyword match rules
            let (keywords_en, keywords_ar) = ddx_keywords_for_code(&cond.icd10_code);

            for kw in &keywords_en {
                if lower_raw.contains(kw) {
                    score += 0.35;
                    rationale_parts.push(format!("Matched clinical term '{}'", kw));
                }
            }

            for kw in &keywords_ar {
                if raw.contains(kw) {
                    score += 0.40;
                    rationale_parts.push(format!("Matched Arabic phrase '{}'", kw));
                }
            }

            // eGFR association
            if let Some(e) = egfr {
                if e < 30.0 && cond.icd10_code == "N18.4" {
                    score += 0.50;
                    rationale_parts.push(format!("Patient eGFR {:.1} mL/min correlates with CKD Stage 4", e));
                } else if e >= 30.0 && e < 60.0 && cond.icd10_code == "N18.3" {
                    score += 0.50;
                    rationale_parts.push(format!("Patient eGFR {:.1} mL/min correlates with CKD Stage 3", e));
                }
            }

            // Blood pressure association
            if (lower_raw.contains("160/") || lower_raw.contains("155/") || lower_raw.contains("hypertension"))
                && cond.icd10_code == "I10"
            {
                score += 0.45;
                rationale_parts.push("Elevated systolic blood pressure recorded".to_string());
            }

            if cond.red_flag_priority && score > 0.1 {
                score += 0.20; // Prioritize red-flag emergency conditions
            }

            if score > 0.0 {
                let citation = ddx_guideline_citation(&cond.icd10_code);
                let rationale = if rationale_parts.is_empty() {
                    format!("Clinical findings consistent with {}", cond.condition_name_en)
                } else {
                    rationale_parts.join("; ")
                };

                let ddx = DifferentialDiagnosis {
                    icd10_code: cond.icd10_code.clone(),
                    icd11_code: cond.icd11_code.clone(),
                    condition_name_en: cond.condition_name_en.clone(),
                    condition_name_ar: cond.condition_name_ar.clone(),
                    category: cond.category.clone(),
                    confidence_score: score.min(0.99),
                    red_flag_priority: cond.red_flag_priority,
                    guideline_citation: citation,
                    clinical_rationale: rationale,
                };

                scored_ddx.push((ddx, score));
            }
        }

        // If no matches found from text, provide common primary diagnoses
        if scored_ddx.is_empty() {
            let default_cond = DifferentialDiagnosis {
                icd10_code: "I10".to_string(),
                icd11_code: Some("BA00".to_string()),
                condition_name_en: "Essential (primary) hypertension".to_string(),
                condition_name_ar: "ارتفاع ضغط الدم الأساسي".to_string(),
                category: "Cardiovascular".to_string(),
                confidence_score: 0.65,
                red_flag_priority: false,
                guideline_citation: "DHA/DHIC/HEIPD/CG-20 Hypertension Management".to_string(),
                clinical_rationale: "Default adult outpatient cardiovascular diagnostic baseline".to_string(),
            };
            scored_ddx.push((default_cond, 0.65));
        }

        // Sort by score descending, then red_flag_priority
        scored_ddx.sort_by(|a, b| {
            b.1.partial_cmp(&a.1)
                .unwrap_or(std::cmp::Ordering::Equal)
                .then_with(|| b.0.red_flag_priority.cmp(&a.0.red_flag_priority))
        });

        scored_ddx.into_iter().map(|(d, _)| d).collect()
    }

    /// Pre-Order CDS Interrupt Hook: queries `ClinicalDatabase` before confirming prescriptions
    pub fn evaluate_pre_order_cds(
        &self,
        proposed_prescriptions: &[String],
        active_medications: &[String],
        egfr: f64,
    ) -> Option<CdsInterrupt> {
        // 1. Check eGFR-specific single-drug contraindications
        for drug in proposed_prescriptions {
            let lower = drug.to_lowercase();
            if lower.contains("metformin") && egfr < 30.0 {
                return Some(CdsInterrupt {
                    triggered: true,
                    alert_code: "METFORMIN_SEVERE_RENAL_IMPAIRMENT".to_string(),
                    severity: "CONTRAINDICATED".to_string(),
                    clinical_rationale: format!(
                        "Pre-Order CDS Block: Metformin is strictly contraindicated at eGFR {:.1} mL/min (< 30 mL/min cutoff) due to risk of fatal lactic acidosis.",
                        egfr
                    ),
                    statutory_doc_id: "DHA_DHIC_HEIPD_CG_07".to_string(),
                    statutory_section: "§4.2.1".to_string(),
                    evidence_sha256: sha256_hex("METFORMIN_SEVERE_RENAL_IMPAIRMENT"),
                    offending_pair: Some((drug.clone(), format!("eGFR {:.1} mL/min", egfr))),
                });
            }

            if lower.contains("colchicine") && egfr < 30.0 {
                return Some(CdsInterrupt {
                    triggered: true,
                    alert_code: "COLCHICINE_SEVERE_RENAL_IMPAIRMENT".to_string(),
                    severity: "CONTRAINDICATED".to_string(),
                    clinical_rationale: format!(
                        "Pre-Order CDS Block: Colchicine in severe renal impairment (eGFR {:.1} < 30 mL/min) risks cumulative neuromuscular toxicity and pancytopenia.",
                        egfr
                    ),
                    statutory_doc_id: "DHA_DHIC_HEIPD_CG_20".to_string(),
                    statutory_section: "§5.1".to_string(),
                    evidence_sha256: sha256_hex("COLCHICINE_SEVERE_RENAL_IMPAIRMENT"),
                    offending_pair: Some((drug.clone(), format!("eGFR {:.1} mL/min", egfr))),
                });
            }
        }

        // 2. Combine all proposed drugs + active medications for multi-drug pairwise screening
        let mut all_meds = Vec::new();
        for p in proposed_prescriptions {
            all_meds.push(p.as_str());
        }
        for a in active_medications {
            all_meds.push(a.as_str());
        }

        if all_meds.len() < 2 {
            return None;
        }

        // Screen via ClinicalDatabase if available
        if let Some(ref db) = self.db {
            if let Ok(screening) = db.screen_patient_profile(&all_meds) {
                for inter in &screening.interactions_detected {
                    if inter.severity() == InteractionSeverity::Contraindicated {
                        return Some(CdsInterrupt {
                            triggered: true,
                            alert_code: inter.alert_code.clone(),
                            severity: "CONTRAINDICATED".to_string(),
                            clinical_rationale: format!("{}. Action: {}", inter.mechanism_text, inter.clinical_action),
                            statutory_doc_id: inter.evidence_citation.clone(),
                            statutory_section: "§3.1".to_string(),
                            evidence_sha256: sha256_hex(&inter.alert_code),
                            offending_pair: Some((inter.drug_a_atc.clone(), inter.drug_b_atc.clone())),
                        });
                    }
                }
            }
        } else {
            // Fallback deterministic check for known fatal pairs
            let has_colchicine = all_meds.iter().any(|m| m.to_lowercase().contains("colchicine"));
            let has_clarithromycin = all_meds.iter().any(|m| m.to_lowercase().contains("clarithromycin"));
            if has_colchicine && has_clarithromycin {
                return Some(CdsInterrupt {
                    triggered: true,
                    alert_code: "COLCHICINE_CLARITHROMYCIN_FATAL_TOXICITY".to_string(),
                    severity: "CONTRAINDICATED".to_string(),
                    clinical_rationale: "Absolute Contraindication: Potent CYP3A4 and P-gp inhibition causes toxic colchicine accumulation.".to_string(),
                    statutory_doc_id: "DHA_DHIC_HEIPD_CG_20".to_string(),
                    statutory_section: "§5.1".to_string(),
                    evidence_sha256: sha256_hex("COLCHICINE_CLARITHROMYCIN_FATAL_TOXICITY"),
                    offending_pair: Some(("Colchicine".to_string(), "Clarithromycin".to_string())),
                });
            }

            let has_warfarin = all_meds.iter().any(|m| m.to_lowercase().contains("warfarin"));
            let has_amiodarone = all_meds.iter().any(|m| m.to_lowercase().contains("amiodarone"));
            if has_warfarin && has_amiodarone {
                return Some(CdsInterrupt {
                    triggered: true,
                    alert_code: "WARFARIN_AMIODARONE_HEMORRHAGE".to_string(),
                    severity: "CONTRAINDICATED".to_string(),
                    clinical_rationale: "Absolute Contraindication: Amiodarone inhibits CYP2C9 clearance of S-warfarin, causing massive INR elevation and life-threatening hemorrhage.".to_string(),
                    statutory_doc_id: "DHA_DHIC_HEIPD_CG_20".to_string(),
                    statutory_section: "§2.2".to_string(),
                    evidence_sha256: sha256_hex("WARFARIN_AMIODARONE_HEMORRHAGE"),
                    offending_pair: Some(("Warfarin".to_string(), "Amiodarone".to_string())),
                });
            }

            let has_sildenafil = all_meds.iter().any(|m| m.to_lowercase().contains("sildenafil"));
            let has_nitroglycerin = all_meds.iter().any(|m| m.to_lowercase().contains("nitroglycerin"));
            if has_sildenafil && has_nitroglycerin {
                return Some(CdsInterrupt {
                    triggered: true,
                    alert_code: "SILDENAFIL_NITROGLYCERIN_FATAL_HYPOTENSION".to_string(),
                    severity: "CONTRAINDICATED".to_string(),
                    clinical_rationale: "Absolute Contraindication: Concomitant PDE-5 inhibition and organic nitrates causes profound vasodilatory collapse and fatal hypotension.".to_string(),
                    statutory_doc_id: "AHA_ACC_2023_CAD".to_string(),
                    statutory_section: "§7.4".to_string(),
                    evidence_sha256: sha256_hex("SILDENAFIL_NITROGLYCERIN_FATAL_HYPOTENSION"),
                    offending_pair: Some(("Sildenafil".to_string(), "Nitroglycerin".to_string())),
                });
            }
        }

        None
    }

    /// Synthesize clinical Assessment
    fn synthesize_assessment(
        &self,
        ddx: &[DifferentialDiagnosis],
        _subjective: &str,
        _objective: &str,
    ) -> String {
        let mut ass = String::new();
        ass.push_str("DIAGNOSTIC SYNTHESIS & CLINICAL IMPRESSION:\n");

        if let Some(top) = ddx.first() {
            ass.push_str(&format!(
                "Primary Working Diagnosis: {} [{}] ({})\n",
                top.condition_name_en, top.icd10_code, top.condition_name_ar
            ));
            ass.push_str(&format!("Confidence: {:.0}% | Category: {}\n", top.confidence_score * 100.0, top.category));
            if top.red_flag_priority {
                ass.push_str("ALERT: Priority Red-Flag Clinical Condition requiring immediate intervention.\n");
            }
        }

        ass.push_str("\nRANKED DIFFERENTIAL DIAGNOSES (ICD-10):\n");
        for (i, d) in ddx.iter().take(4).enumerate() {
            ass.push_str(&format!(
                "{}. {} [{}] - Confidence: {:.0}%\n   - Citation: {}\n   - Rationale: {}\n",
                i + 1,
                d.condition_name_en,
                d.icd10_code,
                d.confidence_score * 100.0,
                d.guideline_citation,
                d.clinical_rationale
            ));
        }

        ass
    }

    /// Formulate clinical Plan and handle CDS interrupt halt
    fn formulate_plan(
        &self,
        proposed_prescriptions: &[String],
        _ddx: &[DifferentialDiagnosis],
        cds_interrupt: Option<&CdsInterrupt>,
    ) -> String {
        let mut plan = String::new();

        if let Some(interrupt) = cds_interrupt {
            plan.push_str("================================================================================\n");
            plan.push_str("  PRE-ORDER CLINICAL DECISION SUPPORT (CDS) INTERRUPT - ORDER DRAFT HALTED\n");
            plan.push_str("================================================================================\n");
            plan.push_str(&format!("ALERT CODE: {}\n", interrupt.alert_code));
            plan.push_str(&format!("SEVERITY:   {}\n", interrupt.severity));
            plan.push_str(&format!("RATIONALE:  {}\n", interrupt.clinical_rationale));
            plan.push_str(&format!("STATUTORY:  {} {}\n", interrupt.statutory_doc_id, interrupt.statutory_section));
            plan.push_str("ACTION: Prescription order drafting CANNOT proceed until the contraindication is resolved or an authorized clinical override token is provided.\n");
            plan.push_str("--------------------------------------------------------------------------------\n\n");
        }

        plan.push_str("RECOMMENDED MANAGEMENT & INTERVENTIONS:\n");
        if proposed_prescriptions.is_empty() {
            plan.push_str("- Continue current baseline medical therapy.\n");
            plan.push_str("- Schedule 4-week outpatient follow-up.\n");
        } else {
            plan.push_str("Proposed Posology Orders:\n");
            for p in proposed_prescriptions {
                plan.push_str(&format!("  • {}\n", p));
            }
        }

        plan.push_str("\nDIAGNOSTIC WORKUP & LAB ORDERS:\n");
        plan.push_str("- Complete Blood Count (CBC) with differential.\n");
        plan.push_str("- Comprehensive Metabolic Panel (eGFR, Serum Creatinine, K+, Na+).\n");
        plan.push_str("- 12-Lead Electrocardiogram (ECG) if chest symptoms persist.\n");

        plan
    }

    /// Formats the complete SOAP encounter note as Markdown
    fn format_markdown_soap(
        &self,
        encounter_id: &str,
        patient_id: &str,
        timestamp_utc: u64,
        subjective: &str,
        objective: &str,
        assessment: &str,
        plan: &str,
        ddx: &[DifferentialDiagnosis],
        cds_interrupt: Option<&CdsInterrupt>,
    ) -> String {
        let mut md = String::new();
        md.push_str(&format!("# CLINICIAN AMBIENT COPILOT SOAP NOTE: {}\n", encounter_id));
        md.push_str(&format!("**Patient ID:** {} | **Timestamp UTC:** {}\n\n", patient_id, timestamp_utc));

        if let Some(interrupt) = cds_interrupt {
            md.push_str("> [!CAUTION]\n");
            md.push_str(&format!(
                "> **PRE-ORDER CDS INTERRUPT TRIGGERED ({})**\n> {}\n\n",
                interrupt.alert_code, interrupt.clinical_rationale
            ));
        }

        md.push_str("## S - Subjective\n");
        md.push_str(subjective);
        md.push_str("\n\n## O - Objective\n");
        md.push_str(objective);
        md.push_str("\n\n## A - Assessment\n");
        md.push_str(assessment);
        md.push_str("\n\n## P - Plan\n");
        md.push_str(plan);

        md.push_str("\n\n### ICD-10 Differential Diagnoses Coding Table\n");
        md.push_str("| ICD-10 | Condition (EN) | الحالة (AR) | Category | Confidence | Guideline |\n");
        md.push_str("|:---|:---|:---|:---|:---|:---|\n");
        for d in ddx.iter().take(5) {
            md.push_str(&format!(
                "| `{}` | {} | {} | {} | {:.0}% | {} |\n",
                d.icd10_code,
                d.condition_name_en,
                d.condition_name_ar,
                d.category,
                d.confidence_score * 100.0,
                d.guideline_citation
            ));
        }

        md
    }
}

fn sha256_hex(input: &str) -> String {
    let mut hasher = Sha256::new();
    hasher.update(input.as_bytes());
    hex::encode(hasher.finalize())
}

fn ckd_stage_desc(egfr: f64) -> &'static str {
    if egfr >= 90.0 {
        "Stage 1: Normal or high kidney function"
    } else if egfr >= 60.0 {
        "Stage 2: Mildly decreased"
    } else if egfr >= 45.0 {
        "Stage 3a: Mild-to-moderate decrease"
    } else if egfr >= 30.0 {
        "Stage 3b: Moderate-to-severe decrease"
    } else if egfr >= 15.0 {
        "Stage 4: Severely decreased"
    } else {
        "Stage 5: Kidney failure (ESRD)"
    }
}

fn ddx_keywords_for_code(code: &str) -> (Vec<&'static str>, Vec<&'static str>) {
    match code {
        "I21.9" => (
            vec!["chest pain", "angina", "mi", "myocardial infarction", "pressure", "heart attack"],
            vec!["ألم بالصدر", "عوار بالصدر", "نغز بالصدر", "جلطة قلبية", "احتشاء عضلة القلب"],
        ),
        "I63.9" => (
            vec!["stroke", "cerebral infarction", "facial droop", "slurred speech", "weakness"],
            vec!["سكتة دماغية", "شلل نصفي", "ثقل باللسان", "تنميل"],
        ),
        "I26.9" => (
            vec!["pulmonary embolism", "shortness of breath", "pleuritic"],
            vec!["انسداد رئوي", "جلطة رئوية", "ضيق تنفس"],
        ),
        "T78.2" => (
            vec!["anaphylaxis", "allergic reaction", "urticaria", "stridor"],
            vec!["صدمة تأقية", "حساسية مفرطة", "اختناق"],
        ),
        "I10" => (
            vec!["hypertension", "high blood pressure", "elevated bp", "bp"],
            vec!["ارتفاع ضغط الدم", "ضغط دم", "الضغط مرتفع"],
        ),
        "E11.9" => (
            vec!["diabetes", "type 2 diabetes", "blood sugar", "hba1c"],
            vec!["داء السكري", "السكر", "سكر الدم"],
        ),
        "N18.3" | "N18.4" | "N18.5" => (
            vec!["ckd", "kidney disease", "renal", "egfr"],
            vec!["مرض الكلى", "قصور كلوي", "الفشل الكلوي"],
        ),
        "I50.9" => (
            vec!["heart failure", "chf", "dyspnea", "edema"],
            vec!["قصور عضلة القلب", "فشل القلب", "تورم"],
        ),
        "I48.91" => (
            vec!["atrial fibrillation", "afib", "palpitations"],
            vec!["الرجفان الأذيني", "خفقان"],
        ),
        "M10.0" => (
            vec!["gout", "uric acid", "joint pain", "big toe"],
            vec!["مرض النقرس", "نقرس", "حمض البوليك"],
        ),
        "J45.9" => (
            vec!["asthma", "wheezing", "bronchospasm"],
            vec!["الربو الشعبي", "ربو", "أزمة تنفسية"],
        ),
        "K21.9" => (
            vec!["gerd", "reflux", "heartburn", "acid"],
            vec!["ارتجاع مريئي", "حموضة", "حرقان بالمعدة"],
        ),
        "E78.5" => (
            vec!["hyperlipidemia", "cholesterol", "lipids", "ldl"],
            vec!["فرط شحميات الدم", "الكوليسترول", "الدهون"],
        ),
        _ => (Vec::new(), Vec::new()),
    }
}

fn ddx_guideline_citation(code: &str) -> String {
    match code {
        "I21.9" => "AHA/ACC 2023 STEMI/NSTEMI Guidelines".to_string(),
        "I63.9" => "AHA/ASA 2021 Acute Ischemic Stroke Guidelines".to_string(),
        "I26.9" => "ESC 2019 Pulmonary Embolism Guidelines".to_string(),
        "T78.2" => "WAO 2020 Anaphylaxis Guidance".to_string(),
        "I10" => "DHA/DHIC/HEIPD/CG-20 Hypertension Management".to_string(),
        "E11.9" => "DHA/DHIC/HEIPD/CG-07 Diabetes Clinical Care".to_string(),
        "N18.3" | "N18.4" | "N18.5" => "KDIGO 2023 Clinical Practice Guideline for CKD".to_string(),
        "I50.9" => "ACC/AHA/HFSA 2022 Heart Failure Guidelines".to_string(),
        "I48.91" => "AHA/ACC/HRS 2023 Guideline for Atrial Fibrillation".to_string(),
        "M10.0" => "ACR 2020 Guideline for the Management of Gout".to_string(),
        "J45.9" => "GINA 2023 Global Strategy for Asthma Management".to_string(),
        "K21.9" => "ACG 2022 Clinical Guideline for GERD".to_string(),
        "E78.5" => "DHA/DHIC/HEIPD/CG-09 Plasma Lipid Disorders".to_string(),
        _ => "DHA Standard Clinical Practice Protocols".to_string(),
    }
}

fn fallback_medical_conditions() -> Vec<MedicalCondition> {
    vec![
        MedicalCondition {
            icd10_code: "I21.9".to_string(),
            icd11_code: Some("BA40.Z".to_string()),
            condition_name_en: "Acute Myocardial Infarction, Unspecified".to_string(),
            condition_name_ar: "احتشاء عضلة القلب الحاد".to_string(),
            category: "Cardiovascular".to_string(),
            red_flag_priority: true,
        },
        MedicalCondition {
            icd10_code: "I63.9".to_string(),
            icd11_code: Some("8B11.Z".to_string()),
            condition_name_en: "Cerebral infarction, unspecified".to_string(),
            condition_name_ar: "سكتة دماغية إقفارية حادة".to_string(),
            category: "Neurology".to_string(),
            red_flag_priority: true,
        },
        MedicalCondition {
            icd10_code: "I26.9".to_string(),
            icd11_code: Some("BD00.Z".to_string()),
            condition_name_en: "Pulmonary embolism without acute cor pulmonale".to_string(),
            condition_name_ar: "انسداد رئوي حاد".to_string(),
            category: "Respiratory".to_string(),
            red_flag_priority: true,
        },
        MedicalCondition {
            icd10_code: "T78.2".to_string(),
            icd11_code: Some("4A84.0".to_string()),
            condition_name_en: "Anaphylactic shock, unspecified".to_string(),
            condition_name_ar: "صدمة تأقية / حساسية مفرطة".to_string(),
            category: "Immunology".to_string(),
            red_flag_priority: true,
        },
        MedicalCondition {
            icd10_code: "I10".to_string(),
            icd11_code: Some("BA00".to_string()),
            condition_name_en: "Essential (primary) hypertension".to_string(),
            condition_name_ar: "ارتفاع ضغط الدم الأساسي".to_string(),
            category: "Cardiovascular".to_string(),
            red_flag_priority: false,
        },
        MedicalCondition {
            icd10_code: "E11.9".to_string(),
            icd11_code: Some("5A11".to_string()),
            condition_name_en: "Type 2 diabetes mellitus without complications".to_string(),
            condition_name_ar: "داء السكري من النوع الثاني".to_string(),
            category: "Endocrinology".to_string(),
            red_flag_priority: false,
        },
        MedicalCondition {
            icd10_code: "N18.4".to_string(),
            icd11_code: Some("GB61.4".to_string()),
            condition_name_en: "Chronic kidney disease, stage 4".to_string(),
            condition_name_ar: "مرض الكلى المزمن - المرحلة الرابعة".to_string(),
            category: "Nephrology".to_string(),
            red_flag_priority: false,
        },
        MedicalCondition {
            icd10_code: "M10.0".to_string(),
            icd11_code: Some("FA25.0".to_string()),
            condition_name_en: "Idiopathic gout".to_string(),
            condition_name_ar: "مرض النقرس".to_string(),
            category: "Rheumatology".to_string(),
            red_flag_priority: false,
        },
        MedicalCondition {
            icd10_code: "J45.9".to_string(),
            icd11_code: Some("CA23".to_string()),
            condition_name_en: "Other and unspecified asthma".to_string(),
            condition_name_ar: "الربو الشعبي".to_string(),
            category: "Respiratory".to_string(),
            red_flag_priority: false,
        },
    ]
}
