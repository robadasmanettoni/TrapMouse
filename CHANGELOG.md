# TrapMouse - CHANGELOG

**Versione:** 1.0  
**Data rilascio:** 2025-07-01  
**Autore:** Francesco Lui  
**Descrizione:** Utility per il blocco del mouse e la gestione sicura della sessione utente  

---

## 🆕 FUNZIONALITÀ INTRODOTTE
- Blocco del cursore del mouse all'interno della finestra (modalità "Cattura!")
- Protezione tramite password cifrata con algoritmi selezionabili tra:
  - XOR
  - XTEA
  - SALSA20
- Impostazioni di cifratura modificabili anche post-configurazione
- Sistema di configurazione salvato localmente nel file `data.cfg`
- Interfaccia minimale con 3 pulsanti principali: "Cattura!", "Password", "Opzioni"
- Tips informativi all’interno del riquadro della finestra

---

## 🔐 SICUREZZA
- Password sempre cifrata e codificata in Base64; mai salvata in chiaro
- Controllo automatico sulla lunghezza della password:
  - Minimo: 6 caratteri
  - Massimo: 100 caratteri
- Uso sistematico di `SecureZeroMemory` per cancellare dal buffer dati sensibili
- Validazioni aggiuntive nella finestra "Password" per evitare modifiche incoerenti
- Prevenzione overflow su input utente tramite `EM_LIMITTEXT`
- Codice difensivo: tutte le operazioni critiche hanno fallback o messaggio di errore

---

## ⚙️ GESTIONE IMPOSTAZIONI
- File `data.cfg` gestito in modalità fail-safe: se corrotto o mancante, viene rigenerato
- Cifrario selezionabile con riassegnazione automatica dei dati criptati
- Feedback utente migliorato in caso di errore durante il salvataggio
- Fallimenti nel salvataggio gestiti tramite messaggi di errore (MessageBox)

---

## 🧩 COMPATIBILITÀ E PORTABILITÀ
- Codice scritto in C++98, compatibile con:
  - Visual C++ 2010 Express (MSVC 16)
  - Dev-C++ con TDM-GCC 4.9.2
- Funziona su tutte le versioni a 32 bit di Windows da XP in poi
- Nessuna dipendenza da librerie esterne (es. `msvcr100.dll`)
- Compilabile con opzione `/MT` per eseguibili completamente stand-alone

---

## 🧪 TEST E VERIFICHE
- Testato su Windows XP, 7, 10 e 11
- Testato con buffer da 6 a 100 caratteri (anche UTF-8 base)
- Debug eseguito su più compilatori e ambienti reali
- Nessun crash o leak riscontrato
- Tutte le combinazioni di cifratura e cambio password testate con successo

---

## 📌 NOTE FINALI
- Versione 1.0 incentrata su funzionalità complete, performance elevate e sicurezza locale robusta
- Non è necessaria installazione: programma 100% portabile (può girare da USB)
- UI semplice ma efficace, adatta anche ad ambienti legacy o industriali

---

## 🚧 ROADMAP FUTURA
- Logging eventi (opzionale)
- Timeout o blocco dopo tentativi errati
- Forzatura password forte con livello (debole/forte)
- Supporto a cifratura AES-128/256
- Timeout automatico con blocco dopo X minuti di inattività
- Separazione tra **configurazioni locali** e **credenziali protette**
- Supporto a più schermi: blocco su monitor primario o secondario
- Modalità “dark mode” per sistemi moderni (se disponibile)
- Aggiunta di icone ai pulsanti e finestre di dialogo
- Versione compilata anche per architettura x64
- Traduzione multi-lingua (es. ITA/ENG)
- Setup automatico con NSIS e pacchetto ZIP portabile

---

## 📂 FILE INCLUSI
- `TrapMouse.exe` – eseguibile principale
- `data.cfg` – file di configurazione generato runtime
- `readme.txt` – guida rapida all'uso
