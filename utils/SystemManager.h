#pragma once
#include <string>
#include <vector>
#include "DatabaseManager.h"
#include "CryptoWrapper.h"
#include <mutex>

class SystemManager {
private:
    std::mutex cartMutex;
    DatabaseManager* db;
    int currentRole;                 
    User currentUser;
	std::vector<BYTE> sessionPubKey;
    std::vector<BYTE> sessionPrivKey;  
    DecryptedInvoice activeCart;           
    bool LoginAsAgent(const User& user); 
    bool LoginAsAdmin(const User& user, const std::string& password);
    void RecalculateCart();
    void _StartNewInvoice();

public:
    SystemManager(const std::string& dbPath);
    ~SystemManager();
    void StartHttpServer(int port);
    int Login(const std::string& username, const std::string& password);
    
    void Logout();
    int GetCurrentRole() const { return currentRole;}
    const DecryptedInvoice& GetActiveCart() const { return activeCart; }
    User GetCurrentUser() const { return currentUser; }

    
    void StartNewInvoice();
    bool AddProductToCart(const std::string& serialnumOrName, int quantity = 1); 
    bool RemoveProductFromCart(int index);
    bool Checkout(const std::string& paymentMethod, const std::string& invoiceStatus = "Paid");
    
    std::vector<User> AdminGetAllUsers();
    bool AdminCreateUser(const std::string& username,
                         const std::string& password,
                         int  permissions,            
                         std::string& outError);
    bool AdminResetUserPassword(const std::string& username,
                                const std::string& newPassword,
                                std::string& outError);
    bool AdminDeleteUser(const std::string& username, std::string& outError);

    
    std::vector<Invoice> AdminGetAllInvoices();
    bool AdminDecryptInvoice(int invoiceId, DecryptedInvoice* out);
    std::vector<Product> AdminGetAllProducts();
    bool AdminCreateProduct(const std::string& serial, const std::string& name,
                        const std::string& desc, float cost,
                        std::string& outError);
    bool AdminUpdateProduct(const Product& oldProd, const std::string& newSerial,
                        const std::string& newName, const std::string& newDesc,
                        float newCost, std::string& outError);
    bool AdminDeleteProduct(int productId, std::string& outError);
};