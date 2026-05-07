#include "AdminDashboard.h"
#include "imgui/imgui.h"
#include <vector>
#include <string>
#include <cstring>
#include <ctime>
#include <chrono>
namespace AdminDashboard {
namespace {
    bool g_dashboardOpen = false;
    bool g_usersWindowOpen    = false;
    bool g_invoicesWindowOpen = false;
    std::vector<User> g_usersCache;
    bool  g_usersCacheLoaded = false;
    char g_newUsername[64]   = "";
    char g_newPassword[64]   = "";
    int  g_newPermissions    = 0; 
    std::string g_createMsg  = "";
    ImVec4      g_createMsgColor = ImVec4(0,1,0,1);
    bool g_resetModalOpen = false;
    char g_resetTargetUser[64] = "";
    char g_resetNewPassword[64] = "";
    std::string g_resetMsg = "";
    ImVec4      g_resetMsgColor = ImVec4(0,1,0,1);
    std::vector<Invoice> g_invoicesCache;
    bool g_invoicesCacheLoaded = false;
    int  g_selectedInvoiceId   = -1;
    DecryptedInvoice g_decryptedInvoice;
    bool g_invoiceDecrypted    = false;
    std::string g_invoiceMsg   = "";
    ImVec4      g_invoiceMsgColor = ImVec4(1,0,0,1);
    bool g_productsWindowOpen   = false;
    std::vector<Product> g_productsCache;
    bool g_productsCacheLoaded  = false;
    char g_newProdSerial[64]   = "";
    char g_newProdName[128]    = "";
    char g_newProdDesc[256]    = "";
    float g_newProdCost        = 0.0f;
    std::string g_prodCreateMsg = "";
    ImVec4 g_prodCreateMsgColor = ImVec4(0,1,0,1);
    bool  g_editProdModalOpen   = false;
    Product g_editTargetProd;
    char  g_editProdSerial[64]  = "";
    char  g_editProdName[128]   = "";
    char  g_editProdDesc[256]   = "";
    float g_editProdCost        = 0.0f;
    std::string g_editProdMsg   = "";
    ImVec4 g_editProdMsgColor   = ImVec4(0,1,0,1);
    void ReloadProducts(SystemManager& mgr) {
       g_productsCache = mgr.AdminGetAllProducts();
       g_productsCacheLoaded = true;
    }
    void ReloadUsers(SystemManager& mgr) {
        g_usersCache = mgr.AdminGetAllUsers();
        g_usersCacheLoaded = true;
    }
    void ReloadInvoices(SystemManager& mgr) {
        g_invoicesCache = mgr.AdminGetAllInvoices();
        g_invoicesCacheLoaded = true;
    }
    void ResetCreateForm() {
        memset(g_newUsername, 0, sizeof(g_newUsername));
        memset(g_newPassword, 0, sizeof(g_newPassword));
        g_newPermissions = 0;
    }
    const char* RoleLabel(int permissions) {
        return permissions == 1 ? "Admin" : "Cashier";
    }
}
void Open()    { g_dashboardOpen = true; }
void Close() {
    g_dashboardOpen = false;
    g_usersWindowOpen = false;
    g_invoicesWindowOpen = false;
    g_usersCacheLoaded = false;
    g_invoicesCacheLoaded = false;
    g_invoiceDecrypted = false;
    g_selectedInvoiceId = -1;
}
bool IsOpen()  { return g_dashboardOpen; }
static void DrawUsersWindow(SystemManager& manager, float s) {
    if (!g_usersWindowOpen) return;
    ImGui::SetNextWindowSize(ImVec2(900 * s, 600 * s), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("User Management", &g_usersWindowOpen)) {
        ImGui::End();
        return;
    }
    if (!g_usersCacheLoaded) ReloadUsers(manager);
    if (ImGui::Button("Refresh", ImVec2(120 * s, 0))) {
        ReloadUsers(manager);
    }
    ImGui::SameLine();
    ImGui::Text("Total users: %zu", g_usersCache.size());
    ImGui::Separator();
    ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.0f, 1.0f), "Create New User");
    ImGui::SetNextItemWidth(220 * s);
    ImGui::InputText("Username##new", g_newUsername, sizeof(g_newUsername));
    ImGui::SameLine();
    ImGui::SetNextItemWidth(220 * s);
    ImGui::InputText("Password##new", g_newPassword, sizeof(g_newPassword),
                     ImGuiInputTextFlags_Password);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(140 * s);
    const char* roles[] = { "Cashier (0)", "Admin (1)" };
    ImGui::Combo("Role##new", &g_newPermissions, roles, IM_ARRAYSIZE(roles));
    ImGui::SameLine();
    if (ImGui::Button("Create", ImVec2(100 * s, 0))) {
        std::string err;
        bool ok = manager.AdminCreateUser(g_newUsername, g_newPassword,
                                          g_newPermissions, err);
        if (ok) {
            g_createMsg = "User created successfully.";
            g_createMsgColor = ImVec4(0, 1, 0, 1);
            ResetCreateForm();
            ReloadUsers(manager);
        } else {
            g_createMsg = "Error: " + err;
            g_createMsgColor = ImVec4(1, 0.3f, 0.3f, 1);
        }
    }
    if (!g_createMsg.empty()) {
        ImGui::TextColored(g_createMsgColor, "%s", g_createMsg.c_str());
    }
    ImGui::Separator();
    ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.0f, 1.0f), "All Users");
    if (ImGui::BeginTable("UsersTable", 4,
            ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
            ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY,
            ImVec2(0, 300 * s))) {
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableSetupColumn("Username");
        ImGui::TableSetupColumn("Role");
        ImGui::TableSetupColumn("Password Hash (truncated)");
        ImGui::TableSetupColumn("Actions");
        ImGui::TableHeadersRow();
        bool shouldOpenResetModal = false;
        for (const auto& u : g_usersCache) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%s", u.username.c_str());
            ImGui::TableSetColumnIndex(1);
            if (u.permissions == 1)
                ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.2f, 1.0f), "Admin");
            else
                ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Cashier");
            ImGui::TableSetColumnIndex(2);
            std::string truncated = u.password_hash.substr(0, 16) + "...";
            ImGui::TextDisabled("%s", truncated.c_str());
            ImGui::TableSetColumnIndex(3);
            ImGui::PushID(u.username.c_str());
            if (ImGui::SmallButton("Reset Password")) {
                strncpy(g_resetTargetUser, u.username.c_str(),
                        sizeof(g_resetTargetUser) - 1);
                g_resetTargetUser[sizeof(g_resetTargetUser) - 1] = '\0';
                memset(g_resetNewPassword, 0, sizeof(g_resetNewPassword));
                g_resetMsg.clear();
                g_resetModalOpen = true;
                shouldOpenResetModal = true;
            }
            ImGui::SameLine();
            if (u.username != manager.GetCurrentUser().username) {
                if (ImGui::SmallButton("Delete")) {
                    ImGui::OpenPopup("Confirm Delete");
                }
            }
            if (ImGui::BeginPopupModal("Confirm Delete", NULL,
                                       ImGuiWindowFlags_AlwaysAutoResize)) {
                ImGui::Text("Delete user \"%s\"?", u.username.c_str());
                ImGui::Text("This action cannot be undone.");
                if (u.permissions == 1) {
                    ImGui::TextColored(ImVec4(1, 0.5f, 0, 1),
                        "Warning: encrypted private key file will be removed.");
                }
                ImGui::Separator();
                if (ImGui::Button("Yes, Delete", ImVec2(120 * s, 0))) {
                    std::string err;
                    if (manager.AdminDeleteUser(u.username, err)) {
                        ReloadUsers(manager);
                    }
                    ImGui::CloseCurrentPopup();
                }
                ImGui::SameLine();
                if (ImGui::Button("Cancel", ImVec2(120 * s, 0))) {
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }
            ImGui::PopID();
        }
        ImGui::EndTable();
        if (shouldOpenResetModal) ImGui::OpenPopup("Reset Password");
    }
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    if (ImGui::BeginPopupModal("Reset Password", &g_resetModalOpen,
                               ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Reset password for user: ");
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(1, 1, 0, 1), "%s", g_resetTargetUser);
        bool targetIsAdmin = false;
        for (const auto& u : g_usersCache) {
            if (u.username == g_resetTargetUser) {
                targetIsAdmin = (u.permissions == 1);
                break;
            }
        }
        if (targetIsAdmin) {
            ImGui::Spacing();
            ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f),
                "Note: Target is an Admin.");
            ImGui::TextWrapped(
                "The system's private key will be re-encrypted with the new "
                "password. Their access to all encrypted invoices is preserved.");
            ImGui::Spacing();
        }
        ImGui::SetNextItemWidth(300 * s);
        ImGui::InputText("New Password##reset", g_resetNewPassword,
                         sizeof(g_resetNewPassword), ImGuiInputTextFlags_Password);
        if (!g_resetMsg.empty()) {
            ImGui::TextColored(g_resetMsgColor, "%s", g_resetMsg.c_str());
        }
        ImGui::Separator();
        if (ImGui::Button("Confirm Reset", ImVec2(150 * s, 0))) {
            std::string err;
            bool ok = manager.AdminResetUserPassword(
                g_resetTargetUser, g_resetNewPassword, err);
            if (ok) {
                g_resetMsg = "Password reset successfully.";
                g_resetMsgColor = ImVec4(0, 1, 0, 1);
                memset(g_resetNewPassword, 0, sizeof(g_resetNewPassword));
                ReloadUsers(manager);
            } else {
                g_resetMsg = "Error: " + err;
                g_resetMsgColor = ImVec4(1, 0.3f, 0.3f, 1);
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Close", ImVec2(120 * s, 0))) {
            g_resetMsg.clear();
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
    ImGui::End();
}
static void DrawProductsWindow(SystemManager& manager, float s) {
    if (!g_productsWindowOpen) return;
    ImGui::SetNextWindowSize(ImVec2(1000 * s, 650 * s), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Product Management", &g_productsWindowOpen)) {
        ImGui::End(); return;
    }
    if (!g_productsCacheLoaded) ReloadProducts(manager);
    if (ImGui::Button("Refresh", ImVec2(120 * s, 0))) ReloadProducts(manager);
    ImGui::SameLine();
    ImGui::Text("Total products: %zu", g_productsCache.size());
    ImGui::Separator();
    ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.0f, 1.0f), "Add New Product");
    ImGui::SetNextItemWidth(160 * s);
    ImGui::InputText("Serial##newp",  g_newProdSerial, sizeof(g_newProdSerial));
    ImGui::SameLine();
    ImGui::SetNextItemWidth(200 * s);
    ImGui::InputText("Name##newp",    g_newProdName,   sizeof(g_newProdName));
    ImGui::SameLine();
    ImGui::SetNextItemWidth(240 * s);
    ImGui::InputText("Desc##newp",    g_newProdDesc,   sizeof(g_newProdDesc));
    ImGui::SameLine();
    ImGui::SetNextItemWidth(100 * s);
    ImGui::InputFloat("Cost##newp",   &g_newProdCost, 0.0f, 0.0f, "%.2f");
    ImGui::SameLine();
    if (ImGui::Button("Add Product", ImVec2(120 * s, 0))) {
        std::string err;
        if (manager.AdminCreateProduct(g_newProdSerial, g_newProdName,
                                        g_newProdDesc, g_newProdCost, err)) {
            g_prodCreateMsg = "Product added successfully.";
            g_prodCreateMsgColor = ImVec4(0,1,0,1);
            memset(g_newProdSerial, 0, sizeof(g_newProdSerial));
            memset(g_newProdName,   0, sizeof(g_newProdName));
            memset(g_newProdDesc,   0, sizeof(g_newProdDesc));
            g_newProdCost = 0.0f;
            ReloadProducts(manager);
        } else {
            g_prodCreateMsg = "Error: " + err;
            g_prodCreateMsgColor = ImVec4(1,0.3f,0.3f,1);
        }
    }
    if (!g_prodCreateMsg.empty())
        ImGui::TextColored(g_prodCreateMsgColor, "%s", g_prodCreateMsg.c_str());
    ImGui::Separator();
    bool shouldOpenEditModal = false;
    if (ImGui::BeginTable("ProductsTable", 6,
            ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
            ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY,
            ImVec2(0, 350 * s))) {
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableSetupColumn("ID",     ImGuiTableColumnFlags_WidthFixed, 50 * s);
        ImGui::TableSetupColumn("Serial");
        ImGui::TableSetupColumn("Name");
        ImGui::TableSetupColumn("Description");
        ImGui::TableSetupColumn("Cost",   ImGuiTableColumnFlags_WidthFixed, 90 * s);
        ImGui::TableSetupColumn("Actions",ImGuiTableColumnFlags_WidthFixed, 160 * s);
        ImGui::TableHeadersRow();
        for (const auto& p : g_productsCache) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0); ImGui::Text("%d", p.id);
            ImGui::TableSetColumnIndex(1); ImGui::Text("%s", p.serial_number.c_str());
            ImGui::TableSetColumnIndex(2); ImGui::Text("%s", p.name.c_str());
            ImGui::TableSetColumnIndex(3); ImGui::TextDisabled("%s", p.description.c_str());
            ImGui::TableSetColumnIndex(4); ImGui::Text("%.2f", p.cost);
            ImGui::TableSetColumnIndex(5);
            ImGui::PushID(p.id);
            if (ImGui::SmallButton("Edit")) {
                g_editTargetProd = p;
                strncpy(g_editProdSerial, p.serial_number.c_str(), sizeof(g_editProdSerial)-1);
                strncpy(g_editProdName,   p.name.c_str(),          sizeof(g_editProdName)-1);
                strncpy(g_editProdDesc,   p.description.c_str(),   sizeof(g_editProdDesc)-1);
                g_editProdCost = p.cost;
                g_editProdMsg.clear();
                g_editProdModalOpen = true;   
                shouldOpenEditModal = true;
            }
            ImGui::SameLine();
            if (ImGui::SmallButton("Delete")) ImGui::OpenPopup("Confirm Delete Product");
            if (ImGui::BeginPopupModal("Confirm Delete Product", NULL,
                                        ImGuiWindowFlags_AlwaysAutoResize)) {
                ImGui::Text("Delete product \"%s\" (ID: %d)?", p.name.c_str(), p.id);
                ImGui::Text("This cannot be undone.");
                ImGui::Separator();
                if (ImGui::Button("Yes, Delete", ImVec2(120*s, 0))) {
                    std::string err;
                    if (manager.AdminDeleteProduct(p.id, err)) ReloadProducts(manager);
                    ImGui::CloseCurrentPopup();
                }
                ImGui::SameLine();
                if (ImGui::Button("Cancel", ImVec2(100*s, 0))) ImGui::CloseCurrentPopup();
                ImGui::EndPopup();
            }
            ImGui::PopID();
        }
        ImGui::EndTable();
    }
    if (shouldOpenEditModal) ImGui::OpenPopup("Edit Product");
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    if (ImGui::BeginPopupModal("Edit Product", &g_editProdModalOpen,
                                ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Editing product ID: %d", g_editTargetProd.id);
        ImGui::Separator();
        ImGui::SetNextItemWidth(200 * s);
        ImGui::InputText("Serial##edit",      g_editProdSerial, sizeof(g_editProdSerial));
        ImGui::SetNextItemWidth(300 * s);
        ImGui::InputText("Name##edit",        g_editProdName,   sizeof(g_editProdName));
        ImGui::SetNextItemWidth(400 * s);
        ImGui::InputText("Description##edit", g_editProdDesc,   sizeof(g_editProdDesc));
        ImGui::SetNextItemWidth(150 * s);
        ImGui::InputFloat("Cost##edit",       &g_editProdCost,  0.0f, 0.0f, "%.2f");
        if (!g_editProdMsg.empty())
            ImGui::TextColored(g_editProdMsgColor, "%s", g_editProdMsg.c_str());
        ImGui::Separator();
        if (ImGui::Button("Save Changes", ImVec2(150*s, 0))) {
            std::string err;
            bool ok = manager.AdminUpdateProduct(g_editTargetProd,
                        g_editProdSerial, g_editProdName,
                        g_editProdDesc,   g_editProdCost, err);
            if (ok) {
                g_editProdMsg = "Saved successfully.";
                g_editProdMsgColor = ImVec4(0,1,0,1);
                ReloadProducts(manager);
            } else {
                g_editProdMsg = "Error: " + err;
                g_editProdMsgColor = ImVec4(1,0.3f,0.3f,1);
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Close", ImVec2(100*s, 0))) {
            g_editProdMsg.clear();
            g_editProdModalOpen = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
    ImGui::End();
}
static void DrawInvoicesWindow(SystemManager& manager, float s) {
    if (!g_invoicesWindowOpen) return;
    ImGui::SetNextWindowSize(ImVec2(1100 * s, 700 * s), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Encrypted Invoices Vault", &g_invoicesWindowOpen)) {
        ImGui::End();
        return;
    }
    if (!g_invoicesCacheLoaded) ReloadInvoices(manager);
    if (ImGui::Button("Refresh", ImVec2(120 * s, 0))) {
        ReloadInvoices(manager);
        g_invoiceDecrypted = false;
        g_selectedInvoiceId = -1;
    }
    ImGui::SameLine();
    ImGui::Text("Total invoices: %zu", g_invoicesCache.size());
    ImGui::SameLine();
    ImGui::TextDisabled("  |  Click \"Decrypt\" to view contents using your private key");
    ImGui::Separator();
    float leftWidth = 380 * s;
    ImGui::BeginChild("InvoicesList", ImVec2(leftWidth, 0), true);
    ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.0f, 1.0f), "Invoices");
    if (ImGui::BeginTable("InvList", 2,
            ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
            ImGuiTableFlags_ScrollY)) {
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthFixed, 60 * s);
        ImGui::TableSetupColumn("Action");
        ImGui::TableHeadersRow();
        for (const auto& inv : g_invoicesCache) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("#%d", inv.id);
            ImGui::TableSetColumnIndex(1);
            ImGui::PushID(inv.id);
            bool isSelected = (g_selectedInvoiceId == inv.id);
            if (isSelected) {
                ImGui::PushStyleColor(ImGuiCol_Button,
                    ImVec4(0.2f, 0.5f, 0.2f, 1.0f));
            }
            if (ImGui::SmallButton(isSelected ? "Selected" : "Decrypt & View")) {
                g_invoiceDecrypted = false;
                g_selectedInvoiceId = inv.id;
                if (manager.AdminDecryptInvoice(inv.id, &g_decryptedInvoice)) {
                    g_invoiceDecrypted = true;
                    g_invoiceMsg.clear();
                } else {
                    g_invoiceMsg = "Failed to decrypt invoice. "
                                   "Wrong key or corrupted data.";
                    g_invoiceMsgColor = ImVec4(1, 0.3f, 0.3f, 1);
                }
            }
            if (isSelected) ImGui::PopStyleColor();
            ImGui::PopID();
        }
        ImGui::EndTable();
    }
    ImGui::EndChild();
    ImGui::SameLine();
    ImGui::BeginChild("InvoiceDetails", ImVec2(0, 0), true);
    if (g_selectedInvoiceId < 0) {
        ImGui::TextDisabled("Select an invoice from the list to decrypt and view it.");
    } else if (!g_invoiceDecrypted) {
        if (!g_invoiceMsg.empty()) {
            ImGui::TextColored(g_invoiceMsgColor, "%s", g_invoiceMsg.c_str());
        } else {
            ImGui::TextDisabled("Decrypting...");
        }
    } else {
        const DecryptedInvoice& inv = g_decryptedInvoice;
        ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.0f, 1.0f),
            "Invoice #%d - %s", inv.db_id, inv.invoice_number.c_str());
        ImGui::TextDisabled("Issued: %s  |  Branch: %s",
            inv.timestamp.c_str(), inv.branch_name.c_str());
        ImGui::TextDisabled("Cashier: %s  |  Status: %s",
            inv.cashier_name.c_str(), inv.status.c_str());
        ImGui::Separator();
        ImGui::Text("Items (%zu):", inv.items.size());
        if (ImGui::BeginTable("InvItems", 6,
                ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
                ImGuiTableFlags_ScrollY,
                ImVec2(0, 240 * s))) {
            ImGui::TableSetupScrollFreeze(0, 1);
            ImGui::TableSetupColumn("Product");
            ImGui::TableSetupColumn("Serial");
            ImGui::TableSetupColumn("Qty");
            ImGui::TableSetupColumn("Unit Price");
            ImGui::TableSetupColumn("Discount");
            ImGui::TableSetupColumn("Total");
            ImGui::TableHeadersRow();
            for (const auto& it : inv.items) {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0); ImGui::Text("%s", it.name.c_str());
                ImGui::TableSetColumnIndex(1); ImGui::Text("%s", it.serial_number.c_str());
                ImGui::TableSetColumnIndex(2); ImGui::Text("%d", it.quantity);
                ImGui::TableSetColumnIndex(3); ImGui::Text("%.2f", it.unit_price);
                ImGui::TableSetColumnIndex(4); ImGui::Text("%.2f", it.discount);
                ImGui::TableSetColumnIndex(5); ImGui::Text("%.2f", it.total_price);
            }
            ImGui::EndTable();
        }
        ImGui::Separator();
        ImGui::Text("Subtotal:       %.2f EGP", inv.subtotal);
        ImGui::Text("Tax:            %.2f EGP", inv.total_tax);
        ImGui::Text("Discount:       %.2f EGP", inv.total_discount);
        ImGui::TextColored(ImVec4(0, 1, 0, 1),
                           "GRAND TOTAL:    %.2f EGP", inv.grand_total);
        ImGui::Separator();
        ImGui::Text("Payment Method: %s", inv.payment_method.c_str());
        ImGui::Text("Paid:           %.2f EGP", inv.paid_amount);
        ImGui::Text("Change:         %.2f EGP", inv.change_amount);
    }
    ImGui::EndChild();
    ImGui::End();
}
void Render(SystemManager& manager, float scale_factor) {
    if (!g_dashboardOpen) return;
    if (manager.GetCurrentRole() != 2) {
        Close();
        return;
    }
    float s = scale_factor;
    ImGui::SetNextWindowSize(ImVec2(500 * s, 0), ImGuiCond_FirstUseEver);
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_FirstUseEver, ImVec2(0.5f, 0.5f));
    if (ImGui::Begin("Admin Dashboard", &g_dashboardOpen)) {
        ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.0f, 1.0f),
            "Welcome, %s", manager.GetCurrentUser().username.c_str());
        ImGui::TextDisabled("Administrator Control Panel");
        ImGui::Separator();
        ImGui::Spacing();
        ImGui::Text("Choose an action:");
        ImGui::Spacing();
        if (ImGui::Button("User Management", ImVec2(-1, 60 * s))) {
            g_usersWindowOpen = true;
            g_usersCacheLoaded = false; 
        }
        ImGui::TextDisabled("  Create new users, reset passwords, manage roles");
        ImGui::Spacing();
        if (ImGui::Button("Encrypted Invoices Vault", ImVec2(-1, 60 * s))) {
            g_invoicesWindowOpen = true;
            g_invoicesCacheLoaded = false;
            g_invoiceDecrypted = false;
            g_selectedInvoiceId = -1;
        }
        ImGui::Spacing();
        if (ImGui::Button("Product Management", ImVec2(-1, 60 * s))) {
            g_productsWindowOpen  = true;
            g_productsCacheLoaded = false;
        }
        ImGui::TextDisabled("  Add, edit, delete products — all changes signed with your private key");
        ImGui::TextDisabled("  Decrypt and view stored invoices using your private key");
        ImGui::Spacing();
        ImGui::Separator();
        if (ImGui::Button("Close Dashboard", ImVec2(-1, 35 * s))) {
            Close();
        }
    }
    ImGui::End();
    DrawUsersWindow(manager, s);
    DrawInvoicesWindow(manager, s);
    DrawProductsWindow(manager, s);
}
}