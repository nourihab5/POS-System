#include "PdfGenerator.h"
#include <wkhtmltox/pdf.h>
#include <iostream>
#include <fstream>
#include <sstream>
void PdfGenerator::ReplaceAll(std::string& source, const std::string& from, const std::string& to) {
    size_t start_pos = 0;
    while ((start_pos = source.find(from, start_pos)) != std::string::npos) {
        source.replace(start_pos, from.length(), to);
        start_pos += to.length();
    }
}
void PdfGenerator::Init() {
    wkhtmltopdf_init(false);
    std::cout << "[PdfGenerator] Engine Initialized.\n";
}
void PdfGenerator::Shutdown() {
    wkhtmltopdf_deinit();
    std::cout << "[PdfGenerator] Engine Shutdown.\n";
}
bool PdfGenerator::GenerateFromFile(const std::string& templatePath, const std::string& outputPath, const std::map<std::string, std::string>& data) {
    std::ifstream file(templatePath);
    if (!file.is_open()) {
        std::cerr << "[PdfGenerator] Error: Could not open template file: " << templatePath << "\n";
        return false;
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string htmlContent = buffer.str();
    file.close();
    for (const auto& pair : data) {
        ReplaceAll(htmlContent, pair.first, pair.second);
    }
    return GenerateFromHtml(htmlContent, outputPath);
}
bool PdfGenerator::GenerateFromHtml(const std::string& htmlContent, const std::string& outputPath) {
    wkhtmltopdf_global_settings* gs = wkhtmltopdf_create_global_settings();
    wkhtmltopdf_set_global_setting(gs, "out", outputPath.c_str());
    wkhtmltopdf_set_global_setting(gs, "size.paperSize", "A4");
    wkhtmltopdf_set_global_setting(gs, "orientation", "Portrait");
    wkhtmltopdf_set_global_setting(gs, "margin.top", "1cm");
    wkhtmltopdf_set_global_setting(gs, "margin.bottom", "1cm");
    wkhtmltopdf_set_global_setting(gs, "margin.left", "1cm");
    wkhtmltopdf_set_global_setting(gs, "margin.right", "1cm");
    wkhtmltopdf_object_settings* os = wkhtmltopdf_create_object_settings();
    wkhtmltopdf_set_object_setting(os, "web.defaultEncoding", "utf-8"); 
    wkhtmltopdf_set_object_setting(os, "web.enableJavascript", "true"); 
    wkhtmltopdf_set_object_setting(os, "load.jsdelay", "500"); 
    wkhtmltopdf_converter* converter = wkhtmltopdf_create_converter(gs);
    wkhtmltopdf_add_object(converter, os, htmlContent.c_str());
    bool success = wkhtmltopdf_convert(converter);
    if (!success) std::cerr << "[PdfGenerator] Error converting HTML to PDF.\n";
    wkhtmltopdf_destroy_converter(converter);
    return success;
}