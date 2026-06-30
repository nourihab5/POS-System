cmake_minimum_required(VERSION 3.10)

project(CheckoutNew LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS_RELEASE "-O2")

file(GLOB UTILS_SOURCES "utils/*.cpp")
file(GLOB IMGUI_SOURCES "imgui/*.cpp")

add_executable(CheckoutNew WIN32
    main.cpp
    AdminDashboard.cpp
    ${UTILS_SOURCES}
    ${IMGUI_SOURCES}
    resource.o
)

target_include_directories(CheckoutNew PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_directories(CheckoutNew PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/libs
)

target_link_libraries(CheckoutNew PRIVATE
    glfw3.dll
    sqlite3.dll
    wkhtmltox.dll
    opengl32
    gdi32
    ws2_32
    bcrypt
    dwmapi
)
