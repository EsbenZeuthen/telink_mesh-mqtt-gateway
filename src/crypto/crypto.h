#ifndef CRYPTO_H
#define CRYPTO_H

#include <vector>
#include <cstdint>
#include <string>

namespace crypto {

std::vector<uint8_t> get_random_bytes(size_t num_bytes);

// AES encryption (ECB mode)
std::vector<uint8_t> encrypt(const std::vector<uint8_t>& key, const std::vector<uint8_t>& data);

// Generate a secret key based on name, password, and other data
std::vector<uint8_t> generate_sk(const std::string& name, const std::string& password,
                                  const std::vector<uint8_t>& data1, const std::vector<uint8_t>& data2);

// Encrypt a key based on the name, password, and the secret key
std::vector<uint8_t> key_encrypt(const std::string& name, const std::string& password, const std::vector<uint8_t>& key);

// Encrypt the packet with the secret key
std::vector<uint8_t> encrypt_packet(const std::vector<uint8_t>& sk, const std::vector<uint8_t>& address, const std::vector<uint8_t>& packet);

// Decrypt the packet with the secret key
std::vector<uint8_t> decrypt_packet(const std::vector<uint8_t>& sk, const std::vector<uint8_t>& address, const std::vector<uint8_t>& packet);

void print_hex(const std::string &label, const std::vector<unsigned char> &data);

} // namespace crypto

#endif // CRYPTO_H
