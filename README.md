# 🌦️ Stazione Meteo Simo

Un progetto IoT avanzato composto da una stazione meteorologica basata su **ESP8266** e una **Web Dashboard intelligente**. Oltre al semplice monitoraggio ambientale, il sistema integra algoritmi di Machine Learning (K-NN) e proiezioni matematiche multidimensionali (PCA) calcolati interamente lato client nel browser.

## 🚀 Architettura del Progetto

Il progetto si divide in due componenti principali:
1. **Firmware (C++)**: Eseguito sul microcontrollore ESP8266 (NodeMCU/Wemos).
2. **Frontend (HTML/JS/CSS)**: Una Single Page Application (SPA) che funge da dashboard interattiva.

I due mondi comunicano tramite **Firebase Realtime Database**, che funge da bridge e archivio storico dei dati.

---

## 🛠️ Hardware e Sensori

La stazione è equipaggiata con:
* **Microcontrollore**: ESP8266 (es. Wemos D1 Mini / NodeMCU).
* **Sensore DHT11**: Misurazione Temperatura e Umidità.
* **Sensore BMP280 (I2C)**: Misurazione Temperatura ad alta precisione e Pressione Atmosferica.
* **Sensore Gas MQ (Analogo)**: Monitoraggio della qualità dell'aria (valore grezzo 0-1024).

*(Nota: Il firmware unisce e fa la media tra le letture di temperatura del DHT11 e del BMP280 per ottenere un dato più stabile).*

---

## 💻 Firmware ESP8266 (Edge Computing)

Il firmware (`stazioneMeteoCollegata1.ino`) non si limita a inviare dati "grezzi", ma esegue un campionamento intelligente per ottimizzare le risorse e inviare dati puliti:
* **Sincronizzazione NTP**: All'avvio, l'ESP si connette a server atomici mondiali per ottenere l'esatto Unix Timestamp UTC.
* **Fase di Campionamento**: Ogni 10 secondi, l'ESP legge i sensori e salva i dati in RAM.
* **Fase di Invio**: Ogni 5 minuti, calcola la media matematica delle letture, crea un payload JSON e lo invia a Firebase (`MeteoStorico` e `MeteoAttuale`).

---

## 🌐 Web Dashboard (Frontend)

L'interfaccia (`index.html`) è il cuore analitico del sistema. Oltre a mostrare i dati attuali e i grafici storici delle ultime 24 ore tramite **Chart.js**, offre funzioni analitiche avanzate:

### ⚡ Sistema di Allerta Dinamico
La dashboard esamina i delta a 3 ore per avvisare di:
* **Ghiaccio/Brina**: Temperatura in forte discesa incrociata con il Punto di Rugiada (*Dew Point*).
* **Nebbia**: Umidità altissima (>90%) con forbice minima tra Temp e Dew Point.
* **Tempeste**: Crolli barometrici violenti (> 3 hPa di diminuzione in 3 ore).

### 🧠 Intelligenza Artificiale (Machine Learning K-NN)
L'app utilizza un algoritmo K-Nearest Neighbors scritto in JS per stimare il tempo reale (Sole, Nuvole, Pioggia, Neve).
Non si limita alle 4 variabili base, ma costruisce uno spazio a **16 Dimensioni** analizzando la cinematica dei dati (valore istantaneo, delta a 3 ore, scostamento dalla media e pendenza della curva).

### 🔬 Analisi PCA (Principal Component Analysis)
Premendo sull'apposito tasto, il sistema esegue complesse riduzioni di dimensionalità tramite il **metodo degli Autovalori di Jacobi**, proiettando in grafici 2D la separazione del dataset meteorologico.
Vengono testati 4 modelli, tra cui un modello assoluto a **20 Dimensioni** che incrocia le dinamiche fisiche con le **coordinate temporali cicliche** (seno e coseno dei minuti del giorno e del giorno dell'anno).

### 🤝 Crowdsourcing
L'app integra un popup ("Oracolo") che richiede saltuariamente all'utente locale di indicare quale sia l'effettivo meteo visibile fuori dalla finestra per continuare ad arricchire e addestrare il dataset.

---

## ⚙️ Installazione e Setup

1. **Configurazione Firebase**:
   * Crea un progetto Firebase e un Realtime Database.
   * Abilita l'autenticazione tramite Email/Password.
   * Aggiorna le chiavi `apiKey`, `authDomain`, `databaseURL` e le credenziali sia nel file `.ino` che in `index.html`.
2. **Caricamento Firmware**:
   * Modifica `WIFI_SSID` e `WIFI_PASSWORD` in `stazioneMeteoCollegata1.ino`.
   * Assicurati di aver scaricato le librerie necessarie (ESP8266, Firebase ESP Client, Adafruit DHT/BMP280).
   * Carica il firmware sulla scheda ESP8266.
3. **Avvio Dashboard**:
   * Apri `index.html` in un qualsiasi browser moderno, non richiede Node.js o server locali in quanto dialoga direttamente con Firebase.

---
*Progetto sviluppato da Simone Magistrali.*
