#pragma once

#include <string>

namespace lumina
{
    class Asset
    {
        friend class AssetManager;
        friend class AssetImporter;

    protected:
        size_t id {0};
        std::string path {};
        std::string source {};
        
    public:
        Asset() = default;
        virtual ~Asset() = default;

        // Returns the hashed resource ID
        [[nodiscard]] size_t GetResourceID() const { return id; }

        // Returns the relative path of the asset in the project
        [[nodiscard]] const std::string& GetPath() const { return path; }

        // Returns the absolute path of the file that this asset was created from
        [[nodiscard]] const std::string& GetSource() const { return source; }

        // Reload allows assets to reload an asset if there were any changes to the source file
        virtual void Reload() = 0;
    };
}
