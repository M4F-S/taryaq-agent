//! Taryaq Sovereign Clinical AI Agent Daemon Entrypoint
use taryaq::safety::{clinical_db_count_drugs, clinical_db_init, triage_patient_symptoms};

#[tokio::main]
async fn main() {
    tracing_subscriber::fmt::init();
    tracing::info!("Initializing Taryaq Sovereign Clinical Agent Runtime v0.1.0...");

    let db_ok = clinical_db_init(None);
    let drug_count = clinical_db_count_drugs();
    tracing::info!(
        db_initialized = db_ok,
        drug_count = drug_count,
        "Sovereign database linked"
    );

    let triage = triage_patient_symptoms("أشعر بألم في الصدر وضيق تنفس مستمر");
    tracing::info!(
        category = %triage.triage_category,
        is_red_flag = triage.is_red_flag,
        "Sample red-flag triage check verified"
    );

    println!("Taryaq Sovereign Clinical Engine online and operational.");
}
