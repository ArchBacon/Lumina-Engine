#include "core/fileio.hpp"

#include <filesystem>
#include <fstream>
#include <sstream>

namespace lumina
{
    FileIO::FileIO()
    {
        directories[Directory::Assets] = "engine/assets/";
        directories[Directory::Config] = "engine/config/";
        directories[Directory::Log]    = "log/";
    }

    std::string FileIO::ReadTextFile(Directory directory, const std::string& filePath)
    {
        const auto fullPath = GetFilePath(directory, filePath);
        const std::ifstream file(fullPath);
        if (!file.is_open())
        {
            printf("FileIO::ReadTextFile: Failed to open file %s\n", fullPath.c_str());
            return "";
        }
        std::stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }

    bool FileIO::WriteTextFile(Directory directory, const std::string& filePath, const std::string& data)
    {
        const std::string fullPath = GetFilePath(directory, filePath);
        std::ofstream file(fullPath);
        if (!file.is_open())
        {
            printf("FileIO::WriteTextFile: Failed to open file %s\n", fullPath.c_str());
            return false;
        }
        file << data;
        return file.good();
    }

    std::vector<char> FileIO::ReadBinaryFile(Directory directory, const std::string& filePath)
    {
        const auto fullPath = GetFilePath(directory, filePath);
        std::ifstream file(fullPath, std::ios::binary | std::ios::ate);
        if (!file.is_open())
        {
            printf("FileIO::ReadBinaryFile: Failed to open file %s\n", fullPath.c_str());
            return {};
        }
        const auto fileSize = file.tellg();
        file.seekg(0, std::ios::beg);
        std::vector<char> fileContent(fileSize);
        if (file.read(fileContent.data(), fileSize))
        {
            return fileContent;
        }

        return {};
    }

    bool FileIO::WriteBinaryFile(Directory directory, const std::string& filePath, const std::vector<char>& data)
    {
        const auto fullPath = GetFilePath(directory, filePath);
        std::ofstream file(fullPath, std::ios::binary);
        if (!file.is_open())
        {
            printf("FileIO::WriteBinaryFile: Failed to open file %s\n", fullPath.c_str());
            return false;
        }
        file.write(data.data(), static_cast<std::streamsize>(data.size()));
        return file.good();
    }

    std::string FileIO::GetFilePath(Directory directory, const std::string& filePath)
    {
        return directories[directory] + filePath;
    }

    bool FileIO::FileExists(Directory directory, const std::string& filePath)
    {
        const auto fullPath = GetFilePath(directory, filePath);
        return std::filesystem::exists(fullPath);
    }

    uint64_t FileIO::LastModified(Directory directory, const std::string& filePath)
    {
        const auto fullPath = GetFilePath(directory, filePath);
        return std::filesystem::last_write_time(fullPath).time_since_epoch().count();
    }
} // namespace lumina
