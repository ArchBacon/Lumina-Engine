#include "core/fileio.hpp"

#include <filesystem>
#include <fstream>
#include <sstream>

#include "core/log.hpp"

namespace lumina
{
    FileIO::FileIO()
    {
        directories[Directory::Assets] = "engine/assets/";
        directories[Directory::Config] = "engine/config/";
        directories[Directory::Log]    = "log/";
    }

    std::string FileIO::ReadTextFile(const Directory directory, const std::string& filePath)
    {
        const auto fullPath = GetFilePath(directory, filePath);
        const std::ifstream file(fullPath);
        if (!file.is_open())
        {
            Log::Error("FileIO::ReadTextFile: Failed to open file {}", fullPath);
            return "";
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }

    bool FileIO::WriteTextFile(const Directory directory, const std::string& filePath, const std::string& data)
    {
        const std::string fullPath = GetFilePath(directory, filePath);
        std::ofstream file(fullPath);
        if (!file.is_open())
        {
            Log::Error("FileIO::WriteTextFile: Failed to open file {}", fullPath);
            return false;
        }

        file << data;
        return file.good();
    }

    std::vector<char> FileIO::ReadBinaryFile(const Directory directory, const std::string& filePath)
    {
        const auto fullPath = GetFilePath(directory, filePath);
        std::ifstream file(fullPath, std::ios::binary | std::ios::ate);
        if (!file.is_open())
        {
            Log::Error("FileIO::ReadBinaryFile: Failed to open file {}", fullPath);
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

    bool FileIO::WriteBinaryFile(const Directory directory, const std::string& filePath, const std::vector<char>& data)
    {
        const auto fullPath = GetFilePath(directory, filePath);
        std::ofstream file(fullPath, std::ios::binary);
        if (!file.is_open())
        {
            Log::Error("FileIO::WriteBinaryFile: Failed to open file {}", fullPath);
            return false;
        }

        file.write(data.data(), static_cast<std::streamsize>(data.size()));
        return file.good();
    }

    std::string FileIO::GetFilePath(const Directory directory, const std::string& filePath) { return directories[directory] + filePath; }

    bool FileIO::FileExists(const Directory directory, const std::string& filePath)
    {
        const auto fullPath = GetFilePath(directory, filePath);
        return std::filesystem::exists(fullPath);
    }

    uint64_t FileIO::LastModified(const Directory directory, const std::string& filePath)
    {
        const auto fullPath = GetFilePath(directory, filePath);
        return std::filesystem::last_write_time(fullPath).time_since_epoch().count();
    }
} // namespace lumina
