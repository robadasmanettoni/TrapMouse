#ifndef CIPHER_SALSA20_H
#define CIPHER_SALSA20_H

#include <stddef.h>
#include <vector>
#include <string>

// Cifra e salva la password su file (data.cfg)
bool SavePassword_SALSA20(const char* plaintext, std::vector<unsigned char>& out);

// Carica e decifra la password dal file (data.cfg)
bool LoadPassword_SALSA20(const std::string& base64, char* out, size_t outSize);

#endif

