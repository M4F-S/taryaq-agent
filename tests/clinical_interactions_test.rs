//! Integration Tests for TARYAQ Sovereign Clinical Database Engine
//! Dubai Create AI Agents Championship 2026–2027 | Best Societal AI Agent
//! Verifies Generalized Relational Schema, 30 Pharmacovigilance Interactions,
//! Dynamic Patient Profile Screening, and Fail-Closed Safety Gates.

use std::time::Instant;
use taryaq::clinical::{
    ClinicalDatabase, InteractionCheckResult, InteractionCheckVerdict, InteractionSeverity,
    UNVERIFIED_DRUG_REQUIRES_PHARMACIST_REVIEW,
};

fn get_db() -> ClinicalDatabase {
    ClinicalDatabase::open_default().expect("Failed to open clinical database")
}

#[test]
fn test_00_schema_tables_and_record_counts() {
    let db = get_db();
    let drug_count = db.count_formulary_drugs().expect("Count formulary drugs");
    assert!(
        drug_count >= 100,
        "Formulary drug count {} must be >= 100",
        drug_count
    );

    let interaction_count = db.count_interactions().expect("Count interactions");
    assert!(
        interaction_count >= 30,
        "Interaction count {} must be >= 30",
        interaction_count
    );
}

// =========================================================================
// SECTION 2.2: 30 HIGH-YIELD CLINICAL INTERACTIONS MATRIX TESTS
// =========================================================================

#[test]
fn test_interaction_01_warfarin_amiodarone() {
    let db = get_db();
    let inter = db
        .get_interaction("B01AA03", "C01BD01")
        .expect("Query failed")
        .expect("Interaction 1 missing");
    assert_eq!(inter.alert_code, "WARFARIN_AMIODARONE_CYP2C9_INHIB");
    assert_eq!(inter.severity(), InteractionSeverity::Major);
    assert!(inter.mechanism_text.contains("CYP2C9/CYP3A4 inhibition"));
    assert!(inter.clinical_action.contains("Reduce warfarin dose 33–50%"));
    assert!(inter.evidence_citation.contains("30122176"));
}

#[test]
fn test_interaction_02_simvastatin_clarithromycin() {
    let db = get_db();
    let inter = db
        .get_interaction("C10AA01", "J01FA09")
        .expect("Query failed")
        .expect("Interaction 2 missing");
    assert_eq!(inter.alert_code, "STATIN_MACROLIDE_CYP3A4_RHABDO");
    assert_eq!(inter.severity(), InteractionSeverity::Contraindicated);
    assert!(inter.is_contraindicated());
    assert!(inter.mechanism_text.contains("CYP3A4 inhibition"));
    assert!(inter.clinical_action.contains("rhabdomyolysis"));
    assert!(inter.evidence_citation.contains("22619236"));
}

#[test]
fn test_interaction_03_lisinopril_losartan() {
    let db = get_db();
    let inter = db
        .get_interaction("C09AA03", "C09CA01")
        .expect("Query failed")
        .expect("Interaction 3 missing");
    assert_eq!(inter.alert_code, "ACEI_ARB_DUAL_RAAS_BLOCKADE");
    assert_eq!(inter.severity(), InteractionSeverity::Contraindicated);
    assert!(inter.mechanism_text.contains("Dual RAAS blockade"));
    assert!(inter.clinical_action.contains("hyperkalemia occur without clinical benefit"));
    assert!(inter.evidence_citation.contains("18378520"));
}

#[test]
fn test_interaction_04_spironolactone_enalapril() {
    let db = get_db();
    let inter = db
        .get_interaction("C03DA01", "C09AA02")
        .expect("Query failed")
        .expect("Interaction 4 missing");
    assert_eq!(inter.alert_code, "MRA_ACEI_ADDITIVE_HYPERKALEMIA");
    assert_eq!(inter.severity(), InteractionSeverity::Major);
    assert!(inter.mechanism_text.contains("mineralocorticoid antagonism"));
    assert!(inter.clinical_action.contains("lethal hyperkalemia"));
    assert!(inter.evidence_citation.contains("10471456"));
}

#[test]
fn test_interaction_05_clopidogrel_omeprazole() {
    let db = get_db();
    let inter = db
        .get_interaction("B01AC04", "A02BC01")
        .expect("Query failed")
        .expect("Interaction 5 missing");
    assert_eq!(inter.alert_code, "CLOPIDOGREL_OMEPRAZOLE_CYP2C19");
    assert_eq!(inter.severity(), InteractionSeverity::Major);
    assert!(inter.mechanism_text.contains("CYP2C19 inhibition"));
    assert!(inter.clinical_action.contains("pantoprazole or rabeprazole"));
    assert!(inter.evidence_citation.contains("20925534"));
}

#[test]
fn test_interaction_06_methotrexate_ibuprofen() {
    let db = get_db();
    let inter = db
        .get_interaction("L01BA01", "M01AE01")
        .expect("Query failed")
        .expect("Interaction 6 missing");
    assert_eq!(inter.alert_code, "MTX_NSAID_RENAL_CLEARANCE_TOX");
    assert_eq!(inter.severity(), InteractionSeverity::Major);
    assert!(inter.mechanism_text.contains("OAT1/OAT3"));
    assert!(inter.clinical_action.contains("pancytopenia"));
    assert!(inter.evidence_citation.contains("28546162"));
}

#[test]
fn test_interaction_07_fluoxetine_tramadol() {
    let db = get_db();
    let inter = db
        .get_interaction("N06AB03", "N02AX02")
        .expect("Query failed")
        .expect("Interaction 7 missing");
    assert_eq!(inter.alert_code, "SSRI_TRAMADOL_SEROTONIN_SYN");
    assert_eq!(inter.severity(), InteractionSeverity::Major);
    assert!(inter.mechanism_text.contains("serotonin reuptake inhibition"));
    assert!(inter.clinical_action.contains("Serotonin Syndrome"));
    assert!(inter.evidence_citation.contains("15784664"));
}

#[test]
fn test_interaction_08_digoxin_clarithromycin() {
    let db = get_db();
    let inter = db
        .get_interaction("C01AA05", "J01FA09")
        .expect("Query failed")
        .expect("Interaction 8 missing");
    assert_eq!(inter.alert_code, "DIGOXIN_CLARITHRO_PGP_TOXICITY");
    assert_eq!(inter.severity(), InteractionSeverity::Major);
    assert!(inter.mechanism_text.contains("P-gp efflux"));
    assert!(inter.clinical_action.contains("AV block and dysrhythmias"));
    assert!(inter.evidence_citation.contains("11980678"));
}

#[test]
fn test_interaction_09_ciprofloxacin_amiodarone() {
    let db = get_db();
    let inter = db
        .get_interaction("J01MA02", "C01BD01")
        .expect("Query failed")
        .expect("Interaction 9 missing");
    assert_eq!(inter.alert_code, "QT_PROLONGATION_SUMMATION_TDP");
    assert_eq!(inter.severity(), InteractionSeverity::Contraindicated);
    assert!(inter.mechanism_text.contains("potassium channels (IKr)"));
    assert!(inter.clinical_action.contains("Torsades de Pointes"));
    assert!(inter.evidence_citation.contains("21676878"));
}

#[test]
fn test_interaction_10_metformin_iodinated_contrast() {
    let db = get_db();
    let inter = db
        .get_interaction("A10BA02", "V08AB02")
        .expect("Query failed")
        .expect("Interaction 10 missing");
    assert_eq!(inter.alert_code, "METFORMIN_CONTRAST_LACTIC_ACIDOSIS");
    assert_eq!(inter.severity(), InteractionSeverity::Major);
    assert!(inter.mechanism_text.contains("Radiocontrast-induced"));
    assert!(inter.clinical_action.contains("lactic acidosis"));
    assert!(inter.evidence_citation.contains("28864703"));
}

#[test]
fn test_interaction_11_sildenafil_nitroglycerin() {
    let db = get_db();
    let inter = db
        .get_interaction("G04BE03", "C01DA02")
        .expect("Query failed")
        .expect("Interaction 11 missing");
    assert_eq!(inter.alert_code, "PDE5I_NITRATE_FATAL_HYPOTENSION");
    assert_eq!(inter.severity(), InteractionSeverity::Contraindicated);
    assert!(inter.mechanism_text.contains("cGMP accumulation"));
    assert!(inter.clinical_action.contains("Strictly contraindicated (24h window)"));
    assert!(inter.evidence_citation.contains("10587354"));
}

#[test]
fn test_interaction_12_allopurinol_azathioprine() {
    let db = get_db();
    let inter = db
        .get_interaction("M04AA01", "L04AX01")
        .expect("Query failed")
        .expect("Interaction 12 missing");
    assert_eq!(inter.alert_code, "ALLOPURINOL_AZATHIOPRINE_MYELOSUPP");
    assert_eq!(inter.severity(), InteractionSeverity::Contraindicated);
    assert!(inter.mechanism_text.contains("Xanthine oxidase inhibition"));
    assert!(inter.clinical_action.contains("Reduce azathioprine dose by 75%"));
    assert!(inter.evidence_citation.contains("29545437"));
}

#[test]
fn test_interaction_13_linezolid_sertraline() {
    let db = get_db();
    let inter = db
        .get_interaction("J01XX08", "N06AB06")
        .expect("Query failed")
        .expect("Interaction 13 missing");
    assert_eq!(inter.alert_code, "LINEZOLID_SSRI_SEROTONIN_TOXICITY");
    assert_eq!(inter.severity(), InteractionSeverity::Contraindicated);
    assert!(inter.mechanism_text.contains("MAO-A inhibition"));
    assert!(inter.clinical_action.contains("Discontinue SSRI 2 weeks prior"));
    assert!(inter.evidence_citation.contains("23143825"));
}

#[test]
fn test_interaction_14_potassium_chloride_spironolactone() {
    let db = get_db();
    let inter = db
        .get_interaction("A12BA01", "C03DA01")
        .expect("Query failed")
        .expect("Interaction 14 missing");
    assert_eq!(inter.alert_code, "POTASSIUM_MRA_SEVERE_HYPERKALEMIA");
    assert_eq!(inter.severity(), InteractionSeverity::Contraindicated);
    assert!(inter.mechanism_text.contains("Exogenous potassium supplementation"));
    assert!(inter.clinical_action.contains("ventricular fibrillation"));
    assert!(inter.evidence_citation.contains("32679173"));
}

#[test]
fn test_interaction_15_lithium_hydrochlorothiazide() {
    let db = get_db();
    let inter = db
        .get_interaction("N05AN01", "C03AA03")
        .expect("Query failed")
        .expect("Interaction 15 missing");
    assert_eq!(inter.alert_code, "LITHIUM_THIAZIDE_TOXICITY_ACCUM");
    assert_eq!(inter.severity(), InteractionSeverity::Major);
    assert!(inter.mechanism_text.contains("sodium depletion"));
    assert!(inter.clinical_action.contains("Reduce lithium dosage by 50%"));
    assert!(inter.evidence_citation.contains("27181414"));
}

#[test]
fn test_interaction_16_warfarin_ciprofloxacin() {
    let db = get_db();
    let inter = db
        .get_interaction("B01AA03", "J01MA02")
        .expect("Query failed")
        .expect("Interaction 16 missing");
    assert_eq!(inter.alert_code, "WARFARIN_FLUOROQUINOLONE_BLEED");
    assert_eq!(inter.severity(), InteractionSeverity::Major);
    assert!(inter.mechanism_text.contains("CYP1A2 metabolic inhibition"));
    assert!(inter.clinical_action.contains("Preemptively reduce warfarin"));
    assert!(inter.evidence_citation.contains("22315268"));
}

#[test]
fn test_interaction_17_theophylline_ciprofloxacin() {
    let db = get_db();
    let inter = db
        .get_interaction("R03DA04", "J01MA02")
        .expect("Query failed")
        .expect("Interaction 17 missing");
    assert_eq!(inter.alert_code, "THEOPHYLLINE_CIPRO_CYP1A2_SEIZURE");
    assert_eq!(inter.severity(), InteractionSeverity::Major);
    assert!(inter.mechanism_text.contains("CYP1A2 clearance pathways"));
    assert!(inter.clinical_action.contains("status epilepticus"));
    assert!(inter.evidence_citation.contains("15535758"));
}

#[test]
fn test_interaction_18_rivaroxaban_ketoconazole() {
    let db = get_db();
    let inter = db
        .get_interaction("B01AF01", "J02AB02")
        .expect("Query failed")
        .expect("Interaction 18 missing");
    assert_eq!(inter.alert_code, "DOAC_AZOLE_DUAL_CLEARANCE_INHIB");
    assert_eq!(inter.severity(), InteractionSeverity::Contraindicated);
    assert!(inter.mechanism_text.contains("CYP3A4 degradation and P-gp"));
    assert!(inter.clinical_action.contains("fatal hemorrhagic stroke"));
    assert!(inter.evidence_citation.contains("23594271"));
}

#[test]
fn test_interaction_19_carbamazepine_ethinylestradiol() {
    let db = get_db();
    let inter = db
        .get_interaction("N03AF01", "G03AA07")
        .expect("Query failed")
        .expect("Interaction 19 missing");
    assert_eq!(inter.alert_code, "CARBAMAZEPINE_OCP_CONTRACEPTIVE_FAIL");
    assert_eq!(inter.severity(), InteractionSeverity::Major);
    assert!(inter.mechanism_text.contains("CYP3A4 and UGT"));
    assert!(inter.clinical_action.contains("Advise non-hormonal contraception"));
    assert!(inter.evidence_citation.contains("27467196"));
}

#[test]
fn test_interaction_20_cyclosporine_st_johns_wort() {
    let db = get_db();
    let inter = db
        .get_interaction("L04AD01", "A13A")
        .expect("Query failed")
        .expect("Interaction 20 missing");
    assert_eq!(inter.alert_code, "CYCLOSPORINE_STJOHNS_ORGAN_REJECT");
    assert_eq!(inter.severity(), InteractionSeverity::Contraindicated);
    assert!(inter.mechanism_text.contains("Hyperforin"));
    assert!(inter.clinical_action.contains("acute allograft rejection"));
    assert!(inter.evidence_citation.contains("10683003"));
}

#[test]
fn test_interaction_21_colchicine_clarithromycin() {
    let db = get_db();
    let inter = db
        .get_interaction("M04AC01", "J01FA09")
        .expect("Query failed")
        .expect("Interaction 21 missing");
    assert_eq!(inter.alert_code, "COLCHICINE_MACROLIDE_FATAL_TOXICITY");
    assert_eq!(inter.severity(), InteractionSeverity::Contraindicated);
    assert!(inter.mechanism_text.contains("CYP3A4 metabolism and P-gp"));
    assert!(inter.clinical_action.contains("multiorgan failure"));
    assert!(inter.evidence_citation.contains("20516676"));
}

#[test]
fn test_interaction_22_levothyroxine_ferrous_sulfate() {
    let db = get_db();
    let inter = db
        .get_interaction("H03AA01", "B03AA07")
        .expect("Query failed")
        .expect("Interaction 22 missing");
    assert_eq!(inter.alert_code, "LEVOTHYROXINE_IRON_CHELATION_HYPO");
    assert_eq!(inter.severity(), InteractionSeverity::Moderate);
    assert!(inter.mechanism_text.contains("Intraluminal chelation"));
    assert!(inter.clinical_action.contains("Separate oral administration by at least 4 hours"));
    assert!(inter.evidence_citation.contains("25110870"));
}

#[test]
fn test_interaction_23_atorvastatin_gemfibrozil() {
    let db = get_db();
    let inter = db
        .get_interaction("C10AA05", "C10AB04")
        .expect("Query failed")
        .expect("Interaction 23 missing");
    assert_eq!(inter.alert_code, "STATIN_FIBRATE_OATP1B1_RHABDOMYO");
    assert_eq!(inter.severity(), InteractionSeverity::Major);
    assert!(inter.mechanism_text.contains("OATP1B1"));
    assert!(inter.clinical_action.contains("fenofibrate preferred"));
    assert!(inter.evidence_citation.contains("17448743"));
}

#[test]
fn test_interaction_24_verapamil_atenolol() {
    let db = get_db();
    let inter = db
        .get_interaction("C08DB01", "C07AB03")
        .expect("Query failed")
        .expect("Interaction 24 missing");
    assert_eq!(inter.alert_code, "NONDIHYDRO_CCB_BB_AV_BLOCK");
    assert_eq!(inter.severity(), InteractionSeverity::Major);
    assert!(inter.mechanism_text.contains("negative chronotropic"));
    assert!(inter.clinical_action.contains("complete heart block"));
    assert!(inter.evidence_citation.contains("23166211"));
}

#[test]
fn test_interaction_25_citalopram_ondansetron() {
    let db = get_db();
    let inter = db
        .get_interaction("N06AB04", "A04AA01")
        .expect("Query failed")
        .expect("Interaction 25 missing");
    assert_eq!(inter.alert_code, "SSRI_5HT3_QT_SUMMATION_ARRHYTHMIA");
    assert_eq!(inter.severity(), InteractionSeverity::Major);
    assert!(inter.mechanism_text.contains("hERG potassium channels"));
    assert!(inter.clinical_action.contains("Limit citalopram to 20 mg/day"));
    assert!(inter.evidence_citation.contains("24352720"));
}

#[test]
fn test_interaction_26_tacrolimus_voriconazole() {
    let db = get_db();
    let inter = db
        .get_interaction("L04AD02", "J02AC03")
        .expect("Query failed")
        .expect("Interaction 26 missing");
    assert_eq!(inter.alert_code, "TACROLIMUS_VORICONAZOLE_NEPHROTOX");
    assert_eq!(inter.severity(), InteractionSeverity::Major);
    assert!(inter.mechanism_text.contains("CYP3A4 by voriconazole"));
    assert!(inter.clinical_action.contains("reduce tacrolimus dose by 66%"));
    assert!(inter.evidence_citation.contains("17391136"));
}

#[test]
fn test_interaction_27_tamoxifen_paroxetine() {
    let db = get_db();
    let inter = db
        .get_interaction("L02BA01", "N06AB05")
        .expect("Query failed")
        .expect("Interaction 27 missing");
    assert_eq!(inter.alert_code, "TAMOXIFEN_PAROXETINE_CYP2D6_FAILURE");
    assert_eq!(inter.severity(), InteractionSeverity::Major);
    assert!(inter.mechanism_text.contains("CYP2D6 by paroxetine"));
    assert!(inter.clinical_action.contains("Select venlafaxine"));
    assert!(inter.evidence_citation.contains("20142325"));
}

#[test]
fn test_interaction_28_phenytoin_valproic_acid() {
    let db = get_db();
    let inter = db
        .get_interaction("N03AB02", "N03AG01")
        .expect("Query failed")
        .expect("Interaction 28 missing");
    assert_eq!(inter.alert_code, "PHENYTOIN_VALPROATE_PROTEIN_DISPLACE");
    assert_eq!(inter.severity(), InteractionSeverity::Major);
    assert!(inter.mechanism_text.contains("albumin binding sites"));
    assert!(inter.clinical_action.contains("Monitor free serum phenytoin"));
    assert!(inter.evidence_citation.contains("28267202"));
}

#[test]
fn test_interaction_29_ramipril_sacubitril_valsartan() {
    let db = get_db();
    let inter = db
        .get_interaction("C09AA05", "C09DX04")
        .expect("Query failed")
        .expect("Interaction 29 missing");
    assert_eq!(inter.alert_code, "NEPRILYSIN_ACEI_FATAL_ANGIOEDEMA");
    assert_eq!(inter.severity(), InteractionSeverity::Contraindicated);
    assert!(inter.mechanism_text.contains("neprilysin and angiotensin-converting enzyme"));
    assert!(inter.clinical_action.contains("Enforce 36h washout"));
    assert!(inter.evidence_citation.contains("25176015"));
}

#[test]
fn test_interaction_30_morphine_diazepam() {
    let db = get_db();
    let inter = db
        .get_interaction("N02AA01", "N05BA01")
        .expect("Query failed")
        .expect("Interaction 30 missing");
    assert_eq!(inter.alert_code, "OPIOID_BENZO_FATAL_RESP_DEPRESSION");
    assert_eq!(inter.severity(), InteractionSeverity::Major);
    assert!(inter.mechanism_text.contains("respiratory center neurons"));
    assert!(inter.clinical_action.contains("co-prescribe naloxone"));
    assert!(inter.evidence_citation.contains("28292881"));
}

// =========================================================================
// COMMUTATIVE ORDER INVARIANCE & FAIL-CLOSED SAFETY GATES
// =========================================================================

#[test]
fn test_commutative_order_invariance_all_30() {
    let db = get_db();
    let rules = db.list_all_interactions().expect("List interactions");
    assert!(rules.len() >= 30);

    for r in &rules {
        let fwd = db
            .get_interaction(&r.drug_a_atc, &r.drug_b_atc)
            .expect("Fwd query")
            .expect("Fwd found");
        let rev = db
            .get_interaction(&r.drug_b_atc, &r.drug_a_atc)
            .expect("Rev query")
            .expect("Rev found");

        assert_eq!(fwd.alert_code, rev.alert_code);
        assert_eq!(fwd.severity_level, rev.severity_level);
        assert_eq!(fwd.interaction_id, rev.interaction_id);
    }
}

#[test]
fn test_fail_closed_unmodeled_drug() {
    let db = get_db();
    // 1. Single pair check with unmodeled drug
    let res = db
        .check_interaction_pair("NovelSyntheticOpioid-99", "Warfarin")
        .expect("Check pair");
    match res {
        InteractionCheckResult::UnverifiedDrug { drug, reason } => {
            assert_eq!(drug, "NovelSyntheticOpioid-99");
            assert!(reason.contains("not modeled"));
        }
        other => panic!("Expected UnverifiedDrug, got {:?}", other),
    }

    // 2. Multi-drug profile with unmodeled drug
    let profile = ["Warfarin", "NovelSyntheticOpioid-99", "Amiodarone"];
    let screen = db.screen_patient_profile(&profile).expect("Screen profile");
    assert!(!screen.is_safe);
    assert_eq!(screen.verdict, InteractionCheckVerdict::UnverifiedReview);
    assert_eq!(screen.primary_alert_code, UNVERIFIED_DRUG_REQUIRES_PHARMACIST_REVIEW);
    assert!(screen.primary_rationale.contains("NovelSyntheticOpioid-99"));
}

#[test]
fn test_fail_closed_unmodeled_combination() {
    let db = get_db();
    // Colchicine and Warfarin are both known modeled drugs, but their specific pair is unmodeled
    let res = db.check_interaction_pair("Colchicine", "Warfarin").expect("Check pair");
    match res {
        InteractionCheckResult::UnverifiedCombination { drug_a, drug_b, reason } => {
            assert!(
                (drug_a == "Colchicine" && drug_b == "Warfarin")
                    || (drug_a == "Warfarin" && drug_b == "Colchicine")
            );
            assert!(reason.contains("not explicitly modeled"));
        }
        other => panic!("Expected UnverifiedCombination, got {:?}", other),
    }

    // Profile screen must also return UNVERIFIED_DRUG_REQUIRES_PHARMACIST_REVIEW
    let screen = db.screen_patient_profile(&["Colchicine", "Warfarin"]).expect("Screen");
    assert!(!screen.is_safe);
    assert_eq!(screen.verdict, InteractionCheckVerdict::UnverifiedReview);
    assert_eq!(screen.primary_alert_code, UNVERIFIED_DRUG_REQUIRES_PHARMACIST_REVIEW);
}

#[test]
fn test_verified_safe_co_prescriptions() {
    let db = get_db();
    // Metformin + Simvastatin is a recognized safe combination in diabetic dyslipidemia
    let screen1 = db.screen_patient_profile(&["Metformin", "Simvastatin"]).expect("Screen");
    assert!(screen1.is_safe);
    assert_eq!(screen1.primary_alert_code, "METFORMIN_SIMVASTATIN_COADMINISTRATION_SAFE");

    // Paracetamol + Amoxicillin is a verified safe analgesic + antibiotic co-prescription
    let screen2 = db.screen_patient_profile(&["Paracetamol", "Amoxicillin"]).expect("Screen");
    assert!(screen2.is_safe);
    assert_eq!(screen2.primary_alert_code, "PARACETAMOL_AMOXICILLIN_SAFE");
}

#[test]
fn test_multi_drug_active_patient_profile_screening() {
    let db = get_db();
    // Regimen with 2 interacting drugs: Warfarin, Amiodarone
    let profile = ["Warfarin", "Amiodarone"];
    let screen = db.screen_patient_profile(&profile).expect("Screen");
    assert!(!screen.is_safe);
    assert_eq!(screen.verdict, InteractionCheckVerdict::MajorWarning);
    assert_eq!(screen.primary_alert_code, "WARFARIN_AMIODARONE_CYP2C9_INHIB");
    assert_eq!(screen.interactions_detected.len(), 1);

    // Regimen with Contraindicated pair: Simvastatin + Clarithromycin
    let profile_contra = ["Simvastatin", "Clarithromycin"];
    let screen_c = db.screen_patient_profile(&profile_contra).expect("Screen");
    assert!(!screen_c.is_safe);
    assert_eq!(screen_c.verdict, InteractionCheckVerdict::ContraindicatedBlock);
    assert_eq!(screen_c.primary_alert_code, "STATIN_MACROLIDE_CYP3A4_RHABDO");

    // Regimen with multi-drug containing unmodeled combination: Metformin + Warfarin + Lisinopril
    let profile_unmodeled = ["Metformin", "Warfarin", "Lisinopril"];
    let screen_u = db.screen_patient_profile(&profile_unmodeled).expect("Screen");
    assert!(!screen_u.is_safe);
    assert_eq!(screen_u.verdict, InteractionCheckVerdict::UnverifiedReview);
    assert_eq!(screen_u.primary_alert_code, UNVERIFIED_DRUG_REQUIRES_PHARMACIST_REVIEW);
}

#[test]
fn test_query_latency_sub_millisecond_benchmark() {
    let db = get_db();
    let queries = [
        "Lisinopril", "Warfarin", "Amiodarone", "Simvastatin",
        "Clarithromycin", "Metformin", "Ciprofloxacin", "Ibuprofen",
        "Omeprazole", "Digoxin"
    ];

    let iterations = 1000;
    let t0 = Instant::now();
    for i in 0..iterations {
        let q = queries[i % queries.len()];
        let drug = db.lookup_drug(q).expect("Lookup");
        assert!(drug.is_some());
    }
    let elapsed = t0.elapsed();
    let avg_ms = elapsed.as_secs_f64() * 1000.0 / (iterations as f64);
    println!("Average indexed lookup latency over {} queries: {:.4} ms", iterations, avg_ms);
    assert!(
        avg_ms < 0.1,
        "Lookup latency {:.4} ms exceeds <0.1 ms threshold!",
        avg_ms
    );
}

#[test]
fn test_medical_conditions_red_flag_priorities() {
    let db = get_db();
    // Acute MI: Red-flag emergency
    let mi = db.get_medical_condition("I21.9").expect("Query").expect("Found");
    assert!(mi.red_flag_priority);
    assert_eq!(mi.category, "Cardiovascular");
    assert_eq!(mi.condition_name_ar, "احتشاء عضلة القلب الحاد");

    // Acute Stroke: Red-flag emergency
    let stroke = db.get_medical_condition("I63.9").expect("Query").expect("Found");
    assert!(stroke.red_flag_priority);
    assert_eq!(stroke.category, "Neurology");

    // Hypertension: Routine care (not red-flag emergency)
    let htn = db.get_medical_condition("I10").expect("Query").expect("Found");
    assert!(!htn.red_flag_priority);
}

#[test]
fn test_lab_diagnostic_panels_and_panic_limits() {
    let db = get_db();
    // Serum Potassium LOINC: 2823-3
    let k = db.get_lab_panel("2823-3").expect("Query").expect("Found");
    assert_eq!(k.standard_unit, "mmol/L");
    assert_eq!(k.reference_low, Some(3.5));
    assert_eq!(k.reference_high, Some(5.0));
    assert_eq!(k.critical_panic_low, Some(2.8));
    assert_eq!(k.critical_panic_high, Some(6.2));

    // eGFR LOINC: 33914-3
    let egfr = db.get_lab_panel("33914-3").expect("Query").expect("Found");
    assert_eq!(egfr.critical_panic_low, Some(15.0));
}

#[test]
fn test_hitl_clinical_quarantine_lifecycle() {
    let db = get_db();
    let staging_id = db
        .insert_hitl_finding(
            "https://pubmed.ncbi.nlm.nih.gov/99999999",
            "Novel interaction observed between DrugX and DrugY",
            Some("L01XX"),
            Some(r#"{"drug_a":"DrugX","drug_b":"DrugY","severity":"MAJOR"}"#),
            Some("99999999"),
        )
        .expect("Insert quarantine");

    let rec = db.get_hitl_record(staging_id).expect("Query").expect("Record found");
    assert_eq!(rec.review_status, "PENDING");
    assert_eq!(rec.evidence_pmid.as_deref(), Some("99999999"));
    assert!(rec.extracted_finding.contains("DrugX"));
}
