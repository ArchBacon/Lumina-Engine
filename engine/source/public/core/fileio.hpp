#pragma once

#include <map>
#include <string_view>
#include <vector>

namespace lumina
{
    class Engine;

    class FileIO
    {
        friend class Engine;
        
    public:
        enum class Directory
        {
            Assets,
            Config,
            Log
        };

        FileIO();
        ~FileIO() = default;

        FileIO(const FileIO&)            = delete;
        FileIO(FileIO&&)                 = delete;
        FileIO& operator=(const FileIO&) = delete;
        FileIO& operator=(FileIO&&)      = delete;

        //Text file read/write
        [[nodiscard]] std::string ReadTextFile(Directory directory, const std::string& filePath);
        bool WriteTextFile(Directory directory, const std::string& filePath, const std::string& data);

        //Binary file read/write
        [[nodiscard]] std::vector<char> ReadBinaryFile(Directory directory, const std::string& filePath);
        bool WriteBinaryFile(Directory directory, const std::string& filePath, const std::vector<char>& data);

        [[nodiscard]] std::string GetFilePath(Directory directory, const std::string& filePath);
        bool FileExists(Directory directory, const std::string& filePath);

        [[nodiscard]] uint64_t LastModified(Directory directory, const std::string& filePath);

    private:
        std::map<Directory, std::string> directories {};
    };
} // namespace lumina

