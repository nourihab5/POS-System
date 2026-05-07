#include "CryptoWrapper.h"
#include <fstream>
#include <sstream>
#include <iomanip>


std::vector<BYTE> CryptoWrapper::HashSHA256(const std::string& data) {
    BCRYPT_ALG_HANDLE hAlg = NULL;
    BCRYPT_HASH_HANDLE hHash = NULL;
    std::vector<BYTE> hash(32); 

    if (NT_SUCCESS(BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_SHA256_ALGORITHM, NULL, 0))) {
        if (NT_SUCCESS(BCryptCreateHash(hAlg, &hHash, NULL, 0, NULL, 0, 0))) {
            BCryptHashData(hHash, (PUCHAR)data.c_str(), data.length(), 0);
            BCryptFinishHash(hHash, hash.data(), hash.size(), 0);
            BCryptDestroyHash(hHash);
        }
        BCryptCloseAlgorithmProvider(hAlg, 0);
    }
    return hash;
}


std::string CryptoWrapper::BytesToHexString(const std::vector<BYTE>& data) {
    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    for (BYTE b : data) ss << std::setw(2) << (int)b;
    return ss.str();
}

std::vector<BYTE> CryptoWrapper::HexStringToBytes(const std::string& hex) {
    std::vector<BYTE> bytes;
    for (size_t i = 0; i < hex.length(); i += 2) {
        std::string byteString = hex.substr(i, 2);
        BYTE b = (BYTE)strtol(byteString.c_str(), NULL, 16);
        bytes.push_back(b);
    }
    return bytes;
}


std::vector<BYTE> CryptoWrapper::DeriveKeyPBKDF2(const std::string& password, const std::vector<BYTE>& salt) {
    BCRYPT_ALG_HANDLE hAlg = NULL;
    std::vector<BYTE> derivedKey(32); 

    if (NT_SUCCESS(BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_SHA256_ALGORITHM, NULL, BCRYPT_ALG_HANDLE_HMAC_FLAG))) {
        BCryptDeriveKeyPBKDF2(hAlg, (PUCHAR)password.data(), password.length(), 
                              (PUCHAR)salt.data(), salt.size(), 
                              310000, derivedKey.data(), derivedKey.size(), 0);
        BCryptCloseAlgorithmProvider(hAlg, 0);
    }
    return derivedKey;
}


std::vector<BYTE> CryptoWrapper::EncryptAES(const std::string& plaintext, const std::vector<BYTE>& key) {
    BCRYPT_ALG_HANDLE hAlg = NULL;
    BCRYPT_KEY_HANDLE hKey = NULL;
    std::vector<BYTE> ciphertext;

    if (NT_SUCCESS(BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_AES_ALGORITHM, NULL, 0))) {
        BCryptSetProperty(hAlg, BCRYPT_CHAINING_MODE, (PUCHAR)BCRYPT_CHAIN_MODE_CBC, sizeof(BCRYPT_CHAIN_MODE_CBC), 0);

        DWORD cbKeyObject = 0, cbData = 0;
        BCryptGetProperty(hAlg, BCRYPT_OBJECT_LENGTH, (PUCHAR)&cbKeyObject, sizeof(DWORD), &cbData, 0);
        std::vector<BYTE> keyObject(cbKeyObject);

        if (NT_SUCCESS(BCryptGenerateSymmetricKey(hAlg, &hKey, keyObject.data(), keyObject.size(), (PUCHAR)key.data(), key.size(), 0))) {
            std::vector<BYTE> originalIv = GenerateRandomBytes(16);
            std::vector<BYTE> ivForSize = originalIv;     
            std::vector<BYTE> ivForEncrypt = originalIv;  

            DWORD cbResult = 0;
            
            if (NT_SUCCESS(BCryptEncrypt(hKey, (PUCHAR)plaintext.data(), plaintext.size(), NULL, ivForSize.data(), ivForSize.size(), NULL, 0, &cbResult, BCRYPT_BLOCK_PADDING))) {
                std::vector<BYTE> encryptedData(cbResult);
                
                
                if (NT_SUCCESS(BCryptEncrypt(hKey, (PUCHAR)plaintext.data(), plaintext.size(), NULL, ivForEncrypt.data(), ivForEncrypt.size(), encryptedData.data(), encryptedData.size(), &cbResult, BCRYPT_BLOCK_PADDING))) {
                    
                    ciphertext.insert(ciphertext.end(), originalIv.begin(), originalIv.end());
                    ciphertext.insert(ciphertext.end(), encryptedData.begin(), encryptedData.begin() + cbResult);
                }
            }
            BCryptDestroyKey(hKey);
        }
        BCryptCloseAlgorithmProvider(hAlg, 0);
    }
    return ciphertext;
}

std::vector<BYTE> CryptoWrapper::DecryptAES(const std::vector<BYTE>& ciphertext, const std::vector<BYTE>& key) {
    if (ciphertext.size() < 16) return {}; 

    BCRYPT_ALG_HANDLE hAlg = NULL;
    BCRYPT_KEY_HANDLE hKey = NULL;
    std::vector<BYTE> plaintextBytes; 

    if (NT_SUCCESS(BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_AES_ALGORITHM, NULL, 0))) {
        BCryptSetProperty(hAlg, BCRYPT_CHAINING_MODE, (PUCHAR)BCRYPT_CHAIN_MODE_CBC, sizeof(BCRYPT_CHAIN_MODE_CBC), 0);

        DWORD cbKeyObject = 0, cbData = 0;
        BCryptGetProperty(hAlg, BCRYPT_OBJECT_LENGTH, (PUCHAR)&cbKeyObject, sizeof(DWORD), &cbData, 0);
        std::vector<BYTE> keyObject(cbKeyObject);

        if (NT_SUCCESS(BCryptGenerateSymmetricKey(hAlg, &hKey, keyObject.data(), keyObject.size(), (PUCHAR)key.data(), key.size(), 0))) {
            
            
            std::vector<BYTE> originalIv(ciphertext.begin(), ciphertext.begin() + 16);
            std::vector<BYTE> actualCiphertext(ciphertext.begin() + 16, ciphertext.end());

            
            std::vector<BYTE> ivForSize = originalIv;
            std::vector<BYTE> ivForDecrypt = originalIv;

            DWORD cbResult = 0;
            
            if (NT_SUCCESS(BCryptDecrypt(hKey, actualCiphertext.data(), actualCiphertext.size(), NULL, ivForSize.data(), ivForSize.size(), NULL, 0, &cbResult, BCRYPT_BLOCK_PADDING))) {
                plaintextBytes.resize(cbResult); 
                if (NT_SUCCESS(BCryptDecrypt(hKey, actualCiphertext.data(), actualCiphertext.size(), NULL, ivForDecrypt.data(), ivForDecrypt.size(), plaintextBytes.data(), plaintextBytes.size(), &cbResult, BCRYPT_BLOCK_PADDING))) {
                    plaintextBytes.resize(cbResult); 
                } else {
                    plaintextBytes.clear(); 
                }
            }
            BCryptDestroyKey(hKey);
        }
        BCryptCloseAlgorithmProvider(hAlg, 0);
    }
    return plaintextBytes;
}


bool CryptoWrapper::SaveToFile(const std::string& filepath, const std::vector<BYTE>& data) {
    std::ofstream file(filepath, std::ios::binary);
    if (!file.is_open()) return false;
    file.write(reinterpret_cast<const char*>(data.data()), data.size());
    return true;
}

std::vector<BYTE> CryptoWrapper::LoadFromFile(const std::string& filepath) {
    std::ifstream file(filepath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) return {};
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    std::vector<BYTE> buffer(size);
    if (file.read(reinterpret_cast<char*>(buffer.data()), size)) return buffer;
    return {};
}


std::vector<BYTE> CryptoWrapper::GenerateRandomBytes(DWORD length) {
    BCRYPT_ALG_HANDLE hAlg = NULL;
    std::vector<BYTE> randomBytes(length);
    if (NT_SUCCESS(BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_RNG_ALGORITHM, NULL, 0))) {
        BCryptGenRandom(hAlg, randomBytes.data(), randomBytes.size(), 0);
        BCryptCloseAlgorithmProvider(hAlg, 0);
    }
    return randomBytes;
}

std::pair<std::string, std::string> CryptoWrapper::EncryptHybrid(const std::string& plaintext, const std::vector<BYTE>& rsaPublicKeyBlob) {
    std::vector<BYTE> sessionAesKey = GenerateRandomBytes(32); 
    
    std::vector<BYTE> encryptedDataBytes = EncryptAES(plaintext, sessionAesKey);

    CryptoWrapper::RSA rsa;
    rsa.ImportPublicKey(rsaPublicKeyBlob);
    std::vector<BYTE> encryptedAesKeyBytes = rsa.Encrypt(sessionAesKey);

    SecureZeroMemory(sessionAesKey.data(), sessionAesKey.size());

    return std::make_pair(BytesToHexString(encryptedDataBytes), BytesToHexString(encryptedAesKeyBytes));
}

std::string CryptoWrapper::DecryptHybrid(const std::string& encryptedDataHex, const std::string& encryptedAesKeyHex, const std::vector<BYTE>& rsaPrivateKeyBlob) {
    std::vector<BYTE> encryptedDataBytes = HexStringToBytes(encryptedDataHex);
    std::vector<BYTE> encryptedAesKeyBytes = HexStringToBytes(encryptedAesKeyHex);

    CryptoWrapper::RSA rsa;
    rsa.ImportPrivateKey(rsaPrivateKeyBlob);
    std::vector<BYTE> sessionAesKey = rsa.Decrypt(encryptedAesKeyBytes);

    if (sessionAesKey.empty()) return ""; 

    std::vector<BYTE> plaintextBytes = DecryptAES(encryptedDataBytes, sessionAesKey);

    SecureZeroMemory(sessionAesKey.data(), sessionAesKey.size());

    if (plaintextBytes.empty()) return "";
    return std::string(plaintextBytes.begin(), plaintextBytes.end());
}


CryptoWrapper::RSA::RSA() : hAlg(NULL), hKey(NULL) {
    BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_RSA_ALGORITHM, NULL, 0);
}

CryptoWrapper::RSA::RSA(DWORD keySize) : hAlg(NULL), hKey(NULL) {
    if (NT_SUCCESS(BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_RSA_ALGORITHM, NULL, 0))) {
        if (NT_SUCCESS(BCryptGenerateKeyPair(hAlg, &hKey, keySize, 0))) {
            BCryptFinalizeKeyPair(hKey, 0);
        }
    }
}

CryptoWrapper::RSA::~RSA() {
    if (hKey) BCryptDestroyKey(hKey);
    if (hAlg) BCryptCloseAlgorithmProvider(hAlg, 0);
}

std::vector<BYTE> CryptoWrapper::RSA::ExportPublicKey() {
    if (!hKey) return {};
    DWORD cbBlob = 0;
    BCryptExportKey(hKey, NULL, BCRYPT_RSAPUBLIC_BLOB, NULL, 0, &cbBlob, 0);
    std::vector<BYTE> blob(cbBlob);
    BCryptExportKey(hKey, NULL, BCRYPT_RSAPUBLIC_BLOB, blob.data(), blob.size(), &cbBlob, 0);
    return blob;
}

std::vector<BYTE> CryptoWrapper::RSA::ExportPrivateKey() {
    if (!hKey) return {};
    DWORD cbBlob = 0;
    BCryptExportKey(hKey, NULL, BCRYPT_RSAPRIVATE_BLOB, NULL, 0, &cbBlob, 0);
    std::vector<BYTE> blob(cbBlob);
    BCryptExportKey(hKey, NULL, BCRYPT_RSAPRIVATE_BLOB, blob.data(), blob.size(), &cbBlob, 0);
    return blob;
}

bool CryptoWrapper::RSA::ImportPublicKey(const std::vector<BYTE>& publicKeyBlob) {
    if (hKey) { BCryptDestroyKey(hKey); hKey = NULL; }
    return NT_SUCCESS(BCryptImportKeyPair(hAlg, NULL, BCRYPT_RSAPUBLIC_BLOB, &hKey, (PUCHAR)publicKeyBlob.data(), publicKeyBlob.size(), 0));
}

bool CryptoWrapper::RSA::ImportPrivateKey(const std::vector<BYTE>& privateKeyBlob) {
    if (hKey) { BCryptDestroyKey(hKey); hKey = NULL; }
    return NT_SUCCESS(BCryptImportKeyPair(hAlg, NULL, BCRYPT_RSAPRIVATE_BLOB, &hKey, (PUCHAR)privateKeyBlob.data(), privateKeyBlob.size(), 0));
}

std::vector<BYTE> CryptoWrapper::RSA::Sign(const std::string& data) {
    if (!hKey) return {};
    std::vector<BYTE> hash = CryptoWrapper::HashSHA256(data);
    BCRYPT_PKCS1_PADDING_INFO paddingInfo = { BCRYPT_SHA256_ALGORITHM };
    DWORD cbSignature = 0;
    std::vector<BYTE> signature;

    if (NT_SUCCESS(BCryptSignHash(hKey, &paddingInfo, hash.data(), hash.size(), NULL, 0, &cbSignature, BCRYPT_PAD_PKCS1))) {
        signature.resize(cbSignature);
        BCryptSignHash(hKey, &paddingInfo, hash.data(), hash.size(), signature.data(), signature.size(), &cbSignature, BCRYPT_PAD_PKCS1);
    }
    return signature;
}

bool CryptoWrapper::RSA::Verify(const std::string& data, const std::vector<BYTE>& signature) {
    if (!hKey || signature.empty()) return false;
    std::vector<BYTE> hash = CryptoWrapper::HashSHA256(data);
    BCRYPT_PKCS1_PADDING_INFO paddingInfo = { BCRYPT_SHA256_ALGORITHM };
    
    NTSTATUS status = BCryptVerifySignature(hKey, &paddingInfo, hash.data(), hash.size(), (PUCHAR)signature.data(), signature.size(), BCRYPT_PAD_PKCS1);
    return (status == 0);
}

std::vector<BYTE> CryptoWrapper::RSA::Encrypt(const std::vector<BYTE>& plaintext) {
    if (!hKey) return {};
    DWORD cbResult = 0;
    std::vector<BYTE> ciphertext;

    BCRYPT_OAEP_PADDING_INFO paddingInfo = { BCRYPT_SHA256_ALGORITHM };

    if (NT_SUCCESS(BCryptEncrypt(hKey, (PUCHAR)plaintext.data(), plaintext.size(),
                                 &paddingInfo, NULL, 0, NULL, 0, &cbResult, BCRYPT_PAD_OAEP))) {
        ciphertext.resize(cbResult);
        if (!NT_SUCCESS(BCryptEncrypt(hKey, (PUCHAR)plaintext.data(), plaintext.size(),
                                      &paddingInfo, NULL, 0, ciphertext.data(), ciphertext.size(), &cbResult, BCRYPT_PAD_OAEP))) {
            ciphertext.clear();
        }
    }
    return ciphertext;
}

std::vector<BYTE> CryptoWrapper::RSA::Decrypt(const std::vector<BYTE>& ciphertext) {
    if (!hKey) return {};
    DWORD cbResult = 0;
    std::vector<BYTE> plaintext;

    BCRYPT_OAEP_PADDING_INFO paddingInfo = { BCRYPT_SHA256_ALGORITHM };

    if (NT_SUCCESS(BCryptDecrypt(hKey, (PUCHAR)ciphertext.data(), ciphertext.size(),
                                 &paddingInfo, NULL, 0, NULL, 0, &cbResult, BCRYPT_PAD_OAEP))) {
        plaintext.resize(cbResult);
        if (!NT_SUCCESS(BCryptDecrypt(hKey, (PUCHAR)ciphertext.data(), ciphertext.size(),
                                      &paddingInfo, NULL, 0, plaintext.data(), plaintext.size(), &cbResult, BCRYPT_PAD_OAEP))) {
            plaintext.clear();
        }
    }
    return plaintext;
}