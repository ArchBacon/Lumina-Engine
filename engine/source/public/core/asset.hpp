#pragma once
#include <string>

namespace lumina
{
    enum class AssetType
    {
        Texture, // Including Samplers or not can be discussed
        Shader,
        Model,   // Collections of meshes
        Mesh,
        Material,
        None
    };

    class Asset
    {
        friend class AssetManager;
    private:
        static size_t nextID;
        
    protected:
        size_t id;
        AssetType type;
        std::string filePath;
        
        Asset(AssetType type) : id(0), type(type) {}
        virtual ~Asset();
        virtual void Reload() {}
        
        static size_t GenerateID(const std::string& path) { return std::hash<std::string>{}(path); }
        static size_t GetNextID() { return nextID++; }
        static const std::string& GetFilePath(const std::string& path) { return path; }
        
    public:
        Asset(Asset&& other) = delete;
        Asset(const Asset& other) = delete;
        Asset& operator=(Asset&& other) = delete;
        Asset& operator=(const Asset& other) = delete;
        
        size_t GetID() const { return id; }
        AssetType GetType() const { return type; }
        const std::string& GetFilePath() const { return filePath; }
    };
}
