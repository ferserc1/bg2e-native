
#include "bg2-loader.h"
#include "errors.h"
#include "buffer-io.h"
#include <stdio.h>

static int g_parseError;

int parseHeader(Bg2ioBufferIterator *it, Bg2File *file);

int parsePlist(Bg2ioBufferIterator *it, Bg2File *file);

int bg2io_getParserError(void)
{
    return g_parseError;
}

Bg2File * bg2io_parseFileBuffer(Bg2ioBuffer * buffer)
{
    g_parseError = BG2IO_NO_ERROR;

    if (buffer == NULL)
    {
        g_parseError = BG2IO_ERR_INVALID_PTR;
        return NULL;
    }

    Bg2File * file = bg2io_createBg2File();
    Bg2ioBufferIterator it = BG2IO_ITERATOR(buffer);

    if (parseHeader(&it, file) != BG2IO_NO_ERROR)
    {
        bg2io_freeBg2File(file);
        return NULL;
    }

    if (parsePlist(&it, file) != BG2IO_NO_ERROR)
    {
        bg2io_freeBg2File(file);
        return NULL;
    }

    return file;
}

int parseHeader(Bg2ioBufferIterator *it, Bg2File *file)
{
    // Read endianess and version
    if (bg2io_readByte(it, &file->header.endianess) < 0 ||
        bg2io_readByte(it, &file->header.majorVersion) < 0 ||
        bg2io_readByte(it, &file->header.minorVersion) < 0 ||
        bg2io_readByte(it, &file->header.revision) < 0)
    {
        g_parseError = BG2IO_ERR_INVALID_FILE_HEADER;
        return g_parseError;
    }

    // Read header block tag
    int block = bg2io_readBlock(it);
    if (block == BG2IO_ERR_INVALID_BLOCK)
    {
        g_parseError = block;
        return g_parseError;
    }
    else if (block != bg2io_Header)
    {
        g_parseError = BG2IO_ERR_UNEXPECTED_BLOCK;
        return g_parseError;
    }

    // Read number of poly list elements
    if (bg2io_readInteger(it, &file->header.numberOfPolyList) < 0)
    {
        g_parseError = BG2IO_ERR_NUMBER_OF_PLIST_EXPECTED;
        return g_parseError;
    }

    // Read materials
    block = bg2io_readBlock(it);
    if (block == BG2IO_ERR_INVALID_BLOCK)
    {
        g_parseError = BG2IO_ERR_MATERIAL_DATA_EXPECTED;
        return g_parseError;
    }

    if (bg2io_readString(it, &file->materialData) < 0)
    {
        g_parseError = BG2IO_ERR_CORRUPTED_MATERIAL_DATA;
        return g_parseError;
    }
    
    block = bg2io_readBlock(it);
    if (block == BG2IO_ERR_INVALID_BLOCK)
    {
        g_parseError = BG2IO_ERR_JOINT_BLOCK_EXPECTED;
        return g_parseError;
    }

    // Read projectors: deprecated, this section only skips the section
    if (block == bg2io_ShadowProjector)
    {
        // Shadow text file: read string to skip the string
        int stringSize = 0;
        bg2io_readInteger(it, &stringSize);
        it->current += stringSize;

        // Attenuation
        it->current += sizeof(float);

        // Projection matrix
        it->current += sizeof(float) * 16;

        // View matrix
        it->current += sizeof(float) * 16;

        // Read next block
        block = bg2io_readBlock(it);
    }
    
    // Read joints
    if (block == bg2io_Joint)
    {
        long long err = bg2io_readString(it, &file->jointData);
        if (err < 0)
        {
            g_parseError = BG2IO_ERR_CORRUPTED_JOINT_DATA;
            return g_parseError;
        }
    }
    else
    {
        g_parseError = BG2IO_ERR_JOINT_BLOCK_EXPECTED;
        return g_parseError;
    }

    return BG2IO_NO_ERROR;
}

int parsePlist(Bg2ioBufferIterator *it, Bg2File *file)
{
    int block = bg2io_readBlock(it);
    if (block == BG2IO_ERR_INVALID_BLOCK)
    {
        g_parseError = BG2IO_ERR_POLY_LIST_BLOCK_EXPECTED;
        return g_parseError;
    }

    file->plists = bg2io_createPolyListArray(file->header.numberOfPolyList);
    int done = 0;
    int currentPlistIndex = 0;
    Bg2ioPolyList * currentPlist = file->plists->data[currentPlistIndex];
    while (done == 0)
    {
        block = bg2io_readBlock(it);

        switch (block) {
        case bg2io_PlistName:
            bg2io_readString(it, &currentPlist->name);
            break;
        case bg2io_MatName:
            bg2io_readString(it, &currentPlist->matName);
            break;
        case bg2io_VertexArray:
            currentPlist->vertex.length = (int) bg2io_readFloatArray(it, &currentPlist->vertex.data);
            if (currentPlist->vertex.length < 0)
            {
                g_parseError = BG2IO_ERR_CORRUPTED_VERTEX_DATA;
                return g_parseError;
            }
            break;
        case bg2io_NormalArray:
            currentPlist->normal.length = (int) bg2io_readFloatArray(it, &currentPlist->normal.data);
            if (currentPlist->normal.length <0 )
            {
                g_parseError = BG2IO_ERR_CORRUPTED_NORMAL_DATA;
                return g_parseError;
            }
            break;
        case bg2io_TexCoord0Array:
            currentPlist->texCoord0.length = (int) bg2io_readFloatArray(it, &currentPlist->texCoord0.data);
            if (currentPlist->texCoord0.length < 0)
            {
                g_parseError = BG2IO_ERR_CORRUPTED_TEXCOORD0_DATA;
                return g_parseError;
            }
            break;
        case bg2io_TexCoord1Array:
            currentPlist->texCoord1.length = (int) bg2io_readFloatArray(it, &currentPlist->texCoord1.data);
            if (currentPlist->texCoord1.length < 0)
            {
                g_parseError = BG2IO_ERR_CORRUPTED_TEXCOORD1_DATA;
                return g_parseError;
            }
            break;
        case bg2io_TexCoord2Array:
            currentPlist->texCoord2.length = (int) bg2io_readFloatArray(it, &currentPlist->texCoord2.data);
            if (currentPlist->texCoord2.length < 0)
            {
                g_parseError = BG2IO_ERR_CORRUPTED_TEXCOORD2_DATA;
                return g_parseError;
            }
            break;
        case bg2io_IndexArray:
            currentPlist->index.length = (int) bg2io_readIntArray(it, &currentPlist->index.data);
            if (currentPlist->index.length < 0)
            {
                g_parseError = BG2IO_ERR_CORRUPTED_INDEX_DATA;
                return g_parseError;
            }
            break;
        case bg2io_PolyList:
        case bg2io_End:
            if (block == bg2io_End)
            {
                int cmpsBlock = bg2io_readBlock(it);
                if (cmpsBlock == bg2io_Components)
                {
                    int errorCode = (int) bg2io_readString(it, &file->componentData);
                    if  (errorCode < 0)
                    {
                        printf("Warning: error reading component data. Error code: %d", errorCode);
                    }
                }

                done = 1;
            }

            ++currentPlistIndex;
            currentPlist = file->plists->data[currentPlistIndex];
            break;
        default:
            g_parseError = BG2IO_ERR_CORRUPTED_POLY_LIST_DATA;
            return g_parseError;
        }
    }
    
    return BG2IO_NO_ERROR;
}
