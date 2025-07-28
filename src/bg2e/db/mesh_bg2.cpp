//
//  mesh_bg2.cpp
//  bg2e
//
//  Created by Fernando Serrano Carpena on 30/6/25.
//

#include <bg2e/db/mesh_bg2.hpp>
#include <bg2e/geo/sphere.hpp>
#include <bg2e/geo/modifiers.hpp>
#include <bg2e/utils/MaterialSerializer.hpp>
#include <bg2e/base/Texture.hpp>

#include <bg2-io.h>

#include <fstream>
#include <cstring>

namespace bg2e::db {

uint32_t readHeader(Bg2ioBufferIterator& it)
{
    unsigned char endian = 0;
    unsigned char major = 0;
    unsigned char minor = 0;
    unsigned char rev = 0;
    bg2io_readByte(&it, &endian);
    bg2io_readByte(&it, &major);
    bg2io_readByte(&it, &minor);
    bg2io_readByte(&it, &rev);
    
    unsigned int header = 0;
    bg2io_readInteger(&it, reinterpret_cast<int*>(&header));
    if (header != bg2io_Header)
    {
        throw std::runtime_error("Malformed bg2 file: expecting header section");
    }
    
    int numberOfPlist = 0;
    bg2io_readInteger(&it, &numberOfPlist);
    return static_cast<uint32_t>(numberOfPlist);
}

std::vector<base::MaterialAttributes> readMaterials(Bg2ioBufferIterator& it, const std::filesystem::path& basePath)
{
    unsigned int token = 0;
    bg2io_readInteger(&it, reinterpret_cast<int*>(&token));
    if (token != bg2io_Materials)
    {
        throw std::runtime_error("Malformed bg2 file: expecting materials block");
    }
    
    char * headerString;
    bg2io_readString(&it, &headerString);

    utils::MaterialSerializer serializer;
    
    std::vector<base::MaterialAttributes> result;
    bool status = serializer.deserializeMaterialArray(headerString, basePath, result);
    
    if (!status)
    {
        std::cout << "WARN: Error loading bg2 file material data." << std::endl;
    }
    
    free(headerString);
    return result;
}

void skipShadowProjectors(Bg2ioBufferIterator& it)
{
    int block = 0;
    bg2io_readInteger(&it, &block);
    if (block == bg2io_ShadowProjector)
    {
        char * fileName;
        float proj[16];
        float trans[16];
        float attenuation;
        bg2io_readString(&it, &fileName);
        bg2io_readFloat(&it, &attenuation);
        for (int i = 0; i<16; ++i)
        {
            bg2io_readFloat(&it, &proj[i]);
        }
        for (int i = 0; i<16; ++i)
        {
            bg2io_readFloat(&it, &trans[i]);
        }
        free(fileName);
    }
    else{
        it.current -= 4;  // Back to the previous block
    }
}

std::string readJoints(Bg2ioBufferIterator& it)
{
    std::string result = "";
    int block = 0;
    bg2io_readInteger(&it, &block);
    if (block == bg2io_Joint)
    {
        char * jointList;
        bg2io_readString(&it, &jointList);
        result = jointList;
        free(jointList);
    }
    else
    {
        it.current -= 4;
    }
    return result;
}

struct Bg2Plist {
    std::string plistName;
    std::string matName;
    std::vector<float> positions;
    std::vector<float> normals;
    std::vector<float> texCoord0;
    std::vector<float> texCoord1;
    std::vector<uint32_t> index;
    base::MaterialAttributes material;
};

std::vector<Bg2Plist> readPolyList(Bg2ioBufferIterator& it, uint32_t numberOfPlist)
{
    std::vector<Bg2Plist> result;
    uint32_t readedPlist = 0;
    int block = 0;
    bg2io_readInteger(&it, &block);
    if (block != bg2io_PolyList)
    {
        throw std::runtime_error("Malformed bg2 file: expecting polyList definition");
    }
    else
    {
        char * plistName = nullptr;
        char * matName = nullptr;
        float * vertex = nullptr;
        int numVertex = 0;
        float * normal = nullptr;
        int numNormals = 0;
        float * t0 = nullptr;
        int numT0 = 0;
        float * t1 = nullptr;
        int numT1 = 0;
        float * t2 = nullptr;
        int numT2 = 0;
        int * index = nullptr;
        Bg2Plist plistData;
        int numIndex = 0;
        int done = 0;
        while (done == 0)
        {
            // Read single poly list
            bg2io_readInteger(&it, &block);
            switch (block) {
            case bg2io_PlistName:
                bg2io_readString(&it, &plistName);
                break;
            case bg2io_MatName:
                bg2io_readString(&it, &matName);
                break;
            case bg2io_VertexArray:
                numVertex = (int) bg2io_readFloatArray(&it, &vertex);
                break;
            case bg2io_NormalArray:
                numNormals = (int) bg2io_readFloatArray(&it, &normal);
                break;
            case bg2io_TexCoord0Array:
                numT0 = (int) bg2io_readFloatArray(&it, &t0);
                break;
            case bg2io_TexCoord1Array:
                numT1 = (int) bg2io_readFloatArray(&it, &t1);
                break;
            case bg2io_TexCoord2Array:
                numT2 = (int) bg2io_readFloatArray(&it, &t2);
                break;
            case bg2io_IndexArray:
                numIndex = (int) bg2io_readIntArray(&it, &index);
                break;
            case bg2io_PolyList:
            case bg2io_End:
                // Done: add a poly list
                if (vertex != NULL)
                {
                    plistData.positions.assign(vertex, vertex + numVertex);
                    free(vertex);
                    vertex = NULL;
                    numVertex = 0;
                }
                if (normal != NULL)
                {
                    plistData.normals.assign(normal, normal + numNormals);
                    free(normal);
                    normal = NULL;
                    numNormals = 0;
                }
                if (t0 != NULL)
                {
                    plistData.texCoord0.assign(t0, t0 + numT0);
                    free(t0);
                    t0 = NULL;
                    numT0 = 0;
                }
                if (t1 != NULL)
                {
                    plistData.texCoord1.assign(t1, t1 + numT1);
                    free(t1);
                    t1 = NULL;
                    numT1 = 0;
                }
                if (t2 != NULL)
                {
                    // Not used
                    free(t2);
                    t2 = NULL;
                    numT2 = 0;
                }
                if (index != NULL)
                {
                    plistData.index.assign(index, index + numIndex);
                    free(index);
                    index = NULL;
                    numIndex = 0;
                }
                if (plistName != NULL)
                {
                    plistData.plistName = plistName;
                    free(plistName);
                    plistName = NULL;
                }
                if (matName != NULL)
                {
                    plistData.matName = matName;
                    free(matName);
                    matName = NULL;
                }
                
                result.push_back(plistData);

                ++readedPlist;
                
                plistData.matName = "";
                plistData.plistName = "";
                plistData.positions.clear();
                plistData.normals.clear();
                plistData.texCoord0.clear();
                plistData.texCoord1.clear();
                plistData.index.clear();
                if (block == bg2io_End)
                {
                    done = 1;
                }
                break;
            }
        }
    }
    
    if (readedPlist != numberOfPlist)
    {
        std::cout << "Warning: inconsistent number of polyList readed in bg2 file" << std::endl;
    }
    return result;
}


std::vector<Bg2Plist> readBg2File(Bg2ioBuffer * buffer, const std::filesystem::path& filePath)
{
    std::ifstream fileStream;
    fileStream.open(filePath, std::ios::binary | std::ios::ate);
    if (!fileStream.is_open())
    {
        throw std::runtime_error("Error opening bg2 file at path '" + filePath.string() + "'");
    }
    
    auto fileSize = fileStream.tellg();
    
    fileStream.seekg(0, std::ios::beg);
    
    if (bg2io_createBuffer(buffer, static_cast<Bg2ioSize>(fileSize)) < 0)
    {
        throw std::runtime_error("Error creating bg2 buffer");
    }
    
    std::vector<uint8_t> bytes(fileSize);
    fileStream.read(reinterpret_cast<char*>(buffer->mem), buffer->length);
    
    Bg2ioBufferIterator it = BG2IO_ITERATOR(buffer);
    auto numberOfPlist = readHeader(it);
    auto materials = readMaterials(it, std::filesystem::path(filePath).remove_filename());
    skipShadowProjectors(it);
    auto joints = readJoints(it);
    
    auto result = readPolyList(it, numberOfPlist);
    for (auto & plist : result)
    {
        for (auto & mat : materials)
        {
            if (mat.name() == plist.matName)
            {
                plist.material = mat;
                break;
            }
        }
    }

    fileStream.close();
    return result;
}

Bg2Mesh * loadMeshBg2(const std::filesystem::path& filePath)
{
    Bg2ioBuffer buffer = BG2IO_BUFFER_INIT;
    auto plists = readBg2File(&buffer, filePath);
    auto result = new Bg2Mesh();
    result->mesh = std::make_shared<bg2e::geo::Mesh>();
    
    uint32_t currentIndex = 0;
    uint32_t submeshOffset = 0;
    uint32_t submeshCount = 0;
    for (auto plist : plists)
    {
        for (auto index : plist.index)
        {
            auto i3 = index * 3;
            auto i2 = index * 2;
            auto position = glm::vec3 { plist.positions[i3], plist.positions[i3 + 1], plist.positions[i3 + 2] };
            auto normal = glm::vec3 { plist.normals[i3], plist.normals[i3 + 1], plist.normals[i3 + 2] };
            // Important: flip Y UV coord
            auto t0 = glm::vec2 { plist.texCoord0[i2], 1.0 - plist.texCoord0[i2 + 1] };
            auto t1 = glm::vec2 { plist.texCoord1[i2], 1.0 - plist.texCoord1[i2 + 1] };
            result->mesh->vertices.push_back({ position, normal, t0, t1, glm::vec3(0.0) });
            result->mesh->indices.push_back(currentIndex);
            ++currentIndex;
            ++submeshCount;
        }
        result->mesh->submeshes.push_back({ submeshOffset, submeshCount });
        result->materials.push_back(plist.material);
        submeshOffset = currentIndex;
        submeshCount = 0;
    }
    geo::GenTangentsModifier<geo::Mesh> tangentGenerator(result->mesh.get());
    tangentGenerator.apply();

    bg2io_freeBuffer(&buffer);
    return result;
}

Bg2Mesh * loadMeshBg2(const std::filesystem::path& filePath, const std::string& fileName)
{
    auto fullPath = filePath;
    fullPath.append(fileName);
    return loadMeshBg2(fullPath);
}

void storeMeshBg2(const std::filesystem::path& filePath, Bg2Mesh * mesh)
{
    auto basePath = filePath;
    basePath.remove_filename();
    Bg2File * file = bg2io_createBg2File();
    file->header.endianess = 0;
    file->header.majorVersion = 1;
    file->header.minorVersion = 2;
    file->header.revision = 0;
    file->header.numberOfPolyList = static_cast<int>(mesh->mesh->submeshes.size());
    
    
    file->componentData = nullptr;
    file->jointData = nullptr;
    
    file->plists = bg2io_createPolyListArray(file->header.numberOfPolyList);
    auto & vertices = mesh->mesh->vertices;
    auto submeshIndex = 0;
    for (auto submesh : mesh->mesh->submeshes)
    {
        Bg2ioPolyList * plist = file->plists->data[submeshIndex];
        auto & material = mesh->materials[submeshIndex];
        
        std::vector<uint32_t> indexes;
        std::vector<float> vertex;
        std::vector<float> normal;
        std::vector<float> uv0;
        std::vector<float> uv1;

        auto submeshIndices = std::vector<uint32_t>(
            mesh->mesh->indices.begin() + submesh.firstIndex,
            mesh->mesh->indices.begin() + submesh.firstIndex + submesh.indexCount
        );
        uint32_t plistIndex = 0;
        for (auto i : submeshIndices)
        {
            auto v = vertices[i];
            vertex.push_back(v.position.x);
            vertex.push_back(v.position.y);
            vertex.push_back(v.position.z);
            
            normal.push_back(v.normal.x);
            normal.push_back(v.normal.y);
            normal.push_back(v.normal.z);
            
            // Flip Y coord
            uv0.push_back(v.texCoord0.x);
            uv0.push_back(1.0 - v.texCoord0.y);
            
            uv1.push_back(v.texCoord1.x);
            uv1.push_back(1.0 - v.texCoord1.y);
            
            indexes.push_back(plistIndex);
            plistIndex++;
        }
        
        bg2io_allocateFloatArray(&plist->vertex, static_cast<int>(vertex.size()));
        bg2io_allocateFloatArray(&plist->normal, static_cast<int>(normal.size()));
        bg2io_allocateFloatArray(&plist->texCoord0, static_cast<int>(uv0.size()));
        bg2io_allocateFloatArray(&plist->texCoord1, static_cast<int>(uv1.size()));
        bg2io_allocateIntArray(&plist->index, static_cast<int>(indexes.size()));
        
        memcpy(plist->vertex.data, vertex.data(), vertex.size() * sizeof(float));
        memcpy(plist->normal.data, normal.data(), normal.size() * sizeof(float));
        memcpy(plist->texCoord0.data, uv0.data(), uv0.size() * sizeof(float));
        memcpy(plist->texCoord1.data, uv1.data(), uv1.size() * sizeof(float));
        memcpy(plist->index.data, indexes.data(), indexes.size() * sizeof(uint32_t));
        
        if (material.name() == "")
        {
            material.setName("material" + std::to_string(submeshIndex));
        }
        plist->name = new char[material.name().size()];
        strcpy(plist->name, material.name().c_str());
        plist->matName = new char[material.name().size()];
        strcpy(plist->matName, material.name().c_str());
        
        
        plist->groupName = new char[material.groupName().size()];
        strcpy(plist->groupName, material.groupName().c_str());
        
        submeshIndex++;
    }
    
    // The component section is empty because this function does not accept a scene node
    
    utils::MaterialSerializer matSerializer;
    std::vector<std::shared_ptr<base::Texture>> textures;
    auto matData = matSerializer.serializeMaterialArray(mesh->materials, textures, true);
    file->materialData = new char[matData.size()];
    strcpy(file->materialData, matData.c_str());
    
    for (auto t : textures)
    {
        std::filesystem::path srcTexture = t->imageFilePath();
        auto fileName = srcTexture.filename();
        auto dstTexture = basePath;
        dstTexture += fileName;
        if (srcTexture != dstTexture)
        {
            std::filesystem::copy(srcTexture, dstTexture, std::filesystem::copy_options::overwrite_existing);
        }
    }
    
    Bg2ioBuffer outBuffer = BG2IO_BUFFER_INIT;
    int error = bg2io_writeFileToBuffer(file, &outBuffer);
    if (error != BG2IO_NO_ERROR)
    {
        throw std::runtime_error("Error writting bg2 file buffer.");
    }
    
    std::ofstream outFile(filePath, std::ios::binary);
    if (!outFile.is_open())
    {
        throw std::runtime_error("Could not open bg2 file to write.");
    }
    
    outFile.write(reinterpret_cast<char*>(outBuffer.mem), outBuffer.actualLength);
    
    outFile.close();
    bg2io_freeBg2File(file);
}

void storeMeshBg2(const std::filesystem::path& filePath, const std::string& fileName, Bg2Mesh * mesh)
{
    auto fullPath = filePath;
    fullPath.append(fileName);
    storeMeshBg2(fullPath, mesh);
}

}
