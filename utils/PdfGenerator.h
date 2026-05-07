#pragma once
#include <string>
#include <map>

class PdfGenerator {
private:
    static void ReplaceAll(std::string& source, const std::string& from, const std::string& to);

public:
    static void Init();
    static void Shutdown();
    static bool GenerateFromHtml(const std::string& htmlContent, const std::string& outputPath);
    static bool GenerateFromFile(const std::string& templatePath, const std::string& outputPath, const std::map<std::string, std::string>& data);
};