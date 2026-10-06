#include <ESP8266WiFi.h>
#include <Firebase_ESP_Client.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h>
#include "DHT.h"
#include <time.h> // Libreria di sistema per la gestione del tempo

// Inclusione helper per Firebase
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

// File locale ignorato da Git per proteggere le credenziali
#include "secrets.h"

// --- CREDENZIALI RETE E CLOUD ---
#define WIFI_SSID SECRET_WIFI_SSID
#define WIFI_PASSWORD SECRET_WIFI_PASSWORD
#define API_KEY SECRET_API_KEY
#define DATABASE_URL "https://stazione-meteo-esp8266-default-rtdb.europe-west1.firebasedatabase.app"

// NUOVE CREDENZIALI FIREBASE AUTH
#define USER_EMAIL SECRET_USER_EMAIL
#define USER_PASSWORD SECRET_USER_PASSWORD

// --- DEFINIZIONI HARDWARE SENSORI ---
#define DHTPIN 14     
#define DHTTYPE DHT11
#define MQ_PIN A0     

DHT dht(DHTPIN, DHTTYPE);
Adafruit_BMP280 bmp;

// Oggetti Firebase
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// --- PARAMETRI SCIENTIFICI DI ACQUISIZIONE ---
// Campionamento ogni 10 secondi, Invio ogni 5 minuti (300.000 ms)
const long INTERVALLO_CAMPIONAMENTO = 10000;  
const long INTERVALLO_INVIO = 300000;         
unsigned long timerCampionamento = 0;
unsigned long timerInvio = 0;

// Accumulatori RAM
float somma_T = 0, somma_H = 0, somma_P = 0;
long somma_Gas = 0;
int conteggio_letture = 0;

// Funzione per sincronizzare l'orologio interno con i server atomici mondiali
void sincroNTP() {
  Serial.print("Sincronizzazione orario NTP (UTC)... ");
  // Imposta server NTP (Zero offset, Nessuna ora legale)
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  
  time_t now = time(nullptr);
  // Attende finché il tempo non è un valore realistico (superiore al 1970)
  while (now < 8 * 3600 * 2) {
    delay(500);
    Serial.print(".");
    now = time(nullptr);
  }
  Serial.println("\n[OK] Orologio globale allineato.");
}

void setup() {
  Serial.begin(115200);
  
  dht.begin();
  if (!bmp.begin(0x76)) {
    Serial.println("[ERRORE] BMP280 non trovato.");
  }
  bmp.setSampling(Adafruit_BMP280::MODE_NORMAL, Adafruit_BMP280::SAMPLING_X2, Adafruit_BMP280::SAMPLING_X16, Adafruit_BMP280::FILTER_X16, Adafruit_BMP280::STANDBY_MS_500);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("\nConnessione Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("\n[OK] Wi-Fi Connesso.");

  // Sincronizzazione tempo prima di avviare Firebase
  sincroNTP();

  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;

  // Assegnazione delle credenziali esplicite
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;

  Firebase.begin(&config, &auth);
  Serial.println("[OK] Connessione a Firebase inizializzata.");
  
  Firebase.reconnectWiFi(true);
}

void loop() {
  unsigned long tempoAttuale = millis();

  // --- 1. FASE DI CAMPIONAMENTO STATISTICO ---
  if (tempoAttuale - timerCampionamento >= INTERVALLO_CAMPIONAMENTO) {
    timerCampionamento = tempoAttuale;

    float t_bmp = bmp.readTemperature();
    float t_dht = dht.readTemperature();
    float h_dht = dht.readHumidity();
    float p_bmp = bmp.readPressure() / 100.0F;
    int gas_raw = analogRead(MQ_PIN);

    float t_m = (t_bmp + t_dht)/2;

    Serial.print(t_bmp);
    Serial.print(" °C bmp; ");
    Serial.print(t_dht);
    Serial.println(" °C dht");

    Serial.print(t_m);
    Serial.print(" °C; ");
    Serial.print(h_dht);
    Serial.print(" %; ");
    Serial.print(p_bmp);
    Serial.print(" hPa; ");
    Serial.print(gas_raw);
    Serial.println(" / 1024");

    if (!isnan(t_m) && !isnan(h_dht)) {
      somma_T += t_m;
      somma_H += h_dht;
      somma_P += p_bmp;
      somma_Gas += gas_raw;
      conteggio_letture++;
    }
  }

  // --- 2. FASE DI AGGREGAZIONE E SALVATAGGIO CLOUD ---
  if (tempoAttuale - timerInvio >= INTERVALLO_INVIO) {
    timerInvio = tempoAttuale;

    if (conteggio_letture > 0 && Firebase.ready()) {
      float media_T = somma_T / conteggio_letture;
      float media_H = somma_H / conteggio_letture;
      float media_P = somma_P / conteggio_letture;
      int media_Gas = somma_Gas / conteggio_letture;
      
      // Otteniamo il tempo assoluto (Unix Epoch Timestamp)
      time_t timestamp = time(nullptr);

      // Costruzione dell'oggetto JSON
      FirebaseJson jsonPayload;
      jsonPayload.set("timestamp", (int)timestamp);
      jsonPayload.set("temperatura", media_T);
      jsonPayload.set("umidita", media_H);
      jsonPayload.set("pressione", media_P);
      jsonPayload.set("gas_raw", media_Gas);

      Serial.println("\n--- TRASMISSIONE DATI ---");

      // A) Aggiornamento Web App (Sovrascrittura)
      if (Firebase.RTDB.setJSON(&fbdo, "/MeteoAttuale", &jsonPayload)) {
         Serial.println("[OK] Dati in tempo reale aggiornati.");
      } else {
         Serial.println("[ERRORE] Fallito aggiornamento in tempo reale.");
      }

      // B) Creazione Storico per IA (Accodamento Push)
      if (Firebase.RTDB.pushJSON(&fbdo, "/MeteoStorico", &jsonPayload)) {
         Serial.println("[OK] Record storico aggiunto al dataset.");
      } else {
         Serial.println("[ERRORE] Fallita scrittura storico.");
      }

      // Reset
      somma_T = 0; somma_H = 0; somma_P = 0; somma_Gas = 0;
      conteggio_letture = 0;
    }
  }
}