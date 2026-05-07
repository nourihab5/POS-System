#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>
#include <iostream>
#include <string>

#include "utils/SystemManager.h"
#include "AdminDashboard.h"


#define ICON_ID 920
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#include <dwmapi.h>

enum AppState { PAGE_LOGIN, PAGE_CASHIER }; // الأدمن والعميل في نفس الصفحة الآن
AppState currentState = PAGE_LOGIN;

SystemManager manager("data/cashier.db");
char username[128] = "";
char password[128] = "";
char barcodeInput[128] = "";
std::string loginErrorMessage = ""; // لعرض رسائل الخطأ
float scale_factor = 1.5f;

void DrawLoginPage() {
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(450 * scale_factor, 0));

    ImGui::Begin("System Login", NULL, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
    
    ImGui::Text("Username:");
    ImGui::SetNextItemWidth(-FLT_MIN); // جعل الحقل يأخذ العرض المتاح بالكامل
    ImGui::InputText("##user", username, 128);

    ImGui::Spacing();
    ImGui::Text("Password:");
    ImGui::SetNextItemWidth(-FLT_MIN);
    ImGui::InputText("##pass", password, 128, ImGuiInputTextFlags_Password);

    if (!loginErrorMessage.empty()) {
        ImGui::Spacing();
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "%s", loginErrorMessage.c_str());
    }

    ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();

    if (ImGui::Button("Login", ImVec2(-1, 45 * scale_factor))) {
        int role = manager.Login(username, password);
        if (role > 0) { 
            currentState = PAGE_CASHIER;
            manager.StartNewInvoice();
            manager.StartHttpServer(8080);
            loginErrorMessage = ""; 
        } else {
            loginErrorMessage = "Error: Incorrect password or user not found!";
        }
    }
    ImGui::End();
}

void DrawCashierPage() {
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
    ImGui::Begin("Terminal", NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize);
    
    // شريط الأدوات العلوي
    ImGui::Text("User: %s (%s)", manager.GetCurrentUser().username.c_str(), 
                manager.GetCurrentRole() == 2 ? "Admin" : "Agent");
    
    ImGui::SameLine(ImGui::GetWindowWidth() - 320 * scale_factor);
    
    // زرار الـ Dashboard يظهر للأدمن فقط
    if (manager.GetCurrentRole() == 2) {
        if (ImGui::Button("Dashboard", ImVec2(150 * scale_factor, 0))) {
            AdminDashboard::Open();
        }
        ImGui::SameLine();
    }

    if (ImGui::Button("Logout", ImVec2(120 * scale_factor, 0))) { 
        currentState = PAGE_LOGIN;
        AdminDashboard::Close(); 
        manager.Logout(); 
    }

    ImGui::Separator();

    // جزء إدخال المنتج
    ImGui::Text("Scan Barcode:");
    ImGui::SetNextItemWidth(300 * scale_factor);
    ImGui::InputText("##scan", barcodeInput, 128);
    ImGui::SameLine();
    if (ImGui::Button("Add (+)") || ImGui::IsKeyPressed(ImGuiKey_Enter)) {
        if (manager.AddProductToCart(barcodeInput, 1)) { // إضافة للميموري وصوت بيب
            memset(barcodeInput, 0, 128);
            ImGui::SetKeyboardFocusHere(-1); // إعادة التركيز للخانة
        }
    }

    // عرض جدول المشتريات
    if (ImGui::BeginTable("Cart", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
        ImGui::TableSetupColumn("Product Name");
        ImGui::TableSetupColumn("Qty");
        ImGui::TableSetupColumn("Unit Price");
        ImGui::TableSetupColumn("Total");
        ImGui::TableHeadersRow();

        auto cart = manager.GetActiveCart(); // جلب السلة
        for (const auto& item : cart.items) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0); ImGui::Text("%s", item.name.c_str());
            ImGui::TableSetColumnIndex(1); ImGui::Text("%d", item.quantity);
            ImGui::TableSetColumnIndex(2); ImGui::Text("%.2f", item.unit_price);
            ImGui::TableSetColumnIndex(3); ImGui::Text("%.2f", item.total_price);
        }
        ImGui::EndTable();
    }

    // منطقة الحساب الإجمالي
    ImGui::SetCursorPosY(ImGui::GetWindowHeight() - 120 * scale_factor);
    ImGui::Separator();
    auto cart = manager.GetActiveCart();
    ImGui::Text("Subtotal: %.2f EGP", cart.subtotal);
    ImGui::Text("Tax (14%%): %.2f EGP", cart.total_tax);
    ImGui::TextColored(ImVec4(0, 1, 0, 1), "GRAND TOTAL: %.2f EGP", cart.grand_total);

    if (ImGui::Button("CHECKOUT (Print PDF)", ImVec2(-1, 50 * scale_factor))) {
        if (manager.Checkout("Cash")) { // حفظ وتوليد PDF
            
        }
    }
    AdminDashboard::Render(manager, scale_factor);
    ImGui::End();
}

int main() {
    if (!glfwInit()) return -1;
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE); // إخفاء للـ Anti-Glitch

    GLFWwindow* window = glfwCreateWindow(1280, 800, "Cashier System", NULL, NULL);
    if (!window) return -1;

    // تفعيل الـ Dark Mode للـ Title Bar (Windows 10/11)
    HWND hwnd = glfwGetWin32Window(window);
    HICON hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(ICON_ID));
    if (hIcon) {
        SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
        SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
    }
    BOOL USE_DARK_MODE = true;
    DwmSetWindowAttribute(hwnd, 20, &USE_DARK_MODE, sizeof(USE_DARK_MODE));
    
    glfwShowWindow(window);
    glfwMakeContextCurrent(window);
    glfwSwapInterval(true);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    ImGui::GetIO().FontGlobalScale = scale_factor;

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        if (currentState == PAGE_LOGIN) DrawLoginPage();
        else DrawCashierPage();

        ImGui::Render();
        glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwTerminate();
    return 0;
}