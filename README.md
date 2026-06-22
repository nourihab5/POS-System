# POS-System
 
Desktop **Point of Sale (POS)** system written in **C++**, equipped with graphical user interface made with **Dear ImGui**, local **SQLite** database to save all data and automatic generation of **PDF** invoices using **wkhtmltopdf**.
 
The application was made to work on Windows and offers cashier panel for quick daily sales and administration panel for managing inventory and users.
 
---
 
## Features

- Lightweight, interactive UI using Dear ImGui library (OpenGL + GLFW)
- Cashier panel for conducting sales operations
- Administrative control panel for managing products, inventory, and reports
- Local SQLite database containing products, sales information, and user information
- Generation of invoice in PDF format for each sale operation
- Automatic storage of invoices in `invoices/` folder
- **AES-256 & RSA encryption** (using Windows CNG/BCrypt API) employed to secure invoice data and private key material
- **Hybrid encryption for invoices**: the content of the invoice is encrypted by a randomly generated AES-256 session key which is, in turn, encrypted by RSA-OAEP (SHA-256) using the recipient's public key
- **Data tamper-proofing system**: product/user records are signed with the administrator's private key using RSA + SHA-256 (PKCS#1) signature algorithm and the signature is verified against the public key prior to the data being considered trustworthy, ensuring the possibility of detecting any tampering with the stored records
- Modular architecture (managers & utilities)

---

## Project Structure

```
POS-System/
├── assets/              # Images, icons, fonts, HTML invoice template and RSA public key
├── data/                 # Application data: SQLite database and encrypted private key
├── imgui/                 # Dear ImGui GUI library sources
├── include/               # Headers from external libraries (GLFW, SQLite, wkhtmltox and more)
├── invoices/             # Generated invoices in PDF format for sales operations
├── lib/                   # Static/Dynamic libraries used for the build process
├── utils/                  # Various helper classes
│   ├── CryptoWrapper.h/.cpp   # AES-256, RSA, hashing, hybrid encryption logic
│   ├── DatabaseManager.h/.cpp # SQLite interaction, record signing and signature verification
│   ├── PdfGenerator.h/.cpp    # Invoices rendering and PDF generation
│   └── SystemManager.h/.cpp   # Authentication and session handling
├── AdminDashboard.h/.cpp # Admin dashboard interface and logic
├── main.cpp              # Program entry point (cashier view)
├── compile.txt            # Ready-to-use g++ build command for Windows
├── glfw3.dll              # GLFW windowing, OpenGL context library
├── sqlite3.dll             # SQLite library
└── wkhtmltox.dll             # Library for generating invoices in PDF format from HTML code
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
3. The receipt will be generated automatically and will be saved as PDF file in the `invoices/` folder.
4. Access the **Admin Dashboard** through the specific button on the interface to manage products, inventories and receipts.
---

## Security and Data Integrity
The system creates an additional layer of cryptography (`utils/CryptoWrapper`) above the Windows CNG (BCrypt) API, not storing sensitive information in plain text.
 
**Invoice encryption (hybrid AES/RSA scheme)**
Invoices are encrypted via a hybrid scheme that implies:
- Invoice contents encryption using a randomly generated **AES-256** session key in CBC mode, where IV is unique and random.
- The session key encryption with a recipient's **RSA public key** with OAEP padding (SHA-256).
This way, the AES session key is never stored or sent in the open.
- Invoice decryption goes in reverse order: RSA private key decrypts the AES session key which is further used for contents decryption.
  
**Protection of private key material**
Private RSA key of a user is never stored in a plaintext form. This key is encrypted in a file at rest using AES-256 algorithm with a combination of user's password and **PBKDF2-HMAC-SHA256** (with 310,000 iterations and a stored salt) as a key.

**Product and User Anti-tampering / Data Integrity**
In order to prevent changes to the stored information, each product and user is signed digitally when created or modified:
- Hashing (SHA-256) of each product or user and signing by the administrator’s **RSA private key** (according to PKCS#1 signature algorithm) is done before saving the information into the database.
- At the time the information is read and used by the system, the signature is verified using the stored **RSA public key**. If the record was changed since signing in any way (direct database modifications or otherwise), then verification will fail, and the system will raise an alarm (logged as a security issue).
It is guaranteed that both the invoice information protection and the integrity of important business data (products, prices, users) are enforced cryptographically rather than by the application itself.

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
