#include "AssetImporter.hpp"

#include "assets/StaticMesh.hpp"
#include "core/fileio.hpp"
#include "core/log.hpp"

#include <fastgltf/core.hpp>

bool lumina::AssetImporter::Import(const std::string& path)
{
    const std::filesystem::path fsPath(path);
    const auto extension = fsPath.extension().u8string();

    std::string outputPath;
    if (extension == ".gltf" || extension == ".glb")
    {
        Log::Info("Creating Static Mesh asset from file with extension \"{}\".", extension);
        CreateStaticMesh(path, outputPath);
    }
    else
    {
        Log::Error("Failed to import asset, file extension \"{}\" is not supported.", extension);
        return false;
    }

    return true;
}

bool lumina::AssetImporter::CreateStaticMesh(const std::string& path, std::string& outputPath)
{
    // TODO: File should be created in user's content directory, the engine content directory is for testing purposes only
    // TODO: asset should be loaded into asset manager

    // TODO: use FilIO to handle directories
    const std::string name = std::filesystem::path(path).filename().u8string();
    const std::string contentDir = "/content/";
    const std::string assetPath = contentDir + name;

    const auto asset = new StaticMesh();
    asset->id = std::hash<std::string>()(assetPath);
    asset->path = assetPath;
    asset->source = path;

    outputPath = assetPath;

    
    FileIO().WriteBinaryFile(FileIO::Directory::Content, std::filesystem::path(path).stem().u8string() + ".lumina", {});

    // 1. create asset
    // 2. add data [id, source, path]
    // 3. create file
    // 4. add data to new file in content directory
    // 5. reload asset to make sure the asset is valid with data from the source

    return true;
}
