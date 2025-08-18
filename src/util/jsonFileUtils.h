#pragma once
#include <cstdio>
#include <ctime>
#include <fstream>
#include <string>
#include <sys/stat.h>

namespace utils {

class FileUtils {
public:
    static bool FileExists(const std::string& filepath) {
        std::ifstream file(filepath);
        return file.good();
    }
    
    static bool ReadFileContent(const std::string& filepath, std::string& content) {
        std::ifstream file(filepath);
        if (!file.is_open()) {
            return false;
        }
        
        content = std::string((std::istreambuf_iterator<char>(file)),
                              std::istreambuf_iterator<char>());
        file.close();
        return true;
    }
    
    static bool WriteFileContent(const std::string& filepath, const std::string& content) {
        std::ofstream file(filepath);
        if (!file.is_open()) {
            return false;
        }
        
        file << content;
        file.close();
        return true;
    }
    
    static bool DeleteFile(const std::string& filepath) {
        return std::remove(filepath.c_str()) == 0;
    }
    
    static time_t GetFileModifyTime(const std::string& filepath) {
        struct stat fileStat;
        if (stat(filepath.c_str(), &fileStat) == 0) {
            return fileStat.st_mtime;
        }
        return 0;
    }
};

}  // namespace utils