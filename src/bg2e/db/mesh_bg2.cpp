//
//  mesh_bg2.cpp
//  bg2e
//
//  Created by Fernando Serrano Carpena on 30/6/25.
//

#include <bg2e/db/mesh_bg2.hpp>

#include <bg2e/geo/sphere.hpp>

#include <bg2-io.h>

#include <fstream>

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

std::string readMaterials(Bg2ioBufferIterator& it)
{
    unsigned int token = 0;
    bg2io_readInteger(&it, reinterpret_cast<int*>(&token));
    if (token != bg2io_Materials)
    {
        throw std::runtime_error("Malformed bg2 file: expecting materials block");
    }
    
    char * headerString;
    bg2io_readString(&it, &headerString);
    std::string result = headerString;
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
    auto materials = readMaterials(it);
    skipShadowProjectors(it);
    auto joints = readJoints(it);
    
    auto result = readPolyList(it, numberOfPlist);
    
    fileStream.close();
    return result;
}

Bg2Mesh * loadMeshBg2(const std::filesystem::path& filePath)
{
    Bg2ioBuffer buffer = BG2IO_BUFFER_INIT;
    auto plists = readBg2File(&buffer, filePath);
    
    // TODO: Read plist and initialize result mesh
    
    auto result = new Bg2Mesh();
    //result->mesh = std::make_shared<geo::MeshPNUUT>();
    result->mesh = std::shared_ptr<bg2e::geo::Mesh>(geo::createSphere(1.3, 40, 40));
    
    base::MaterialAttributes mat;
    mat.setAlbedo(base::Color::Red());
    mat.setMetalness(0.92);
    mat.setRoughness(0.12);
    result->materials.push_back(mat);
    
    bg2io_freeBuffer(&buffer);
    return result;
}

Bg2Mesh * loadMeshBg2(const std::filesystem::path& filePath, const std::string& fileName)
{
    auto fullPath = filePath;
    fullPath.append(fileName);
    return loadMeshBg2(fullPath);
}

}
