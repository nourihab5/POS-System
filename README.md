# POS-System
 
Desktop **Point of Sale (POS)** system written in **C++**, equipped with graphical user interface made with **Dear ImGui**, local **SQLite** database to save all data and automatic generation of **PDF/HTML** invoices using **wkhtmltopdf**.
 
The application was made to work on Windows and offers cashier panel for quick daily sales and administration panel for managing inventory and users.
 
---
 
## Features
 
- Graphical user interface created using Dear ImGui library (OpenGL + GLFW)
- Cashier panel for processing sales
- Administration panel for managing products, inventory and generating reports
- Local SQLite database used to store products, sales data and users' data
- Automatic generation of PDF/HTML invoices for each transaction
- Automatic storing invoices in `invoices/` directory
- Modular code base (managers and utilities)

---
## Project Structure

```
POS-System/
├── assets/            # Images, icons and fonts used by the interface
├── data/              # Data of application (SQLite database files)
├── imgui/             # Dear ImGui library sources (GUI library)
├── include/           # External header files (GLFW, SQLite, etc)
├── invoices/          # Invoices (PDF/HTML) generated from sales transaction
├── lib/               # Static / Dynamic libraries needed for building
├── utils/             # Help classes and utilities (system management etc)
├── AdminDashboard.h/.cpp    # Admin Dashboard interface and functionality
├── main.cpp               # Application starting point (cashier screen)
├── compile.txt               # Pre-build g++ build command for Windows
├── glfw3.dll                # GLFW library (window creation and OpenGL context)
├── sqlite3.dll              # SQLite library
└── wkhtmltox.dll         # PDF generation library (HTML to PDF)
```
 
---

## Tech Stack

|Technology|Purpose|
|---|---|
|C++17|Core programming language|
|Dear ImGui|Graphical user interface|
|GLFW + OpenGL|Window creation and rendering context|
|SQLite3|Local database for storing|
|wkhtmltopdf (wkhtmltox)|Converting HTML invoices to PDF|
 
---

## Requirements

Before starting to work on the project make sure you have:
 
- **Windows** OS (as the project is intended for Windows due to the requirement of the flags: `-mwindows`, `gdi32`, `dwmapi` and others)
- **MinGW-w64 (g++)** compiler supporting C++17
- runtime libraries (`glfw3.dll`, `sqlite3.dll`, `wkhtmltox.dll`) provided in the repository
---
 
## Compilation instructions
 
### 1. Get the repository
 
```bash
git clone https://github.com/nourihab5/POS-System.git
cd POS-System
```

### 2. Compile the project
 
In the repository there is pre-configured compiling script in `compile.txt` file:

```bash
g++ -std=c++17 -O2 ^
main.cpp ^
AdminDashboard.cpp ^
utils/*.cpp ^
imgui/*.cpp ^
resource.o ^
-I. -Iinclude ^
-L. -Llibs ^
-l:glfw3.dll -l:sqlite3.dll -l:wkhtmltox.dll ^
-lopengl32 -lgdi32 -lws2_32 ^
-lbcrypt -ldwmapi ^
-mwindows ^
-o cashier.exe
```

> Execute the command from a shell where the `^` line-continuation works, for example, Windows Command Prompt and MinGW Developer Command Prompt. For bash-like shells, change `^` with `\`.
 
### 3. Run the application
 
After successfully building the application, execute the binary file as follows:
 
```bash
./cashier.exe
```
 
Make sure that the `glfw3.dll`, `sqlite3.dll` and `wkhtmltox.dll` files exist in the same folder as `cashier.exe`.
 
---
 
## Usage
 
1. Execute the application to display the **Cashier** window.
2. Put products into the current transaction and pay.
3. The receipt will be generated automatically and will be saved as PDF/HTML file in the `invoices/` folder.
4. Access the **Admin Dashboard** through the specific button on the interface to manage products, inventories and receipts.
---
 
## Contributing
 
Contributions are very much appreciated. In order to contribute to this project:
 
1. Fork this repository
2. Create a branch (`git checkout -b feature/feature-name`)
3. Make the necessary changes and commit them (`git commit -m "Add new feature"`)
4. Push your branch (`git push origin feature/feature-name`)
5. Create pull request

---

## License
As of now, there is no specific license assigned for this repository. In case of any plan to redistribute or make this available for third-party usage, you can include a LICENSE file (example: MIT).
 
---
 
## Author
 
**Nour Ihab** — [@nourihab5](https://github.com/nourihab5)
