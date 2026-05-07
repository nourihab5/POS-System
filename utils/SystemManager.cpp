#include <httplib.h>
#include <filesystem>
#include "SystemManager.h"
#include "PdfGenerator.h"
#include <windows.h>
#include <iostream>
#include <chrono>
#include <ctime>
#include <iomanip> 
#include <sstream>
#include <json.hpp>
#include <map>
SystemManager::SystemManager(const std::string& dbPath) {
    db = new DatabaseManager(dbPath);
    currentRole = 0; 
	sessionPubKey = CryptoWrapper::LoadFromFile("assets/public_key.bin");
    PdfGenerator::Init();
}
SystemManager::~SystemManager() {
    PdfGenerator::Shutdown();
    delete db;
}
void SystemManager::StartHttpServer(int port) {
    std::thread([this, port]() {
        httplib::Server svr;
        svr.Get("/ScanClass", [this](const httplib::Request& req, httplib::Response& res) {
            if (req.has_param("text")) {
                std::string barcode = req.get_param_value("text");
                if (this->AddProductToCart(barcode, 1)) {
                    res.set_content("Success: Product Added", "text/plain");
                    std::cout << "\n[REMOTE_SCAN] Barcode received: " << barcode << std::endl;
                } else {
                    res.set_content("Error: Product Not Found", "text/plain");
                }
            } else {
                res.status = 400;
                res.set_content("Bad Request: Missing 'text' parameter", "text/plain");
            }
            res.set_header("Access-Control-Allow-Origin", "*");
        });
        std::cout << "[SERVER] HTTP Listener is active on port " << port << "..." << std::endl;
        svr.listen("0.0.0.0", port); 
    }).detach(); 
}
int SystemManager::Login(const std::string& username, const std::string& password) {
    Logout();
    if (!db || !db->IsOpen()) {
        std::cerr << "[SYSTEM] Database is not accessible.\n";
        return 0; 
    }
    User attemptUser;
    attemptUser.username = username;
    if (!db->Get(&attemptUser)) {
        std::cerr << "[LOGIN] User not found.\n";
        return 0; 
    }
    std::string hashedInput = CryptoWrapper::BytesToHexString(CryptoWrapper::HashSHA256(password));
    if (attemptUser.password_hash != hashedInput) { 
        std::cerr << "[LOGIN] Incorrect password.\n";
        return 0; 
    }
    if (!db->VerifyUserSignature(&attemptUser, &sessionPubKey)) {
        std::cerr << "[LOGIN] SECURITY ALERT: User data has been tampered with!\n";
        return 0; 
    }
    if (attemptUser.permissions == 0) {
        if (LoginAsAgent(attemptUser)) {
            return 1; 
        }
    } 
    else if (attemptUser.permissions == 1) {
        if (LoginAsAdmin(attemptUser, password)) {
            return 2; 
        }
    }
    return 0; 
}
bool SystemManager::LoginAsAgent(const User& user) {
    currentUser = user;
    currentRole = 1;
    sessionPrivKey.clear(); 
    std::cout << "[SYSTEM] Logged in successfully as AGENT: " << user.username << "\n";
    return true;
}
bool SystemManager::LoginAsAdmin(const User& user, const std::string& password) {
    std::string keyPath = "data/" + user.username + "_key.bin";
    std::vector<BYTE> encryptedPrivKey = CryptoWrapper::LoadFromFile(keyPath);
    if (encryptedPrivKey.empty()) {
        std::cerr << "[SYSTEM] Admin login failed: Cannot find or read encrypted Private Key at " << keyPath << ".\n";
        return false;
    }
    std::string saltStr = user.username + "_298LmAPcm";
    std::vector<BYTE> saltBytes(saltStr.begin(), saltStr.end());
    std::vector<BYTE> aesKey = CryptoWrapper::DeriveKeyPBKDF2(password, saltBytes);
    std::vector<BYTE> decryptedPrivKey = CryptoWrapper::DecryptAES(encryptedPrivKey, aesKey);
    if (decryptedPrivKey.empty()) {
        std::cerr << "[SYSTEM] Admin login failed: Wrong password for Private Key decryption. Key is corrupted!\n";
        return false;
    }
    currentUser = user;
    currentRole = 2;
    sessionPrivKey = decryptedPrivKey; 
    std::cout << "[SYSTEM] Logged in successfully as ADMIN: " << user.username << "\n";
    std::cout << "[SYSTEM] Private Key decrypted and loaded into secure memory.\n";
    return true;
}
void SystemManager::Logout() {
    currentUser = User();       
    currentRole = 0;            
    std::fill(sessionPrivKey.begin(), sessionPrivKey.end(), 0);
    sessionPrivKey.clear();
    activeCart = DecryptedInvoice(); 
    std::cout << "[SYSTEM] User logged out securely.\n";
}
void SystemManager::RecalculateCart() {
    activeCart.subtotal = 0.0f;
    for (const auto& item : activeCart.items) {
        activeCart.subtotal += item.total_price;
    }
    activeCart.total_tax = activeCart.subtotal * 0.14f; 
    activeCart.grand_total = activeCart.subtotal + activeCart.total_tax - activeCart.total_discount;
}
std::string GetCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&now_c), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}
void SystemManager::StartNewInvoice() {
    std::lock_guard<std::mutex> lock(cartMutex);
    _StartNewInvoice();
}
void SystemManager::_StartNewInvoice() {
    activeCart = DecryptedInvoice(); 
    auto epoch_time = std::chrono::system_clock::now().time_since_epoch().count();
    activeCart.invoice_number = "INV-" + std::to_string(epoch_time).substr(0, 10); 
    activeCart.cashier_name = currentUser.username.empty() ? "Unknown Agent" : currentUser.username;
    activeCart.branch_name = "Main Branch"; 
    activeCart.subtotal = 0.0f;
    activeCart.total_tax = 0.0f;
    activeCart.total_discount = 0.0f;
    activeCart.grand_total = 0.0f;
    std::cout << "[CART] Started new invoice: " << activeCart.invoice_number << "\n";
}
bool SystemManager::AddProductToCart(const std::string& serialnumOrName, int quantity) {
    std::lock_guard<std::mutex> lock(cartMutex);
    if (currentRole == 0) {
        std::cerr << "[CART] Error: No active user logged in.\n";
        return false;
    }
    std::vector<Product> results = db->SearchProducts(serialnumOrName);
    if (results.empty()) {
        std::cerr << "[CART] Product not found.\n";
        return false;
    }
    Product selectedProduct = results[0];
    if (!db->VerifyProductSignature(&selectedProduct, &sessionPubKey)) {
        std::cerr << "[SECURITY ALERT] Product signature invalid! Tampering detected for product: " << selectedProduct.name << "\n";
        return false; 
    }
    bool foundInCart = false;
    for (auto& item : activeCart.items) {
        if (item.product_id == selectedProduct.id) {
            item.quantity += quantity;
            item.total_price = item.quantity * item.unit_price; 
            foundInCart = true;
            break;
        }
    }
    if (!foundInCart) {
        InvoiceItem newItem;
        newItem.product_id = selectedProduct.id;
        newItem.name = selectedProduct.name;
        newItem.serial_number = selectedProduct.serial_number;
        newItem.quantity = quantity;
        newItem.unit_price = selectedProduct.cost;
        newItem.discount = 0.0f;
        newItem.total_price = selectedProduct.cost * quantity;
        activeCart.items.push_back(newItem);
    }
    RecalculateCart();
    Beep(1200, 400);
    std::cout << "[CART] Added " << quantity << "x " << selectedProduct.name << " to cart.\n";
    return true;
}
bool SystemManager::RemoveProductFromCart(int index) {
    std::lock_guard<std::mutex> lock(cartMutex);
    if (index < 0 || index >= activeCart.items.size() || currentRole == 0) {
        std::cerr << "[CART] Invalid item index or no active user logged in..\n";
        return false;
    }
    std::cout << "[CART] Removed " << activeCart.items[index].name << " from cart.\n";
    activeCart.items.erase(activeCart.items.begin() + index);
    RecalculateCart();
    return true;
}
bool SystemManager::Checkout(const std::string& paymentMethod, const std::string& invoiceStatus) {
    std::lock_guard<std::mutex> lock(cartMutex);
    if (activeCart.items.empty()) {
        std::cerr << "[CHECKOUT] Cart is empty, cannot checkout.\n";
        return false;
    }
    if (currentRole == 0) {
        std::cerr << "[CART] Error: No active user logged in.\n";
        return false;
    }
    activeCart.payment_method = paymentMethod;
    activeCart.timestamp = GetCurrentTimestamp();
    activeCart.status = invoiceStatus;
    activeCart.paid_amount = activeCart.grand_total; 
    activeCart.change_amount = 0.0f;                 
    nlohmann::json j;
    j["invoice_number"] = activeCart.invoice_number;
    j["timestamp"]      = activeCart.timestamp;
    j["cashier_name"]   = activeCart.cashier_name;
    j["branch_name"]    = activeCart.branch_name;
    j["subtotal"]       = activeCart.subtotal;
    j["total_tax"]      = activeCart.total_tax;
    j["total_discount"] = activeCart.total_discount;
    j["grand_total"]    = activeCart.grand_total;
    j["payment_method"] = activeCart.payment_method;
    j["paid_amount"]    = activeCart.paid_amount;
    j["change_amount"]  = activeCart.change_amount;
    j["status"]         = activeCart.status;
    nlohmann::json itemsJson = nlohmann::json::array();
    for (const auto& item : activeCart.items) {
        nlohmann::json itemJ;
        itemJ["product_id"]    = item.product_id;
        itemJ["name"]          = item.name;
        itemJ["serial_number"] = item.serial_number;
        itemJ["quantity"]      = item.quantity;
        itemJ["unit_price"]    = item.unit_price;
        itemJ["discount"]      = item.discount;
        itemJ["total_price"]   = item.total_price;
        itemsJson.push_back(itemJ);
    }
    j["items"] = itemsJson;
    std::string jsonString = j.dump();
    if (db->StoreInvoice(&jsonString, &sessionPubKey)) {
        std::cout << "[CHECKOUT] Successfully checked out invoice: " << activeCart.invoice_number << "\n";
        std::map<std::string, std::string> pdfData;
        pdfData["{{INVOICE_NUMBER}}"] = activeCart.invoice_number;
        pdfData["{{TIMESTAMP}}"]      = activeCart.timestamp;
        pdfData["{{CASHIER_NAME}}"]   = activeCart.cashier_name;
        pdfData["{{BRANCH_NAME}}"]    = activeCart.branch_name;
        pdfData["{{PAYMENT_METHOD}}"] = activeCart.payment_method;
        auto formatPrice = [](float price) {
            std::stringstream ss;
            ss << std::fixed << std::setprecision(2) << price;
            return ss.str();
        };
        pdfData["{{SUBTOTAL}}"]       = formatPrice(activeCart.subtotal);
        pdfData["{{TOTAL_TAX}}"]      = formatPrice(activeCart.total_tax);
        pdfData["{{TOTAL_DISCOUNT}}"] = formatPrice(activeCart.total_discount);
        pdfData["{{GRAND_TOTAL}}"]    = formatPrice(activeCart.grand_total);
        pdfData["{{PAID_AMOUNT}}"]    = formatPrice(activeCart.paid_amount);
        pdfData["{{CHANGE_AMOUNT}}"]  = formatPrice(activeCart.change_amount);
        std::stringstream itemsHtml;
        for (const auto& item : activeCart.items) {
            itemsHtml << "<tr>"
                      << "<td><strong>" << item.name << "</strong></td>"
                      << "<td>" << item.serial_number << "</td>"
                      << "<td class='right'>" << item.quantity << "</td>"
                      << "<td class='right'>" << formatPrice(item.unit_price) << "</td>"
                      << "<td class='right'>" << formatPrice(item.total_price) << "</td>"
                      << "</tr>";
        }
        pdfData["{{ITEMS_HTML}}"] = itemsHtml.str();
        std::string templatePath = "assets/invoice_template.html"; 
        std::string outputPath = "invoices/" + activeCart.invoice_number + ".pdf";
        if (PdfGenerator::GenerateFromFile(templatePath, outputPath, pdfData)) {
            std::cout << "[PDF_SUCCESS] Invoice generated securely at: " << outputPath << "\n";
        } else {
            std::cerr << "[PDF_ERROR] Failed to generate invoice PDF.\n";
        }
        _StartNewInvoice(); 
        return true;
    } else {
        std::cerr << "[CHECKOUT_ERROR] Failed to store encrypted invoice in DB.\n";
        return false;
    }
}
std::vector<User> SystemManager::AdminGetAllUsers() {
    if (currentRole != 2) {
        std::cerr << "[ADMIN] Access denied: Admin role required.\n";
        return {};
    }
    if (!db || !db->IsOpen()) return {};
    return db->GetAllUsers();
}
bool SystemManager::AdminCreateUser(const std::string& username,
                                    const std::string& password,
                                    int permissions,
                                    std::string& outError) {
    outError.clear();
    if (currentRole != 2) {
        outError = "Access denied: Admin role required.";
        return false;
    }
    if (sessionPrivKey.empty()) {
        outError = "Admin private key not loaded in session.";
        return false;
    }
    if (username.empty() || password.empty()) {
        outError = "Username and password are required.";
        return false;
    }
    if (username.length() < 3 || username.length() > 32) {
        outError = "Username must be between 3 and 32 characters.";
        return false;
    }
    if (password.length() < 6) {
        outError = "Password must be at least 6 characters.";
        return false;
    }
    if (permissions != 0 && permissions != 1) {
        outError = "Permissions must be 0 (Cashier) or 1 (Admin).";
        return false;
    }
    if (db->UserExists(username)) {
        outError = "Username already exists.";
        return false;
    }
    User newUser;
    newUser.username      = username;
    newUser.password_hash = CryptoWrapper::BytesToHexString(
                                CryptoWrapper::HashSHA256(password));
    newUser.permissions   = permissions;
    newUser.admin_signature = db->SignUser(&newUser, &sessionPrivKey);
    if (newUser.admin_signature.empty()) {
        outError = "Failed to sign user data.";
        return false;
    }
    if (permissions == 1) {
        std::string saltStr = username + "_298LmAPcm";
        std::vector<BYTE> saltBytes(saltStr.begin(), saltStr.end());
        std::vector<BYTE> aesKey = CryptoWrapper::DeriveKeyPBKDF2(password, saltBytes);
        std::string privStr(sessionPrivKey.begin(), sessionPrivKey.end());
        std::vector<BYTE> encryptedPriv = CryptoWrapper::EncryptAES(privStr, aesKey);
        std::fill(aesKey.begin(), aesKey.end(), 0);
        SecureZeroMemory(&privStr[0], privStr.size());
        if (encryptedPriv.empty()) {
            outError = "Failed to encrypt private key for new admin.";
            return false;
        }
        std::filesystem::create_directories("data");
        std::string keyPath = "data/" + username + "_key.bin";
        if (!CryptoWrapper::SaveToFile(keyPath, encryptedPriv)) {
            outError = "Failed to save encrypted private key file.";
            return false;
        }
        std::cout << "[ADMIN] System private key re-encrypted with new admin's "
                  << "password and saved at: " << keyPath << "\n";
    }
    if (!db->Append(newUser)) {
        outError = "Database error: failed to insert user.";
        if (permissions == 1) {
            std::error_code ec;
            std::filesystem::remove("data/" + username + "_key.bin", ec);
        }
        return false;
    }
    std::cout << "[ADMIN] User created successfully: " << username
              << " (role=" << (permissions == 1 ? "Admin" : "Cashier") << ")\n";
    return true;
}
bool SystemManager::AdminResetUserPassword(const std::string& username,
                                           const std::string& newPassword,
                                           std::string& outError) {
    outError.clear();
    if (currentRole != 2) {
        outError = "Access denied: Admin role required.";
        return false;
    }
    if (sessionPrivKey.empty()) {
        outError = "Admin private key not loaded in session.";
        return false;
    }
    if (newPassword.length() < 6) {
        outError = "Password must be at least 6 characters.";
        return false;
    }
    User oldUser;
    oldUser.username = username;
    if (!db->Get(&oldUser)) {
        outError = "User not found.";
        return false;
    }
    if (username == currentUser.username) {
        outError = "You cannot reset your own password from here. "
                   "Ask another admin to do it.";
        return false;
    }
    User newUser = oldUser;
    newUser.password_hash = CryptoWrapper::BytesToHexString(
                                CryptoWrapper::HashSHA256(newPassword));
    newUser.admin_signature = db->SignUser(&newUser, &sessionPrivKey);
    if (newUser.admin_signature.empty()) {
        outError = "Failed to sign updated user data.";
        return false;
    }
    if (oldUser.permissions == 1) {
        std::string saltStr = username + "_298LmAPcm";
        std::vector<BYTE> saltBytes(saltStr.begin(), saltStr.end());
        std::vector<BYTE> aesKey = CryptoWrapper::DeriveKeyPBKDF2(newPassword, saltBytes);
        std::string privStr(sessionPrivKey.begin(), sessionPrivKey.end());
        std::vector<BYTE> encryptedPriv = CryptoWrapper::EncryptAES(privStr, aesKey);
        std::fill(aesKey.begin(), aesKey.end(), 0);
        SecureZeroMemory(&privStr[0], privStr.size());
        if (encryptedPriv.empty()) {
            outError = "Failed to re-encrypt private key with new password.";
            return false;
        }
        std::filesystem::create_directories("data");
        std::string keyPath = "data/" + username + "_key.bin";
        std::string backupPath = keyPath + ".bak";
        std::error_code ec;
        std::filesystem::copy_file(keyPath, backupPath,
                                   std::filesystem::copy_options::overwrite_existing, ec);
        if (!CryptoWrapper::SaveToFile(keyPath, encryptedPriv)) {
            outError = "Failed to write re-encrypted private key.";
            return false;
        }
        std::cout << "[ADMIN] Re-encrypted system private key for: " << username
                  << " (old key file backed up as .bak)\n";
    }
    if (!db->Update(oldUser, newUser)) {
        outError = "Database error: failed to update user.";
        return false;
    }
    std::cout << "[ADMIN] Password reset successfully for: " << username << "\n";
    return true;
}
bool SystemManager::AdminDeleteUser(const std::string& username, std::string& outError) {
    outError.clear();
    if (currentRole != 2) {
        outError = "Access denied: Admin role required.";
        return false;
    }
    if (username == currentUser.username) {
        outError = "You cannot delete your own account.";
        return false;
    }
    User u;
    u.username = username;
    if (!db->Get(&u)) {
        outError = "User not found.";
        return false;
    }
    if (!db->Delete(u)) {
        outError = "Database error: failed to delete user.";
        return false;
    }
    if (u.permissions == 1) {
        std::error_code ec;
        std::filesystem::remove("data/" + username + "_key.bin", ec);
    }
    std::cout << "[ADMIN] User deleted: " << username << "\n";
    return true;
}
std::vector<Invoice> SystemManager::AdminGetAllInvoices() {
    if (currentRole != 2) {
        std::cerr << "[ADMIN] Access denied: Admin role required.\n";
        return {};
    }
    if (!db || !db->IsOpen()) return {};
    return db->GetAllInvoices();
}
bool SystemManager::AdminDecryptInvoice(int invoiceId, DecryptedInvoice* out) {
    if (currentRole != 2) {
        std::cerr << "[ADMIN] Access denied: Admin role required.\n";
        return false;
    }
    if (!out) return false;
    if (sessionPrivKey.empty()) {
        std::cerr << "[ADMIN] Private key not available in session.\n";
        return false;
    }
    return db->RetrieveInvoice(invoiceId, &sessionPrivKey, out);
}
std::vector<Product> SystemManager::AdminGetAllProducts() {
    if (currentRole != 2) return {};
    if (!db || !db->IsOpen()) return {};
    return db->SearchProducts("");
}
bool SystemManager::AdminCreateProduct(const std::string& serial,
                                       const std::string& name,
                                       const std::string& desc,
                                       float cost,
                                       std::string& outError) {
    outError.clear();
    if (currentRole != 2) { outError = "Access denied."; return false; }
    if (sessionPrivKey.empty()) { outError = "Private key not loaded."; return false; }
    if (serial.empty() || name.empty()) { outError = "Serial and name are required."; return false; }
    if (cost < 0.0f) { outError = "Cost cannot be negative."; return false; }
    Product p;
    p.serial_number = serial;
    p.name          = name;
    p.description   = desc;
    p.cost          = cost;
    p.admin_signature = db->SignProduct(&p, &sessionPrivKey);
    if (p.admin_signature.empty()) { outError = "Failed to sign product."; return false; }
    if (!db->Append(p)) { outError = "Database error: failed to insert product."; return false; }
    std::cout << "[ADMIN] Product created: " << name << "\n";
    return true;
}
bool SystemManager::AdminUpdateProduct(const Product& oldProd,
                                       const std::string& newSerial,
                                       const std::string& newName,
                                       const std::string& newDesc,
                                       float newCost,
                                       std::string& outError) {
    outError.clear();
    if (currentRole != 2) { outError = "Access denied."; return false; }
    if (sessionPrivKey.empty()) { outError = "Private key not loaded."; return false; }
    if (newSerial.empty() || newName.empty()) { outError = "Serial and name are required."; return false; }
    if (newCost < 0.0f) { outError = "Cost cannot be negative."; return false; }
    Product newProd;
    newProd.id            = oldProd.id;
    newProd.serial_number = newSerial;
    newProd.name          = newName;
    newProd.description   = newDesc;
    newProd.cost          = newCost;
    newProd.admin_signature = db->SignProduct(&newProd, &sessionPrivKey);
    if (newProd.admin_signature.empty()) { outError = "Failed to sign updated product."; return false; }
    if (!db->Update(oldProd, newProd)) { outError = "Database error: failed to update product."; return false; }
    std::cout << "[ADMIN] Product updated: " << newName << "\n";
    return true;
}
bool SystemManager::AdminDeleteProduct(int productId, std::string& outError) {
    outError.clear();
    if (currentRole != 2) { outError = "Access denied."; return false; }
    Product p;
    p.id = productId;
    if (!db->Delete(p)) { outError = "Database error: failed to delete product."; return false; }
    std::cout << "[ADMIN] Product deleted. ID: " << productId << "\n";
    return true;
}