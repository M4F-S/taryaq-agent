#!/usr/bin/env python3
"""tools/build_clinical_db.py - Builds SQLite Clinical Formulary Database for Taryaq
Dubai Create AI Agents Championship 2026-2027 | Best Societal AI Agent
Specification: docs/spec/TECH_SPEC.md §2 & §3

Populates corpus/taryaq_clinical.db with:
- 106 Outpatient medications across 15 therapeutic categories
- Detailed ATC codes, CYP450 / P-gp metabolizer profiles, eGFR cutoffs
- UAE Law 30/2021 Controlled Medication Schedules
- High-yield pharmacovigilance contraindications & drug interactions
- Renal dosing adjustment guidelines by eGFR
- Clinical encounters schema
"""

import os
import sys
import json
import sqlite3
import hashlib

ROOT_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
CORPUS_DIR = os.path.join(ROOT_DIR, "corpus")
CLINICAL_DB_PATH = os.path.join(CORPUS_DIR, "taryaq_clinical.db")

# Helper to compute passage hash for evidence provenance
def hash_evidence(text: str) -> str:
    return hashlib.sha256(text.encode("utf-8")).hexdigest()

DRUG_CLASSES_DATA = [
    (1, "ACE Inhibitors", "Cardiovascular", "NONE"),
    (2, "Angiotensin II Receptor Blockers (ARBs)", "Cardiovascular", "NONE"),
    (3, "Calcium Channel Blockers (DHP)", "Cardiovascular", "SUBSTRATE"),
    (4, "Calcium Channel Blockers (Non-DHP)", "Cardiovascular", "INHIBITOR"),
    (5, "Beta-Adrenergic Blockers", "Cardiovascular", "SUBSTRATE"),
    (6, "Thiazide & Related Diuretics", "Cardiovascular", "NONE"),
    (7, "Loop Diuretics", "Heart Failure & Diuretics", "NONE"),
    (8, "Mineralocorticoid Receptor Antagonists", "Heart Failure & Diuretics", "NONE"),
    (9, "Cardiac Glycosides", "Heart Failure & Diuretics", "NONE"),
    (10, "Angiotensin Receptor-Neprilysin Inhibitors", "Heart Failure & Diuretics", "NONE"),
    (11, "HMG-CoA Reductase Inhibitors (Statins)", "Lipid-Lowering", "SUBSTRATE"),
    (12, "Cholesterol Absorption Inhibitors & Fibrates", "Lipid-Lowering", "NONE"),
    (13, "Biguanides", "Antidiabetics", "NONE"),
    (14, "Sulfonylureas", "Antidiabetics", "SUBSTRATE"),
    (15, "DPP-4 Inhibitors", "Antidiabetics", "NONE"),
    (16, "SGLT2 Inhibitors", "Antidiabetics", "NONE"),
    (17, "GLP-1 Receptor Agonists", "Antidiabetics", "NONE"),
    (18, "Insulins", "Antidiabetics", "NONE"),
    (19, "Vitamin K Antagonists", "Antithrombotics & Anticoagulants", "SUBSTRATE"),
    (20, "Direct Oral Anticoagulants (DOACs)", "Antithrombotics & Anticoagulants", "SUBSTRATE"),
    (21, "Antiplatelet Agents", "Antithrombotics & Anticoagulants", "SUBSTRATE"),
    (22, "Antiarrhythmics", "Antiarrhythmics", "INHIBITOR"),
    (23, "Anti-Gout & Uricosurics", "Anti-Gout & Hyperuricemia", "SUBSTRATE"),
    (24, "Penicillins & Beta-Lactams", "Antimicrobials", "NONE"),
    (25, "Cephalosporins", "Antimicrobials", "NONE"),
    (26, "Fluoroquinolones", "Antimicrobials", "INHIBITOR"),
    (27, "Macrolides", "Antimicrobials", "INHIBITOR"),
    (28, "Tetracyclines & Other Antibacterials", "Antimicrobials", "NONE"),
    (29, "Antifungals (Azoles)", "Antimicrobials", "INHIBITOR"),
    (30, "Non-Opioid Analgesics & Antipyretics", "Analgesics & NSAIDs", "NONE"),
    (31, "Non-Steroidal Anti-Inflammatory Drugs (NSAIDs)", "Analgesics & NSAIDs", "SUBSTRATE"),
    (32, "Controlled Opioid Analgesics", "Controlled Analgesics & Opioids (Law 30/2021)", "SUBSTRATE"),
    (33, "Benzodiazepines & Sedatives", "Psychotropics & Sedatives (Law 30/2021)", "SUBSTRATE"),
    (34, "Gabapentinoids", "Psychotropics & Sedatives (Law 30/2021)", "NONE"),
    (35, "Antidepressants (SSRIs & SNRIs)", "Antidepressants", "INHIBITOR"),
    (36, "Antidepressants (TCAs & Others)", "Antidepressants", "SUBSTRATE"),
    (37, "Proton Pump Inhibitors & H2 Blockers", "Gastroenterology", "INHIBITOR"),
    (38, "GI Prokinetics & Antiemetics", "Gastroenterology", "SUBSTRATE"),
    (39, "Beta-2 Agonists & Bronchodilators", "Respiratory", "NONE"),
    (40, "Inhaled Corticosteroids & Combinations", "Respiratory", "SUBSTRATE"),
    (41, "Anticholinergics & Leukotriene Antagonists", "Respiratory", "NONE"),
    (42, "Thyroid & Antithyroid Agents", "Endocrine & Steroids", "NONE"),
    (43, "Systemic Corticosteroids", "Endocrine & Steroids", "INDUCER"),
    (44, "Immunosuppressants & Antimetabolites", "Immunology & Oncology", "SUBSTRATE"),
    (45, "Antiepileptics & Mood Stabilizers", "Neurology & Psychiatry", "INDUCER"),
    (46, "Vasodilators & Nitrates", "Cardiovascular", "NONE"),
    (47, "Phosphodiesterase-5 Inhibitors", "Urology", "SUBSTRATE"),
    (48, "Diagnostic Radiopaque Media", "Diagnostic Agents", "NONE"),
    (49, "Minerals & Electrolytes", "Nutrition & Electrolytes", "NONE"),
    (50, "Oral Contraceptives & Estrogens", "Endocrine & Reproduction", "SUBSTRATE"),
    (51, "Herbal & Dietary Supplements", "Complementary Medicine", "INDUCER"),
    (52, "Xanthines & Bronchodilators", "Respiratory", "SUBSTRATE"),
    (53, "Selective Estrogen Receptor Modulators", "Oncology & Endocrine", "SUBSTRATE"),
    (54, "5-HT3 Receptor Antagonists", "Gastroenterology", "SUBSTRATE"),
]

# 106 Drugs definitions:
# canonical_name, arabic_name, brand_names, atc_code, class_id, renal_egfr_cutoff, hepatic_pct, max_daily_dose_mg,
# is_controlled, schedule_law_30, narrow_therapeutic, cyp3a4, cyp2d6, cyp2c9, p_gp, qt_risk, hyperk, serotonin, indications, pearls
DRUGS_DATA = [
    # 1. Cardiovascular / Antihypertensives (14)
    ("Lisinopril", "ليزينوبريل", ["Zestril", "Prinivil"], "C09AA03", 1, 0.0, 0.0, 40.0, 0, None, 0, "NONE", "NONE", "NONE", "NONE", "NONE", 1, 0, ["I10", "I50.9"], "ACE inhibitor; monitor eGFR and serum potassium; risk of hyperkalemia"),
    ("Ramipril", "راميبريل", ["Tritace", "Altace"], "C09AA05", 1, 0.0, 20.0, 10.0, 0, None, 0, "NONE", "NONE", "NONE", "NONE", "NONE", 1, 0, ["I10", "I25.9"], "ACE inhibitor with cardioprotective data (HOPE trial); check renal function"),
    ("Enalapril", "إنالابريل", ["Renitec", "Vasotec"], "C09AA02", 1, 0.0, 0.0, 40.0, 0, None, 0, "NONE", "NONE", "NONE", "NONE", "NONE", 1, 0, ["I10", "I50.9"], "Prodrug converted to enalaprilat; dose reduction if eGFR < 30"),
    ("Losartan", "لوسارتان", ["Cozaar"], "C09CA01", 2, 0.0, 65.0, 100.0, 0, None, 0, "NONE", "NONE", "SUBSTRATE", "NONE", "NONE", 1, 0, ["I10", "E11.22"], "ARB with modest uricosuric effect; ideal for hypertensive patients with gout"),
    ("Valsartan", "فالسارتان", ["Diovan"], "C09CA03", 2, 0.0, 70.0, 320.0, 0, None, 0, "NONE", "NONE", "NONE", "NONE", "NONE", 1, 0, ["I10", "I50.9"], "ARB; monitor potassium in combination with potassium-sparing diuretics"),
    ("Candesartan", "كانديسارتان", ["Atacand"], "C09CA06", 2, 0.0, 60.0, 32.0, 0, None, 0, "NONE", "NONE", "NONE", "NONE", "NONE", 1, 0, ["I10", "I50.9"], "Potent AT1 receptor antagonist with tight binding; CHARM trial evidence in HF"),
    ("Amlodipine", "أملوديبين", ["Norvasc", "Amlor"], "C08CA01", 3, 0.0, 90.0, 10.0, 0, None, 0, "SUBSTRATE", "NONE", "NONE", "NONE", "NONE", 0, 0, ["I10", "I20.9"], "Dihydropyridine CCB; safe in renal impairment; caps Simvastatin dose at 20mg"),
    ("Diltiazem", "ديلتيازيم", ["Cardizem", "Dilzem"], "C08DB01", 4, 0.0, 80.0, 360.0, 0, None, 0, "INHIBITOR", "NONE", "NONE", "INHIBITOR", "NONE", 0, 0, ["I48.9", "I20.9"], "Non-DHP CCB; negative inotrope/chronotrope; moderate CYP3A4 and P-gp inhibitor"),
    ("Verapamil", "فيراباميل", ["Isoptin", "Calan"], "C08DA01", 4, 0.0, 80.0, 480.0, 0, None, 0, "INHIBITOR", "NONE", "NONE", "INHIBITOR", "NONE", 0, 0, ["I48.9", "I47.1"], "Potent P-gp and CYP3A4 inhibitor; markedly raises digoxin and dabigatran levels"),
    ("Metoprolol", "ميتوبرولول", ["Betaloc", "Lopressor", "Toprol-XL"], "C07AB02", 5, 0.0, 95.0, 200.0, 0, None, 0, "NONE", "SUBSTRATE", "NONE", "NONE", "NONE", 0, 0, ["I10", "I25.9", "I50.9"], "Cardioselective beta-1 blocker; CYP2D6 substrate; caution with fluoxetine/paroxetine"),
    ("Bisoprolol", "بيسوبرولول", ["Concor"], "C07AB07", 5, 0.0, 50.0, 20.0, 0, None, 0, "NONE", "NONE", "NONE", "NONE", "NONE", 0, 0, ["I10", "I50.9"], "Balanced 50/50 renal/hepatic clearance; highly cardioselective beta blocker"),
    ("Carvedilol", "كارفيديلول", ["Dilatrend", "Coreg"], "C07AG02", 5, 0.0, 98.0, 50.0, 0, None, 0, "NONE", "SUBSTRATE", "NONE", "INHIBITOR", "NONE", 0, 0, ["I50.9", "I10"], "Non-selective beta plus alpha-1 blocker; antioxidant properties in heart failure"),
    ("Hydrochlorothiazide", "هيدروكلوروثيازيد", ["Esidrex"], "C03AA03", 6, 30.0, 0.0, 50.0, 0, None, 0, "NONE", "NONE", "NONE", "NONE", "NONE", 0, 0, ["I10", "I50.9"], "Thiazide diuretic; loses natriuretic efficacy when eGFR < 30 mL/min; causes hypokalemia"),
    ("Indapamide", "إنداباميد", ["Natrilix"], "C03BA11", 6, 0.0, 70.0, 2.5, 0, None, 0, "NONE", "NONE", "NONE", "NONE", "NONE", 0, 0, ["I10"], "Thiazide-like diuretic; metabolic neutral; retains efficacy at lower eGFR than HCTZ"),

    # 2. Heart Failure & Diuretics (6)
    ("Furosemide", "فوروسيميد", ["Lasix"], "C03CA01", 7, 0.0, 30.0, 600.0, 0, None, 0, "NONE", "NONE", "NONE", "NONE", "NONE", 0, 0, ["I50.9", "N18.9"], "Potent loop diuretic; effective in advanced renal failure; monitor electrolytes"),
    ("Torsemide", "تورسيميد", ["Diuver", "Demadex"], "C03CA04", 7, 0.0, 80.0, 200.0, 0, None, 0, "NONE", "NONE", "SUBSTRATE", "NONE", "NONE", 0, 0, ["I50.9"], "Loop diuretic with higher and more predictable oral bioavailability (80%) than furosemide"),
    ("Spironolactone", "سبيرونولاكتون", ["Aldactone"], "C03DA01", 8, 30.0, 90.0, 100.0, 0, None, 0, "NONE", "NONE", "NONE", "NONE", "NONE", 1, 0, ["I50.9", "I10"], "Aldosterone antagonist; contraindicated if eGFR < 30 or baseline K+ > 5.0 mEq/L"),
    ("Eplerenone", "إبليرينون", ["Inspra"], "C03DA04", 8, 30.0, 95.0, 50.0, 0, None, 0, "SUBSTRATE", "NONE", "NONE", "NONE", "NONE", 1, 0, ["I50.9", "I21.9"], "Selective aldosterone blocker; CYP3A4 substrate; contra-indicated with strong 3A4 inhibitors"),
    ("Digoxin", "ديجوكسين", ["Lanoxin"], "C01AA05", 9, 15.0, 20.0, 0.25, 0, None, 1, "NONE", "NONE", "NONE", "SUBSTRATE", "NONE", 0, 0, ["I48.9", "I50.9"], "Narrow therapeutic index; renally eliminated; P-gp substrate; toxic with amiodarone/verapamil"),
    ("Sacubitril/Valsartan", "ساكوبيتريل/فالسارتان", ["Entresto"], "C09DX04", 10, 15.0, 40.0, 400.0, 0, None, 0, "NONE", "NONE", "NONE", "NONE", "NONE", 1, 0, ["I50.9"], "ARNI; requires 36-hour washout when switching from ACE inhibitors to avoid angioedema"),

    # 3. Lipid-Lowering (6)
    ("Atorvastatin", "أتورفاستاتين", ["Lipitor"], "C10AA05", 11, 0.0, 98.0, 80.0, 0, None, 0, "SUBSTRATE", "NONE", "NONE", "SUBSTRATE", "NONE", 0, 0, ["E78.0", "I25.9"], "High-intensity statin; CYP3A4 substrate; avoid excessive grapefruit juice"),
    ("Rosuvastatin", "روستوفاستاتين", ["Crestor"], "C10AA07", 11, 0.0, 90.0, 40.0, 0, None, 0, "NONE", "NONE", "SUBSTRATE", "NONE", "NONE", 0, 0, ["E78.0", "I25.9"], "High-intensity statin; minimal CYP3A4 metabolism; start at 5mg in severe CKD"),
    ("Simvastatin", "سيمفاستاتين", ["Zocor"], "C10AA01", 11, 30.0, 85.0, 40.0, 0, None, 0, "SUBSTRATE", "NONE", "NONE", "SUBSTRATE", "NONE", 0, 0, ["E78.0"], "Sensitive CYP3A4 substrate; CONTRAINDICATED with clarithromycin/ketoconazole (rhabdomyolysis)"),
    ("Pravastatin", "برافاستاتين", ["Pravachol"], "C10AA03", 11, 0.0, 50.0, 80.0, 0, None, 0, "NONE", "NONE", "NONE", "NONE", "NONE", 0, 0, ["E78.0"], "Hydrophilic statin cleared via sulfation; lowest drug-drug interaction liability"),
    ("Ezetimibe", "إيزيتيميب", ["Ezetrol", "Zetia"], "C10AX09", 12, 0.0, 90.0, 10.0, 0, None, 0, "NONE", "NONE", "NONE", "NONE", "NONE", 0, 0, ["E78.0"], "NPC1L1 cholesterol absorption inhibitor; synergistic with statins; no renal adjustment"),
    ("Fenofibrate", "فينوفيبرات", ["Lipanthyl", "Tricor"], "C10AB05", 12, 30.0, 0.0, 200.0, 0, None, 0, "NONE", "NONE", "INHIBITOR", "NONE", "NONE", 0, 0, ["E78.1"], "Triglyceride lowering; contra-indicated in severe CKD (eGFR < 30); causes reversible Cr elevation"),

    # 4. Antidiabetics (12)
    ("Metformin", "ميتفورمين", ["Glucophage", "Cidophage"], "A10BA02", 13, 30.0, 0.0, 2550.0, 0, None, 0, "NONE", "NONE", "NONE", "NONE", "NONE", 0, 0, ["E11.9"], "First-line T2D; absolute contraindication at eGFR < 30 (lactic acidosis); max 1000mg in 3b"),
    ("Gliclazide", "جليكلازيد", ["Diamicron"], "A10BB09", 14, 15.0, 95.0, 120.0, 0, None, 0, "NONE", "NONE", "SUBSTRATE", "NONE", "NONE", 0, 0, ["E11.9"], "Sulfonylurea with lower hypoglycemia risk than glibenclamide; hepatically cleared"),
    ("Glimepiride", "جليميبيريد", ["Amaryl"], "A10BB12", 14, 30.0, 95.0, 8.0, 0, None, 0, "NONE", "NONE", "SUBSTRATE", "NONE", "NONE", 0, 0, ["E11.9"], "Long-acting sulfonylurea; caution in renal impairment due to active metabolite accumulation"),
    ("Sitagliptin", "سيتاجليبتين", ["Januvia"], "A10BH01", 15, 0.0, 20.0, 100.0, 0, None, 0, "NONE", "NONE", "NONE", "SUBSTRATE", "NONE", 0, 0, ["E11.9"], "DPP-4 inhibitor; renal dose reduction required: 50mg if eGFR 30-45, 25mg if < 30"),
    ("Linagliptin", "ليناجليبتين", ["Trajenta"], "A10BH05", 15, 0.0, 95.0, 5.0, 0, None, 0, "SUBSTRATE", "NONE", "NONE", "SUBSTRATE", "NONE", 0, 0, ["E11.9"], "Biliary excretion (95%); no dose adjustment needed across all stages of CKD"),
    ("Vildagliptin", "فيلداجليبتين", ["Galvus"], "A10BH02", 15, 0.0, 85.0, 100.0, 0, None, 0, "NONE", "NONE", "NONE", "NONE", "NONE", 0, 0, ["E11.9"], "Reduce dose to 50mg once daily in moderate-to-severe renal impairment; monitor LFTs"),
    ("Empagliflozin", "إمباغليفلوزين", ["Jardiance"], "A10BK03", 16, 20.0, 50.0, 25.0, 0, None, 0, "NONE", "NONE", "NONE", "NONE", "NONE", 0, 0, ["E11.9", "I50.9"], "SGLT2 inhibitor; cardioprotective & nephroprotective; glycemic efficacy declines < 45"),
    ("Dapagliflozin", "داباجليفلوزين", ["Forxiga"], "A10BK01", 16, 25.0, 50.0, 10.0, 0, None, 0, "NONE", "NONE", "NONE", "NONE", "NONE", 0, 0, ["E11.9", "I50.9", "N18.9"], "SGLT2 inhibitor approved for CKD and HFrEF/HFpEF down to eGFR 25 mL/min"),
    ("Semaglutide", "سيماجلوتايد", ["Ozempic", "Rybelsus", "Wegovy"], "A10BJ06", 17, 15.0, 95.0, 2.0, 0, None, 0, "NONE", "NONE", "NONE", "NONE", "NONE", 0, 0, ["E11.9", "E66.9"], "GLP-1 RA; dramatic HbA1c/weight reduction; FLOW trial proven renal protection"),
    ("Dulaglutide", "دولاغلوتايد", ["Trulicity"], "A10BJ05", 17, 15.0, 95.0, 4.5, 0, None, 0, "NONE", "NONE", "NONE", "NONE", "NONE", 0, 0, ["E11.9"], "Once-weekly GLP-1 RA; no renal dose titration required down to eGFR 15"),
    ("Insulin Glargine", "إنسولين جلارجين", ["Lantus", "Toujeo"], "A10AE04", 18, 0.0, 50.0, 200.0, 0, None, 0, "NONE", "NONE", "NONE", "NONE", "NONE", 0, 0, ["E11.9", "E10.9"], "Long-acting basal insulin analog; reduce dose as eGFR falls due to reduced insulin clearance"),
    ("Insulin Aspart", "إنسولين أسبارت", ["Novorapid"], "A10AB05", 18, 0.0, 50.0, 150.0, 0, None, 0, "NONE", "NONE", "NONE", "NONE", "NONE", 0, 0, ["E11.9", "E10.9"], "Rapid-acting prandial insulin; dose adjust according to carb counting and renal clearance"),

    # 5. Antithrombotics & Anticoagulants (7)
    ("Warfarin", "وارفارين", ["Coumadin", "Marevan"], "B01AA03", 19, 15.0, 99.0, 10.0, 0, None, 1, "NONE", "NONE", "SUBSTRATE", "NONE", "NONE", 0, 0, ["I48.9", "I82.9"], "Vitamin K antagonist; narrow therapeutic index; CYP2C9 substrate; extreme risk with amiodarone"),
    ("Dabigatran", "دابيجاتران", ["Pradaxa"], "B01AE07", 20, 30.0, 20.0, 300.0, 0, None, 0, "NONE", "NONE", "NONE", "SUBSTRATE", "NONE", 0, 0, ["I48.9"], "Direct thrombin inhibitor; 80% renal elimination; contraindicated if eGFR < 30"),
    ("Rivaroxaban", "ريفاروكسابان", ["Xarelto"], "B01AF01", 20, 15.0, 65.0, 20.0, 0, None, 0, "SUBSTRATE", "NONE", "NONE", "SUBSTRATE", "NONE", 0, 0, ["I48.9", "I26.9"], "Factor Xa inhibitor; dual clearance; reduce to 15mg once daily if eGFR 15-49 mL/min"),
    ("Apixaban", "أبيكسابان", ["Eliquis"], "B01AF02", 20, 15.0, 75.0, 10.0, 0, None, 0, "SUBSTRATE", "NONE", "NONE", "SUBSTRATE", "NONE", 0, 0, ["I48.9"], "Preferred DOAC in CKD (only 27% renal clearance); reduce to 2.5mg BID if 2 of: age>=80, wt<=60, Cr>=1.5"),
    ("Clopidogrel", "كلوبيدوجريل", ["Plavix"], "B01AC04", 21, 0.0, 50.0, 75.0, 0, None, 0, "NONE", "NONE", "NONE", "NONE", "NONE", 0, 0, ["I25.9", "I63.9"], "P2Y12 antiplatelet prodrug; activated by CYP2C19; reduced efficacy with omeprazole"),
    ("Aspirin", "أسبرين", ["Aspocid", "Bayer"], "B01AC06", 21, 10.0, 90.0, 325.0, 0, None, 0, "NONE", "NONE", "NONE", "NONE", "NONE", 0, 0, ["I25.9", "I20.9"], "Irreversible COX-1 inhibitor; gastroprotection recommended when combined with anticoagulants"),
    ("Ticagrelor", "تيكاجريلور", ["Brilinta"], "B01AC24", 21, 0.0, 99.0, 180.0, 0, None, 0, "SUBSTRATE", "NONE", "NONE", "NONE", "NONE", 0, 0, ["I21.9"], "Direct-acting P2Y12 inhibitor; CYP3A4 substrate; avoids CYP2C19 resistance issues of clopidogrel"),

    # 6. Antiarrhythmics (4)
    ("Amiodarone", "أميودارون", ["Cordarone"], "C01BD01", 22, 15.0, 99.0, 400.0, 0, None, 1, "INHIBITOR", "INHIBITOR", "INHIBITOR", "INHIBITOR", "HIGH", 0, 0, ["I48.9", "I47.2"], "Multi-target inhibitor (CYP2C9, 2D6, 3A4, P-gp); QT prolonger; doubles warfarin and digoxin levels"),
    ("Flecainide", "فليكاينيد", ["Tambocor"], "C01BC04", 22, 35.0, 70.0, 300.0, 0, None, 1, "NONE", "SUBSTRATE", "NONE", "NONE", "MODERATE", 0, 0, ["I48.9"], "Class Ic antiarrhythmic; narrow therapeutic window; contraindicated in structural heart disease"),
    ("Sotalol", "سوتالول", ["Betapace", "Sotacor"], "C07AA07", 22, 30.0, 0.0, 320.0, 0, None, 1, "NONE", "NONE", "NONE", "NONE", "HIGH", 0, 0, ["I48.9"], "Class III antiarrhythmic with beta blockade; 100% renal elimination; extreme TdP risk if eGFR < 30"),
    ("Propafenone", "بروبافينون", ["Rytmonorm"], "C01BC03", 22, 0.0, 95.0, 900.0, 0, None, 1, "NONE", "SUBSTRATE", "NONE", "NONE", "MODERATE", 0, 0, ["I48.9"], "Class Ic antiarrhythmic with mild beta-blocking effects; CYP2D6 dependent clearance"),

    # 7. Anti-Gout & Hyperuricemia (4)
    ("Colchicine", "كولشيسين", ["Colchicine"], "M04AC01", 23, 30.0, 80.0, 1.5, 0, None, 1, "SUBSTRATE", "NONE", "NONE", "SUBSTRATE", "NONE", 0, 0, ["M10.0"], "Fatal toxicity when combined with potent CYP3A4/P-gp inhibitors (Clarithromycin) in CKD"),
    ("Allopurinol", "ألوبيورينول", ["Zyloric"], "M04AA01", 23, 15.0, 20.0, 800.0, 0, None, 0, "NONE", "NONE", "NONE", "NONE", "NONE", 0, 0, ["M10.9"], "Active metabolite oxypurinol is renally excreted; titrate slowly in CKD; HLA-B*5801 screening"),
    ("Febuxostat", "فيبوكسوستات", ["Adenuric", "Uloric"], "M04AA03", 23, 15.0, 95.0, 120.0, 0, None, 0, "NONE", "NONE", "NONE", "NONE", "NONE", 0, 0, ["M10.9"], "Xanthine oxidase inhibitor; safe alternative in mild-to-moderate CKD without dose adjustment"),
    ("Probenecid", "بروبينسيد", ["Benemid"], "M04AB01", 23, 30.0, 95.0, 2000.0, 0, None, 0, "NONE", "NONE", "NONE", "NONE", "NONE", 0, 0, ["M10.9"], "Uricosuric; ineffective when eGFR < 30 mL/min; maintain high fluid intake to avoid urate stones"),

    # 8. Antimicrobials (14)
    ("Amoxicillin", "أموكسيسيلين", ["Amoxil"], "J01CA04", 24, 10.0, 20.0, 3000.0, 0, None, 0, "NONE", "NONE", "NONE", "NONE", "NONE", 0, 0, ["J02.9", "J01.9"], "Broad-spectrum aminopenicillin; renally cleared; reduce frequency in severe CKD"),
    ("Amoxicillin-Clavulanate", "أموكسيسيلين-كلافولانات", ["Augmentin", "Curam"], "J01CR02", 24, 30.0, 20.0, 2000.0, 0, None, 0, "NONE", "NONE", "NONE", "NONE", "NONE", 0, 0, ["J18.9", "N39.0"], "Avoid Augmentin 1g/875mg formulations if eGFR < 30 due to clavulanate accumulation"),
    ("Cefuroxime", "سيفوروكسيم", ["Zinnat", "Ceftin"], "J01DC02", 25, 20.0, 10.0, 1000.0, 0, None, 0, "NONE", "NONE", "NONE", "NONE", "NONE", 0, 0, ["J01.9", "L03.9"], "Second-generation cephalosporin; high stability against beta-lactamase; renally cleared"),
    ("Ceftriaxone", "سيفترياكسون", ["Rocephin"], "J01DD04", 25, 0.0, 50.0, 4000.0, 0, None, 0, "NONE", "NONE", "NONE", "NONE", "NONE", 0, 0, ["J18.9", "A41.9"], "Third-generation cephalosporin; dual biliary/renal elimination; no renal dose adjustment required"),
    ("Ciprofloxacin", "سيبروفلوكساسين", ["Cipro"], "J01MA02", 26, 30.0, 40.0, 1500.0, 0, None, 0, "NONE", "NONE", "NONE", "NONE", "MODERATE", 0, 0, ["N30.0", "A09.9"], "Fluoroquinolone; CYP1A2 inhibitor; chelates with polyvalent cations (iron, calcium); QT prolongation"),
    ("Levofloxacin", "ليفوفلوكساسين", ["Tavanic", "Levaquin"], "J01MA12", 26, 20.0, 15.0, 750.0, 0, None, 0, "NONE", "NONE", "NONE", "NONE", "MODERATE", 0, 0, ["J18.9", "N39.0"], "80% renal excretion unchanged; neurotoxicity / seizure risk if unadjusted in CKD"),
    ("Azithromycin", "أزيثرومايسين", ["Zithromax"], "J01FA10", 27, 0.0, 95.0, 500.0, 0, None, 0, "NONE", "NONE", "NONE", "NONE", "MODERATE", 0, 0, ["J20.9", "J02.9"], "Biliary elimination; does not inhibit CYP3A4; safe macrolide alternative to clarithromycin"),
    ("Clarithromycin", "كلاريثروميسين", ["Klacid", "Biaxin"], "J01FA09", 27, 30.0, 60.0, 1000.0, 0, None, 0, "INHIBITOR", "NONE", "NONE", "INHIBITOR", "MODERATE", 0, 0, ["J02.9", "J18.9"], "Potent mechanism-based CYP3A4 and P-gp inhibitor; contra-indicated with simvastatin and colchicine"),
    ("Doxycycline", "دوكسيسيكلين", ["Vibramycin"], "J01AA02", 28, 0.0, 90.0, 200.0, 0, None, 0, "NONE", "NONE", "NONE", "NONE", "NONE", 0, 0, ["J01.9", "L70.0"], "Tetracycline excreted via GI tract; safe in all stages of renal impairment; avoid dairy cations"),
    ("Trimethoprim-Sulfamethoxazole", "تريميثوبريم-سلفاميثوكسازول", ["Septrin", "Bactrim"], "J01EE01", 28, 15.0, 30.0, 1920.0, 0, None, 0, "NONE", "NONE", "INHIBITOR", "NONE", "NONE", 1, 0, ["N39.0", "B59"], "Inhibits tubular creatinine secretion (causes pseudonephrotoxicity); severe hyperkalemia risk with ACEIs"),
    ("Nitrofurantoin", "نيتروفورانتوين", ["Macrodantin", "Furadantin"], "J01XE01", 28, 30.0, 60.0, 400.0, 0, None, 0, "NONE", "NONE", "NONE", "NONE", "NONE", 0, 0, ["N30.0"], "Ineffective for UTI and causes peripheral neuropathy and pulmonary fibrosis if eGFR < 30 mL/min"),
    ("Metronidazole", "ميترونيدازول", ["Flagyl"], "J01XD01", 28, 0.0, 80.0, 1500.0, 0, None, 0, "NONE", "NONE", "INHIBITOR", "NONE", "NONE", 0, 0, ["A04.7", "K52.9"], "Anaerobic and antiprotozoal coverage; inhibits CYP2C9; severe disulfiram-like ethanol reaction"),
    ("Fluconazole", "فلوكونازول", ["Diflucan"], "J02AC01", 29, 20.0, 20.0, 400.0, 0, None, 0, "INHIBITOR", "NONE", "INHIBITOR", "NONE", "MODERATE", 0, 0, ["B37.0"], "Potent CYP2C9 and moderate 3A4 inhibitor; renally excreted; cut dose by 50% if eGFR < 50"),
    ("Itraconazole", "إيتراكونازول", ["Sporanox"], "J02AC02", 29, 30.0, 95.0, 400.0, 0, None, 0, "INHIBITOR", "NONE", "NONE", "INHIBITOR", "MODERATE", 0, 0, ["B35.1"], "Potent CYP3A4 and P-gp inhibitor; negative inotrope; cyclodextrin IV vehicle contra-indicated in CKD"),

    # 9. Analgesics & NSAIDs (8)
    ("Paracetamol", "باراسيتامول", ["Panadol", "Adol", "Tylenol"], "N02BE01", 30, 0.0, 95.0, 4000.0, 0, None, 0, "NONE", "NONE", "NONE", "NONE", "NONE", 0, 0, ["R52", "R50.9"], "First-line analgesic/antipyretic; safe in renal failure; hepatic toxicity ceiling 4000mg/day"),
    ("Ibuprofen", "إيبوبروفين", ["Brufen", "Advil"], "M01AE01", 31, 30.0, 99.0, 2400.0, 0, None, 0, "NONE", "NONE", "SUBSTRATE", "NONE", "NONE", 0, 0, ["M79.1", "M25.5"], "NSAID; blunts afferent arteriolar vasodilation; triggers acute renal failure with ACEI and diuretics"),
    ("Naproxen", "نابروكسين", ["Naprosyn", "Aleve"], "M01AE02", 31, 30.0, 99.0, 1000.0, 0, None, 0, "NONE", "NONE", "SUBSTRATE", "NONE", "NONE", 0, 0, ["M06.9", "M10.0"], "NSAID with relatively lower cardiovascular risk profile; contra-indicated in severe CKD"),
    ("Celecoxib", "سيليكوكسيب", ["Celebrex"], "M01AH01", 31, 30.0, 98.0, 400.0, 0, None, 0, "NONE", "INHIBITOR", "SUBSTRATE", "NONE", "NONE", 0, 0, ["M15.9", "M06.9"], "COX-2 selective inhibitor; reduced upper GI ulceration risk; retains renal hemodynamic toxicity"),
    ("Diclofenac", "ديكلوفيناك", ["Voltaren", "Cataflam"], "M01AB05", 31, 30.0, 99.0, 150.0, 0, None, 0, "NONE", "NONE", "SUBSTRATE", "NONE", "NONE", 0, 0, ["M54.5", "M25.5"], "Potent NSAID; highest thrombotic cardiovascular event rate among traditional NSAIDs"),
    ("Meloxicam", "ميلوكسيكام", ["Mobic"], "M01AC06", 31, 30.0, 99.0, 15.0, 0, None, 0, "NONE", "NONE", "SUBSTRATE", "NONE", "NONE", 0, 0, ["M15.9"], "Preferential COX-2 inhibitor; once-daily posology; contra-indicated in advanced renal failure"),
    ("Ketorolac", "كيتورولاك", ["Toradol"], "M01AB15", 31, 30.0, 95.0, 40.0, 0, None, 0, "NONE", "NONE", "NONE", "NONE", "NONE", 0, 0, ["R52"], "Severe nephrotoxicity and GI bleeding risk; max 5 consecutive days duration; absolute contraindication in CKD"),
    ("Etoricoxib", "إيتوريكوكسيب", ["Arcoxia"], "M01AH05", 31, 30.0, 98.0, 120.0, 0, None, 0, "NONE", "NONE", "NONE", "NONE", "NONE", 0, 0, ["M10.0", "M15.9"], "Second-generation COX-2 inhibitor; fast onset for acute gout flares; monitor blood pressure closely"),

    # 10. Controlled Analgesics & Opioids (Law 30/2021) (7)
    ("Tramadol", "ترامادول", ["Tramal", "Ultram"], "N02AX02", 32, 30.0, 80.0, 400.0, 1, "Controlled_B", 0, "SUBSTRATE", "SUBSTRATE", "NONE", "NONE", "NONE", 0, 1, ["M54.5", "R52"], "Opioid with SNRI action; UAE Law 30/2021 Schedule IV Controlled; risk of serotonin syndrome and seizures"),
    ("Codeine", "كودايين", ["Solpadeine", "Codipront"], "R05DA04", 32, 30.0, 90.0, 240.0, 1, "Controlled_B", 0, "SUBSTRATE", "SUBSTRATE", "NONE", "NONE", "NONE", 0, 0, ["R05", "R52"], "Prodrug converted to morphine by CYP2D6; ultra-rapid metabolizers face respiratory depression risk"),
    ("Morphine", "مورفين", ["MST Continus", "Sevredol"], "N02AA01", 32, 20.0, 90.0, 200.0, 1, "Narcotic", 1, "NONE", "NONE", "NONE", "NONE", "NONE", 0, 0, ["R52", "C80.1"], "UAE Law 30/2021 Narcotic; active metabolites (M6G, M3G) accumulate in CKD causing myoclonus and coma"),
    ("Fentanyl", "فنتانيل", ["Durogesic"], "N01AH01", 32, 0.0, 92.0, 2.4, 1, "Narcotic", 1, "SUBSTRATE", "NONE", "NONE", "NONE", "NONE", 0, 0, ["R52"], "Potent synthetic opioid; CYP3A4 substrate; preferred opioid in CKD (no active toxic metabolites)"),
    ("Oxycodone", "أوكسيكودون", ["OxyContin", "OxyNorm"], "N02AA05", 32, 20.0, 85.0, 160.0, 1, "Narcotic", 1, "SUBSTRATE", "SUBSTRATE", "NONE", "NONE", "NONE", 0, 0, ["R52"], "Semi-synthetic opioid; reduce dose and extend interval in moderate-to-severe renal dysfunction"),
    ("Pethidine", "بيثيدين", ["Demerol"], "N02AB02", 32, 30.0, 95.0, 300.0, 1, "Narcotic", 0, "NONE", "NONE", "NONE", "NONE", "NONE", 0, 0, ["R52"], "Toxic metabolite norpethidine accumulates in renal impairment causing convulsions; CONTRAINDICATED in CKD"),
    ("Buprenorphine", "بوبرينورفين", ["Subutex", "Transtec"], "N02AE01", 32, 0.0, 90.0, 24.0, 1, "Controlled_A", 0, "SUBSTRATE", "NONE", "NONE", "NONE", "NONE", 0, 0, ["F11.2", "R52"], "Partial mu-opioid agonist with ceiling effect on respiratory depression; safe in CKD"),

    # 11. Psychotropics & Sedatives (Law 30/2021) (8)
    ("Alprazolam", "ألبرازولام", ["Xanax"], "N05BA12", 33, 0.0, 80.0, 4.0, 1, "Controlled_B", 0, "SUBSTRATE", "NONE", "NONE", "NONE", "NONE", 0, 0, ["F41.1", "F41.0"], "UAE Law 30/2021 Controlled; sensitive CYP3A4 substrate; rapid tolerance and physical dependence"),
    ("Diazepam", "ديازيبام", ["Valium"], "N05BA01", 33, 0.0, 98.0, 40.0, 1, "Controlled_B", 0, "SUBSTRATE", "NONE", "NONE", "NONE", "NONE", 0, 0, ["F41.1", "G40.9"], "Long-acting benzodiazepine with active desmethyldiazepam metabolite; avoid in elderly"),
    ("Lorazepam", "لورازيبام", ["Ativan"], "N05BA06", 33, 0.0, 95.0, 10.0, 1, "Controlled_B", 0, "NONE", "NONE", "NONE", "NONE", "NONE", 0, 0, ["F41.1", "G40.9"], "Cleared solely via direct glucuronidation; preferred benzodiazepine in liver failure and elderly"),
    ("Clonazepam", "كلونازيبام", ["Rivotril"], "N03AE01", 33, 0.0, 90.0, 20.0, 1, "Controlled_B", 0, "SUBSTRATE", "NONE", "NONE", "NONE", "NONE", 0, 0, ["G40.9", "F41.0"], "Long-acting high-potency benzodiazepine with potent anticonvulsant and antipanic properties"),
    ("Zolpidem", "زولبيديم", ["Ambien", "Stilnox"], "N05CF02", 33, 0.0, 99.0, 10.0, 1, "Controlled_B", 0, "SUBSTRATE", "NONE", "NONE", "NONE", "NONE", 0, 0, ["G47.0"], "Non-benzodiazepine GABA-A agonist; female starting dose capped at 5mg; risk of complex sleep behaviors"),
    ("Pregabalin", "بريجابالين", ["Lyrica"], "N02BF02", 34, 15.0, 2.0, 600.0, 1, "Semi_Controlled", 0, "NONE", "NONE", "NONE", "NONE", "NONE", 0, 0, ["M79.7", "G62.9"], "98% renal excretion unchanged; UAE Semi-Controlled; reduce dose strictly according to eGFR"),
    ("Gabapentin", "جابابنتين", ["Neurontin"], "N02BF01", 34, 15.0, 0.0, 3600.0, 1, "Semi_Controlled", 0, "NONE", "NONE", "NONE", "NONE", "NONE", 0, 0, ["G62.9", "G40.9"], "100% renal excretion; severe risk of respiratory depression and neurotoxicity if not titrated to eGFR"),
    ("Midazolam", "ميدازولام", ["Dormicum"], "N05CD08", 33, 0.0, 99.0, 15.0, 1, "Controlled_A", 0, "SUBSTRATE", "NONE", "NONE", "NONE", "NONE", 0, 0, ["G40.9"], "Short-acting sedative; UAE Controlled A; potent CYP3A4 substrate; rapid onset for procedural sedation"),

    # 12. Antidepressants (8)
    ("Escitalopram", "إسيتالوبرام", ["Cipralex", "Lexapro"], "N06AB10", 35, 0.0, 90.0, 20.0, 0, None, 0, "NONE", "NONE", "NONE", "NONE", "MODERATE", 0, 1, ["F32.9", "F41.1"], "Purest SSRI enantiomer; dose-dependent QT prolongation; max 10mg in elderly; serotonin syndrome risk"),
    ("Sertraline", "سيرترالين", ["Zoloft", "Lustral"], "N06AB06", 35, 0.0, 98.0, 200.0, 0, None, 0, "NONE", "NONE", "NONE", "NONE", "NONE", 0, 1, ["F32.9", "F42.9"], "Cardiovascularly safest SSRI post-MI; minimal CYP interactions; mild initial diarrhea common"),
    ("Fluoxetine", "فلوكسيتين", ["Prozac"], "N06AB03", 35, 0.0, 90.0, 80.0, 0, None, 0, "NONE", "INHIBITOR", "INHIBITOR", "NONE", "NONE", 0, 1, ["F32.9", "F42.9"], "Potent CYP2D6 inhibitor; very long half-life (norfluoxetine active for weeks); 5-week washout for MAOIs"),
    ("Paroxetine", "باروكستين", ["Seroxat", "Paxil"], "N06AB05", 35, 0.0, 95.0, 50.0, 0, None, 0, "NONE", "INHIBITOR", "NONE", "NONE", "NONE", 0, 1, ["F32.9", "F40.1"], "Potent CYP2D6 inhibitor; anticholinergic side effects; highest discontinuation syndrome among SSRIs"),
    ("Venlafaxine", "فينلافاكسين", ["Efexor-XR"], "N06AX16", 35, 30.0, 95.0, 225.0, 0, None, 0, "NONE", "SUBSTRATE", "NONE", "NONE", "NONE", 0, 1, ["F32.9", "F41.1"], "SNRI; dose-dependent hypertension at >150mg daily; reduce dose by 50% in renal impairment"),
    ("Duloxetine", "دولوكسيتين", ["Cymbalta"], "N06AX21", 35, 30.0, 99.0, 120.0, 0, None, 0, "NONE", "INHIBITOR", "NONE", "NONE", "NONE", 0, 1, ["M79.7", "G62.9"], "SNRI effective in diabetic neuropathy; CONTRAINDICATED in eGFR < 30 mL/min and hepatic impairment"),
    ("Amitriptyline", "أميتربتيلين", ["Tryptizol", "Elavil"], "N06AA09", 36, 0.0, 95.0, 150.0, 0, None, 0, "NONE", "SUBSTRATE", "NONE", "NONE", "MODERATE", 0, 1, ["G43.9", "M79.7"], "TCA; potent anticholinergic & quinidine-like cardiotoxicity; dangerous in overdose; sedating"),
    ("Mirtazapine", "ميرتازابين", ["Remeron"], "N06AX11", 36, 0.0, 85.0, 45.0, 0, None, 0, "SUBSTRATE", "SUBSTRATE", "NONE", "NONE", "NONE", 0, 1, ["F32.9", "G47.0"], "Noradrenergic and specific serotonergic antidepressant; causes weight gain and marked sedation at 15mg"),

    # 13. Gastroenterology (6)
    ("Omeprazole", "أوميبرازول", ["Losec", "Prilosec"], "A02BC01", 37, 0.0, 99.0, 40.0, 0, None, 0, "NONE", "NONE", "INHIBITOR", "NONE", "NONE", 0, 0, ["K21.9", "K25.9"], "PPI; inhibits CYP2C19; diminishes bioactivation of clopidogrel; hypomagnesemia on chronic use"),
    ("Esomeprazole", "إيزوميبرازول", ["Nexium"], "A02BC05", 37, 0.0, 99.0, 40.0, 0, None, 0, "NONE", "NONE", "INHIBITOR", "NONE", "NONE", 0, 0, ["K21.9", "K25.9"], "S-isomer of omeprazole; lower first-pass hepatic metabolism; monitor long-term bone density"),
    ("Pantoprazole", "بانتوبرازول", ["Controloc", "Protonix"], "A02BC02", 37, 0.0, 99.0, 80.0, 0, None, 0, "NONE", "NONE", "NONE", "NONE", "NONE", 0, 0, ["K21.9", "K25.9"], "Preferred PPI in cardiac patients on Clopidogrel due to minimal CYP2C19 interaction"),
    ("Famotidine", "فاموتيدين", ["Antodine", "Pepcid"], "A02BA02", 37, 30.0, 30.0, 40.0, 0, None, 0, "NONE", "NONE", "NONE", "NONE", "NONE", 0, 0, ["K21.9"], "H2 blocker; 70% renal excretion; reduce dose by 50% if eGFR < 50 to prevent CNS confusion"),
    ("Domperidone", "دومبيريدون", ["Motilium"], "A03FA03", 38, 0.0, 98.0, 30.0, 0, None, 0, "SUBSTRATE", "NONE", "NONE", "NONE", "HIGH", 0, 0, ["K30", "R11.0"], "Peripheral D2 blocker; dose capped at 30mg/day due to serious cardiac arrhythmia / QT prolongation"),
    ("Metoclopramide", "ميتوكلوبراميد", ["Primperan", "Reglan"], "A03FA01", 38, 30.0, 20.0, 30.0, 0, None, 0, "NONE", "SUBSTRATE", "NONE", "NONE", "NONE", 0, 0, ["R11.0"], "Centrally-acting D2 antagonist; risk of extrapyramidal symptoms and tardive dyskinesia; reduce in CKD"),

    # 14. Respiratory (6)
    ("Salbutamol", "سالبوتامول", ["Ventolin"], "R03AC02", 39, 0.0, 50.0, 16.0, 0, None, 0, "NONE", "NONE", "NONE", "NONE", "NONE", 0, 0, ["J45.9", "J44.9"], "Short-acting beta-2 agonist (SABA); rescue bronchodilator; causes hypokalemia and tachycardia"),
    ("Fluticasone/Salmeterol", "فلوتيكازون/سالميتيرول", ["Seretide", "Advair"], "R03AK06", 40, 0.0, 99.0, 1.0, 0, None, 0, "SUBSTRATE", "NONE", "NONE", "NONE", "NONE", 0, 0, ["J45.9", "J44.9"], "ICS/LABA maintenance; fluticasone is a sensitive CYP3A4 substrate; risk of Cushing's with ritonavir"),
    ("Budesonide/Formoterol", "بوديزونيد/فورموتيرول", ["Symbicort"], "R03AK07", 40, 0.0, 90.0, 1.28, 0, None, 0, "SUBSTRATE", "NONE", "NONE", "NONE", "NONE", 0, 0, ["J45.9", "J44.9"], "SMART protocol therapy for asthma; high first-pass hepatic metabolism minimizes systemic steroid side effects"),
    ("Ipratropium", "إبراتروبيوم", ["Atrovent"], "R03BB01", 41, 0.0, 50.0, 0.32, 0, None, 0, "NONE", "NONE", "NONE", "NONE", "NONE", 0, 0, ["J44.9", "J45.9"], "Short-acting muscarinic antagonist (SAMA); negligible systemic absorption; dry mouth common"),
    ("Tiotropium", "تيوتروبيوم", ["Spiriva"], "R03BB04", 41, 0.0, 25.0, 0.018, 0, None, 0, "NONE", "NONE", "NONE", "NONE", "NONE", 0, 0, ["J44.9"], "Long-acting muscarinic antagonist (LAMA); once-daily cornerstone of COPD maintenance therapy"),
    ("Montelukast", "مونتيلوكاست", ["Singulair"], "R03DC03", 41, 0.0, 99.0, 10.0, 0, None, 0, "SUBSTRATE", "NONE", "SUBSTRATE", "NONE", "NONE", 0, 0, ["J45.9", "J30.1"], "CysLT1 receptor antagonist; take in the evening; FDA boxed warning for neuropsychiatric events"),

    # 15. Endocrine & Steroids (6)
    ("Levothyroxine", "ليفوثيروكسين", ["Euthyrox", "Synthroid"], "H03AA01", 42, 0.0, 90.0, 0.3, 0, None, 1, "NONE", "NONE", "NONE", "NONE", "NONE", 0, 0, ["E03.9"], "Narrow therapeutic index; take on empty stomach 30-60 min before breakfast; separate from calcium/iron"),
    ("Carbimazole", "كاربيمازول", ["Neo-Mercazole"], "H03BB01", 42, 0.0, 90.0, 40.0, 0, None, 0, "NONE", "NONE", "NONE", "NONE", "NONE", 0, 0, ["E05.9"], "Prodrug converted to thiamazole; monitor FBC for acute sore throat (agranulocytosis warning)"),
    ("Prednisolone", "بريدنيزولون", ["Predo", "Solupred"], "H02AB06", 43, 0.0, 80.0, 60.0, 0, None, 0, "SUBSTRATE", "NONE", "NONE", "NONE", "NONE", 0, 0, ["M06.9", "J45.9"], "Active corticosteroid; induces hyperglycemia, hypertension, and osteoporosis on prolonged therapy"),
    ("Dexamethasone", "ديكساميثازون", ["Decadron"], "H02AB02", 43, 0.0, 80.0, 16.0, 0, None, 0, "INDUCER", "NONE", "NONE", "NONE", "NONE", 0, 0, ["R05", "J18.9"], "Long-acting steroid with zero mineralocorticoid activity; induces CYP3A4 with repeated high doses"),
    ("Hydrocortisone", "هيدروكورتيزون", ["Cortef"], "H02AB09", 43, 0.0, 90.0, 300.0, 0, None, 0, "SUBSTRATE", "NONE", "NONE", "NONE", "NONE", 0, 0, ["E27.1"], "Physiological replacement glucocorticoid with significant mineralocorticoid salt-retaining activity"),
    ("Methylprednisolone", "ميثيل بريدنيزولون", ["Medrol"], "H02AB04", 43, 0.0, 80.0, 64.0, 0, None, 0, "SUBSTRATE", "NONE", "NONE", "NONE", "NONE", 0, 0, ["M06.9", "J44.9"], "Intermediate-acting steroid with minimal mineralocorticoid effect; metabolized by CYP3A4"),

    # SPRINT-02: 24 Additional Active Substances for 30 Interactions Schema
    ("Methotrexate", "ميثوتريكسات", ["Trexall", "Rheumatrex", "Metoject"], "L01BA01", 44, 30.0, 20.0, 25.0, 0, None, 1, "NONE", "NONE", "NONE", "SUBSTRATE", "NONE", 0, 0, ["M05.9", "M06.9", "L40.0"], "Antimetabolite folate antagonist; eliminated via renal OAT1/OAT3; NSAIDs inhibit clearance causing fatal marrow suppression"),
    ("Iodinated Contrast", "صبغة اليود الإشعاعية", ["Omnipaque", "Iohexol", "Visipaque", "Ultravist"], "V08AB02", 48, 30.0, 0.0, 150000.0, 0, None, 0, "NONE", "NONE", "NONE", "NONE", "NONE", 0, 0, ["Z01.818", "R93.8"], "Nephrotoxic contrast media; transient renal failure induces metformin retention and fatal lactic acidosis; withhold metformin 48h prior/after if eGFR < 60"),
    ("Sildenafil", "سيلدينافيل", ["Viagra", "Revatio"], "G04BE03", 47, 0.0, 80.0, 100.0, 0, None, 0, "SUBSTRATE", "NONE", "SUBSTRATE", "NONE", "NONE", 0, 0, ["N52.9", "I27.0"], "PDE5 inhibitor; synergistic cGMP vasodilation with nitrates causes life-threatening refractory hypotension; strictly contraindicated within 24h"),
    ("Nitroglycerin", "نيتروجليسرين", ["Nitrostat", "Nitrolingual", "Nitro-Dur"], "C01DA02", 46, 0.0, 99.0, 1.2, 0, None, 0, "NONE", "NONE", "NONE", "NONE", "NONE", 0, 0, ["I20.9", "I25.9", "I21.9"], "Nitric oxide donor triggering intracellular cGMP accumulation; concurrent PDE5 inhibitors cause cardiovascular collapse"),
    ("Azathioprine", "أزاثيوبرين", ["Imuran", "Azasan"], "L04AX01", 44, 0.0, 50.0, 200.0, 0, None, 1, "NONE", "NONE", "NONE", "NONE", "NONE", 0, 0, ["M06.9", "K50.9", "K51.9", "Z94.0"], "Prodrug converted to 6-mercaptopurine; metabolized by xanthine oxidase; allopurinol blocks clearance causing lethal bone marrow aplasia"),
    ("Linezolid", "لينيزوليد", ["Zyvox", "Zyvoxid"], "J01XX08", 28, 0.0, 65.0, 1200.0, 0, None, 0, "NONE", "NONE", "NONE", "NONE", "NONE", 0, 1, ["A49.0", "J18.9", "L08.9"], "Oxazolidinone antibacterial with reversible non-selective MAO-A/B inhibition; co-administration with SSRIs precipitates severe serotonin toxicity"),
    ("Potassium Chloride", "كلوريد البوتاسيوم", ["Klor-Con", "Slow-K", "Micro-K"], "A12BA01", 49, 30.0, 0.0, 4000.0, 0, None, 1, "NONE", "NONE", "NONE", "NONE", "NONE", 1, 0, ["E87.6"], "Electrolyte replenisher; fatal hyperkalemia and cardiac arrest when co-administered with mineralocorticoid antagonists or ACEI/ARBs in renal impairment"),
    ("Lithium", "ليثيوم", ["Lithobid", "Eskalith", "Priadel"], "N05AN01", 45, 30.0, 0.0, 1800.0, 0, None, 1, "NONE", "NONE", "NONE", "NONE", "NONE", 0, 0, ["F31.9", "F33.9"], "Renally cleared monovalent cation; thiazides reduce sodium clearance and increase lithium reabsorption by 30-40%, leading to neurotoxicity and coma"),
    ("Theophylline", "ثيوفيلين", ["Theo-24", "Uniphyl", "Theochron"], "R03DA04", 52, 0.0, 90.0, 900.0, 0, None, 1, "SUBSTRATE", "NONE", "NONE", "NONE", "NONE", 0, 0, ["J44.9", "J45.9"], "Methylxanthine bronchodilator; narrow index; Ciprofloxacin potently inhibits CYP1A2, doubling serum levels and precipitating status epilepticus"),
    ("Ketoconazole", "كيتوكونازول", ["Nizoral", "Extina"], "J02AB02", 29, 0.0, 85.0, 400.0, 0, None, 0, "INHIBITOR", "NONE", "INHIBITOR", "INHIBITOR", "HIGH", 0, 0, ["B35.9", "B37.9"], "Extremely potent dual CYP3A4 and P-gp inhibitor; contraindicated with DOACs (Rivaroxaban), statins, and colchicine"),
    ("Carbamazepine", "كاربامازيبين", ["Tegretol", "Carbatrol", "Tegretol-XR"], "N03AF01", 45, 0.0, 98.0, 1200.0, 0, None, 1, "INDUCER", "NONE", "INDUCER", "INDUCER", "NONE", 0, 0, ["G40.9", "G50.0", "F31.9"], "Potent auto-inducer and CYP3A4/UGT inducer; dramatically lowers estrogen concentrations, causing contraceptive failure"),
    ("Ethinylestradiol", "إيثينيل إستراديول", ["Microgynon", "Yasmin", "Diane-35"], "G03AA07", 50, 0.0, 90.0, 0.05, 0, None, 0, "SUBSTRATE", "NONE", "NONE", "NONE", "NONE", 0, 0, ["Z30.011", "E28.2"], "Synthetic estrogen; metabolized by CYP3A4 and glucuronidation; enzyme inducers (carbamazepine, rifampin) cause breakthrough ovulation"),
    ("Cyclosporine", "سيكلوسبورين", ["Sandimmune", "Neoral", "Gengraf"], "L04AD01", 44, 0.0, 95.0, 500.0, 0, None, 1, "SUBSTRATE", "NONE", "NONE", "SUBSTRATE", "NONE", 1, 0, ["Z94.0", "Z94.4", "L40.0"], "Calcineurin inhibitor; substrate of CYP3A4 and P-gp; St. John's Wort induces expression causing acute transplant allograft rejection"),
    ("St. John's Wort", "عشبة سانت جون (عرن مثقوب)", ["Hypericum Perforatum", "St John's Wort Extract"], "A13A", 51, 0.0, 50.0, 900.0, 0, None, 0, "INDUCER", "NONE", "INDUCER", "INDUCER", "NONE", 0, 1, ["F32.9"], "Contains hyperforin, a potent PXR activator inducing CYP3A4/P-gp; precipitates organ rejection with cyclosporine/tacrolimus"),
    ("Ferrous Sulfate", "كبريتات الحديدوز", ["Fer-In-Sol", "Fero-Gradumet", "Iron"], "B03AA07", 49, 0.0, 0.0, 600.0, 0, None, 0, "NONE", "NONE", "NONE", "NONE", "NONE", 0, 0, ["D50.9"], "Polyvalent iron cations chelate oral levothyroxine in the GI tract, severely reducing bioavailability; separate dosing by >= 4 hours"),
    ("Gemfibrozil", "جيمفيبروزيل", ["Lopid"], "C10AB04", 12, 30.0, 70.0, 1200.0, 0, None, 0, "NONE", "NONE", "INHIBITOR", "NONE", "NONE", 0, 0, ["E78.1", "E78.2"], "Fibric acid derivative; inhibits OATP1B1 and statin glucuronidation (UGT1A1/3), markedly raising atorvastatin/simvastatin levels and rhabdomyolysis risk"),
    ("Atenolol", "أتينولول", ["Tenormin"], "C07AB03", 5, 15.0, 10.0, 100.0, 0, None, 0, "NONE", "NONE", "NONE", "NONE", "NONE", 0, 0, ["I10", "I20.9"], "Hydrophilic beta-1 blocker; co-administration with non-DHP CCBs (Verapamil, Diltiazem) causes severe bradycardia, AV block, and cardiac arrest"),
    ("Citalopram", "سيتالوبرام", ["Celexa", "Cipramil"], "N06AB04", 35, 0.0, 85.0, 40.0, 0, None, 0, "SUBSTRATE", "NONE", "NONE", "NONE", "HIGH", 0, 1, ["F32.9", "F41.1"], "Dose-dependent QTc prolongation; cap at 20mg in elderly (>60) or with CYP2C19 inhibitors; additive TdP risk with Ondansetron"),
    ("Ondansetron", "أوندانسيترون", ["Zofran", "Zuplenz"], "A04AA01", 54, 0.0, 95.0, 24.0, 0, None, 0, "SUBSTRATE", "SUBSTRATE", "NONE", "NONE", "HIGH", 0, 0, ["R11.0", "R11.2"], "Blocks 5-HT3 receptors and hERG potassium channels; combination with QT-prolonging SSRIs (Citalopram) triggers fatal Torsades de Pointes"),
    ("Tacrolimus", "تاكروليموس", ["Prograf", "Advagraf", "Envarsus XR"], "L04AD02", 44, 0.0, 99.0, 10.0, 0, None, 1, "SUBSTRATE", "NONE", "NONE", "SUBSTRATE", "MODERATE", 1, 0, ["Z94.0", "Z94.4", "Z94.1"], "Calcineurin inhibitor; Voriconazole inhibits CYP3A4, tripling tacrolimus AUC and causing acute nephrotoxicity; preemptively reduce tacrolimus by 66%"),
    ("Voriconazole", "فوريكونازول", ["Vfend"], "J02AC03", 29, 0.0, 98.0, 400.0, 0, None, 1, "INHIBITOR", "NONE", "INHIBITOR", "INHIBITOR", "HIGH", 0, 0, ["B44.9", "B37.9"], "Broad-spectrum triazole antifungal; potent inhibitor of CYP3A4/2C19/2C9; massive interaction with immunosuppressants"),
    ("Tamoxifen", "تاموكسيفين", ["Nolvadex", "Soltamox"], "L02BA01", 53, 0.0, 90.0, 40.0, 0, None, 0, "SUBSTRATE", "SUBSTRATE", "SUBSTRATE", "NONE", "NONE", 0, 0, ["C50.9"], "Prodrug bioactivated by CYP2D6 to endoxifen; Paroxetine irreversibly inhibits CYP2D6, blocking endoxifen formation and increasing breast cancer recurrence"),
    ("Phenytoin", "فينيتوين", ["Dilantin", "Phenytek"], "N03AB02", 45, 0.0, 95.0, 400.0, 0, None, 1, "INDUCER", "NONE", "SUBSTRATE", "INDUCER", "NONE", 0, 0, ["G40.9"], "Non-linear Michaelis-Menten kinetics; highly protein bound (90%); Valproic acid displaces it from albumin and inhibits CYP2C9, precipitating severe toxicity"),
    ("Valproic Acid", "حمض الفالبرويك", ["Depakene", "Depakote", "Convulex"], "N03AG01", 45, 0.0, 95.0, 3000.0, 0, None, 1, "NONE", "NONE", "INHIBITOR", "NONE", "NONE", 0, 0, ["G40.9", "F31.9", "G43.9"], "Broad-spectrum anticonvulsant and enzyme/transporter inhibitor; displaces phenytoin and inhibits its metabolism; high teratogenicity"),
]

# High-yield interactions table:
# (drug_a_name, drug_b_name, severity, alert_code, clinical_rationale, action_directive, evidence_doc_id, evidence_section)
DRUG_INTERACTIONS_DATA = [
    (
        "Colchicine", "Clarithromycin", "CONTRAINDICATED",
        "FATAL_CYP3A4_RENAL_CONTRAINDICATION",
        "Concurrent Colchicine + Clarithromycin in renal impairment (eGFR < 60) causes fatal colchicine accumulation via dual CYP3A4/P-gp inhibition, leading to multi-organ failure and bone marrow aplasia.",
        "Select alternative antibiotic (Azithromycin) or hold Colchicine during macrolide therapy.",
        "DRUG_COLCHICINE_MONOGRAPH", "COLCHICINE_MONOGRAPH"
    ),
    (
        "Simvastatin", "Clarithromycin", "CONTRAINDICATED",
        "FATAL_CYP3A4_RHABDOMYOLYSIS_CONTRAINDICATION",
        "Concurrent Simvastatin and Clarithromycin (potent mechanism-based CYP3A4 inhibitor) is contraindicated due to a >10-fold increase in statin plasma levels, precipitating fatal rhabdomyolysis and acute kidney injury.",
        "Temporarily suspend Simvastatin or switch to non-CYP3A4 statin (Pravastatin / Rosuvastatin).",
        "DRUG_SIMVASTATIN_MONOGRAPH", "SIMVASTATIN_MONOGRAPH"
    ),
    (
        "Warfarin", "Amiodarone", "CONTRAINDICATED",
        "WARFARIN_AMIODARONE_HEMORRHAGE_RISK",
        "Concurrent Warfarin + Amiodarone inhibits CYP2C9 and CYP3A4, causing profound INR elevation and life-threatening hemorrhagic complications.",
        "Reduce Warfarin dose by 33% to 50% upon initiating Amiodarone and monitor INR weekly.",
        "DRUG_WARFARIN_MONOGRAPH", "WARFARIN_MONOGRAPH"
    ),
    (
        "Lisinopril", "Losartan", "CONTRAINDICATED",
        "DUAL_RAAS_BLOCKADE_CONTRAINDICATION",
        "Dual blockade of the renin-angiotensin system (ACE inhibitor + ARB) produces severe hypotension, syncope, acute renal failure, and fatal hyperkalemia without added cardiovascular benefit (ONTARGET trial).",
        "Discontinue either the ACE inhibitor or the ARB; maintain monotherapy RAAS blockade.",
        "BNF-86-SEC-2.5", "RAAS_BLOCKADE_GUIDELINE"
    ),
    (
        "Ramipril", "Valsartan", "CONTRAINDICATED",
        "DUAL_RAAS_BLOCKADE_RAMIPRIL_VALSARTAN",
        "Dual RAAS blockade is contraindicated due to increased risk of hyperkalemia and acute kidney injury.",
        "Discontinue one agent.",
        "BNF-86-SEC-2.5", "RAAS_BLOCKADE_GUIDELINE"
    ),
    (
        "Spironolactone", "Lisinopril", "MAJOR",
        "HYPERKALEMIA_RAAS_MRA_RISK",
        "Combination of aldosterone antagonist and ACE inhibitor in chronic kidney disease causes synergistic potassium retention and life-threatening cardiac arrhythmias.",
        "Monitor serum potassium and creatinine within 7-14 days; ensure eGFR > 30 and baseline K+ < 5.0.",
        "KDIGO-2024-CKD", "SEC-3.2_POTASSIUM"
    ),
    (
        "Spironolactone", "Valsartan", "MAJOR",
        "HYPERKALEMIA_SPIRONOLACTONE_VALSARTAN",
        "Combination of aldosterone antagonist and ARB carries high hyperkalemia risk in patients with impaired renal clearance.",
        "Check serum potassium within 1 week of combination initiation.",
        "KDIGO-2024-CKD", "SEC-3.2_POTASSIUM"
    ),
    (
        "Warfarin", "Ibuprofen", "CONTRAINDICATED",
        "WARFARIN_NSAID_GI_HEMORRHAGE_RISK",
        "Concurrent NSAID and oral anticoagulant causes synergistic gastrointestinal mucosal injury, platelet inhibition, and fatal upper GI hemorrhage.",
        "Avoid NSAIDs in anticoagulated patients; select Paracetamol or topical analgesics.",
        "BNF-86-SEC-2.8", "ANTICOAG_NSAID_RISK"
    ),
    (
        "Rivaroxaban", "Ibuprofen", "CONTRAINDICATED",
        "DOAC_NSAID_BLEEDING_RISK",
        "Concomitant DOAC and NSAID therapy significantly elevates major bleeding hazard ratio (>2.5x).",
        "Avoid systemic NSAIDs; use alternative non-ulcerogenic analgesia.",
        "BNF-86-SEC-2.8", "DOAC_SAFETY_BOX"
    ),
    (
        "Apixaban", "Naproxen", "CONTRAINDICATED",
        "DOAC_NSAID_APIXABAN_NAPROXEN",
        "Synergistic bleeding risk with direct factor Xa inhibitors and non-selective NSAIDs.",
        "Avoid concurrent use.",
        "BNF-86-SEC-2.8", "DOAC_SAFETY_BOX"
    ),
    (
        "Tramadol", "Escitalopram", "MAJOR",
        "SEROTONIN_SYNDROME_SSRI_TRAMADOL",
        "Concurrent Tramadol and SSRIs increases risk of life-threatening Serotonin Syndrome (hyperthermia, clonus, autonomic instability) and lowers seizure threshold.",
        "Avoid combination or monitor closely for early serotonergic toxicity.",
        "BNF-86-SEC-4.3", "SEROTONIN_TOXICITY"
    ),
    (
        "Tramadol", "Sertraline", "MAJOR",
        "SEROTONIN_SYNDROME_TRAMADOL_SERTRALINE",
        "Concurrent opioid SNRI and SSRI precipitating serotonin toxicity and seizures.",
        "Choose non-serotonergic analgesic or monitor neurological status.",
        "BNF-86-SEC-4.3", "SEROTONIN_TOXICITY"
    ),
    (
        "Digoxin", "Verapamil", "CONTRAINDICATED",
        "DIGOXIN_VERAPAMIL_TOXICITY_P_GP",
        "Verapamil inhibits renal and biliary P-glycoprotein transport of Digoxin, doubling digoxin serum concentrations and inducing severe bradycardia or AV block.",
        "Reduce Digoxin dose by 50% and monitor ECG and serum digoxin levels.",
        "BNF-86-SEC-2.1", "DIGOXIN_MONOGRAPH"
    ),
    (
        "Digoxin", "Amiodarone", "CONTRAINDICATED",
        "DIGOXIN_AMIODARONE_TOXICITY_P_GP",
        "Amiodarone inhibits P-gp efflux of Digoxin, leading to rapid digitalis toxicity.",
        "Halve digoxin dose immediately and monitor serum levels.",
        "BNF-86-SEC-2.1", "DIGOXIN_MONOGRAPH"
    ),
    (
        "Metformin", "Simvastatin", "MODERATE",
        "METFORMIN_SIMVASTATIN_COADMINISTRATION_SAFE",
        "Standard cardioprotective combination in diabetic dyslipidemia; verified safe with standard monitoring.",
        "Maintain routine annual metabolic panel.",
        "ADA-STANDARDS-2026", "SEC-10_LIPIDS"
    ),
    (
        "Paracetamol", "Amoxicillin", "MODERATE",
        "PARACETAMOL_AMOXICILLIN_SAFE",
        "Analgesic and antimicrobial combination verified fully safe and compatible.",
        "No dose alteration needed.",
        "BNF-86-SEC-5.1", "COMPATIBILITY"
    ),
]

# Renal dosing guidelines:
# (drug_name, egfr_min, egfr_max, recommended_dose, freq_hours, contraindicated, rationale)
RENAL_DOSING_DATA = [
    ("Metformin", 0.0, 29.99, "Contraindicated (0 mg)", 0, 1, "Contraindicated due to high risk of fatal Metformin-Associated Lactic Acidosis (MALA)."),
    ("Metformin", 30.0, 44.99, "Max 500 mg twice daily (1000 mg/day)", 12, 0, "Dose ceiling 1000 mg daily in CKD Stage 3b; quarterly eGFR monitoring required (KDIGO 2024)."),
    ("Metformin", 45.0, 59.99, "Max 1000 mg twice daily (2000 mg/day)", 12, 0, "Moderate renal reduction: maximum permissible daily dose capped at 2000 mg."),
    ("Metformin", 60.0, 250.0, "Standard dose up to 1000 mg BID with meals", 12, 0, "Standard posology in normal renal clearance."),

    ("Colchicine", 0.0, 29.99, "Contraindicated for acute flares; max 0.5 mg every 2-3 days for prophylaxis", 72, 1, "Contraindicated for acute treatment; extreme accumulation and neuromyopathy hazard."),
    ("Colchicine", 30.0, 59.99, "0.5 mg once daily", 24, 0, "Dose reduction warranted in mild-to-moderate CKD."),

    ("Sitagliptin", 0.0, 29.99, "25 mg once daily", 24, 0, "Dose reduction to 25% of standard dose due to primary renal elimination."),
    ("Sitagliptin", 30.0, 44.99, "50 mg once daily", 24, 0, "50% dose reduction required in CKD Stage 3b."),
    ("Sitagliptin", 45.0, 250.0, "100 mg once daily", 24, 0, "Standard therapeutic dose."),

    ("Allopurinol", 0.0, 14.99, "50-100 mg every 2-3 days", 72, 0, "Start at lowest possible dose to avoid allopurinol hypersensitivity syndrome."),
    ("Allopurinol", 15.0, 29.99, "100 mg once daily", 24, 0, "Renal dose cap at 100 mg daily due to oxypurinol retention."),
    ("Allopurinol", 30.0, 59.99, "200 mg once daily", 24, 0, "Titrate based on serum urate targets < 6.0 mg/dL."),

    ("Ciprofloxacin", 0.0, 29.99, "250-500 mg every 18-24 hours", 24, 0, "50% dose reduction to avoid neurotoxicity, QT prolongation, and crystal nephropathy."),
    ("Ciprofloxacin", 30.0, 250.0, "500-750 mg every 12 hours", 12, 0, "Standard posology."),

    ("Levofloxacin", 0.0, 19.99, "500 mg initial loading, then 250 mg every 48 hours", 48, 0, "Severe renal adjustment required to avoid central nervous system toxicity."),
    ("Levofloxacin", 20.0, 49.99, "500 mg initial loading, then 250 mg every 24 hours", 24, 0, "Reduced dosing frequency."),

    ("Spironolactone", 0.0, 29.99, "Contraindicated (0 mg)", 0, 1, "Contraindicated due to extreme risk of fatal hyperkalemia and acute anuria."),
    ("Ketorolac", 0.0, 29.99, "Contraindicated (0 mg)", 0, 1, "Absolute contraindication in renal impairment (FDA Boxed Warning)."),
    ("Pethidine", 0.0, 29.99, "Contraindicated (0 mg)", 0, 1, "Contraindicated due to neurotoxic norpethidine metabolite accumulation causing seizures."),
    ("Nitrofurantoin", 0.0, 29.99, "Contraindicated (0 mg)", 0, 1, "Contraindicated: inadequate urinary antibacterial concentrations and high systemic toxicity."),
]



# Section 2.2: Complete Pharmacovigilance Matrix: 30 High-Yield Clinical Interactions
# Tuple format: drug_a_atc, drug_b_atc, alert_code, severity_level, mechanism_text, clinical_action, evidence_citation, doc_id, section
INTERACTIONS_30_SPEC = [
    # 1
    ("B01AA03", "C01BD01", "WARFARIN_AMIODARONE_CYP2C9_INHIB", "MAJOR",
     "CYP2C9/CYP3A4 inhibition by amiodarone impairs S-warfarin clearance",
     "Marked INR elevation causes life-threatening hemorrhage. Reduce warfarin dose 33–50%; monitor INR weekly.",
     "CHEST 2018 (PMID: 30122176); DailyMed Warfarin Sec 7.3",
     "DRUG_WARFARIN_MONOGRAPH", "WARFARIN_MONOGRAPH"),
    # 2
    ("C10AA01", "J01FA09", "STATIN_MACROLIDE_CYP3A4_RHABDO", "CONTRAINDICATED",
     "Potent CYP3A4 inhibition increases simvastatin AUC up to 10-fold",
     "Massive statin exposure induces severe rhabdomyolysis and acute kidney injury. Concomitant use contraindicated; suspend statin or switch to azithromycin.",
     "FDA Safety Comm. (2012); DailyMed Simvastatin Sec 4; PMID: 22619236",
     "DRUG_SIMVASTATIN_MONOGRAPH", "SIMVASTATIN_MONOGRAPH"),
    # 3
    ("C09AA03", "C09CA01", "ACEI_ARB_DUAL_RAAS_BLOCKADE", "CONTRAINDICATED",
     "Dual RAAS blockade produces additive efferent arteriolar vasodilation",
     "Severe hypotension, acute renal failure, and hyperkalemia occur without clinical benefit. Avoid concomitant use.",
     "ONTARGET Trial, N Engl J Med 2008 (PMID: 18378520); DailyMed Lisinopril Sec 5.1",
     "DRUG_LISINOPRIL_MONOGRAPH", "LISINOPRIL_MONOGRAPH"),
    # 4
    ("C03DA01", "C09AA02", "MRA_ACEI_ADDITIVE_HYPERKALEMIA", "MAJOR",
     "Additive mineralocorticoid antagonism and suppressed distal potassium secretion",
     "Causes lethal hyperkalemia and ventricular arrhythmias. Contraindicated if K+ > 5.0 mEq/L or eGFR < 30 mL/min; monitor serum K+.",
     "RALES Trial, N Engl J Med 1999 (PMID: 10471456); AHA HF Guidelines 2022 (PMID: 35363499)",
     "DRUG_SPIRONOLACTONE_MONOGRAPH", "SPIRONOLACTONE_MONOGRAPH"),
    # 5
    ("B01AC04", "A02BC01", "CLOPIDOGREL_OMEPRAZOLE_CYP2C19", "MAJOR",
     "Competitive CYP2C19 inhibition prevents bioactivation of clopidogrel prodrug",
     "Attenuated platelet inhibition increases risk of stent thrombosis and recurrent MI. Substitute with pantoprazole or rabeprazole.",
     "COGENT Trial, N Engl J Med 2010 (PMID: 20925534); DailyMed Plavix Sec 7.1",
     "DRUG_CLOPIDOGREL_MONOGRAPH", "CLOPIDOGREL_MONOGRAPH"),
    # 6
    ("L01BA01", "M01AE01", "MTX_NSAID_RENAL_CLEARANCE_TOX", "MAJOR",
     "NSAIDs inhibit renal OAT1/OAT3 transporters and reduce medullary perfusion",
     "Markedly reduced renal methotrexate clearance precipitates pancytopenia and severe hepatotoxicity. Avoid concurrent NSAIDs.",
     "Ann Rheum Dis 2017 (PMID: 28546162); DailyMed Methotrexate Sec 7",
     "DRUG_METHOTREXATE_MONOGRAPH", "METHOTREXATE_MONOGRAPH"),
    # 7
    ("N06AB03", "N02AX02", "SSRI_TRAMADOL_SEROTONIN_SYN", "MAJOR",
     "Additive synaptic serotonin reuptake inhibition plus CYP2D6 blockade",
     "Triggers Serotonin Syndrome manifested by clonus, autonomic instability, hyperthermia, and seizures. Avoid combination.",
     "Boyer & Shannon, N Engl J Med 2005 (PMID: 15784664); DailyMed Tramadol Sec 5.6",
     "DRUG_FLUOXETINE_MONOGRAPH", "FLUOXETINE_MONOGRAPH"),
    # 8
    ("C01AA05", "J01FA09", "DIGOXIN_CLARITHRO_PGP_TOXICITY", "MAJOR",
     "Inhibition of intestinal and renal P-gp efflux pumps and gut flora eradication",
     "Serum digoxin levels increase >2-fold, precipitating lethal AV block and dysrhythmias. Halve digoxin dose and monitor ECG.",
     "Circulation 2002 (PMID: 11980678); DailyMed Lanoxin Sec 7.1",
     "DRUG_DIGOXIN_MONOGRAPH", "DIGOXIN_MONOGRAPH"),
    # 9
    ("J01MA02", "C01BD01", "QT_PROLONGATION_SUMMATION_TDP", "CONTRAINDICATED",
     "Synergistic blockade of myocardial rapid delayed rectifier potassium channels (IKr)",
     "Significant QTc prolongation precipitates Torsades de Pointes. Combination strictly contraindicated; use non-quinolone antibacterial.",
     "CredibleMeds Category 'Known Risk'; DailyMed Ciprofloxacin Sec 5.3; PMID: 21676878",
     "DRUG_CIPROFLOXACIN_MONOGRAPH", "CIPROFLOXACIN_MONOGRAPH"),
    # 10
    ("A10BA02", "V08AB02", "METFORMIN_CONTRAST_LACTIC_ACIDOSIS", "MAJOR",
     "Radiocontrast-induced acute renal tubular injury impairs metformin elimination",
     "Biguanide accumulation triggers lactic acidosis with high mortality. Discontinue metformin prior to contrast if eGFR < 60 mL/min.",
     "ACR Manual on Contrast Media (2023); DailyMed Glucophage Sec 5.1; PMID: 28864703",
     "DRUG_METFORMIN_MONOGRAPH", "METFORMIN_MONOGRAPH"),
    # 11
    ("G04BE03", "C01DA02", "PDE5I_NITRATE_FATAL_HYPOTENSION", "CONTRAINDICATED",
     "Synergistic cGMP accumulation via PDE5 inhibition and nitrate NO donation",
     "Produces profound refractory systemic vasodilation, hemodynamic collapse, and coronary hypoperfusion. Strictly contraindicated (24h window).",
     "ACC/AHA Practice Guidelines, Circulation 1999 (PMID: 10587354); DailyMed Revatio Sec 4",
     "DRUG_SILDENAFIL_MONOGRAPH", "SILDENAFIL_MONOGRAPH"),
    # 12
    ("M04AA01", "L04AX01", "ALLOPURINOL_AZATHIOPRINE_MYELOSUPP", "CONTRAINDICATED",
     "Xanthine oxidase inhibition blocks primary catabolic clearance of 6-MP",
     "Dramatic shunting into active cytotoxic thioguanine metabolites induces pancytopenia and lethal bone marrow failure. Reduce azathioprine dose by 75%.",
     "J Rheumatol 2018 (PMID: 29545437); DailyMed Imuran Sec 7.1",
     "DRUG_ALLOPURINOL_MONOGRAPH", "ALLOPURINOL_MONOGRAPH"),
    # 13
    ("J01XX08", "N06AB06", "LINEZOLID_SSRI_SEROTONIN_TOXICITY", "CONTRAINDICATED",
     "Non-selective, reversible MAO-A inhibition by linezolid combined with SSRI",
     "Rapid accumulation of synaptic 5-HT triggers malignant hyperthermia, delirium, and neuromuscular rigidity. Discontinue SSRI 2 weeks prior.",
     "FDA Drug Safety Comm. (2011); DailyMed Zyvox Sec 4.2; PMID: 23143825",
     "DRUG_LINEZOLID_MONOGRAPH", "LINEZOLID_MONOGRAPH"),
    # 14
    ("A12BA01", "C03DA01", "POTASSIUM_MRA_SEVERE_HYPERKALEMIA", "CONTRAINDICATED",
     "Exogenous potassium supplementation combined with mineralocorticoid antagonism",
     "Impaired distal tubular excretion leads to rapid hyperkalemia, cardiac arrest, and ventricular fibrillation. Prohibit routine concurrent supplementation.",
     "Kidney Int 2020 (PMID: 32679173); DailyMed Klor-Con Sec 5.1",
     "DRUG_POTASSIUM_MONOGRAPH", "POTASSIUM_MONOGRAPH"),
    # 15
    ("N05AN01", "C03AA03", "LITHIUM_THIAZIDE_TOXICITY_ACCUM", "MAJOR",
     "Distal tubular sodium depletion increases compensatory proximal reabsorption of lithium",
     "Renal lithium clearance decreases by 25–40%, inducing neurotoxicity, tremor, ataxia, and coma. Reduce lithium dosage by 50%.",
     "Am J Psychiatry 2016 (PMID: 27181414); DailyMed Lithobid Sec 7",
     "DRUG_LITHIUM_MONOGRAPH", "LITHIUM_MONOGRAPH"),
    # 16
    ("B01AA03", "J01MA02", "WARFARIN_FLUOROQUINOLONE_BLEED", "MAJOR",
     "CYP1A2 metabolic inhibition by ciprofloxacin and disruption of vitamin K gut flora",
     "Increases free warfarin plasma concentration, causing sudden prolongation of PT/INR and bleeding. Preemptively reduce warfarin by 25–33%.",
     "Chest 2012 (PMID: 22315268); OpenFDA Label Ciprofloxacin Sec 7",
     "DRUG_WARFARIN_MONOGRAPH", "WARFARIN_MONOGRAPH"),
    # 17
    ("R03DA04", "J01MA02", "THEOPHYLLINE_CIPRO_CYP1A2_SEIZURE", "MAJOR",
     "Potent inhibition of hepatic CYP1A2 clearance pathways by ciprofloxacin",
     "Elevates serum theophylline levels 2- to 3-fold, triggering tachyarrhythmias and status epilepticus. Reduce theophylline dose by 50%.",
     "Clin Pharmacokinet 2004 (PMID: 15535758); DailyMed Theo-24 Sec 7.2",
     "DRUG_THEOPHYLLINE_MONOGRAPH", "THEOPHYLLINE_MONOGRAPH"),
    # 18
    ("B01AF01", "J02AB02", "DOAC_AZOLE_DUAL_CLEARANCE_INHIB", "CONTRAINDICATED",
     "Dual potent inhibition of hepatic CYP3A4 degradation and P-gp efflux transport",
     "Systemic rivaroxaban AUC increases >150%, escalating fatal hemorrhagic stroke risks. Combination contraindicated.",
     "DailyMed Xarelto Sec 4; Br J Clin Pharmacol 2013 (PMID: 23594271)",
     "DRUG_RIVAROXABAN_MONOGRAPH", "RIVAROXABAN_MONOGRAPH"),
    # 19
    ("N03AF01", "G03AA07", "CARBAMAZEPINE_OCP_CONTRACEPTIVE_FAIL", "MAJOR",
     "Potent hepatic CYP3A4 and UGT enzyme induction accelerates hormone clearance",
     "Markedly lower hormone concentrations result in breakthrough ovulation and unplanned pregnancy. Advise non-hormonal contraception.",
     "CDC Contraceptive Guidance, MMWR 2016 (PMID: 27467196); DailyMed Tegretol Sec 7",
     "DRUG_CARBAMAZEPINE_MONOGRAPH", "CARBAMAZEPINE_MONOGRAPH"),
    # 20
    ("L04AD01", "A13A", "CYCLOSPORINE_STJOHNS_ORGAN_REJECT", "CONTRAINDICATED",
     "Hyperforin potently induces intestinal and hepatic CYP3A4 and P-gp expression",
     "Precipitous drops in cyclosporine trough levels below therapeutic ranges trigger acute allograft rejection. Concomitant administration contraindicated.",
     "Lancet 2000 (PMID: 10683003); DailyMed Sandimmune Sec 7.4",
     "DRUG_CYCLOSPORINE_MONOGRAPH", "CYCLOSPORINE_MONOGRAPH"),
    # 21
    ("M04AC01", "J01FA09", "COLCHICINE_MACROLIDE_FATAL_TOXICITY", "CONTRAINDICATED",
     "Dual inhibition of CYP3A4 metabolism and P-gp-mediated renal excretion",
     "Colchicine accumulation causes mitotic arrest, neuromuscular toxicity, multiorgan failure, and death. Strictly contraindicated in renal/hepatic impairment.",
     "FDA Safety Comm. (2009); DailyMed Colcrys Sec 4; PMID: 20516676",
     "DRUG_COLCHICINE_MONOGRAPH", "COLCHICINE_MONOGRAPH"),
    # 22
    ("H03AA01", "B03AA07", "LEVOTHYROXINE_IRON_CHELATION_HYPO", "MODERATE",
     "Intraluminal chelation: polyvalent iron ions bind levothyroxine into insoluble chelate",
     "Drastically impaired thyroxine absorption leads to clinical hypothyroidism. Separate oral administration by at least 4 hours.",
     "Thyroid 2014 (PMID: 25110870); DailyMed Synthroid Sec 7.1",
     "DRUG_LEVOTHYROXINE_MONOGRAPH", "LEVOTHYROXINE_MONOGRAPH"),
    # 23
    ("C10AA05", "C10AB04", "STATIN_FIBRATE_OATP1B1_RHABDOMYO", "MAJOR",
     "Gemfibrozil inhibits statin glucuronidation (UGT1A1/3) and blocks OATP1B1 hepatic uptake",
     "Increases circulating atorvastatin levels, escalating rhabdomyolysis and acute renal failure risks. If fibrate needed, fenofibrate preferred.",
     "Am J Cardiol 2007 (PMID: 17448743); DailyMed Lipitor Sec 7.1",
     "DRUG_ATORVASTATIN_MONOGRAPH", "ATORVASTATIN_MONOGRAPH"),
    # 24
    ("C08DB01", "C07AB03", "NONDIHYDRO_CCB_BB_AV_BLOCK", "MAJOR",
     "Synergistic negative chronotropic, dromotropic, and inotropic actions on SA/AV nodes",
     "Severe symptomatic sinus bradycardia, complete heart block, and acute heart failure. Avoid combination in left ventricular dysfunction.",
     "ACC/AHA Stable Ischemic Heart Disease Guidelines 2012 (PMID: 23166211); DailyMed Calan Sec 7",
     "DRUG_VERAPAMIL_MONOGRAPH", "VERAPAMIL_MONOGRAPH"),
    # 25
    ("N06AB04", "A04AA01", "SSRI_5HT3_QT_SUMMATION_ARRHYTHMIA", "MAJOR",
     "Additive pharmacodynamic blockade of myocardial hERG potassium channels",
     "Synergistic QTc prolongation increases incidence of ventricular tachydysrhythmias. Limit citalopram to 20 mg/day in adults >60.",
     "FDA Drug Safety Announce. (2012); DailyMed Celexa Sec 5.2; PMID: 24352720",
     "DRUG_CITALOPRAM_MONOGRAPH", "CITALOPRAM_MONOGRAPH"),
    # 26
    ("L04AD02", "J02AC03", "TACROLIMUS_VORICONAZOLE_NEPHROTOX", "MAJOR",
     "Potent inhibition of hepatic and intestinal mucosal CYP3A4 by voriconazole",
     "Increases tacrolimus AUC by 300%, causing severe nephrotoxicity and neurotoxicity. Preemptively reduce tacrolimus dose by 66%; monitor trough.",
     "Am J Transplant 2007 (PMID: 17391136); DailyMed Vfend Sec 7.1",
     "DRUG_TACROLIMUS_MONOGRAPH", "TACROLIMUS_MONOGRAPH"),
    # 27
    ("L02BA01", "N06AB05", "TAMOXIFEN_PAROXETINE_CYP2D6_FAILURE", "MAJOR",
     "Irreversible mechanism-based inhibition of hepatic CYP2D6 by paroxetine",
     "Prevents metabolic conversion of tamoxifen into active endoxifen, causing therapeutic failure and breast cancer relapse. Select venlafaxine.",
     "BMJ 2010 (PMID: 20142325); DailyMed Nolvadex Sec 7.1",
     "DRUG_TAMOXIFEN_MONOGRAPH", "TAMOXIFEN_MONOGRAPH"),
    # 28
    ("N03AB02", "N03AG01", "PHENYTOIN_VALPROATE_PROTEIN_DISPLACE", "MAJOR",
     "Valproate displaces phenytoin from albumin binding sites and inhibits CYP2C9",
     "Unbound (free active) phenytoin fractions increase significantly, precipitating ataxia, nystagmus, and delirium. Monitor free serum phenytoin.",
     "Epilepsia 2017 (PMID: 28267202); DailyMed Dilantin Sec 7.2",
     "DRUG_PHENYTOIN_MONOGRAPH", "PHENYTOIN_MONOGRAPH"),
    # 29
    ("C09AA05", "C09DX04", "NEPRILYSIN_ACEI_FATAL_ANGIOEDEMA", "CONTRAINDICATED",
     "Simultaneous inhibition of neprilysin and angiotensin-converting enzyme pathways",
     "Impairs metabolic degradation of bradykinin and substance P, triggering severe, potentially fatal airway angioedema. Enforce 36h washout.",
     "PARADIGM-HF, N Engl J Med 2014 (PMID: 25176015); DailyMed Entresto Sec 4",
     "DRUG_RAMIPRIL_MONOGRAPH", "RAMIPRIL_MONOGRAPH"),
    # 30
    ("N02AA01", "N05BA01", "OPIOID_BENZO_FATAL_RESP_DEPRESSION", "MAJOR",
     "Synergistic depressive actions on respiratory center neurons in the brainstem",
     "Produces profound sedation, hypercapnic respiratory depression, coma, and fatal respiratory arrest. Restrict to absolute minimum duration; co-prescribe naloxone.",
     "FDA Black Box Warning (2016); DailyMed MS Contin Sec 5.1; PMID: 28292881",
     "DRUG_MORPHINE_MONOGRAPH", "MORPHINE_MONOGRAPH"),
]

# Medical conditions / Disease taxonomy (ICD-10, ICD-11, English, Arabic, Category, Red-Flag Priority)
MEDICAL_CONDITIONS_DATA = [
    ("I21.9", "BA40.Z", "Acute Myocardial Infarction, Unspecified", "احتشاء عضلة القلب الحاد", "Cardiovascular", 1),
    ("I63.9", "8B11.Z", "Cerebral infarction, unspecified", "سكتة دماغية إقفارية حادة", "Neurology", 1),
    ("I26.9", "BD00.Z", "Pulmonary embolism without acute cor pulmonale", "انسداد رئوي حاد", "Respiratory", 1),
    ("T78.2", "4A84.0", "Anaphylactic shock, unspecified", "صدمة تأقية / حساسية مفرطة", "Immunology", 1),
    ("R65.21", "1G41", "Severe sepsis with septic shock", "صدمة إنتانية حادة", "Infectious Disease", 1),
    ("I46.9", "BC70.Z", "Cardiac arrest, cause unspecified", "توقف عضلة القلب", "Cardiovascular", 1),
    ("E11.9", "5A11", "Type 2 diabetes mellitus without complications", "داء السكري من النوع الثاني", "Endocrinology", 0),
    ("I10", "BA00", "Essential (primary) hypertension", "ارتفاع ضغط الدم الأساسي", "Cardiovascular", 0),
    ("N18.3", "GB61.3", "Chronic kidney disease, stage 3", "مرض الكلى المزمن - المرحلة الثالثة", "Nephrology", 0),
    ("N18.4", "GB61.4", "Chronic kidney disease, stage 4", "مرض الكلى المزمن - المرحلة الرابعة", "Nephrology", 0),
    ("N18.5", "GB61.5", "Chronic kidney disease, stage 5 (End stage)", "الفشل الكلوي في المرحلة النهائية", "Nephrology", 0),
    ("I50.9", "BD11.Z", "Heart failure, unspecified", "قصور عضلة القلب", "Cardiovascular", 0),
    ("I48.91", "BC81.3", "Unspecified atrial fibrillation", "الرجفان الأذيني", "Cardiovascular", 0),
    ("E78.5", "5C80.4", "Hyperlipidemia, unspecified", "فرط شحميات الدم", "Metabolic", 0),
    ("F32.9", "6A70", "Major depressive disorder, single episode, unspecified", "اضطراب الاكتئاب الحاد", "Psychiatry", 0),
    ("M06.9", "FA20.Z", "Rheumatoid arthritis, unspecified", "التهاب المفاصل الروماتويدي", "Rheumatology", 0),
    ("M10.0", "FA25.0", "Idiopathic gout", "مرض النقرس", "Rheumatology", 0),
    ("J45.9", "CA23", "Other and unspecified asthma", "الربو الشعبي", "Respiratory", 0),
    ("K21.9", "DA22.Z", "Gastro-esophageal reflux disease without esophagitis", "مرض الارتجاع المعدي المريئي", "Gastroenterology", 0),
    ("E03.9", "5A00", "Hypothyroidism, unspecified", "قصور الغدة الدرقية", "Endocrinology", 0),
]

# Lab diagnostic panels (LOINC, test_name, specimen, standard_unit, ref_low, ref_high, panic_low, panic_high)
LAB_DIAGNOSTIC_PANELS_DATA = [
    ("2823-3", "Potassium [Moles/volume] in Serum or Plasma", "Serum/Plasma", "mmol/L", 3.5, 5.0, 2.8, 6.2),
    ("2951-2", "Sodium [Moles/volume] in Serum or Plasma", "Serum/Plasma", "mmol/L", 136.0, 145.0, 120.0, 160.0),
    ("2160-0", "Creatinine [Mass/volume] in Serum or Plasma", "Serum/Plasma", "mg/dL", 0.7, 1.3, None, 5.0),
    ("33914-3", "Glomerular filtration rate/1.73 sq M.predicted (CKD-EPI)", "Serum/Plasma", "mL/min/1.73m2", 90.0, 120.0, 15.0, None),
    ("4548-4", "Hemoglobin A1c/Hemoglobin.total in Blood", "Whole Blood", "%", 4.0, 5.6, None, 10.0),
    ("1558-6", "Fasting Glucose [Mass/volume] in Serum or Plasma", "Serum/Plasma", "mg/dL", 70.0, 99.0, 50.0, 400.0),
    ("6301-6", "INR in Platelet poor plasma by Coagulation assay", "Plasma", "ratio", 0.9, 1.1, None, 4.5),
    ("10535-3", "Digoxin [Mass/volume] in Serum or Plasma", "Serum/Plasma", "ng/mL", 0.8, 2.0, None, 2.5),
    ("14334-7", "Lithium [Moles/volume] in Serum or Plasma", "Serum/Plasma", "mmol/L", 0.6, 1.2, None, 2.0),
    ("4086-5", "Theophylline [Mass/volume] in Serum or Plasma", "Serum/Plasma", "ug/mL", 10.0, 20.0, None, 25.0),
    ("15290-0", "Tacrolimus [Mass/volume] in Whole blood", "Whole Blood", "ng/mL", 5.0, 15.0, None, 20.0),
    ("3520-4", "Cyclosporine [Mass/volume] in Whole blood", "Whole Blood", "ng/mL", 100.0, 400.0, None, 500.0),
    ("1742-6", "Alanine aminotransferase (ALT) in Serum or Plasma", "Serum/Plasma", "U/L", 7.0, 56.0, None, 500.0),
    ("14958-3", "Albumin/Creatinine [Mass Ratio] in Urine", "Urine", "mg/g", 0.0, 30.0, None, 300.0),
    ("2571-8", "Triglyceride [Mass/volume] in Serum or Plasma", "Serum/Plasma", "mg/dL", 0.0, 150.0, None, 500.0),
    ("13457-7", "Cholesterol in LDL in Serum or Plasma by calculation", "Serum/Plasma", "mg/dL", 0.0, 100.0, None, 190.0),
]

# HITL Clinical Quarantine staging records
HITL_QUARANTINE_DATA = [
    (
        "https://www.fda.gov/drugs/drug-safety-and-availability/fda-updates-paxlovid-alerts",
        "Ritonavir-boosted nirmatrelvir introduces severe CYP3A4 inhibition with calcineurin inhibitors and statins.",
        "J05AE",
        json.dumps({"drug_a": "Paxlovid", "drug_b": "Tacrolimus", "severity": "CONTRAINDICATED", "evidence": "FDA Alert 2024"}),
        "37592014",
        "PENDING",
        "Requires licensed pharmacist validation prior to committing to production matrix.",
        None
    ),
    (
        "https://www.who.int/news/item/glp1-anesthesia-aspiration-risk",
        "GLP-1 receptor agonists induce delayed gastric emptying increasing pulmonary aspiration risk during general anesthesia.",
        "A10BJ06",
        json.dumps({"drug": "Semaglutide", "procedure": "General Anesthesia", "recommendation": "Withhold 1 week prior to elective surgery"}),
        "38291045",
        "PENDING",
        "Pending DHA clinical guideline circular alignment.",
        None
    ),
]


def build_clinical_database():
    os.makedirs(CORPUS_DIR, exist_ok=True)
    if os.path.exists(CLINICAL_DB_PATH):
        try:
            os.remove(CLINICAL_DB_PATH)
        except OSError:
            pass

    conn = sqlite3.connect(CLINICAL_DB_PATH)
    cur = conn.cursor()

    cur.execute("PRAGMA foreign_keys = ON;")

    # 1. Create drug_classes table
    cur.execute("""
    CREATE TABLE drug_classes (
        class_id INTEGER PRIMARY KEY AUTOINCREMENT,
        class_name TEXT NOT NULL UNIQUE COLLATE NOCASE,
        category TEXT NOT NULL,
        default_cyp_profile TEXT DEFAULT 'NONE'
    );
    """)

    # 2. Create formulary_drugs table (Canonical Master Spec Section 2.1)
    cur.execute("""
    CREATE TABLE formulary_drugs (
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
    """)
    cur.execute("CREATE INDEX idx_formulary_canonical ON formulary_drugs(canonical_name);")
    cur.execute("CREATE INDEX idx_formulary_arabic ON formulary_drugs(arabic_name);")

    # 3. Create backward-compatible drugs table for C99 native library & python bridge
    cur.execute("""
    CREATE TABLE drugs (
        drug_id INTEGER PRIMARY KEY AUTOINCREMENT,
        canonical_name TEXT NOT NULL UNIQUE COLLATE NOCASE,
        arabic_name TEXT NOT NULL COLLATE NOCASE,
        brand_names_json TEXT NOT NULL,
        atc_code TEXT NOT NULL UNIQUE,
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
        p_gp_profile TEXT DEFAULT 'NONE',
        qt_risk TEXT DEFAULT 'NONE',
        hyperkalemia_risk BOOLEAN DEFAULT 0,
        serotonin_risk BOOLEAN DEFAULT 0,
        indications_icd10_json TEXT NOT NULL,
        clinical_pearls TEXT,
        FOREIGN KEY(drug_class_id) REFERENCES drug_classes(class_id)
    );
    """)
    cur.execute("CREATE INDEX idx_drugs_name ON drugs(canonical_name);")
    cur.execute("CREATE INDEX idx_drugs_arabic ON drugs(arabic_name);")
    cur.execute("CREATE INDEX idx_drugs_atc ON drugs(atc_code);")

    # 4. Create drug_interactions table (Master Spec Section 2.1 + backward compatibility columns)
    cur.execute("""
    CREATE TABLE drug_interactions (
        interaction_id INTEGER PRIMARY KEY AUTOINCREMENT,
        drug_a_atc VARCHAR(10) NOT NULL,
        drug_b_atc VARCHAR(10) NOT NULL,
        alert_code VARCHAR(64) UNIQUE NOT NULL,
        severity_level VARCHAR(16) CHECK(severity_level IN ('CONTRAINDICATED', 'MAJOR', 'MODERATE')),
        mechanism_text TEXT NOT NULL,
        clinical_action TEXT NOT NULL,
        evidence_citation TEXT NOT NULL,
        -- Backward-compatible columns for C adapter
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
    """)
    cur.execute("CREATE INDEX idx_interactions_atc_pair ON drug_interactions(drug_a_atc, drug_b_atc);")
    cur.execute("CREATE INDEX idx_interactions_alert ON drug_interactions(alert_code);")

    # 5. Create medical_conditions table (Master Spec Section 2.1)
    cur.execute("""
    CREATE TABLE medical_conditions (
        icd10_code VARCHAR(10) PRIMARY KEY,
        icd11_code VARCHAR(10),
        condition_name_en VARCHAR(256) NOT NULL,
        condition_name_ar VARCHAR(256) NOT NULL,
        category VARCHAR(64) NOT NULL,
        red_flag_priority BOOLEAN DEFAULT 0
    );
    """)
    cur.execute("CREATE INDEX idx_med_conditions_cat ON medical_conditions(category);")
    cur.execute("CREATE INDEX idx_med_conditions_redflag ON medical_conditions(red_flag_priority);")

    # 6. Create lab_diagnostic_panels table (Master Spec Section 2.1)
    cur.execute("""
    CREATE TABLE lab_diagnostic_panels (
        loinc_code VARCHAR(16) PRIMARY KEY,
        test_name VARCHAR(128) NOT NULL,
        specimen VARCHAR(32) NOT NULL,
        standard_unit VARCHAR(32) NOT NULL,
        reference_low REAL,
        reference_high REAL,
        critical_panic_low REAL,
        critical_panic_high REAL
    );
    """)
    cur.execute("CREATE INDEX idx_lab_panels_name ON lab_diagnostic_panels(test_name);")

    # 7. Create hitl_clinical_quarantine table (Master Spec Section 2.1)
    cur.execute("""
    CREATE TABLE hitl_clinical_quarantine (
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
    """)
    cur.execute("CREATE INDEX idx_quarantine_status ON hitl_clinical_quarantine(review_status);")

    # 8. Create renal_dosing_guidelines table
    cur.execute("""
    CREATE TABLE renal_dosing_guidelines (
        rule_id INTEGER PRIMARY KEY AUTOINCREMENT,
        drug_id INTEGER NOT NULL,
        egfr_min REAL NOT NULL,
        egfr_max REAL NOT NULL,
        recommended_dose TEXT NOT NULL,
        frequency_hours INTEGER NOT NULL,
        contraindicated BOOLEAN DEFAULT 0,
        clinical_rationale TEXT NOT NULL,
        evidence_sha256 TEXT NOT NULL,
        FOREIGN KEY(drug_id) REFERENCES drugs(drug_id)
    );
    """)

    # 9. Create clinical_encounters table
    cur.execute("""
    CREATE TABLE clinical_encounters (
        encounter_id TEXT PRIMARY KEY,
        patient_id TEXT NOT NULL,
        timestamp_utc INTEGER NOT NULL,
        baseline_egfr REAL NOT NULL,
        weight_kg REAL NOT NULL,
        age_years INTEGER NOT NULL,
        gender TEXT NOT NULL,
        active_medications_json TEXT NOT NULL,
        allergies_json TEXT NOT NULL,
        icd10_codes_json TEXT NOT NULL,
        soap_note_text TEXT,
        hmac_signature TEXT NOT NULL
    );
    """)

    # Populate drug_classes
    for row in DRUG_CLASSES_DATA:
        cur.execute(
            "INSERT INTO drug_classes (class_id, class_name, category, default_cyp_profile) VALUES (?, ?, ?, ?)",
            row
        )

    # Populate drugs and formulary_drugs
    drug_id_map = {}
    atc_to_drug_id_map = {}
    name_to_atc_map = {}

    # CYP2C19 profiling heuristics
    cyp2c19_inhibitors = {"A02BC01", "A02BC05", "J02AC01", "J02AB02", "J02AC03", "C01BD01", "N03AG01"}
    cyp2c19_substrates = {"B01AC04", "N05BA01", "N06AB04", "N06AB10", "N03AB02"}
    cyp2c19_inducers = {"N03AF01", "A13A"}

    for d in DRUGS_DATA:
        (
            canon_name, ar_name, brands, atc, class_id, renal_cutoff, hepatic_pct,
            max_dose, is_ctrl, sched, narrow, cyp3a4, cyp2d6, cyp2c9, pgp, qt,
            hyperk, sero, icd10, pearls
        ) = d

        if atc in cyp2c19_inhibitors:
            cyp2c19 = "INHIBITOR"
        elif atc in cyp2c19_substrates:
            cyp2c19 = "SUBSTRATE"
        elif atc in cyp2c19_inducers:
            cyp2c19 = "INDUCER"
        else:
            cyp2c19 = "NONE"

        # Insert into formulary_drugs
        cur.execute("""
        INSERT INTO formulary_drugs (
            atc_code, canonical_name, arabic_name, brand_names_json, drug_class_id,
            renal_egfr_cutoff, hepatic_metabolism_pct, max_daily_dose_mg,
            is_controlled_uae, schedule_law_30_2021, narrow_therapeutic_index,
            cyp3a4_profile, cyp2d6_profile, cyp2c9_profile, cyp2c19_profile, p_gp_profile,
            qt_risk, hyperkalemia_risk, serotonin_risk,
            indications_icd10_json, clinical_pearls
        ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
        """, (
            atc, canon_name, ar_name, json.dumps(brands), class_id,
            renal_cutoff, hepatic_pct, max_dose,
            is_ctrl, sched, narrow,
            cyp3a4, cyp2d6, cyp2c9, cyp2c19, pgp,
            qt, hyperk, sero,
            json.dumps(icd10), pearls
        ))

        # Insert into drugs
        cur.execute("""
        INSERT INTO drugs (
            canonical_name, arabic_name, brand_names_json, atc_code, drug_class_id,
            renal_egfr_cutoff, hepatic_metabolism_pct, max_daily_dose_mg,
            is_controlled_uae, schedule_law_30_2021, narrow_therapeutic_index,
            cyp3a4_profile, cyp2d6_profile, cyp2c9_profile, p_gp_profile,
            qt_risk, hyperkalemia_risk, serotonin_risk,
            indications_icd10_json, clinical_pearls
        ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
        """, (
            canon_name, ar_name, json.dumps(brands), atc, class_id,
            renal_cutoff, hepatic_pct, max_dose,
            is_ctrl, sched, narrow,
            cyp3a4, cyp2d6, cyp2c9, pgp,
            qt, hyperk, sero,
            json.dumps(icd10), pearls
        ))
        row_id = cur.lastrowid
        drug_id_map[canon_name.lower()] = row_id
        atc_to_drug_id_map[atc] = row_id
        name_to_atc_map[canon_name.lower()] = atc

    # Populate 30 Section 2.2 interactions
    for inter in INTERACTIONS_30_SPEC:
        (
            drug_a_atc, drug_b_atc, alert_code, severity_level,
            mech_text, clin_action, ev_citation, doc_id, section
        ) = inter

        da_id = atc_to_drug_id_map.get(drug_a_atc)
        db_id = atc_to_drug_id_map.get(drug_b_atc)
        ev_hash = hash_evidence(clin_action)

        cur.execute("""
        INSERT INTO drug_interactions (
            drug_a_atc, drug_b_atc, alert_code, severity_level,
            mechanism_text, clinical_action, evidence_citation,
            drug_a_id, drug_b_id, severity, clinical_rationale,
            action_directive, evidence_doc_id, evidence_section,
            evidence_sha256, mechanism
        ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
        """, (
            drug_a_atc, drug_b_atc, alert_code, severity_level,
            mech_text, clin_action, ev_citation,
            da_id, db_id, severity_level, clin_action,
            clin_action, doc_id, section,
            ev_hash, mech_text
        ))

    # Populate legacy interactions for backward compatibility (with INSERT OR IGNORE)
    for inter in DRUG_INTERACTIONS_DATA:
        (
            drug_a, drug_b, severity, alert_code, rationale, action,
            doc_id, section
        ) = inter
        da_id = drug_id_map.get(drug_a.lower())
        db_id = drug_id_map.get(drug_b.lower())
        da_atc = name_to_atc_map.get(drug_a.lower())
        db_atc = name_to_atc_map.get(drug_b.lower())
        if not da_atc or not db_atc:
            continue
        ev_hash = hash_evidence(rationale)

        cur.execute("""
        INSERT OR IGNORE INTO drug_interactions (
            drug_a_atc, drug_b_atc, alert_code, severity_level,
            mechanism_text, clinical_action, evidence_citation,
            drug_a_id, drug_b_id, severity,
            clinical_rationale, action_directive,
            evidence_doc_id, evidence_section, evidence_sha256, mechanism
        ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
        """, (
            da_atc, db_atc, alert_code, severity,
            rationale, action, f"Legacy ({doc_id} {section})",
            da_id, db_id, severity,
            rationale, action,
            doc_id, section, ev_hash, rationale
        ))

    # Populate medical_conditions
    for cond in MEDICAL_CONDITIONS_DATA:
        cur.execute("""
        INSERT INTO medical_conditions (
            icd10_code, icd11_code, condition_name_en, condition_name_ar, category, red_flag_priority
        ) VALUES (?, ?, ?, ?, ?, ?)
        """, cond)

    # Populate lab_diagnostic_panels
    for panel in LAB_DIAGNOSTIC_PANELS_DATA:
        cur.execute("""
        INSERT INTO lab_diagnostic_panels (
            loinc_code, test_name, specimen, standard_unit,
            reference_low, reference_high, critical_panic_low, critical_panic_high
        ) VALUES (?, ?, ?, ?, ?, ?, ?, ?)
        """, panel)

    # Populate hitl_clinical_quarantine
    for q in HITL_QUARANTINE_DATA:
        cur.execute("""
        INSERT INTO hitl_clinical_quarantine (
            source_url, extracted_finding, target_atc_code, proposed_interaction_json,
            evidence_pmid, review_status, reviewer_notes, reviewed_at
        ) VALUES (?, ?, ?, ?, ?, ?, ?, ?)
        """, q)

    # Populate renal_dosing_guidelines
    for rule in RENAL_DOSING_DATA:
        (
            drug_name, egfr_min, egfr_max, dose, freq, contra, rationale
        ) = rule
        d_id = drug_id_map.get(drug_name.lower())
        if d_id is None:
            continue
        ev_hash = hash_evidence(rationale)
        cur.execute("""
        INSERT INTO renal_dosing_guidelines (
            drug_id, egfr_min, egfr_max, recommended_dose, frequency_hours,
            contraindicated, clinical_rationale, evidence_sha256
        ) VALUES (?, ?, ?, ?, ?, ?, ?, ?)
        """, (
            d_id, egfr_min, egfr_max, dose, freq,
            contra, rationale, ev_hash
        ))

    conn.commit()

    # Integrity verification
    cur.execute("SELECT COUNT(*) FROM formulary_drugs")
    formulary_count = cur.fetchone()[0]
    cur.execute("SELECT COUNT(*) FROM drugs")
    drug_count = cur.fetchone()[0]
    cur.execute("SELECT COUNT(*) FROM drug_classes")
    class_count = cur.fetchone()[0]
    cur.execute("SELECT COUNT(*) FROM drug_interactions")
    inter_count = cur.fetchone()[0]
    cur.execute("SELECT COUNT(*) FROM medical_conditions")
    cond_count = cur.fetchone()[0]
    cur.execute("SELECT COUNT(*) FROM lab_diagnostic_panels")
    lab_count = cur.fetchone()[0]
    cur.execute("SELECT COUNT(*) FROM hitl_clinical_quarantine")
    quarantine_count = cur.fetchone()[0]
    cur.execute("SELECT COUNT(*) FROM renal_dosing_guidelines")
    renal_count = cur.fetchone()[0]

    conn.close()

    print(f"[SUCCESS] Built Taryaq Sovereign Clinical Database: {CLINICAL_DB_PATH}")
    print(f"          - Formulary Drugs: {formulary_count} (Mandate: 100+ across 15 categories)")
    print(f"          - Legacy Drugs: {drug_count}")
    print(f"          - Drug Classes: {class_count}")
    print(f"          - Clinical Interactions: {inter_count}")
    print(f"          - Medical Conditions: {cond_count}")
    print(f"          - Lab Diagnostic Panels: {lab_count}")
    print(f"          - HITL Quarantine Buffer: {quarantine_count}")
    print(f"          - Renal Dosing Guidelines: {renal_count}")
    return formulary_count, class_count, inter_count, renal_count


if __name__ == '__main__':
    build_clinical_database()
