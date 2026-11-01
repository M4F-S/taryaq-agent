//! Patient Health Companion Interface (Onlim Pattern)
//! Dubai Create AI Agents Championship 2026–2027

use crate::safety::TaryaqTriageLevel;
use serde::{Deserialize, Serialize};

#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
pub enum DialectCategory {
    Gulf,      // اللهجة الخليجية / الإماراتية
    Egyptian,  // اللهجة المصرية
    Levantine, // اللهجة الشامية
    StandardArabic,
    English,
}

#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
pub enum RedFlagCategory {
    CardiovascularAcute,
    NeurologicalStrokeFast,
    RespiratoryAnaphylaxisAcute,
    SevereHemorrhage,
    CardiacArrestSyncope,
    None,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct EmergencyDispatchDirective {
    pub is_emergency: bool,
    pub requires_immediate_dispatch: bool,
    pub red_flag_category: RedFlagCategory,
    pub triage_level: TaryaqTriageLevel,
    pub emergency_dispatch_numbers: Vec<String>,
    pub arabic_emergency_alert: String,
    pub english_emergency_alert: String,
    pub clinical_rationale: String,
    pub matched_trigger: String,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct PosologyGuidance {
    pub drug_name: String,
    pub timing_instruction_ar: String,
    pub timing_instruction_en: String,
    pub food_relation_ar: String,
    pub food_relation_en: String,
    pub key_precautions_ar: String,
    pub key_precautions_en: String,
    pub what_to_avoid_ar: String,
    pub what_to_avoid_en: String,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct PatientCompanionConsultation {
    pub patient_input: String,
    pub detected_dialect: DialectCategory,
    pub emergency_directive: Option<EmergencyDispatchDirective>,
    pub triage_level: TaryaqTriageLevel,
    pub conversational_response_ar: String,
    pub conversational_response_en: String,
    pub medication_guidance: Vec<PosologyGuidance>,
    pub lifestyle_counseling_ar: String,
    pub lifestyle_counseling_en: String,
    pub next_refill_reminder: Option<String>,
}

pub struct PatientHealthCompanion;

impl PatientHealthCompanion {
    pub fn new() -> Self {
        Self
    }

    /// Intake patient symptoms (in dialectal Arabic or English) and process triage & posology counseling
    pub fn consult(
        &self,
        patient_message: &str,
        active_medications: &[String],
        _patient_egfr: Option<f64>,
    ) -> PatientCompanionConsultation {
        let detected_dialect = detect_dialect(patient_message);

        // 1. Evaluate Emergency Red-Flag Triage
        let emergency_directive = evaluate_red_flag_triage(patient_message);

        if let Some(ref em) = emergency_directive {
            if em.requires_immediate_dispatch {
                return PatientCompanionConsultation {
                    patient_input: patient_message.to_string(),
                    detected_dialect,
                    emergency_directive: Some(em.clone()),
                    triage_level: TaryaqTriageLevel::TriageEmergency,
                    conversational_response_ar: em.arabic_emergency_alert.clone(),
                    conversational_response_en: em.english_emergency_alert.clone(),
                    medication_guidance: Vec::new(),
                    lifestyle_counseling_ar: "حالة طوارئ فورية - تم تعليق الإرشادات الدوائية الروتينية لحين الفحص الطبي العاجل.".to_string(),
                    lifestyle_counseling_en: "Critical medical emergency - routine posology counseling suspended for immediate clinical dispatch.".to_string(),
                    next_refill_reminder: None,
                };
            }
        }

        // 2. Generate Plain-Language Medication Counseling
        let medication_guidance = generate_posology_guidance(active_medications);

        // 3. Formulate empathetic conversational guidance
        let (resp_ar, resp_en) = formulate_counseling_response(
            patient_message,
            &detected_dialect,
            &medication_guidance,
        );

        let (life_ar, life_en) = formulate_lifestyle_counseling(&medication_guidance);

        PatientCompanionConsultation {
            patient_input: patient_message.to_string(),
            detected_dialect,
            emergency_directive,
            triage_level: TaryaqTriageLevel::TriageRoutine,
            conversational_response_ar: resp_ar,
            conversational_response_en: resp_en,
            medication_guidance,
            lifestyle_counseling_ar: life_ar,
            lifestyle_counseling_en: life_en,
            next_refill_reminder: Some("Refill window opens in 7 days before supply exhaustion.".to_string()),
        }
    }
}

/// Detect spoken or written dialect
fn detect_dialect(text: &str) -> DialectCategory {
    let _lower = text.to_lowercase();

    // Check for English characters predominance
    let ascii_letters = text.chars().filter(|c| c.is_ascii_alphabetic()).count();
    let arabic_letters = text.chars().filter(|c| ('\u{0600}'..='\u{06FF}').contains(c)).count();

    if ascii_letters > arabic_letters && ascii_letters > 5 {
        return DialectCategory::English;
    }

    // Gulf (Khaliji / Emirati) dialect triggers
    let gulf_patterns = [
        "عوار", "نغز", "كتمة", "وايد", "أحس", "شسوي", "تعبان حيل", "حاس بدوخة", "ما أقدر", "خلاص", "صدري يعورني", "مو قادر", "حيل"
    ];

    // Egyptian dialect triggers
    let egyptian_patterns = [
        "بيوجعني", "أوي", "سكاكين", "مش قادر", "مش عارف", "وشه", "وشي", "خالص", "بنهج", "جامد", "دلوقتي", "عشان", "ازاي", "الحقني"
    ];

    // Levantine dialect triggers
    let levantine_patterns = [
        "عم يوجعني", "كتير", "خنقة", "شو أعمل", "عم بستفرغ", "هيك", "هلق", "منيح", "تنميل", "حاسس بخنقة", "عم "
    ];

    let mut gulf_score = 0;
    for p in &gulf_patterns {
        if text.contains(p) {
            gulf_score += 1;
        }
    }

    let mut egyptian_score = 0;
    for p in &egyptian_patterns {
        if text.contains(p) {
            egyptian_score += 1;
        }
    }

    let mut levantine_score = 0;
    for p in &levantine_patterns {
        if text.contains(p) {
            levantine_score += 1;
        }
    }

    let max_score = gulf_score.max(egyptian_score).max(levantine_score);
    if max_score > 0 {
        if levantine_score == max_score {
            return DialectCategory::Levantine;
        } else if egyptian_score == max_score {
            return DialectCategory::Egyptian;
        } else if gulf_score == max_score {
            return DialectCategory::Gulf;
        }
    }

    if arabic_letters > 0 {
        DialectCategory::StandardArabic
    } else {
        DialectCategory::English
    }
}

/// Evaluate emergency red-flag symptoms with immediate 998/999 dispatch
fn evaluate_red_flag_triage(text: &str) -> Option<EmergencyDispatchDirective> {
    let lower = text.to_lowercase();

    // 1. Cardiovascular Acute (Crushing chest pain, radiation, pressure)
    let cv_triggers = [
        "chest pain", "crushing chest", "radiating to jaw", "radiating to arm", "heart attack",
        "عوار بالصدر", "نغز بالصدر", "ألم في الصدر", "ألم بالصدر", "صدري بيوجعني", "سكاكين في صدري",
        "عم يوجعني صدري", "عوار شديد في قلبي", "ضغط قوي على صدري", "ألم يعصر صدري", "عوار", "نغز"
    ];
    let has_chest_indication = text.contains("صدر") || text.contains("قلب") || lower.contains("chest") || lower.contains("heart");
    for t in &cv_triggers {
        if lower.contains(t) || (text.contains(t) && has_chest_indication) || text.contains("عوار بالصدر") || text.contains("نغز بالصدر") {
            return Some(EmergencyDispatchDirective {
                is_emergency: true,
                requires_immediate_dispatch: true,
                red_flag_category: RedFlagCategory::CardiovascularAcute,
                triage_level: TaryaqTriageLevel::TriageEmergency,
                emergency_dispatch_numbers: vec!["998".to_string(), "999".to_string()],
                arabic_emergency_alert: "🚨 تنبيه طوارئ طبي عاجل (إسعاف دبي): أعراض ألم وضغط الصدر تستدعي تدخلاً إسعافياً فورياً! اتصل بالإسعاف على الرقم 998 فوراً أو الطوارئ 999. لا تقم بقيادة السيارة بنفسك واجلس في وضعية مريحة حتى وصول المسعفين.".to_string(),
                english_emergency_alert: "🚨 CRITICAL EMERGENCY DIRECTIVE (DUBAI AMBULANCE): Acute chest pain symptoms indicate potential myocardial infarction! Immediately call 998 (Dubai Ambulance) or 999 (Emergency). Do not drive yourself. Rest comfortably until paramedics arrive.".to_string(),
                clinical_rationale: "Acute coronary syndrome (ACS) / Myocardial infarction red-flag symptoms detected.".to_string(),
                matched_trigger: t.to_string(),
            });
        }
    }

    // 2. Neurological Stroke (FAST: Facial droop, Arm weakness, Slurred speech)
    let stroke_triggers = [
        "facial droop", "slurred speech", "weakness in arm", "unilateral weakness", "stroke",
        "سكتة دماغية", "شلل في الوجه", "وشي معوج", "وشه معوج", "مش عارف اتكلم", "مش عارف يتكلم", "ثقل في اللسان", "ثقل بلساني", "ثقل باللسان", "تنميل نصفي",
        "ضعف في اليد", "صعوبة في النطق", "تنميل بنص وجهي"
    ];
    for t in &stroke_triggers {
        if lower.contains(t) || text.contains(t) {
            return Some(EmergencyDispatchDirective {
                is_emergency: true,
                requires_immediate_dispatch: true,
                red_flag_category: RedFlagCategory::NeurologicalStrokeFast,
                triage_level: TaryaqTriageLevel::TriageEmergency,
                emergency_dispatch_numbers: vec!["998".to_string(), "999".to_string()],
                arabic_emergency_alert: "🚨 تنبيه طوارئ سكتة دماغية (FAST): ظهور ثقل اللسان أو اعوجاج الوجه أو ضعف الأطراف يستدعي الاتصال الفوري بالإسعاف على الرقم 998! كل دقيقة تصنع فارقاً لإنقاذ خلايا الدماغ.".to_string(),
                english_emergency_alert: "🚨 CRITICAL STROKE ALERT (FAST PROTOCOL): Sudden facial drooping, speech difficulty, or extremity weakness requires immediate dispatch. Call 998 (Ambulance) now. Time is brain tissue.".to_string(),
                clinical_rationale: "Acute neurological deficit meeting FAST stroke criteria.".to_string(),
                matched_trigger: t.to_string(),
            });
        }
    }

    // 3. Respiratory Anaphylaxis / Severe Dyspnea
    let resp_triggers = [
        "can't breathe", "suffocating", "severe shortness of breath", "throat swelling", "anaphylaxis",
        "اختناق", "مش قادر أتنفس خالص", "عم اختنق", "كتمة شديدة", "تورم في الحلق", "صعوبة شديدة في التنفس", "حاسس بخنقة"
    ];
    for t in &resp_triggers {
        if lower.contains(t) || text.contains(t) {
            return Some(EmergencyDispatchDirective {
                is_emergency: true,
                requires_immediate_dispatch: true,
                red_flag_category: RedFlagCategory::RespiratoryAnaphylaxisAcute,
                triage_level: TaryaqTriageLevel::TriageEmergency,
                emergency_dispatch_numbers: vec!["998".to_string(), "999".to_string()],
                arabic_emergency_alert: "🚨 تنبيه طوارئ تنفسي عاجل: صعوبة التنفس الحادة أو الشعور بالاختناق تتطلب رعاية إسعافية فورية. اتصل بالرقم 998 الآن واجلس بوضعية قائمة.".to_string(),
                english_emergency_alert: "🚨 CRITICAL RESPIRATORY ALERT: Acute dyspnea or choking requires immediate paramedic assistance. Call 998 immediately and sit upright.".to_string(),
                clinical_rationale: "Acute respiratory distress / potential airway compromise.".to_string(),
                matched_trigger: t.to_string(),
            });
        }
    }

    // 4. Severe Hemorrhage
    let bleed_triggers = [
        "vomiting blood", "coughing blood", "severe bleeding", "massive hemorrhage",
        "ترجيع دم", "استفراغ دم", "عم بستفرغ دم", "نزيف حاد", "سعال مع دم", "دم كثير"
    ];
    for t in &bleed_triggers {
        if lower.contains(t) || text.contains(t) {
            return Some(EmergencyDispatchDirective {
                is_emergency: true,
                requires_immediate_dispatch: true,
                red_flag_category: RedFlagCategory::SevereHemorrhage,
                triage_level: TaryaqTriageLevel::TriageEmergency,
                emergency_dispatch_numbers: vec!["998".to_string(), "999".to_string()],
                arabic_emergency_alert: "🚨 تنبيه طوارئ نزيف حاد: تقيؤ الدم أو النزيف الشديد يستدعي نقلاً عاجلاً إلى الطوارئ. اتصل بالإسعاف على الرقم 998 فوراً.".to_string(),
                english_emergency_alert: "🚨 CRITICAL HEMORRHAGE ALERT: Hematemesis or massive acute bleeding requires immediate emergency medical transport. Call 998 now.".to_string(),
                clinical_rationale: "Severe gastrointestinal or systemic hemorrhage.".to_string(),
                matched_trigger: t.to_string(),
            });
        }
    }

    None
}

/// Plain-language medication counseling & posology explanation
fn generate_posology_guidance(medications: &[String]) -> Vec<PosologyGuidance> {
    let mut guidance = Vec::new();

    for med in medications {
        let lower = med.to_lowercase();

        if lower.contains("metformin") {
            guidance.push(PosologyGuidance {
                drug_name: med.clone(),
                timing_instruction_ar: "تناول الميتفورمين مع وجبة الطعام الرئيسية (الفطور أو العشاء).".to_string(),
                timing_instruction_en: "Take Metformin with meals (breakfast or dinner).".to_string(),
                food_relation_ar: "تناوله دائماً وسط الطعام لتقليل اضطراب المعدة والشعور بالغثيان.".to_string(),
                food_relation_en: "Always take with food to minimize gastrointestinal discomfort and nausea.".to_string(),
                key_precautions_ar: "احرص على شرب كميات كافية من الماء، وتوقف عنه وأبلغ طبيبك فوراً إذا شعرت بإرهاق شديد أو ألم عضلي غير معتاد.".to_string(),
                key_precautions_en: "Stay well hydrated. Report unexplained severe muscle fatigue or weakness to your doctor promptly.".to_string(),
                what_to_avoid_ar: "تجنب المشروبات الكحولية وتجنب الصيام الطويل دون استشارة الطبيب.".to_string(),
                what_to_avoid_en: "Avoid excessive alcohol intake and prolonged unmonitored fasting.".to_string(),
            });
        } else if lower.contains("lisinopril") || lower.contains("ramipril") || lower.contains("enalapril") {
            guidance.push(PosologyGuidance {
                drug_name: med.clone(),
                timing_instruction_ar: "تناول حبة واحدة صباحاً كل يوم في نفس الموعد.".to_string(),
                timing_instruction_en: "Take one tablet every morning at the same time.".to_string(),
                food_relation_ar: "يمكن تناوله مع الطعام أو بدونه.".to_string(),
                food_relation_en: "May be taken with or without food.".to_string(),
                key_precautions_ar: "قد تشعر بدوخة خفيفة عند الوقوف المفاجئ؛ قف ببطء. أبلغ طبيبك إذا ظهر سعال جاف مستمر.".to_string(),
                key_precautions_en: "Rise slowly from seated positions to prevent dizziness. Contact clinician if persistent dry cough develops.".to_string(),
                what_to_avoid_ar: "تجنب مسكنات البروفين (NSAIDs) وبدائل ملح الطعام التي تحتوي على البوتاسيوم دون إذن الطبيب.".to_string(),
                what_to_avoid_en: "Avoid regular NSAIDs (Ibuprofen) and potassium-based salt substitutes without physician approval.".to_string(),
            });
        } else if lower.contains("atorvastatin") || lower.contains("simvastatin") {
            guidance.push(PosologyGuidance {
                drug_name: med.clone(),
                timing_instruction_ar: "يفضل تناول حبة الستاتين مساءً قبل النوم.".to_string(),
                timing_instruction_en: "Preferably take your statin tablet in the evening before bed.".to_string(),
                food_relation_ar: "يمكن تناوله مع العشاء أو قبل النوم مباشرة مع كوب ماء.".to_string(),
                food_relation_en: "May be taken with evening meal or directly at bedtime with water.".to_string(),
                key_precautions_ar: "أبلغ الطبيب أو الصيدلي فوراً إذا شعرت بآلام أو ضعف عضلي شديد غير مبرر.".to_string(),
                key_precautions_en: "Report any unexplained severe muscle pain, tenderness, or weakness immediately.".to_string(),
                what_to_avoid_ar: "تجنب تناول الجريب فروت أو عصير الجريب فروت تماماً لأنه يزيد من تركيز الدواء في الدم.".to_string(),
                what_to_avoid_en: "Strictly avoid grapefruit and grapefruit juice as it increases drug concentration.".to_string(),
            });
        } else if lower.contains("warfarin") {
            guidance.push(PosologyGuidance {
                drug_name: med.clone(),
                timing_instruction_ar: "تناول جرعة الوارفارين المحددة يومياً في نفس الساعة تماماً (مساءً 6:00 م).".to_string(),
                timing_instruction_en: "Take prescribed Warfarin dose at the exact same hour daily (e.g. 6:00 PM).".to_string(),
                food_relation_ar: "حافظ على ثبات كمية الخضروات الورقية الخضراء يومياً دون تغييرات مفاجئة.".to_string(),
                food_relation_en: "Maintain consistent dietary intake of green leafy vegetables (Vitamin K).".to_string(),
                key_precautions_ar: "التزم بفحص سيولة الدم (INR) الدوري في الموعد المحدد.".to_string(),
                key_precautions_en: "Adhere strictly to scheduled routine INR blood coagulation monitoring.".to_string(),
                what_to_avoid_ar: "لا تتناول أي مسكنات مثل البروفين أو الأسبرين أو أي مكملات عشبية دون مراجعة الصيدلي.".to_string(),
                what_to_avoid_en: "Never take NSAIDs, Aspirin, or herbal supplements without prior clinical pharmacist clearance.".to_string(),
            });
        } else {
            guidance.push(PosologyGuidance {
                drug_name: med.clone(),
                timing_instruction_ar: "تناول الدواء بانتظام وفق الجرعة والجدول الزمني الموصوف.".to_string(),
                timing_instruction_en: "Take medication regularly according to the prescribed posology schedule.".to_string(),
                food_relation_ar: "تناوله مع كوب كامل من الماء النقي.".to_string(),
                food_relation_en: "Take with a full glass of water.".to_string(),
                key_precautions_ar: "لا تتوقف عن تناول الدواء دون استشارة طبيبك أو صيدلانيك.".to_string(),
                key_precautions_en: "Do not discontinue therapy abruptly without consulting your healthcare provider.".to_string(),
                what_to_avoid_ar: "تجنب مضاعفة الجرعة في حال نسيانها.".to_string(),
                what_to_avoid_en: "Do not double up doses if a previous dose was missed.".to_string(),
            });
        }
    }

    guidance
}

fn formulate_counseling_response(
    _input: &str,
    dialect: &DialectCategory,
    guidance: &[PosologyGuidance],
) -> (String, String) {
    let mut ar = String::new();
    let mut en = String::new();

    // Empathetic dialect-adapted greeting
    match dialect {
        DialectCategory::Gulf => {
            ar.push_str("يا هلا فيك وسلامتك إن شاء الله، ما تشوف شر. ");
        }
        DialectCategory::Egyptian => {
            ar.push_str("أهلاً بحضرتك وألف سلامة عليك، إن شاء الله تكون بخير. ");
        }
        DialectCategory::Levantine => {
            ar.push_str("مية أهلاً وسلامتك وألف عافية يا رب، ما على قلبك شر. ");
        }
        _ => {
            ar.push_str("أهلاً بك، متمنين لك دوام الصحة والعافية. ");
        }
    }

    ar.push_str("قمت بمراجعة جدول أدويتك بعناية لضمان سلامتك واستفادتك القصوى من العلاج:\n");
    en.push_str("Hello, wishing you vibrant health and well-being. I have carefully reviewed your medication schedule:\n");

    for g in guidance {
        ar.push_str(&format!(
            "\n• **{}**:\n  - الموعد: {}\n  - مع الطعام: {}\n  - تنبيه هام: {}\n",
            g.drug_name, g.timing_instruction_ar, g.food_relation_ar, g.key_precautions_ar
        ));
        en.push_str(&format!(
            "\n• **{}**:\n  - Timing: {}\n  - Food: {}\n  - Precaution: {}\n",
            g.drug_name, g.timing_instruction_en, g.food_relation_en, g.key_precautions_en
        ));
    }

    (ar, en)
}

fn formulate_lifestyle_counseling(guidance: &[PosologyGuidance]) -> (String, String) {
    let mut ar = String::from("إرشادات العافية ونمط الحياة:\n");
    ar.push_str("- احرص على تناول كميات كافية من الماء (1.5 إلى 2 لتر يومياً ما لم يوصِ الطبيب بخلاف ذلك).\n");
    ar.push_str("- التزم بمواعيد النوم المنتظمة والنشاط البدني الخفيف كالمشي يومياً.\n");

    let mut en = String::from("Lifestyle & Wellness Guidance:\n");
    en.push_str("- Maintain adequate hydration (1.5–2 L daily unless fluid restricted by your nephrologist).\n");
    en.push_str("- Prioritize regular restorative sleep and gentle daily physical activity (e.g. 20-30 min brisk walk).\n");

    for g in guidance {
        if !g.what_to_avoid_ar.is_empty() {
            ar.push_str(&format!("- تنبيه بخصوص {}: {}\n", g.drug_name, g.what_to_avoid_ar));
            en.push_str(&format!("- Caution for {}: {}\n", g.drug_name, g.what_to_avoid_en));
        }
    }

    (ar, en)
}
