#include <iostream>
#include <vector>
#include <openssl/aes.h>
#include <openssl/rand.h>
#include <algorithm>  
#include <iomanip>
#include <sstream>
#include <random>
#include <cassert>

namespace crypto {

std::vector<uint8_t> get_random_bytes(size_t num_bytes) {
    std::random_device rd;  // Non-deterministic random number generator
    std::mt19937 gen(rd()); // Seed the Mersenne Twister
    std::uniform_int_distribution<uint8_t> dist(0, 255);

    std::vector<uint8_t> random_bytes(num_bytes);
    for (size_t i = 0; i < num_bytes; ++i) {
        random_bytes[i] = dist(gen);
    }
    return random_bytes;
}

std::vector<unsigned char> encrypt(const std::vector<unsigned char> &key, const std::vector<unsigned char> &data) {
    
    assert(key.size() == 16 && "Key size must be 16 bytes (128 bits) for AES encryption.");
    // Reverse the key
    std::vector<unsigned char> reversed_key(key.rbegin(), key.rend());
    
    assert(reversed_key.size() == 16 && "Reversed key size must be 16 bytes (128 bits) for AES encryption.");

    // Set up AES key with the reversed key
    AES_KEY aes_key = {};
    AES_set_encrypt_key(reversed_key.data(), 128, &aes_key); // 128-bit AES key

    // Reverse the data before encryption
    std::vector<unsigned char> reversed_data(data.rbegin(), data.rend());

    // Encrypt the reversed data
    std::vector<unsigned char> encrypted(reversed_data.size());
    for (size_t i = 0; i < reversed_data.size(); i += AES_BLOCK_SIZE) {
        AES_ecb_encrypt(reversed_data.data() + i, encrypted.data() + i, &aes_key, AES_ENCRYPT);
    }

    // Reverse the encrypted data to match the Python version
    std::reverse(encrypted.begin(), encrypted.end());

    return encrypted;
}


// Function to generate session key (sk)
std::vector<unsigned char> generate_sk(const std::string &name, const std::string &password, const std::vector<unsigned char> &data1, const std::vector<unsigned char> &data2) {
    std::string padded_name = name;
    padded_name.resize(16, '\0'); // Pad name to 16 bytes
    std::string padded_password = password;
    padded_password.resize(16, '\0'); // Pad password to 16 bytes

    std::vector<unsigned char> key(16);
    for (size_t i = 0; i < 16; ++i) {
        key[i] = padded_name[i] ^ padded_password[i]; // XOR operation
    }

    // Concatenate data1 and data2 (both truncated to 8 bytes)
    std::vector<unsigned char> data(16);
    std::copy(data1.begin(), data1.begin() + 8, data.begin());
    std::copy(data2.begin(), data2.begin() + 8, data.begin() + 8);

    return encrypt(key, data);
}

// Function to encrypt key with session key (sk)
std::vector<unsigned char> key_encrypt(const std::string &name, const std::string &password, const std::vector<unsigned char> &key) {
    std::string padded_name = name;
    padded_name.resize(16, '\0'); // Pad name to 16 bytes
    std::string padded_password = password;
    padded_password.resize(16, '\0'); // Pad password to 16 bytes

    std::vector<unsigned char> data(16);
    for (size_t i = 0; i < 16; ++i) {
        data[i] = padded_name[i] ^ padded_password[i]; // XOR operation
    }

    return encrypt(key, data);
}

// Function to encrypt a packet
std::vector<unsigned char> encrypt_packet(const std::vector<unsigned char> &sk, const std::vector<unsigned char> &address, const std::vector<unsigned char> &packet) {
    // Construct the authentication nonce
    std::vector<unsigned char> auth_nonce = {
        address[0], address[1], address[2], address[3], 0x01,
        packet[0], packet[1], packet[2], 15, 0, 0, 0, 0, 0, 0, 0
    };

    auto encrypted_packet = std::vector<uint8_t>(packet);

    // Encrypt authentication nonce
    std::vector<unsigned char> authenticator = encrypt(sk, auth_nonce);

    // XOR the authenticator with packet data (packet[5:])
    for (size_t i = 0; i < 15; ++i) {
        authenticator[i] ^= packet[i + 5];
    }

    // Encrypt the authenticator to get MAC
    std::vector<unsigned char> mac = encrypt(sk, authenticator);

    // Set MAC in the packet (first 2 bytes)
    encrypted_packet[3] = mac[0];
    encrypted_packet[4] = mac[1];

    // Construct the IV
    std::vector<unsigned char> iv = {
        0, address[0], address[1], address[2], address[3], 0x01,
        packet[0], packet[1], packet[2], 0, 0, 0, 0, 0, 0, 0
    };

    // Encrypt the IV
    std::vector<unsigned char> temp_buffer = encrypt(sk, iv);

    // XOR the packet data (packet[5:])
    for (size_t i = 0; i < 15; ++i) {
        encrypted_packet[i + 5] ^= temp_buffer[i];
    }

    return encrypted_packet;
}

void print_hex(const std::string &label, const std::vector<unsigned char> &data) {
    //g_debug("%s:", label.c_str());
    for (unsigned char byte : data) {
        std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)byte << " ";
    }
    std::cout << std::dec << std::endl; // Reset to decimal format after printing
}

// Function to decrypt a packet
std::vector<unsigned char> decrypt_packet(const std::vector<unsigned char> &sk, const std::vector<unsigned char> &address, const std::vector<unsigned char> &packet)
{
    // Construct the IV
    std::vector<unsigned char> iv = {
        address[0], address[1], address[2], packet[0], packet[1], packet[2],
        packet[3], packet[4], 0, 0, 0, 0, 0, 0, 0
    };

    //print_hex("IV", iv);

    std::vector<unsigned char> plaintext = { 0 };
    plaintext.insert(plaintext.end(), iv.begin(), iv.end());

    //print_hex("Plaintext", plaintext);

    // Encrypt the plaintext (IV) to get a result
    std::vector<unsigned char> result = encrypt(sk, plaintext);

    //print_hex("Result (Encrypted IV)", result);


    assert(result.size() >= packet.size()-7 && "result is smaller than packet size-7");

    auto decrypted_packet = std::vector<uint8_t>(packet);

    // XOR the packet data (packet[7:])
    for (size_t i = 0; i < decrypted_packet.size() - 7; i++) {

        decrypted_packet[i + 7] ^= result[i];
    }

    return decrypted_packet;
}

}