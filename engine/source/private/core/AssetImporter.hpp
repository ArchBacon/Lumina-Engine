#pragma once

#include <filesystem>
#include <string>

namespace fs = std::filesystem;

namespace lumina
{
    class StaticMesh;
    
    class AssetImporter
    {
    public:
        static bool Import(const std::string& path);

    protected:
        static bool CreateStaticMesh(const std::string& path, std::string& outputPath);
    };
}
