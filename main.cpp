// ====================================================================
// Indice delle funzioni:
// ====================================================================
// CaptureMouse   : Blocca o sblocca il cursore del mouse nell’area di cattura.
// WndProc        : Gestisce i messaggi della finestra principale (creazione UI, comandi, chiusura condizionata, ecc.).
// WinMain        : Punto di ingresso, registra la classe finestra e avvia il ciclo dei messaggi.
// ====================================================================

#include <windows.h>
#include "resource.h"
#include "settings.h"
#include "messages.h"
#include "base64.h"
#include "cipher_xor.h"
#include "cipher_xtea.h"
#include "cipher_salsa20.h"
#include "keyboard_hook.h"
#include <cstdio>
#include <ctime>
#include <cstdlib>

#define MAX_PASSWORD_LEN     128  // buffer interno per la password + '\0'
#define MAX_PASSWORD_INPUT   100  // limite massimo consentito nella GUI
#define MIN_PASSWORD_LENGTH  6    // soglia minima consigliata

// Dichiarazioni delle variabili globali
// Handle per i controlli della finestra
HWND hwndToggleCaptureButton;
HWND hwndPasswordButton;
HWND hwndSettingsButton;
HWND hwndPasswordEdit;
HWND hwndCaptureAreaLabel;
HWND hwndPasswordLabel;
HWND hwndShowPassCheckbox;
HWND hwndTipsIcon;
HWND hwndTipsEdit;
HWND hwndTipsLabel;

HFONT hFont;                     // Font personalizzato per i controlli
RECT labelRect;                  // Area di cattura per il mouse (definita dal rettangolo di hwndCaptureAreaLabel)

static Settings settings;
bool isMouseCaptured = false;    // Flag utilizzato per indicare se il mouse è attualmente bloccato
bool isShowPassChecked = false;  // Flag relativo allo stato del checkbox "Mostra password"

bool isPasswordSet() {
    Settings s;
    if (!LoadSettings(s)) return false;

    // Se la password è vuota, non è stata impostata
    return !s.passwordEncrypted.empty();
}

bool RegisterAutoStart() {
    const char* appName = "TrapMouse";
    char path[MAX_PATH];

    if (GetModuleFileNameA(NULL, path, MAX_PATH) == 0)
        return false;

    std::string quotedPath = std::string("\"") + path + "\"";

    HKEY hKey;
    if (RegOpenKeyExA(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_SET_VALUE, &hKey) != ERROR_SUCCESS)
        return false;

    LONG result;
    if (settings.autolock) {
        result = RegSetValueExA(hKey, appName, 0, REG_SZ, (const BYTE*)quotedPath.c_str(), quotedPath.length() + 1);
    } else {
        result = RegDeleteValueA(hKey, appName);
    }

    RegCloseKey(hKey);
    return (result == ERROR_SUCCESS);
}


// ---------------------------------------------------------------------------------------
// SEZIONE: Funzioni UpdateStatus + ShowStartupTip
// DESCRIZIONE: Gestiscono la barra di stato dell'applicazione. UpdateStatus aggiorna il
//              messaggio e l'icona visibile, mentre ShowStartupTip visualizza un consiglio
//              casuale all'avvio per informare o guidare l'utente.
// ---------------------------------------------------------------------------------------
void UpdateStatus(const char* message, HICON hIcon = NULL) {
    if (hwndTipsEdit) SetWindowText(hwndTipsEdit, message);
    if (hwndTipsIcon && hIcon)
        SendMessage(hwndTipsIcon, STM_SETICON, (WPARAM)hIcon, 0);
}

void ShowStartupTip() {
    if (!isPasswordSet()) {
        UpdateStatus(MSG_NO_PASSWORD, LoadIcon(NULL, IDI_WARNING));
        return;
    }

    static const char* tips[] = {
        MSG_CAN_LOCK,
        MSG_TIPS_OPTIONS,
        MSG_USE_SHOWPASS,
        MSG_START_MINIMIZED,
        MSG_KEYBLOCK_NOTE,
        MSG_REMINDER_SAVE,
        MSG_SECURE_PASS,
        MSG_NO_SHARE_PASS,
        MSG_CHANGE_PASS,
        MSG_XOR_SIMPLE,
        MSG_XTEA_BALANCED,
        MSG_SALSA20_SECURE
    };
    static const int tipCount = sizeof(tips) / sizeof(tips[0]);

    std::srand((unsigned int)std::time(NULL));
    int index = std::rand() % tipCount;

    UpdateStatus(tips[index], LoadIcon(NULL, IDI_INFORMATION));
}

void MsgTipsBox(const char* message, HICON hIcon = NULL) {
    if (hwndTipsEdit)
        SetWindowText(hwndTipsEdit, message);
    if (hwndTipsIcon)
        SendMessage(hwndTipsIcon, STM_SETICON, (WPARAM)(hIcon ? hIcon : LoadIcon(NULL, IDI_INFORMATION)), 0);
}



// ---------------------------------------------------------------------------------------
// SEZIONE: Callback SettingsDlgProc
// DESCRIZIONE: Gestisce la finestra di dialogo delle impostazioni dell'applicazione.
//              Consente di configurare opzioni come avvio automatico bloccato, avvio
//              minimizzato, blocco tastiera, posizione della finestra principale, e il
//              tipo di cifratura da utilizzare (XOR, XTEA, SALSA20).
//              Le impostazioni vengono caricate all'avvio e salvate alla conferma.
// ---------------------------------------------------------------------------------------
INT_PTR CALLBACK SettingsDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_INITDIALOG: {
            // 1) Carica le impostazioni; se non esiste il file, usiamo valori di default
            if (!LoadSettings(settings)) {
                settings.autolock      = 0;
                settings.block_keys    = false;
                settings.no_pass_rules = false;
                settings.cipher        = 0;
                settings.win_pos_x     = 10;
                settings.win_pos_y     = 10;
                settings.win_center    = true;
            }

            // 2) Imposta i checkbox nello stato di default con le impostazioni
            CheckDlgButton(hDlg, IDC_CHK_AUTOLOCK,  settings.autolock      ? BST_CHECKED : BST_UNCHECKED);
            CheckDlgButton(hDlg, IDC_CHK_PASS_EASY, settings.no_pass_rules ? BST_CHECKED : BST_UNCHECKED);
            CheckDlgButton(hDlg, IDC_CHK_BLOCKKEYS, settings.block_keys    ? BST_CHECKED : BST_UNCHECKED);

            // 3) Seleziona il radio button per la posizione centrale della finestra rispetto allo schermo
            CheckDlgButton(hDlg, settings.win_center ? IDC_RADIO_CENTER : IDC_RADIO_CUSTOM, BST_CHECKED);

            // 4) Popola la combo box per i cifrari: XOR, XTEA e SALSA20
            HWND hCombo = GetDlgItem(hDlg, IDC_COMBO_CIPHER);
            if (hCombo) {
                SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)"XOR");
                SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)"XTEA");
                SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)"SALSA20");

                // Seleziona la voce corrente in base alle impostazioni
				if (settings.cipher >= 0 && settings.cipher <= 2)
                    SendMessage(hCombo, CB_SETCURSEL, settings.cipher, 0);
                else
                    SendMessage(hCombo, CB_SETCURSEL, 0, 0);
            }

            return TRUE;
        }

        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDC_BTN_OK: {
                    // 1) Legge lo stato dei checkbox
                    settings.autolock      = (IsDlgButtonChecked(hDlg, IDC_CHK_AUTOLOCK)   == BST_CHECKED);
                    settings.no_pass_rules = (IsDlgButtonChecked(hDlg, IDC_CHK_PASS_EASY)  == BST_CHECKED);
                    settings.block_keys    = (IsDlgButtonChecked(hDlg, IDC_CHK_BLOCKKEYS)  == BST_CHECKED);

                    // 2) Legge lo stato dei radio button
                    settings.win_center = (IsDlgButtonChecked(hDlg, IDC_RADIO_CENTER) == BST_CHECKED);

                    // 3) Legge il cifrario selezionato nella combo
                    int oldCipher = settings.cipher;
                    
                    HWND hCombo = GetDlgItem(hDlg, IDC_COMBO_CIPHER);
                    if (hCombo != NULL) {
                        // Ottiene l'indice della voce selezionata
                        int sel = (int)SendMessage(hCombo, CB_GETCURSEL, 0, 0);
                    
                        // Se la selezione è valida (tra 0 e 2) aggiorno il cifrario
                        if (sel >= 0 && sel <= 2) {
                            settings.cipher = sel;
                        } else {
                            // Se fuori range mantango il valore precedente
                            settings.cipher = oldCipher;
                        }
                    }
	                
                    // 4) Se c'è una password salvata e il cifrario è stato cambiato, ricodifichiamo!
                    if (isPasswordSet() && settings.cipher != oldCipher) {
                        char decrypted[MAX_PASSWORD_LEN] = {};
                        bool ok = false;
	                
                        // Decodifica col vecchio cifrario
                        switch (oldCipher) {
                            case 0: ok = LoadPassword_XOR(settings.passwordEncrypted, decrypted, sizeof(decrypted)); break;
                            case 1: ok = LoadPassword_XTEA(settings.passwordEncrypted, decrypted, sizeof(decrypted)); break;
                            case 2: ok = LoadPassword_SALSA20(settings.passwordEncrypted, decrypted, sizeof(decrypted)); break;
                        }
	                
                        if (!ok) {
                            MsgTipsBox("Errore durante la decodifica della password con il vecchio cifrario!", LoadIcon(NULL, IDI_ERROR));
                            settings.cipher = oldCipher; // Ripristina il vecchio valore
                            return TRUE;
                        }
	                
                        // Ricodifica col nuovo cifrario
                        std::vector<unsigned char> encrypted;
                        bool saved = false;
                        switch (settings.cipher) {
                            case 0: saved = SavePassword_XOR(decrypted, encrypted); break;
                            case 1: saved = SavePassword_XTEA(decrypted, encrypted); break;
                            case 2: saved = SavePassword_SALSA20(decrypted, encrypted); break;
                        }
	                
                        if (!saved) {
                            MsgTipsBox("Errore durante la ricodifica della password!", LoadIcon(NULL, IDI_ERROR));
                            settings.cipher = oldCipher;
                            return TRUE;
                        }
	                
                        // Aggiorna il valore cifrato
                        settings.passwordEncrypted = EncodeBase64(encrypted.data(), encrypted.size());
                        SecureZeroMemory(decrypted, sizeof(decrypted));
                    }

					RegisterAutoStart();

                    // 6) Salvataggio su file (es. data.cfg)
					if (!SaveSettings(settings)) {
                        MsgTipsBox("Errore durante il salvataggio delle impostazioni!", LoadIcon(NULL, IDI_ERROR));
                    } else {
                        MsgTipsBox("Impostazioni salvate correttamente!", LoadIcon(NULL, IDI_INFORMATION));
                        EndDialog(hDlg, IDOK);
                    }

                    // 7) Chiude la finestra restituendo IDOK
                    return TRUE;
                }

                // Chiude la finestra senza salvare
				case IDC_BTN_CANCEL:
                    EndDialog(hDlg, IDCANCEL);
                    return TRUE;
            }
            break;

        case WM_SYSCOMMAND:
            if ((wParam & 0xFFF0) == SC_CLOSE) {
                EndDialog(hDlg, IDCANCEL); // Come se l’utente avesse premuto Annulla
                return TRUE;
            }
            break;

        case WM_CLOSE:
            EndDialog(hDlg, IDCANCEL); // Chiusura via [X] in alto
            return TRUE;
    }

    return FALSE;
}

// ---------------------------------------------------------------------------------------
// SEZIONE: Callback PasswordDlgProc
// DESCRIZIONE: Gestisce la finestra di dialogo per modificare o creare la password
// ---------------------------------------------------------------------------------------
INT_PTR CALLBACK PasswordDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_INITDIALOG: {
            if (!isPasswordSet()) {
                EnableWindow(GetDlgItem(hDlg, IDC_EDIT_OLD_PASSWORD), FALSE);
            }
        
            // Imposta comunque il limite massimo per tutti i campi
            SendMessage(GetDlgItem(hDlg, IDC_EDIT_OLD_PASSWORD), EM_LIMITTEXT, MAX_PASSWORD_INPUT, 0);
            SendMessage(GetDlgItem(hDlg, IDC_EDIT_NEW_PASSWORD), EM_LIMITTEXT, MAX_PASSWORD_INPUT, 0);
            SendMessage(GetDlgItem(hDlg, IDC_EDIT_CONFIRM),      EM_LIMITTEXT, MAX_PASSWORD_INPUT, 0);
        
            return TRUE;
        }

        case WM_COMMAND:
            if (LOWORD(wParam) == IDC_BUTTON_SAVE) {
                char oldPass[MAX_PASSWORD_LEN] = {0};
                char newPass[MAX_PASSWORD_LEN] = {0};
                char confirm[MAX_PASSWORD_LEN] = {0};
            
                GetDlgItemText(hDlg, IDC_EDIT_OLD_PASSWORD, oldPass, sizeof(oldPass));
                GetDlgItemText(hDlg, IDC_EDIT_NEW_PASSWORD, newPass, sizeof(newPass));
                GetDlgItemText(hDlg, IDC_EDIT_CONFIRM, confirm, sizeof(confirm));
            
                bool requireOld = isPasswordSet();
            
                // === Controllo sicurezza password ===
				if (!settings.no_pass_rules) {
                    // Controllo lunghezza adeguata della password
					size_t len = strlen(newPass);
                    if (len < MIN_PASSWORD_LENGTH) {
				    	SecureZeroMemory(newPass, sizeof(newPass));
                        SecureZeroMemory(confirm, sizeof(confirm));
                        SecureZeroMemory(oldPass, sizeof(oldPass));
                        MsgTipsBox("La password è troppo corta, inserisci almeno 6 caratteri.", LoadIcon(NULL, IDI_ERROR));
                        return TRUE;
                    } else if (len > MAX_PASSWORD_INPUT) {
				    	SecureZeroMemory(newPass, sizeof(newPass));
                        SecureZeroMemory(confirm, sizeof(confirm));
                        SecureZeroMemory(oldPass, sizeof(oldPass));
                        MsgTipsBox("La password è troppo lunga, il limite massimo è 100 caratteri.", LoadIcon(NULL, IDI_ERROR));
                        return TRUE;
                    }

					// Confronto fra la password vecchia e quella nuova
                    if (strcmp(newPass, oldPass) == 0) {
                        SecureZeroMemory(newPass, sizeof(newPass));
                        SecureZeroMemory(confirm, sizeof(confirm));
                        SecureZeroMemory(oldPass, sizeof(oldPass));
                        MsgTipsBox("La nuova password non può essere uguale a quella attuale.", LoadIcon(NULL, IDI_WARNING));
                        return TRUE;
                    }
				}
                
				// Controllo password vuota
                if (strlen(newPass) == 0) {
                    SecureZeroMemory(newPass, sizeof(newPass));
                    SecureZeroMemory(confirm, sizeof(confirm));
                    SecureZeroMemory(oldPass, sizeof(oldPass));
                    MsgTipsBox("La nuova password non può essere vuota.", LoadIcon(NULL, IDI_ERROR));
                    return TRUE;
                }

				// Controllo password di conferma
                if (strcmp(newPass, confirm) != 0) {
                    SecureZeroMemory(newPass, sizeof(newPass));
                    SecureZeroMemory(confirm, sizeof(confirm));
                    SecureZeroMemory(oldPass, sizeof(oldPass));
                    MsgTipsBox("La nuova password non corrisponde a quella di conferma.", LoadIcon(NULL, IDI_ERROR));
                    return TRUE;
                }
            
                Settings s;
                if (!LoadSettings(s)) {
                    MsgTipsBox("Impossibile caricare le impostazioni!", LoadIcon(NULL, IDI_ERROR));
                    return TRUE;
                }
            

                char decrypted[MAX_PASSWORD_LEN] = {};
                bool ok = true;
                
                // Se esiste già una password la decifriamo e verifichiamo
                if (requireOld) {
                    switch (s.cipher) {
                        case 0: ok = LoadPassword_XOR(s.passwordEncrypted, decrypted, sizeof(decrypted)); break;
                        case 1: ok = LoadPassword_XTEA(s.passwordEncrypted, decrypted, sizeof(decrypted)); break;
                        case 2: ok = LoadPassword_SALSA20(s.passwordEncrypted, decrypted, sizeof(decrypted)); break;
                        default:
                            MsgTipsBox("Tipo di cifratura non valido nelle impostazioni!", LoadIcon(NULL, IDI_ERROR));
                            return TRUE;
                    }
                
                    if (!ok) {
                        MsgTipsBox("Errore durante la lettura della password attuale!", LoadIcon(NULL, IDI_ERROR));
                        return TRUE;
                    }
                
                    if (strcmp(oldPass, decrypted) != 0) {
                        MsgTipsBox("La password attuale non è corretta!", LoadIcon(NULL, IDI_ERROR));
                        return TRUE;
                    }
                }
                
                // Decifra la nuova password
                std::vector<unsigned char> encrypted;
                bool saved = false;
                
                switch (s.cipher) {
                    case 0: saved = SavePassword_XOR(newPass, encrypted); break;
                    case 1: saved = SavePassword_XTEA(newPass, encrypted); break;
                    case 2: saved = SavePassword_SALSA20(newPass, encrypted); break;
                }
                
                if (!saved) {
                    MsgTipsBox("Errore durante la crittografia della password!", LoadIcon(NULL, IDI_ERROR));
                } else {
                    s.passwordEncrypted = EncodeBase64(encrypted.data(), encrypted.size());
                
                    if (!SaveSettings(s)) {
                        MsgTipsBox("Errore durante il salvataggio delle impostazioni!", LoadIcon(NULL, IDI_ERROR));
                    } else {
                        MsgTipsBox(requireOld ? "Password aggiornata con successo!" : "Password creata con successo!", LoadIcon(NULL, IDI_INFORMATION));
                        EndDialog(hDlg, IDOK);
                    }
                }
                
                SecureZeroMemory(decrypted, sizeof(decrypted)); // Cancella la vecchia password decifrata
                SecureZeroMemory(oldPass, sizeof(oldPass));     // Cancella la password vecchia
                SecureZeroMemory(newPass, sizeof(newPass));     // Cancella la password nuova
                SecureZeroMemory(confirm, sizeof(confirm));     // Cancella la password di conferma

                return TRUE;
            }

            break;

        case WM_CLOSE:
            EndDialog(hDlg, IDCANCEL);
            return TRUE;
    }

    return FALSE;
}

// ---------------------------------------------------------------------------------------
// SEZIONE: Funzione CaptureMouse
// DESCRIZIONE: Blocca o sblocca il cursore del mouse nell’area di cattura
// ---------------------------------------------------------------------------------------
void CaptureMouse(HWND hwnd, bool& isMouseCaptured, RECT& labelRect) { 
	static int originalAutolock = 0;

    if (!isMouseCaptured) {
        // Verifica esistenza file password
        if (!isPasswordSet()) {
            MsgTipsBox("La password non è stata ancora impostata. Impostala per usare il blocco.", LoadIcon(NULL, IDI_INFORMATION));
            DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_PASSWORD_DIALOG), hwnd, PasswordDlgProc);
            return; // esci, l'utente deve impostarla prima
        }

		// Blocco il valore di autostart prima di tutto
		originalAutolock = settings.autolock;
        settings.autolock = 2;
        SaveSettings(settings);

        // Mostra campi per inserire la password
        ShowWindow(hwndPasswordLabel, SW_SHOW);
        ShowWindow(hwndPasswordEdit, SW_SHOW);
        ShowWindow(hwndShowPassCheckbox, SW_SHOW);

        // Calcola dinamicamente l'area rettangolare per bloccare il mouse
        GetWindowRect(hwndCaptureAreaLabel, &labelRect);

        // Blocca il cursore nell'area
        ClipCursor(&labelRect);

        // Imposta la finestra sempre in primo piano
        SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
        SetWindowText(hwndToggleCaptureButton, "Sblocca!");
        isMouseCaptured = true;

		EnableWindow(hwndPasswordButton, FALSE); // Disabilita il pulsante Password
		EnableWindow(hwndSettingsButton, FALSE); // Disabilita il pulsante Opzioni
		InstallKeyBlocker();                     // Blocca i tasti di sistema

    } else {
        // Legge la password inserita
        char buffer[MAX_PASSWORD_LEN] = {0};
        GetWindowText(hwndPasswordEdit, buffer, sizeof(buffer));

        // Carica impostazioni (per accedere a cipher e passwordEncrypted)
        Settings s;
        if (!LoadSettings(s)) {
            MsgTipsBox("Impossibile caricare le impostazioni!", LoadIcon(NULL, IDI_INFORMATION));
            return;
        }

        // Decritta la password salvata
        char decodedPassword[MAX_PASSWORD_LEN] = {0};
        bool ok = false;

        switch (s.cipher) {
            case 0: ok = LoadPassword_XOR(s.passwordEncrypted, decodedPassword, sizeof(decodedPassword)); break;
            case 1: ok = LoadPassword_XTEA(s.passwordEncrypted, decodedPassword, sizeof(decodedPassword)); break;
            case 2: ok = LoadPassword_SALSA20(s.passwordEncrypted, decodedPassword, sizeof(decodedPassword)); break;
        }

        if (ok) {
            if (strcmp(buffer, decodedPassword) == 0) {
                // Password corretta
                ShowWindow(hwndPasswordLabel, SW_HIDE);
                ShowWindow(hwndPasswordEdit, SW_HIDE);
                ShowWindow(hwndShowPassCheckbox, SW_HIDE);
                ClipCursor(NULL);
                SetWindowPos(hwnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
                SetWindowText(hwndToggleCaptureButton, "Cattura!");
                isMouseCaptured = false;

				// Svuota il campo della password e ne azzera il buffer
				char wipeBox[128] = {};
				GetWindowText(hwndPasswordEdit, wipeBox, sizeof(wipeBox));
				SecureZeroMemory(wipeBox, sizeof(wipeBox));
				SetWindowText(hwndPasswordEdit, "");
	
				// Toglie la spunta dal checkbox e resetta lo stato del campo della password
				SendMessage(hwndShowPassCheckbox, BM_SETCHECK, BST_UNCHECKED, 0);
				SendMessage(hwndPasswordEdit, EM_SETPASSWORDCHAR, '*', 0);
				InvalidateRect(hwndPasswordEdit, NULL, TRUE);

				EnableWindow(hwndPasswordButton, TRUE); // Abilita il pulsante Password
				EnableWindow(hwndSettingsButton, TRUE); // Abilita il pulsante Opzioni
				UninstallKeyBlocker();                  // Sblocca i tasti di sistema

				settings.autolock = originalAutolock;
				SaveSettings(settings);
                MsgTipsBox("La password è corretta!", LoadIcon(NULL, IDI_INFORMATION));
            } else {
                MsgTipsBox("La password è sbagliata!", LoadIcon(NULL, IDI_ERROR));
            }

            SecureZeroMemory(decodedPassword, sizeof(decodedPassword));
        } else {
            MsgTipsBox("Impossibile leggere o decifrare la password!", LoadIcon(NULL, IDI_ERROR));
        }

        SecureZeroMemory(buffer, sizeof(buffer));
    }
}

// ---------------------------------------------------------------------------------------
// SEZIONE: Callback WndProc
// DESCRIZIONE: Gestisce i messaggi della finestra principale
// ---------------------------------------------------------------------------------------
LRESULT CALLBACK WndProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam) {
    switch (Message) {
        case WM_CREATE: {
            // Crea il font da usare nei controlli
            hFont = CreateFontW(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");

            // Etichetta principale (area di cattura)
            hwndCaptureAreaLabel = CreateWindow("STATIC", NULL, WS_VISIBLE | WS_CHILD | WS_BORDER,
                                                10, 10, 470, 250, hwnd, (HMENU)1, NULL, NULL);

            // Etichetta "Password"
            hwndPasswordLabel = CreateWindow("STATIC", "Password:", WS_VISIBLE | WS_CHILD,
                                             20, 20, 72, 24, hwnd, (HMENU)2, NULL, NULL);
            SendMessage(hwndPasswordLabel, WM_SETFONT, (WPARAM)hFont, TRUE);
            ShowWindow(hwndPasswordLabel, SW_HIDE);

            // Casella di testo per la password
            hwndPasswordEdit = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", NULL, WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL | WS_TABSTOP,
                                              100, 20, 260, 24, hwnd, (HMENU)3, NULL, NULL);
            SendMessage(hwndPasswordEdit, EM_LIMITTEXT, MAX_PASSWORD_INPUT, 0);
			SendMessage(hwndPasswordEdit, WM_SETFONT, (WPARAM)hFont, TRUE);
            SendMessage(hwndPasswordEdit, EM_SETPASSWORDCHAR, '*', 0);
            ShowWindow(hwndPasswordEdit, SW_HIDE);

            // Pulsante "Cattura!"
            hwndToggleCaptureButton = CreateWindow("BUTTON", "Cattura!", WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON | WS_TABSTOP,
                                                   380, 20, 90, 24, hwnd, (HMENU)4, NULL, NULL);
            SendMessage(hwndToggleCaptureButton, WM_SETFONT, (WPARAM)hFont, TRUE);

            // Pulsante "Password"
            hwndPasswordButton = CreateWindow("BUTTON", "Password", WS_VISIBLE | WS_CHILD | WS_TABSTOP,
                                              380, 50, 90, 24, hwnd, (HMENU)5, NULL, NULL);
            SendMessage(hwndPasswordButton, WM_SETFONT, (WPARAM)hFont, TRUE);

			// Pulsante "Opzioni"
			hwndSettingsButton = CreateWindow("BUTTON", "Opzioni", WS_VISIBLE | WS_CHILD | WS_TABSTOP, 
				                              380, 80, 90, 24, hwnd, (HMENU)6, NULL, NULL);
            SendMessage(hwndSettingsButton, WM_SETFONT, (WPARAM)hFont, TRUE);

            // Checkbox per mostrare/nascondere la password
            hwndShowPassCheckbox = CreateWindow("BUTTON", "Mostra password", WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX,
                                                20, 50, 150, 24, hwnd, (HMENU)7, NULL, NULL);
            SendMessage(hwndShowPassCheckbox, WM_SETFONT, (WPARAM)hFont, TRUE);
			ShowWindow(hwndShowPassCheckbox, SW_HIDE);
            
			//// Icona di stato (32x32), visibile in fase di sviluppo
            hwndTipsIcon = CreateWindow("STATIC", NULL, WS_VISIBLE | WS_CHILD | SS_ICON,
                                          14, 220, 32, 32, hwnd, (HMENU)8, NULL, NULL);
            SendMessage(hwndTipsIcon, STM_SETICON, (WPARAM)LoadIcon(NULL, IDI_INFORMATION), 0);

			hwndTipsEdit = CreateWindow("EDIT", NULL, WS_VISIBLE | WS_CHILD | ES_MULTILINE | ES_READONLY | ES_NOHIDESEL,
                                             54, 220, 420, 36, hwnd, (HMENU)9, NULL, NULL);
            SendMessage(hwndTipsEdit, WM_SETFONT, (WPARAM)hFont, TRUE);

            hwndTipsLabel = CreateWindow("STATIC", NULL, WS_CHILD | WS_VISIBLE | SS_NOTIFY,
                                           10, 210, 470, 50, hwnd, (HMENU)10, NULL, NULL);
			SetWindowPos(hwndTipsLabel, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
			SendMessage(hwndTipsLabel, WM_SETFONT, (WPARAM)hFont, TRUE);

            // Mostra un tip iniziale all’avvio
            ShowStartupTip();

            break;
        }

        case WM_MOUSEACTIVATE:
            if ((HWND)lParam == hwndTipsEdit) {
                return MA_NOACTIVATEANDEAT;
            }
            break;

        case WM_CTLCOLORSTATIC: {
        	HDC hdcStatic = (HDC)wParam;
        	HWND hwndStatic = (HWND)lParam;
        	
        	if (hwndStatic == hwndCaptureAreaLabel || hwndStatic == hwndTipsIcon || hwndStatic == hwndTipsEdit || hwndStatic == hwndPasswordLabel || hwndStatic == hwndShowPassCheckbox) {
                SetBkColor(hdcStatic, GetSysColor(COLOR_3DSHADOW));
                return (INT_PTR)GetSysColorBrush(COLOR_3DSHADOW);
            }
			
			if (hwndStatic == hwndTipsLabel) {
                SetBkMode(hdcStatic, TRANSPARENT);
                return (INT_PTR)GetSysColorBrush(COLOR_BTNFACE);
            }

        	break;
        }
        
		case WM_CTLCOLOREDIT: {
            HDC hdc = (HDC)wParam;
            HWND ctl = (HWND)lParam;
            if (ctl == hwndTipsEdit) {
                SetBkMode(hdc, TRANSPARENT);
                SetTextColor(hdc, GetSysColor(COLOR_WINDOWTEXT));
                return (INT_PTR)GetSysColorBrush(COLOR_BTNFACE);
            }
            break;
        }

        case WM_COMMAND: {
            // Gestione pulsanti
            switch (LOWORD(wParam)) {
                case 4: // BTN_TOGGLE_CAPTURE = hwndToggleCaptureButton
                    CaptureMouse(hwnd, isMouseCaptured, labelRect);
                    break;
	        
                case 5: // BTN_PASSWORD = hwndPasswordButton
                    // Qui andrà la logica per impostare/salvare la password
                    DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_PASSWORD_DIALOG), hwnd, PasswordDlgProc);
                    break;
				case 6: // hwndSettingsButton
                    DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_SETTINGS_DIALOG), hwnd, SettingsDlgProc);
                    break;
                case 7: // CHK_SHOW_PASSWORD = hwndShowPassCheckbox
                    isShowPassChecked = SendMessage(hwndShowPassCheckbox, BM_GETCHECK, 0, 0) == BST_CHECKED;
                    SendMessage(hwndPasswordEdit, EM_SETPASSWORDCHAR, isShowPassChecked ? 0 : '*', 0);
                    InvalidateRect(hwndPasswordEdit, NULL, TRUE);
                    break;
			}
            break;
        }

        case WM_DESTROY: {
            DeleteObject(hFont);   // Pulisce il font
            ClipCursor(NULL);      // Rilascia il mouse se ancora bloccato

			// Salva posizione finestra
            Settings s;
            if (LoadSettings(s)) {
                RECT rc;
                if (GetWindowRect(hwnd, &rc)) {
                    s.win_pos_x = rc.left;
                    s.win_pos_y = rc.top;
                }
                SaveSettings(s);  // Salvi solo posizione aggiornata
            }

            PostQuitMessage(0);    // Termina l'app
            break;
        }

        case WM_SETCURSOR: {
            // Imposta il cursore a freccia quando è sopra l'area hwndCaptureAreaLabel (facoltativo)
            if (hwnd == hwndCaptureAreaLabel && isMouseCaptured) {
                SetCursor(LoadCursor(NULL, IDC_ARROW));
                return TRUE;
            }
            break;
        }

        case WM_CLOSE: {
            // Blocca la chiusura se il mouse è catturato
            if (isMouseCaptured) {
                MsgTipsBox("ATTENZIONE: Non puoi chiudere la finestra finché il blocco è attivo.\nInserisci la password per sbloccare.", LoadIcon(NULL, IDI_WARNING));
            } else {
                DestroyWindow(hwnd);
            }
            return 0;
        }

        default:
            return DefWindowProc(hwnd, Message, wParam, lParam);
    }
    return 0;
}

// ---------------------------------------------------------------------------------------
// SEZIONE: Funzione WinMain
// DESCRIZIONE: Funzione principale, registra classe finestra e avvia il ciclo dei messaggi
// ---------------------------------------------------------------------------------------
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    WNDCLASSEX wc;
    HWND hwnd;
    MSG msg;
    
    // Verifica se il file data.cfg esiste, altrimenti richiedi la creazione della password
    // Inizializzazione file di configurazione con valori di default
	FILE* f = fopen("data.cfg", "r");
    if (!f) {
        // Password cifrata base64 di stringa vuota
        settings.autolock          = 0;
        settings.block_keys        = false;
        settings.no_pass_rules     = false;
        settings.win_pos_x         = 100;
        settings.win_pos_y         = 100;
        settings.win_center        = true;
        settings.passwordEncrypted = "";
    
        SaveSettings(settings);
    }
    else {
        fclose(f);
		LoadSettings(settings);
    }

    // Inizializzazione della struttura della finestra
    memset(&wc, 0, sizeof(wc));
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = GetSysColorBrush(COLOR_BTNFACE);
    wc.lpszClassName = "WindowClass";

    // Caricamento icona: da risorsa o da file
    wc.hIcon   = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MYICON));
    wc.hIconSm = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MYICON));

    // Fallback se fallisce
    if (wc.hIcon == NULL || wc.hIconSm == NULL) {
        wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
        wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
	}

    // Registrazione della classe della finestra
    if (!RegisterClassEx(&wc)) {
        MessageBox(hwnd, "ATTENZIONE: Registrazione della finestra fallita!", "ERRORE", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

		// Determina posizione iniziale finestra
    int startX = settings.win_center ? CW_USEDEFAULT : settings.win_pos_x;
    int startY = settings.win_center ? CW_USEDEFAULT : settings.win_pos_y;

    // Creazione della finestra principale
    hwnd = CreateWindowEx(WS_EX_CLIENTEDGE, "WindowClass", "TrapMouse",
        WS_VISIBLE | WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        startX, startY, 500, 300, NULL, NULL, hInstance, NULL);

    if (hwnd == NULL) {
        MessageBox(hwnd, "ATTENZIONE: Creazione della finestra fallita!", "ERRORE", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }
    
    // Se richiesta centratura: riposiziona la finestra al centro dello schermo
    if (settings.win_center) {
        RECT rc;
        GetWindowRect(hwnd, &rc);
    
        int winWidth  = rc.right - rc.left;
        int winHeight = rc.bottom - rc.top;
    
        int screenWidth  = GetSystemMetrics(SM_CXSCREEN);
        int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    
        int posX = (screenWidth  - winWidth)  / 2;
        int posY = (screenHeight - winHeight) / 2;
    
        SetWindowPos(hwnd, NULL, posX, posY, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
    }

	if (settings.autolock == 2) {
        CaptureMouse(hwnd, isMouseCaptured, labelRect);
    }

    // Loop principale dei messaggi
    while (GetMessage(&msg, NULL, 0, 0) > 0) {
        if (!IsDialogMessage(hwnd, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    return msg.wParam;
}

