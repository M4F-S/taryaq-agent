//! Taryaq Sovereign Clinical AI Agent Daemon & CLI Suite
//! Dubai Create AI Agents Championship 2026–2027

use std::env;
use taryaq::clinical::ClinicalDatabase;
use taryaq::trimodal::copilot::{ClinicianAmbientCopilot, ClinicianEncounterInput};
use taryaq::trimodal::pharmacist::{
    ClinicianOverrideSubmission, PharmacistVerificationConsole, PharmacistVerificationInput,
};
use taryaq::trimodal::companion::PatientHealthCompanion;
use taryaq::voice::VoicePipeline;

#[tokio::main]
async fn main() {
    tracing_subscriber::fmt::init();

    let args: Vec<String> = env::args().collect();
    if args.len() < 2 {
        run_default_daemon().await;
        return;
    }

    match args[1].as_str() {
        "copilot" => run_copilot_cli(&args[2..]),
        "pharmacist" => run_pharmacist_cli(&args[2..]),
        "companion" => run_companion_cli(&args[2..]),
        "voice-transcribe" => run_voice_transcribe_cli(&args[2..]),
        "daemon" | "--daemon" => run_default_daemon().await,
        "--help" | "-h" | "help" => print_usage(),
        unknown => {
            eprintln!("Unknown command: '{}'", unknown);
            print_usage();
            std::process::exit(1);
        }
    }
}

fn print_usage() {
    println!(r#"TARYAQ (ترياق) Sovereign Clinical AI Agent CLI Suite
Dubai Create AI Agents Championship 2026–2027

USAGE:
  taryaq_agent_daemon <COMMAND> [OPTIONS]

COMMANDS:
  copilot           Run Clinician Ambient Copilot (ambient dialogue -> SOAP note & DDx)
  pharmacist        Run Pharmacist Verification Console (polypharmacy screening & claims penalty)
  companion         Run Patient Health Companion (dialectal intake & emergency 998 triage)
  voice-transcribe  Ingest audio file (.wav, .ogg, .mp3), transcribe offline & route to interface
  daemon            Start persistent sovereign daemon runtime

EXAMPLES:
  # 1. Clinician Ambient Copilot
  taryaq_agent_daemon copilot --dialogue "Patient: عندي عوار بالصدر وضيق تنفس. Doctor: BP 160/95" --egfr 25.0 --drugs "Metformin, Lisinopril"

  # 2. Pharmacist Verification Console
  taryaq_agent_daemon pharmacist --meds "Colchicine, Clarithromycin" --egfr 45.0 --claim 2500.0 --delay 45

  # 3. Patient Health Companion
  taryaq_agent_daemon companion --message "حاسس بسكاكين في صدري ومش قادر أتنفس خالص"

  # 4. Local Voice Transcription & Routing
  taryaq_agent_daemon voice-transcribe test_audio.wav --route copilot
"#);
}

async fn run_default_daemon() {
    println!("================================================================================");
    println!("  TARYAQ (ترياق) SOVEREIGN CLINICAL AI AGENT RUNTIME v0.1.0");
    println!("  Dubai Create AI Agents Championship 2026–2027 | Best Societal AI Agent");
    println!("================================================================================");

    let db = ClinicalDatabase::open_default();
    match db {
        Ok(d) => {
            let drugs = d.count_formulary_drugs().unwrap_or(0);
            let interactions = d.count_interactions().unwrap_or(0);
            println!("[+] Sovereign Clinical Database Linked:");
            println!("    • Formulary Drugs: {}", drugs);
            println!("    • Drug-Drug Interactions: {}", interactions);
        }
        Err(e) => {
            eprintln!("[-] Warning: Could not open default database: {}", e);
        }
    }

    let companion = PatientHealthCompanion::new();
    let sample_triage = companion.consult("أشعر بألم في الصدر وضيق تنفس مستمر", &[], None);
    if let Some(em) = sample_triage.emergency_directive {
        println!("[+] Emergency Dispatch Gate Verified: Category: {:?}", em.red_flag_category);
        println!("    Directives: Call 998 (Dubai Ambulance) / 999 (Emergency)");
    }

    println!("[+] Tri-Modal Clinical Interfaces & Local Voice Pipeline Online.");
    println!("    Type 'taryaq_agent_daemon --help' for CLI command references.");
}

fn run_copilot_cli(args: &[String]) {
    let mut dialogue = String::new();
    let mut egfr = 90.0;
    let mut drugs_csv = String::new();

    let mut i = 0;
    while i < args.len() {
        match args[i].as_str() {
            "--dialogue" => {
                if i + 1 < args.len() {
                    dialogue = args[i + 1].clone();
                    i += 1;
                }
            }
            "--file" => {
                if i + 1 < args.len() {
                    dialogue = std::fs::read_to_string(&args[i + 1]).unwrap_or_default();
                    i += 1;
                }
            }
            "--egfr" => {
                if i + 1 < args.len() {
                    egfr = args[i + 1].parse().unwrap_or(90.0);
                    i += 1;
                }
            }
            "--drugs" => {
                if i + 1 < args.len() {
                    drugs_csv = args[i + 1].clone();
                    i += 1;
                }
            }
            _ => {}
        }
        i += 1;
    }

    if dialogue.trim().is_empty() {
        dialogue = "Doctor: Good morning. How are you feeling today? \n\
                    Patient: Doctor, عندي عوار ونغز قوي بالصدر وضيق تنفس من الصباح. \n\
                    Doctor: Let me check your vitals. BP is 155/95 mmHg, heart rate is 98 bpm. eGFR is 25 ml/min. \n\
                    Doctor: I am considering prescribing Metformin and Lisinopril for your symptoms."
            .to_string();
    }

    let proposed_drugs: Vec<String> = drugs_csv
        .split(',')
        .map(|s| s.trim().to_string())
        .filter(|s| !s.is_empty())
        .collect();

    let copilot = ClinicianAmbientCopilot::with_default_db();
    let input = ClinicianEncounterInput {
        dialogue,
        turns: Vec::new(),
        patient_id: Some("DXB-PAT-CLIN".to_string()),
        patient_egfr: Some(egfr),
        proposed_prescriptions: proposed_drugs,
        active_medications: Vec::new(),
        documented_vitals: None,
    };

    let note = copilot.process_encounter(&input);
    println!("{}", note.markdown_note);
}

fn run_pharmacist_cli(args: &[String]) {
    let mut meds_csv = "Colchicine, Clarithromycin".to_string();
    let mut egfr = 45.0;
    let mut claim_aed = 2500.0;
    let mut delay_days = 45;
    let mut override_token = None;
    let mut override_rationale = None;

    let mut i = 0;
    while i < args.len() {
        match args[i].as_str() {
            "--meds" => {
                if i + 1 < args.len() {
                    meds_csv = args[i + 1].clone();
                    i += 1;
                }
            }
            "--egfr" => {
                if i + 1 < args.len() {
                    egfr = args[i + 1].parse().unwrap_or(45.0);
                    i += 1;
                }
            }
            "--claim" => {
                if i + 1 < args.len() {
                    claim_aed = args[i + 1].parse().unwrap_or(2500.0);
                    i += 1;
                }
            }
            "--delay" => {
                if i + 1 < args.len() {
                    delay_days = args[i + 1].parse().unwrap_or(45);
                    i += 1;
                }
            }
            "--override-token" => {
                if i + 1 < args.len() {
                    override_token = Some(args[i + 1].clone());
                    i += 1;
                }
            }
            "--rationale" => {
                if i + 1 < args.len() {
                    override_rationale = Some(args[i + 1].clone());
                    i += 1;
                }
            }
            _ => {}
        }
        i += 1;
    }

    let meds: Vec<String> = meds_csv
        .split(',')
        .map(|s| s.trim().to_string())
        .filter(|s| !s.is_empty())
        .collect();

    let clinician_override = if let (Some(t), Some(r)) = (override_token, override_rationale) {
        Some(ClinicianOverrideSubmission {
            override_token: t,
            clinician_id: "DHA-DOC-4091".to_string(),
            clinical_rationale: r,
        })
    } else {
        None
    };

    let input = PharmacistVerificationInput {
        patient_id: "DXB-PAT-PHARMA".to_string(),
        medication_regimen: meds,
        patient_egfr: egfr,
        primary_diagnosis_icd10: None,
        net_claim_aed: claim_aed,
        claim_submission_delay_days: delay_days,
        is_resubmission: false,
        resubmission_delay_days: None,
        clinician_override,
    };

    let console = PharmacistVerificationConsole::with_default();
    match console.verify_patient_regimen(&input) {
        Ok(report) => {
            println!("================================================================================");
            println!("  PHARMACIST VERIFICATION CONSOLE REPORT");
            println!("================================================================================");
            println!("Patient ID:              {}", report.patient_id);
            println!("Total Drugs Screened:    {}", report.total_drugs_screened);
            println!("Dispensation Approved:   {}", report.is_dispensation_approved);
            println!("Contraindication Block:  {}", report.is_contraindication_blocked);
            println!("Primary Verdict:         {}", report.primary_verdict);
            println!("Primary Alert Code:      {}", report.primary_alert_code);
            println!("Clinical Rationale:      {}", report.primary_clinical_rationale);
            println!("Renal CKD Stage:         {}", report.renal_ckd_stage);
            if let Some(w) = report.renal_dosing_warning {
                println!("Renal Warning:           {}", w);
            }
            println!("--------------------------------------------------------------------------------");
            println!("DHA SN 03/2019 CLAIMS SCRUBBING:");
            println!("  Net Claim AED:         {:.2}", report.dha_claims_scrubbing.net_claim_aed);
            println!("  Submission Delay:      {} days", report.dha_claims_scrubbing.submission_delay_days);
            println!("  Statutory Overdue:     {} days", report.dha_claims_scrubbing.overdue_days);
            println!("  Penalty Rate:          {:.4}%", report.dha_claims_scrubbing.total_penalty_rate_pct);
            println!("  Penalty Amount AED:    {:.2}", report.dha_claims_scrubbing.penalty_amount_aed);
            println!("  Adjusted Net Claim:    AED {:.2}", report.dha_claims_scrubbing.adjusted_net_claim_aed);
            println!("--------------------------------------------------------------------------------");
            println!("Audit Committed:         {}", report.audit_committed);
            if let Some(a) = report.audit_record {
                println!("Audit MAC:               {}", a.mac);
                println!("Audit Canonical:         {}", a.canonical);
            }
        }
        Err(e) => {
            eprintln!("[-] Pharmacist Verification Rejected: {}", e);
        }
    }
}

fn run_companion_cli(args: &[String]) {
    let mut message = "حاسس بسكاكين في صدري ومش قادر أتنفس خالص".to_string();
    let mut meds_csv = "Lisinopril, Atorvastatin".to_string();

    let mut i = 0;
    while i < args.len() {
        match args[i].as_str() {
            "--message" => {
                if i + 1 < args.len() {
                    message = args[i + 1].clone();
                    i += 1;
                }
            }
            "--file" => {
                if i + 1 < args.len() {
                    message = std::fs::read_to_string(&args[i + 1]).unwrap_or_default();
                    i += 1;
                }
            }
            "--meds" => {
                if i + 1 < args.len() {
                    meds_csv = args[i + 1].clone();
                    i += 1;
                }
            }
            _ => {}
        }
        i += 1;
    }

    let meds: Vec<String> = meds_csv
        .split(',')
        .map(|s| s.trim().to_string())
        .filter(|s| !s.is_empty())
        .collect();

    let companion = PatientHealthCompanion::new();
    let consult = companion.consult(&message, &meds, None);

    println!("================================================================================");
    println!("  PATIENT HEALTH COMPANION CONSULTATION");
    println!("================================================================================");
    println!("Detected Dialect:   {:?}", consult.detected_dialect);
    println!("Triage Level:       {:?}", consult.triage_level);

    if let Some(em) = consult.emergency_directive {
        println!("\n🚨 EMERGENCY DIRECTIVE TRIGGERED: {:?}", em.red_flag_category);
        println!("Immediate Dispatch Required: {}", em.requires_immediate_dispatch);
        println!("Call Numbers: {:?}", em.emergency_dispatch_numbers);
        println!("\n[Arabic Emergency Alert]:\n{}", em.arabic_emergency_alert);
        println!("\n[English Emergency Alert]:\n{}", em.english_emergency_alert);
    } else {
        println!("\n[Arabic Guidance]:\n{}", consult.conversational_response_ar);
        println!("\n[English Guidance]:\n{}", consult.conversational_response_en);
        println!("\n[Lifestyle Counseling]:\n{}", consult.lifestyle_counseling_en);
    }
}

fn run_voice_transcribe_cli(args: &[String]) {
    if args.is_empty() {
        eprintln!("Usage: taryaq_agent_daemon voice-transcribe <audio_file> [--route <copilot|pharmacist|companion>]");
        return;
    }

    let audio_file = &args[0];
    let mut route_target = None;

    let mut i = 1;
    while i < args.len() {
        if args[i] == "--route" && i + 1 < args.len() {
            route_target = Some(args[i + 1].to_lowercase());
            i += 1;
        }
        i += 1;
    }

    let pipeline = VoicePipeline::new();
    match pipeline.transcribe_audio_file(audio_file) {
        Ok(res) => {
            println!("================================================================================");
            println!("  VOICE TRANSCRIPTION RESULT");
            println!("================================================================================");
            println!("Audio File:        {}", res.audio_path);
            println!("Container Format:  {:?}", res.format);
            println!("Detected Language: {}", res.detected_language);
            println!("Backend Used:      {}", res.backend_used);
            println!("Estimated Dur:     {:.2} sec", res.duration_sec);
            println!("Segments:          {}", res.segments.len());
            println!("--------------------------------------------------------------------------------");
            println!("TRANSCRIPT:\n{}", res.text);
            println!("================================================================================");

            if let Some(target) = route_target {
                match target.as_str() {
                    "copilot" => {
                        let note = pipeline.route_to_copilot(&res, Some(60.0), Vec::new(), Vec::new());
                        println!("\n{}", note.markdown_note);
                    }
                    "pharmacist" => {
                        let report = pipeline.route_to_pharmacist(&res, 55.0, 1500.0, 10);
                        match report {
                            Ok(r) => println!("\nPharmacist Verdict: {} ({})", r.primary_verdict, r.primary_alert_code),
                            Err(e) => eprintln!("Pharmacist Routing Error: {}", e),
                        }
                    }
                    "companion" => {
                        let consult = pipeline.route_to_companion(&res, &[], None);
                        if let Some(em) = consult.emergency_directive {
                            println!("\n🚨 Emergency Detected: {:?}", em.red_flag_category);
                        } else {
                            println!("\nCompanion Arabic:\n{}", consult.conversational_response_ar);
                        }
                    }
                    _ => eprintln!("Unknown routing target: '{}'", target),
                }
            }
        }
        Err(e) => {
            eprintln!("[-] Audio Transcription Failed: {}", e);
        }
    }
}
