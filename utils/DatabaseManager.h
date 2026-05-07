#pragma once
#include <windows.h>
#include <vector>    
#include <string>
#include <sqlite3.h>
#include <iostream>
struct User {
    std::string username;
    std::string password_hash;     
    int permissions;               
    std::string admin_signature;   
};
struct Product {
    int id;                              
    std::string serial_number;           
    std::string name;
    std::string description;
    float cost;
    std::string admin_signature;         
};
struct Invoice {
    int id;                              
    std::string encrypted_aes_key;       
    std::string encrypted_data;          
};
struct InvoiceItem {
    int product_id;
    std::string name;
    std::string serial_number;
    int quantity;
    float unit_price;       
    float discount;         
    float total_price;      
};
struct DecryptedInvoice {
    int db_id;                  
    std::string invoice_number; 
    std::string timestamp;      
    std::string cashier_name;   
    std::string branch_name;    
    std::vector<InvoiceItem> items; 
    float subtotal;             
    float total_tax;            
    float total_discount;       
    float grand_total;          
    std::string payment_method; 
    float paid_amount;          
    float change_amount;        
    std::string status;         
};
enum class DbTemplate {
    USERS_DB,
    PRODUCTS_DB,
    INVOICES_DB
};
class DatabaseManager {
private:
    sqlite3* db; 
    bool ExecuteSQL(const std::string& sql);
public:
    DatabaseManager(const std::string& filepath);
    ~DatabaseManager();
    bool IsOpen() const;
    bool CreateTables(DbTemplate dbTemplate);
    bool Append(const User& obj);
    bool Get(User* userObj);            
    bool Delete(const User& searchObj);
    bool Update(const User& oldObj, const User& newObj);
    bool Append(const Product& obj);
    bool Get(Product* prodObj);          
	std::vector<Product> SearchProducts(const std::string& keyword);
    bool Delete(const Product& searchObj);
    bool Update(const Product& oldObj, const Product& newObj);
    bool Append(const Invoice& obj);
    bool Get(Invoice* invObj);           
    bool StoreInvoice(const std::string* jsonString, const std::vector<BYTE>* publicKey);
	bool RetrieveInvoice(int id, const std::vector<BYTE>* privateKey, DecryptedInvoice* outInvoice);
	std::string SignProduct(const Product* prod, const std::vector<BYTE>* privateKey);
    bool VerifyProductSignature(const Product* prod, const std::vector<BYTE>* publicKey);
	std::string SignUser(const User* user, const std::vector<BYTE>* privateKey);
    bool VerifyUserSignature(const User* user, const std::vector<BYTE>* publicKey);
    std::vector<User>    GetAllUsers();
    std::vector<Invoice> GetAllInvoices();
    bool                 UserExists(const std::string& username);
};