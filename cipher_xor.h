#ifndef CIPHER_XOR_H
#define CIPHER_XOR_H

#include <stddef.h>
#include <vector>
#include <string>

// Cifra e salva la password in esadecimale su file
bool SavePassword_XOR(const char* plaintext, std::vector<unsigned char>& out);

// Decifra una password cifrata in esadecimale letta da file
bool LoadPassword_XOR(const std::string& base64, char* out, size_t outSize);

#endif

