#pragma once
#include <windows.h>
#include <bcrypt.h>
#include <string>
#include <vector>
#include <utility>
#include <stdexcept>

#ifndef NT_SUCCESS
#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)
#endif

class CryptoWrapper {
public:
    static std::vector<BYTE> HashSHA256(const std::string& data);
    static std::vector<BYTE> DeriveKeyPBKDF2(const std::string& password, const std::vector<BYTE>& salt);
    static std::vector<BYTE> EncryptAES(const std::string& plaintext, const std::vector<BYTE>& key);
    static std::vector<BYTE> DecryptAES(const std::vector<BYTE>& ciphertext, const std::vector<BYTE>& key);
    static std::string BytesToHexString(const std::vector<BYTE>& data);
    static std::vector<BYTE> HexStringToBytes(const std::string& hex);
    static bool SaveToFile(const std::string& filepath, const std::vector<BYTE>& data);
    static std::vector<BYTE> LoadFromFile(const std::string& filepath);
    static std::vector<BYTE> GenerateRandomBytes(DWORD length);
    static std::pair<std::string, std::string> EncryptHybrid(const std::string& plaintext, const std::vector<BYTE>& rsaPublicKeyBlob);
    static std::string DecryptHybrid(const std::string& encryptedDataHex, const std::string& encryptedAesKeyHex, const std::vector<BYTE>& rsaPrivateKeyBlob);
    class RSA {
    private:
        BCRYPT_ALG_HANDLE hAlg;
        BCRYPT_KEY_HANDLE hKey;

    public:
        RSA(); 
        RSA(DWORD keySize); 
        ~RSA();
        std::vector<BYTE> ExportPublicKey();
        std::vector<BYTE> ExportPrivateKey();
        bool ImportPublicKey(const std::vector<BYTE>& publicKeyBlob);
        bool ImportPrivateKey(const std::vector<BYTE>& privateKeyBlob);
        std::vector<BYTE> Encrypt(const std::vector<BYTE>& plaintext);
        std::vector<BYTE> Decrypt(const std::vector<BYTE>& ciphertext);
        std::vector<BYTE> Sign(const std::string& data);
        bool Verify(const std::string& data, const std::vector<BYTE>& signature);
    };
};