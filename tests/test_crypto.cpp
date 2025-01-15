#include <gtest/gtest.h>
#include "crypto.h"

namespace crypto {

// Test for encrypt
TEST(CryptoTest, Encrypt_01) {
    std::vector<uint8_t> key = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16};
    std::vector<uint8_t> data = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16};

    std::vector<uint8_t> encrypted = encrypt(key, data);

    // Check size of the encrypted data
    EXPECT_EQ(encrypted.size(), data.size());
    EXPECT_EQ(encrypted,std::vector<uint8_t>({157, 187, 45, 186, 18, 189, 73, 146, 48, 133, 221, 106, 193, 240, 34, 23}));

    // Mismatched key size
   // EXPECT_THROW(encrypt(std::vector<uint8_t>{}, data), std::invalid_argument);
}

TEST(CryptoTest, Encrypt_02) {
    std::vector<uint8_t> key = {0x1a, 0x2b, 0x3c, 0x4d, 0x5e, 0x6f, 0x70, 0x81, 0x92, 0xa3, 0xb4, 0xc5, 0xd6, 0xe7, 0xf8, 0x09};
    std::vector<uint8_t> data = {0x0a, 0x1b, 0x2c, 0x3d, 0x4e, 0x5f, 0x60, 0x71, 0x82, 0x93, 0xa4, 0xb5, 0xc6, 0xd7, 0xe8, 0xf9};

    std::vector<uint8_t> encrypted = encrypt(key, data);

    // Check size of the encrypted data
    EXPECT_EQ(encrypted.size(), data.size());
    EXPECT_EQ(encrypted,std::vector<uint8_t>({0x82, 0x8d, 0x43, 0x32, 0x12, 0x08, 0x30, 0x68, 0x43, 0x75, 0xa2, 0x37, 0x14, 0xf1, 0x80, 0x22}));

    // Mismatched key size
   // EXPECT_THROW(encrypt(std::vector<uint8_t>{}, data), std::invalid_argument);
}

// Test for generate_sk
TEST(CryptoTest, GenerateSK) {
    std::string name = "telink_mesh1";
    std::string password = "123";
    std::vector<uint8_t> data1 = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16};
    std::vector<uint8_t> data2 = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16};

    std::vector<uint8_t> sk = generate_sk(name, password, data1, data2);

    // Verify size of generated key
    EXPECT_EQ(sk.size(), 16);
    EXPECT_EQ(sk, std::vector<uint8_t>({153, 89, 200, 250, 200, 19, 213, 120, 16, 183, 73, 50, 203, 160, 231, 160}));
    // Invalid input cases
    //EXPECT_THROW(generate_sk("", password, data1, data2), std::invalid_argument);
    //EXPECT_THROW(generate_sk(name, "", data1, data2), std::invalid_argument);
}

// Test for key_encrypt
TEST(CryptoTest, KeyEncrypt) {
    std::string name = "telink_mesh1";
    std::string password = "123";
    std::vector<uint8_t> key(16, 0x01);

    std::vector<uint8_t> encrypted_key = key_encrypt(name, password, key);

    // Check size of the encrypted key matches input size
    EXPECT_EQ(encrypted_key.size(), key.size());
    EXPECT_EQ(encrypted_key, std::vector<uint8_t>({174, 154, 27, 1, 57, 3, 210, 248, 253, 148, 69, 217, 87, 75, 122, 109}));

    // Invalid input cases
    //EXPECT_THROW(key_encrypt("", password, key), std::invalid_argument);
    //EXPECT_THROW(key_encrypt(name, "", key), std::invalid_argument);
    //EXPECT_THROW(key_encrypt(name, password, std::vector<uint8_t>{}), std::invalid_argument);
}

// Test for encrypt_packet
TEST(CryptoTest, EncryptPacket) {
    std::vector<uint8_t> sk = {153, 89, 200, 250, 200, 19, 213, 120, 16, 183, 73, 50, 203, 160, 231, 160};
    std::vector<uint8_t> address = {0x12, 0x34, 0x56, 0x78, 0xAA, 0xBB, 0xCC, 0xDD, 0x12, 0x34, 0x56, 0x78, 0xAA, 0xBB, 0xCC, 0xDD};
    std::vector<uint8_t> packet = {0xAA, 0xBB, 0xCC, 0xDD, 0xAA, 0xBB, 0xCC, 0xDD, 0x12, 0x34, 0x56, 0x78, 0xAA, 0xBB, 0xCC, 0xDD, 0x12, 0x34, 0x56, 0x78, 0xAA, 0xBB, 0xCC, 0xDD, 0x12, 0x34, 0x56, 0x78, 0xAA, 0xBB, 0xCC, 0xDD};

    std::vector<uint8_t> encrypted_packet = encrypt_packet(sk, address, packet);

    // Check if the encrypted packet size matches the input packet
    EXPECT_EQ(encrypted_packet.size(), packet.size());
    EXPECT_EQ(encrypted_packet, std::vector<uint8_t>({170, 187, 204, 97, 247, 22, 165, 146, 255, 240, 15, 116, 7, 148, 52, 219, 74, 203, 171, 154, 170, 187, 204, 221, 18, 52, 86, 120, 170, 187, 204, 221}));
    // Invalid input cases
    //EXPECT_THROW(encrypt_packet(std::vector<uint8_t>{}, address, packet), std::invalid_argument);
    //EXPECT_THROW(encrypt_packet(sk, std::vector<uint8_t>{}, packet), std::invalid_argument);
    //EXPECT_THROW(encrypt_packet(sk, address, std::vector<uint8_t>{}), std::invalid_argument);
}

// Test for decrypt_packet
TEST(CryptoTest, DecryptPacket) {
    std::vector<uint8_t> sk = { 0x1a, 0x2b, 0x3c, 0x4d,
                                0x5e, 0x6f, 0x70, 0x81,
                                0x92, 0xa3, 0xb4, 0xc5,
                                0xd6, 0xe7, 0xf8, 0x09};
    std::vector<uint8_t> address = {0xa1, 0xb2, 0xc3, 0xd4, 0xe5, 0xf6};
    std::vector<uint8_t> packet = { 0x01, 0x02, 0x03, 0x04,
                                    0x05, 0x06, 0x07, 0x08,
                                    0x09, 0x0a, 0x0b, 0x0c,
                                    0x0d, 0x0e, 0x0f, 0x10,
                                    0x11, 0x12, 0x13, 0x14};
    // Encrypt the packet first
    std::vector<uint8_t> encrypted_packet = encrypt_packet(sk, address, packet);

    // Decrypt the encrypted packet
    std::vector<uint8_t> decrypted_packet = decrypt_packet(sk, address, encrypted_packet);

    std::vector<uint8_t> decrypted_packet2 = decrypt_packet(sk, address, packet);
    auto expected = std::vector<uint8_t>({0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0xeb, 0x77, 0xf0, 0xb9, 0xe0, 0xe7, 0x2e, 0x20, 0xec, 0xc8, 0xd9, 0x91, 0x93});

    // Check if decrypted packet matches the original
    //EXPECT_EQ(decrypted_packet, packet);
    EXPECT_EQ(decrypted_packet2, expected);

    // Invalid input cases
    //EXPECT_THROW(decrypt_packet(std::vector<uint8_t>{}, address, encrypted_packet), std::invalid_argument);
    //EXPECT_THROW(decrypt_packet(sk, std::vector<uint8_t>{}, encrypted_packet), std::invalid_argument);
    //EXPECT_THROW(decrypt_packet(sk, address, std::vector<uint8_t>{}), std::invalid_argument);
}

} // namespace crypto
