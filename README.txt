
 ████████╗██████╗  █████╗ ██████╗ ███╗   ███╗ ██████╗ ██╗   ██╗███████╗███████╗
 ╚══██╔══╝██╔══██╗██╔══██╗██╔══██╗████╗ ████║██╔═══██╗██║   ██║██╔════╝██╔════╝
    ██║   ██████╔╝███████║██████╔╝██╔████╔██║██║   ██║██║   ██║███████╗█████╗  
    ██║   ██╔══██╗██╔══██║██╔═══╝ ██║╚██╔╝██║██║   ██║██║   ██║╚════██║██╔══╝  
    ██║   ██║  ██║██║  ██║██║     ██║ ╚═╝ ██║╚██████╔╝╚██████╔╝███████║███████╗
    ╚═╝   ╚═╝  ╚═╝╚═╝  ╚═╝╚═╝     ╚═╝     ╚═╝ ╚═════╝  ╚═════╝ ╚══════╝╚══════╝

====================================================

Versione: 1.0
Autore: Francesco Lui
Data: 2025-07-01
Info: Blocco del cursore per ambienti Windows
Licenza: MIT

DESCRIZIONE:
TrapMouse è un'applicazione per Windows che blocca il cursore del mouse all'interno
della finestra corrente. È pensata per scenari industriali, ambienti di kiosk o
sistemi embedded dove è necessario impedire al mouse di uscire da un'area specifica
dello schermo.

FUNZIONALITÀ PRINCIPALI:
- Blocco del mouse nella finestra con un solo clic (pulsante "Cattura!")
- Protezione opzionale con password cifrata
- Cifratura supportata: XOR, XTEA, SALSA20 (selezionabile)
- Interfaccia grafica semplice e minimale
- Sistema di configurazione tramite file `data.cfg`
- Avvio automatico con Windows (opzionale)
- Completamente portabile: non richiede installazione

REQUISITI:
- Sistema operativo: Windows XP, Vista, 7, 10, 11 (32 bit)

COME SI USA:
1. Avvia TrapMouse.
2. Premi il pulsante "Cattura!" per bloccare il cursore nella finestra.
3. Per sbloccare, inserisci la password (se attiva) o chiudi l'app dal task manager.
4. Puoi impostare opzioni aggiuntive dal pulsante "Opzioni".


SICUREZZA:
- La password è memorizzata solo in forma cifrata e codificata Base64
- Supporta password da 6 a 100 caratteri

LICENZA:
Questo software è distribuito con licenza MIT, è possibile usarlo, copiarlo, modificarlo 
e distribuirlo liberamente, anche in progetti commerciali.

Contatti: robadasmanettoni [at] github
