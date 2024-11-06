#include "AssetImporter.hpp"

#include "assets/StaticMesh.hpp"

bool lumina::AssetImporter::Import(const std::string& path)
{
    const std::filesystem::path fsPath(path);
    const auto extension = fsPath.extension();

    std::string outputPath;
    
    switch (extension)
    {
        case ".gltf":
        case ".glb":
            CreateStaticMesh(path, outputPath);
            break;
        default:
            Log::Error("Failed to import asset, file extension \"{}\" is not supported.", extension.u8string());
            return false;
    }

    return true;
}

bool lumina::AssetImporter::CreateStaticMesh(const std::string& path, std::string& outputPath)
{
    // TODO: File should be created in user's content directory, the engine content directory is for testing purposes only
    // TODO: asset should be loaded into asset manager

    const std::string name = std::filesystem::path(path).filename().u8string();
    const std::string contentDir = "/content/";
    const std::string assetPath = contentDir + name;
    
    auto asset = new StaticMesh();
    asset->id = std::hash<std::string>()(assetPath);
    asset->path = assetPath;
    asset->source = path;

    outputPath = assetPath;

    // 1. create asset
    // 2. add data [id, source, path]
    // 3. create file
    // 4. add data to new file in content directory
    // 5. reload asset to make sure the asset is valid with data from the source

    return true;
}
