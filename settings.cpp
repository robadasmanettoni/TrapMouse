#include "settings.h"
#include <fstream>
#include <sstream>
#include <cstring>
#include <map>

// Converte una stringa in intero (es. "42" in 42)
static int ToInt(const std::string& s) {
    std::istringstream iss(s);
    int v=0; iss>>v;
    return v;
}

// Converte una stringa in booleano (accetta "1", "true", "TRUE")
static bool ToBool(const std::string& s) {
    return s=="1"||s=="true"||s=="TRUE";
}

// Converte un intero in stringa (es. 42 > "42")
static std::string FromInt(int v) {
    std::ostringstream oss; oss<<v; return oss.str();
}

// Converte un booleano in stringa ("1" se true, "0" se false)
static std::string FromBool(bool b) {
    return b ? "1" : "0";
}

// Carica le impostazioni da "data.cfg" e riempie la struct Settings
bool LoadSettings(Settings& s) {
    std::ifstream f("data.cfg");  // Apre il file in lettura
    if (!f) return false;         // Se il file non esiste o fallisce, ritorna false

    std::map<std::string, std::string> cfg; // Mappa chiave-valore per configurazione
    std::string line;

    // Legge ogni riga del file
    while (std::getline(f, line)) {
        // Ignora righe vuote o che iniziano con # o ;
        if (line.empty() || line[0] == '#' || line[0] == ';') continue;

        // Trova il simbolo '=' che separa chiave e valore
        size_t pos = line.find('=');
        if (pos == std::string::npos) continue; // se manca '=', salta la riga

        // Estrae chiave e valore separati
        std::string key = line.substr(0, pos);
        std::string val = line.substr(pos + 1);

        // Salva la coppia nella mappa
        cfg[key] = val;
    }

    // Estrae i valori dalla mappa e li assegna alla struct Settings
    s.autolock          =  ToInt(cfg["autolock"]);
    s.block_keys        = ToBool(cfg["block_sys_keys"]);
    s.no_pass_rules     = ToBool(cfg["no_pass_rules"]);
    s.cipher            =  ToInt(cfg["cipher"]);
    s.win_pos_x         =  ToInt(cfg["window_pos_x"]);
    s.win_pos_y         =  ToInt(cfg["window_pos_y"]);
    s.win_center        = ToBool(cfg["window_centered"]);
    s.passwordEncrypted =        cfg["password"];

    return true;
}

// Salva la struct Settings nel file "data.cfg"
bool SaveSettings(const Settings& s) {
    std::ofstream f("data.cfg");  // Apre il file in scrittura (sovrascrive)
    if (!f) return false;         // Se il file non può essere aperto, ritorna false

    // Scrive ogni campo come riga "chiave=valore"
    f << "autolock="        << FromInt(s.autolock)       << "\n"
      << "block_sys_keys="  << FromBool(s.block_keys)    << "\n"
      << "no_pass_rules="   << FromBool(s.no_pass_rules) << "\n"
      << "cipher="          << FromInt(s.cipher)         << "\n"
      << "window_pos_x="    << FromInt(s.win_pos_x)      << "\n"
      << "window_pos_y="    << FromInt(s.win_pos_y)      << "\n"
      << "window_centered=" << FromBool(s.win_center)    << "\n"
      << "password="        << s.passwordEncrypted       << "\n";

    return true;
}
