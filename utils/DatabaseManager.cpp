#include "DatabaseManager.h"
#include "CryptoWrapper.h" 
#include <json.hpp>
#include <sstream>  
#include <iomanip>
DatabaseManager::DatabaseManager(const std::string& filepath) {
    db = nullptr;
    if (sqlite3_open(filepath.c_str(), &db) != SQLITE_OK) {
        std::cerr << "[DB_ERROR] Failed to open DB: " << sqlite3_errmsg(db) << "\n";
		if (db) {
			sqlite3_close(db); 
			db = nullptr;	
		}
    }
}
DatabaseManager::~DatabaseManager() {
    if (db) {
        sqlite3_close(db);
        std::cout << "[DB_INFO] Database connection closed safely.\n";
    }
}
bool DatabaseManager::IsOpen() const {
    return db != nullptr;
}
bool DatabaseManager::ExecuteSQL(const std::string& sql) {
    if (!db) return false;
    char* errMsg = nullptr;
    int rc = sqlite3_exec(db, sql.c_str(), 0, 0, &errMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "[SQL_ERROR] " << errMsg << "\n";
        sqlite3_free(errMsg);
    }
    return rc == SQLITE_OK;
}
bool DatabaseManager::CreateTables(DbTemplate dbTemplate) {
    std::string sql;
    switch (dbTemplate) {
        case DbTemplate::USERS_DB:
            sql = "CREATE TABLE IF NOT EXISTS Users ("
                  "username TEXT PRIMARY KEY, "
                  "password_hash TEXT NOT NULL, "
                  "permissions INTEGER NOT NULL, "
                  "admin_signature TEXT NOT NULL);";
            break;
        case DbTemplate::PRODUCTS_DB:
            sql = "CREATE TABLE IF NOT EXISTS Products ("
                  "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                  "serial_number TEXT UNIQUE NOT NULL, " 
                  "name TEXT NOT NULL, "
                  "description TEXT, "
                  "cost REAL NOT NULL, "
                  "admin_signature TEXT NOT NULL);";
            break;
        case DbTemplate::INVOICES_DB:
            sql = "CREATE TABLE IF NOT EXISTS Invoices ("
                  "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                  "encrypted_aes_key TEXT NOT NULL, "
                  "encrypted_data TEXT NOT NULL);";
            break;
    }
    return ExecuteSQL(sql);
}
bool DatabaseManager::Append(const User& obj) {
    if (!db) return false;
    std::string sql = "INSERT INTO Users (username, password_hash, permissions, admin_signature) VALUES (?, ?, ?, ?);";
    sqlite3_stmt* stmt;
    bool success = false;
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, obj.username.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, obj.password_hash.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 3, obj.permissions);
        sqlite3_bind_text(stmt, 4, obj.admin_signature.c_str(), -1, SQLITE_TRANSIENT);
        success = (sqlite3_step(stmt) == SQLITE_DONE);
    }
    sqlite3_finalize(stmt);
    return success;
}
bool DatabaseManager::Get(User* userObj) {
    if (!db || !userObj) return false;
    std::string sql = "SELECT password_hash, permissions, admin_signature FROM Users WHERE username = ? LIMIT 1;";
    sqlite3_stmt* stmt;
    bool found = false;
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, userObj->username.c_str(), -1, SQLITE_TRANSIENT);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            userObj->password_hash = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
            userObj->permissions = sqlite3_column_int(stmt, 1);
            userObj->admin_signature = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
            found = true;
        }
    }
    sqlite3_finalize(stmt);
    return found;
}
bool DatabaseManager::Update(const User& oldObj, const User& newObj) {
    if (!db) return false;
    std::string sql = "UPDATE Users SET username = ?, password_hash = ?, permissions = ?, admin_signature = ? WHERE username = ?;";
    sqlite3_stmt* stmt;
    bool success = false;
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, newObj.username.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, newObj.password_hash.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 3, newObj.permissions);
        sqlite3_bind_text(stmt, 4, newObj.admin_signature.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 5, oldObj.username.c_str(), -1, SQLITE_TRANSIENT);
        success = (sqlite3_step(stmt) == SQLITE_DONE);
    }
    sqlite3_finalize(stmt);
    return success;
}
bool DatabaseManager::Delete(const User& searchObj) {
    if (!db) return false;
    std::string sql = "DELETE FROM Users WHERE username = ?;";
    sqlite3_stmt* stmt;
    bool success = false;
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, searchObj.username.c_str(), -1, SQLITE_TRANSIENT);
        success = (sqlite3_step(stmt) == SQLITE_DONE);
    }
    sqlite3_finalize(stmt);
    return success;
}
bool DatabaseManager::Append(const Product& obj) {
    if (!db) return false;
    std::string sql = "INSERT INTO Products (serial_number, name, description, cost, admin_signature) VALUES (?, ?, ?, ?, ?);";
    sqlite3_stmt* stmt;
    bool success = false;
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, obj.serial_number.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, obj.name.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 3, obj.description.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_double(stmt, 4, obj.cost); 
        sqlite3_bind_text(stmt, 5, obj.admin_signature.c_str(), -1, SQLITE_TRANSIENT);
        success = (sqlite3_step(stmt) == SQLITE_DONE);
    }
    sqlite3_finalize(stmt);
    return success;
}
bool DatabaseManager::Get(Product* prodObj) {
    if (!db || !prodObj) return false;
    std::string sql = "SELECT serial_number, name, description, cost, admin_signature FROM Products WHERE id = ? LIMIT 1;";
    sqlite3_stmt* stmt;
    bool found = false;
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, prodObj->id);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            prodObj->serial_number = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
            prodObj->name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            prodObj->description = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
            prodObj->cost = static_cast<float>(sqlite3_column_double(stmt, 3));
            prodObj->admin_signature = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
            found = true;
        }
    }
    sqlite3_finalize(stmt);
    return found;
}
bool DatabaseManager::Delete(const Product& searchObj) {
    if (!db) return false;
    std::string sql = "DELETE FROM Products WHERE id = ?;";
    sqlite3_stmt* stmt;
    bool success = false;
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, searchObj.id);
        success = (sqlite3_step(stmt) == SQLITE_DONE);
    }
    sqlite3_finalize(stmt);
    return success;
}
bool DatabaseManager::Update(const Product& oldObj, const Product& newObj) {
    if (!db) return false;
    std::string sql = "UPDATE Products SET serial_number = ?, name = ?, description = ?, cost = ?, admin_signature = ? WHERE id = ?;";
    sqlite3_stmt* stmt;
    bool success = false;
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, newObj.serial_number.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, newObj.name.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 3, newObj.description.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_double(stmt, 4, newObj.cost);
        sqlite3_bind_text(stmt, 5, newObj.admin_signature.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 6, oldObj.id);
        success = (sqlite3_step(stmt) == SQLITE_DONE);
    }
    sqlite3_finalize(stmt);
    return success;
}
bool DatabaseManager::Append(const Invoice& obj) {
    if (!db) return false;
    std::string sql = "INSERT INTO Invoices (encrypted_aes_key, encrypted_data) VALUES (?, ?);";
    sqlite3_stmt* stmt;
    bool success = false;
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, obj.encrypted_aes_key.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, obj.encrypted_data.c_str(), -1, SQLITE_TRANSIENT);
        success = (sqlite3_step(stmt) == SQLITE_DONE);
    }
    sqlite3_finalize(stmt);
    return success;
}
bool DatabaseManager::Get(Invoice* invObj) {
    if (!db || !invObj) return false;
    std::string sql = "SELECT encrypted_aes_key, encrypted_data FROM Invoices WHERE id = ? LIMIT 1;";
    sqlite3_stmt* stmt;
    bool found = false;
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, invObj->id);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            invObj->encrypted_aes_key = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
            invObj->encrypted_data = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            found = true;
        }
    }
    sqlite3_finalize(stmt);
    return found;
}
bool DatabaseManager::StoreInvoice(const std::string* jsonString, const std::vector<BYTE>* publicKey) {
    if (!jsonString || jsonString->empty()) {
        std::cerr << "[DB_ERROR] Cannot store an empty or null JSON invoice.\n";
        return false;
    }
    auto securePayload = CryptoWrapper::EncryptHybrid(*jsonString, *publicKey);
    if (securePayload.first.empty() || securePayload.second.empty()) {
        std::cerr << "[CRYPTO_ERROR] Hybrid encryption failed for the invoice.\n";
        return false;
    }
    Invoice newInvoice;
    newInvoice.encrypted_data = securePayload.first;
    newInvoice.encrypted_aes_key = securePayload.second;
    return Append(newInvoice);
}
bool DatabaseManager::RetrieveInvoice(int id, const std::vector<BYTE>* privateKey, DecryptedInvoice* outInvoice) {
    if (!privateKey || privateKey->empty() || !outInvoice) {
        std::cerr << "[ERROR] Invalid pointer passed to RetrieveInvoice.\n";
        return false;
    }
    Invoice encryptedInv;
    encryptedInv.id = id;
    if (!Get(&encryptedInv)) {
        std::cerr << "[DB_ERROR] Invoice with ID " << id << " not found in database.\n";
        return false;
    }
    std::string jsonString = CryptoWrapper::DecryptHybrid(
        encryptedInv.encrypted_data, 
        encryptedInv.encrypted_aes_key, 
        *privateKey
    );
    if (jsonString.empty()) {
        std::cerr << "[CRYPTO_ERROR] Failed to decrypt invoice. Wrong Private Key or corrupted data.\n";
        return false;
    }
    try {
        auto j = nlohmann::json::parse(jsonString);
        outInvoice->db_id = id;
        outInvoice->invoice_number = j.value("invoice_number", "UNKNOWN");
        outInvoice->timestamp      = j.value("timestamp", "1970-01-01 00:00:00");
        outInvoice->cashier_name   = j.value("cashier_name", "System");
        outInvoice->branch_name    = j.value("branch_name", "Main Branch");
        outInvoice->items.clear();
        if (j.contains("items") && j["items"].is_array()) {
            for (const auto& item_json : j["items"]) {
                InvoiceItem item;
                item.product_id    = item_json.value("product_id", 0);
                item.name          = item_json.value("name", "Unknown Product");
                item.serial_number = item_json.value("serial_number", "");
                item.quantity      = item_json.value("quantity", 1);
                item.unit_price    = item_json.value("unit_price", 0.0f);
                item.discount      = item_json.value("discount", 0.0f);
                item.total_price   = item_json.value("total_price", 0.0f);
                outInvoice->items.push_back(item);
            }
        }
        outInvoice->subtotal       = j.value("subtotal", 0.0f);
        outInvoice->total_tax      = j.value("total_tax", 0.0f);
        outInvoice->total_discount = j.value("total_discount", 0.0f);
        outInvoice->grand_total    = j.value("grand_total", 0.0f);
        outInvoice->payment_method = j.value("payment_method", "Cash");
        outInvoice->paid_amount    = j.value("paid_amount", 0.0f);
        outInvoice->change_amount  = j.value("change_amount", 0.0f);
        outInvoice->status         = j.value("status", "Paid");
        return true;
    } catch (const nlohmann::json::exception& e) {
        std::cerr << "[JSON_ERROR] Failed to parse decrypted JSON: " << e.what() << "\n";
        return false;
    }
}
std::string GenerateProductDataString(const Product* prod) {
    std::ostringstream oss;
    oss << prod->serial_number << "|" 
        << prod->name << "|" 
        << prod->description << "|" 
        << std::fixed << std::setprecision(2) << prod->cost;
    return oss.str();
}
std::string DatabaseManager::SignProduct(const Product* prod, const std::vector<BYTE>* privateKey) {
    if (!prod || !privateKey || privateKey->empty()) {
        std::cerr << "[ERROR] Invalid product or private key for signing.\n";
        return "";
    }
    std::string dataToSign = GenerateProductDataString(prod);
    CryptoWrapper::RSA rsa;
    if (!rsa.ImportPrivateKey(*privateKey)) {
        std::cerr << "[CRYPTO_ERROR] Failed to import Private Key for signing.\n";
        return "";
    }
    std::vector<BYTE> signatureBytes = rsa.Sign(dataToSign);
    return CryptoWrapper::BytesToHexString(signatureBytes);
}
bool DatabaseManager::VerifyProductSignature(const Product* prod, const std::vector<BYTE>* publicKey) {
    if (!prod || !publicKey || publicKey->empty() || prod->admin_signature.empty()) {
        std::cerr << "[ERROR] Missing product data, public key, or signature.\n";
        return false;
    }
    std::string dataToVerify = GenerateProductDataString(prod);
    CryptoWrapper::RSA rsa;
    if (!rsa.ImportPublicKey(*publicKey)) {
        std::cerr << "[CRYPTO_ERROR] Failed to import Public Key for verification.\n";
        return false;
    }
    std::vector<BYTE> signatureBytes = CryptoWrapper::HexStringToBytes(prod->admin_signature);
    return rsa.Verify(dataToVerify, signatureBytes);
}
std::string GenerateUserDataString(const User* user) {
    std::ostringstream oss;
    oss << user->username << "|"
        << user->password_hash << "|"
        << user->permissions;
    return oss.str();
}
std::string DatabaseManager::SignUser(const User* user, const std::vector<BYTE>* privateKey) {
    if (!user || !privateKey || privateKey->empty()) {
        std::cerr << "[ERROR] Invalid user or private key for signing.\n";
        return "";
    }
    std::string dataToSign = GenerateUserDataString(user);
    CryptoWrapper::RSA rsa;
    if (!rsa.ImportPrivateKey(*privateKey)) {
        std::cerr << "[CRYPTO_ERROR] Failed to import Private Key for signing user.\n";
        return "";
    }
    std::vector<BYTE> signatureBytes = rsa.Sign(dataToSign);
    return CryptoWrapper::BytesToHexString(signatureBytes);
}
bool DatabaseManager::VerifyUserSignature(const User* user, const std::vector<BYTE>* publicKey) {
    if (!user || !publicKey || publicKey->empty() || user->admin_signature.empty()) {
        std::cerr << "[ERROR] Missing user data, public key, or signature.\n";
        return false;
    }
    std::string dataToVerify = GenerateUserDataString(user);
    CryptoWrapper::RSA rsa;
    if (!rsa.ImportPublicKey(*publicKey)) {
        std::cerr << "[CRYPTO_ERROR] Failed to import Public Key for verifying user.\n";
        return false;
    }
    std::vector<BYTE> signatureBytes = CryptoWrapper::HexStringToBytes(user->admin_signature);
    return rsa.Verify(dataToVerify, signatureBytes);
}
std::vector<Product> DatabaseManager::SearchProducts(const std::string& keyword) {
    std::vector<Product> results;
    if (!db) return results;
    std::string sql = "SELECT id, serial_number, name, description, cost, admin_signature "
                      "FROM Products "
                      "WHERE serial_number LIKE ? OR name LIKE ? OR description LIKE ?;";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
        std::string searchPattern = "%" + keyword + "%";
        sqlite3_bind_text(stmt, 1, searchPattern.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, searchPattern.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 3, searchPattern.c_str(), -1, SQLITE_TRANSIENT);
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            Product prod;
            prod.id = sqlite3_column_int(stmt, 0);
            prod.serial_number = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            prod.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
            const char* desc = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
            prod.description = desc ? desc : "";
            prod.cost = static_cast<float>(sqlite3_column_double(stmt, 4));
            prod.admin_signature = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
            results.push_back(prod);
        }
    }
    sqlite3_finalize(stmt);
    return results; 
}
std::vector<User> DatabaseManager::GetAllUsers() {
    std::vector<User> results;
    if (!db) return results;
    std::string sql = "SELECT username, password_hash, permissions, admin_signature "
                      "FROM Users ORDER BY permissions DESC, username ASC;";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            User u;
            u.username         = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
            u.password_hash    = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            u.permissions      = sqlite3_column_int(stmt, 2);
            u.admin_signature  = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
            results.push_back(u);
        }
    }
    sqlite3_finalize(stmt);
    return results;
}
std::vector<Invoice> DatabaseManager::GetAllInvoices() {
    std::vector<Invoice> results;
    if (!db) return results;
    std::string sql = "SELECT id, encrypted_aes_key, encrypted_data FROM Invoices ORDER BY id DESC;";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            Invoice inv;
            inv.id                 = sqlite3_column_int(stmt, 0);
            inv.encrypted_aes_key  = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            inv.encrypted_data     = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
            results.push_back(inv);
        }
    }
    sqlite3_finalize(stmt);
    return results;
}
bool DatabaseManager::UserExists(const std::string& username) {
    if (!db) return false;
    std::string sql = "SELECT 1 FROM Users WHERE username = ? LIMIT 1;";
    sqlite3_stmt* stmt;
    bool exists = false;
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);
        if (sqlite3_step(stmt) == SQLITE_ROW) exists = true;
    }
    sqlite3_finalize(stmt);
    return exists;
}