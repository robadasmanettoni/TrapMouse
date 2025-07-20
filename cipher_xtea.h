#ifndef CIPHER_XTEA_H
#define CIPHER_XTEA_H

#include <stddef.h>
#include <vector>
#include <string>

// Cifra e salva la password su file
bool SavePassword_XTEA(const char* plaintext, std::vector<unsigned char>& out);

// Carica e decifra la password dal file
bool LoadPassword_XTEA(const std::string& base64, char* out, size_t outSize);

#endif

