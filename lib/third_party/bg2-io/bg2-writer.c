
#include "bg2-writer.h"
#include "errors.h"
#include "buffer-memory.h"
#include "buffer-io.h"

#include <string.h>

// String size in buffer: 4 bytes for the string size and 1 byte for each character
#define STR_SIZE(s)  sizeof(char) * strlen(s) + sizeof(int)
// Array size in buffer: 4 bytes for the buffer size and 4 bytes for each element
#define ARR_SIZE(v,T)  v.length * sizeof(T) + sizeof(int)

#define ASSERT_ERR(e,writtenBytes)   if (e < 0) return e; else writtenBytes += e
#define ASSERT_ERR_NO_INC(e)         if (e < 0) return e


Bg2ioSize calculatePolyListSize(Bg2File *file);
Bg2ioSize writeHeaderToBuffer(Bg2File *file, Bg2ioBufferIterator *it);
Bg2ioSize writePolyListsToBuffer(Bg2File *file, Bg2ioBufferIterator *it);
Bg2ioSize writeBuffer(Bg2ioBufferIterator *it, Bg2ioFloatArray *arr,int blockType);
Bg2ioSize writeIntdexBuffer(Bg2ioBufferIterator *it, Bg2ioIntArray *arr);

Bg2ioSize bg2io_calculateBufferSize(Bg2File *file)
{
    if (file == NULL)
    {
        return BG2IO_ERR_INVALID_FILE_PTR;
    }
    else if (file->materialData == NULL)
    {
        return BG2IO_ERR_INVALID_MATERIAL_DATA;
    }

    // Header: endianess, major version, minor version, revision: 4 bytes
    // hedr block: 4 bytes
    // Number of poly lists: 4 bytes
    // mtrl block: 4 bytes
    Bg2ioSize total = 16;

    // Material string
    total += STR_SIZE(file->materialData);

    // Joint block: 4 bytes
    // Joint string
    if (file->jointData != NULL)
    {
        total += 4 + STR_SIZE(file->jointData);
    }
    else
    {
        // Joint data is optional in Bg2File struct, but mandatory on file buffer
        total += 4 + STR_SIZE("{}");
    }

    Bg2ioSize plistSize = calculatePolyListSize(file);
    if (plistSize < 0)
    {
        return plistSize;   // Error
    }
    total += plistSize;

    // components (if present)
    if (file->componentData != NULL)
    {
        // cmps block (4 bytes) + block string
        total += 4 + STR_SIZE(file->componentData);
    }

    return total;
}

int bg2io_writeFileToBuffer(Bg2File * file, Bg2ioBuffer *dest)
{
    if (dest == NULL)
    {
        return BG2IO_ERR_INVALID_BUFFER_PTR;
    }


    Bg2ioSize requiredSize = bg2io_calculateBufferSize(file);
    if (requiredSize<0)
    {
        return (int) requiredSize;
    }

    Bg2ioSize err = bg2io_createBuffer(dest, requiredSize);
    if (err < 0)
    {
        return (int) err;
    }

    Bg2ioBufferIterator it = BG2IO_ITERATOR(dest);

    err = writeHeaderToBuffer(file, &it);
    ASSERT_ERR_NO_INC((int) err);

    err = writePolyListsToBuffer(file, &it);
    ASSERT_ERR_NO_INC((int) err);

    if (file->componentData != NULL)
    {
        err = bg2io_writeBlock(&it, bg2io_Components);
        ASSERT_ERR_NO_INC((int) err);

        err = bg2io_writeString(&it, file->componentData);
        ASSERT_ERR_NO_INC((int) err);
    }

    return BG2IO_NO_ERROR;
}

Bg2ioSize calculatePolyListSize(Bg2File * file)
{
    if (file->plists == NULL || file->plists->length == 0)
    {
        return BG2IO_ERR_INVALID_PLIST_DATA;
    }

    Bg2ioSize total = 0;

    for (int i = 0; i < file->plists->length; ++i)
    {
        Bg2ioPolyList * plist = file->plists->data[i];
        if (plist->vertex.length == 0)
        {
            return BG2IO_ERR_INVALID_VERTEX_DATA;
        }
        if (plist->index.length == 0)
        {
            return BG2IO_ERR_INVALID_INDEX_DATA;
        }

        total += 4; // plst block

        if (plist->name != NULL)
        {
            // pnam block (4 bytes) + name
            total += 4 + STR_SIZE(plist->name);
        }
        if (plist->matName != NULL)
        {
            // mnam block (4 bytes) + matName
            total += 4 + STR_SIZE(plist->matName);
        }
        // varr block (4 bytes) + vertex array length
        total += 4 + ARR_SIZE(plist->vertex, float); 
        if (plist->normal.length > 0)
        {
            // narr block (4 bytes) + normal array length
            total += 4 + ARR_SIZE(plist->normal, float);
        }
        if (plist->texCoord0.length > 0)
        {
            // t0ar block (4 bytes) + array length
            total += 4 + ARR_SIZE(plist->texCoord0, float);
        }
        if (plist->texCoord1.length > 0)
        {
            // t1ar block (4 bytes) + array length
            total += 4 + ARR_SIZE(plist->texCoord1, float);
        }
        if (plist->texCoord2.length > 0)
        {
            // t0ar block (4 bytes) + array length
            total += 4 + ARR_SIZE(plist->texCoord2, float);
        }
        //  indx block (4 bytes) +  array length
        total += 4 + ARR_SIZE(plist->index, int);
    }

    // end4 block: 4 bytes
    total += 4;

    return total;
}

Bg2ioSize writeHeaderToBuffer(Bg2File * file, Bg2ioBufferIterator * it)
{
    Bg2ioSize writtenBytes = 0;

    // endianness | majorVersion | minorVersion | revision
    Bg2ioSize err = bg2io_writeByte(it,file->header.endianess);
    ASSERT_ERR(err,writtenBytes);
    err = bg2io_writeByte(it,file->header.majorVersion);
    ASSERT_ERR(err,writtenBytes);
    err = bg2io_writeByte(it,file->header.minorVersion);
    ASSERT_ERR(err,writtenBytes);
    err = bg2io_writeByte(it,file->header.revision);
    ASSERT_ERR(err,writtenBytes);

    // hedr block
    err = bg2io_writeBlock(it,bg2io_Header);
    ASSERT_ERR(err,writtenBytes);

    // Number of poly lists
    err = bg2io_writeInteger(it,file->header.numberOfPolyList);
    ASSERT_ERR(err,writtenBytes);

    // mtrl block
    err = bg2io_writeBlock(it,bg2io_Materials);
    ASSERT_ERR(err,writtenBytes);

    // material string
    if (!file->materialData)
    {
        return BG2IO_ERR_INVALID_MATERIAL_DATA;
    }
    err = bg2io_writeString(it,file->materialData);
    ASSERT_ERR(err,writtenBytes);

    // join block
    err = bg2io_writeBlock(it,bg2io_Joint);
    ASSERT_ERR(err,writtenBytes);

    // joint string
    if (file->jointData)
    {
        err = bg2io_writeString(it,file->jointData);
        ASSERT_ERR(err,writtenBytes);
    }
    else
    {
        err = bg2io_writeString(it,"{}");
        ASSERT_ERR(err,writtenBytes);
    }

    return writtenBytes;
}

Bg2ioSize writePolyListsToBuffer(Bg2File *file, Bg2ioBufferIterator *it)
{
    Bg2ioSize writtenBytes = 0;
    Bg2ioSize err = BG2IO_NO_ERROR;
    
    for (int i = 0; i < file->plists->length; ++i)
    {
        Bg2ioPolyList * plist = file->plists->data[i];
        err = bg2io_writeBlock(it,bg2io_PolyList);
        ASSERT_ERR(err,writtenBytes);

        err = bg2io_writeBlock(it,bg2io_PlistName);
        ASSERT_ERR(err,writtenBytes);

        err = bg2io_writeString(it,plist->name);
        ASSERT_ERR(err,writtenBytes);

        err = bg2io_writeBlock(it,bg2io_MatName);
        ASSERT_ERR(err,writtenBytes);

        err = bg2io_writeString(it,plist->matName);
        ASSERT_ERR(err,writtenBytes);

        err = writeBuffer(it, &plist->vertex,bg2io_VertexArray);
        ASSERT_ERR(err,writtenBytes);

        err = writeBuffer(it, &plist->normal,bg2io_NormalArray);
        ASSERT_ERR(err,writtenBytes);

        err = writeBuffer(it, &plist->texCoord0,bg2io_TexCoord0Array);
        ASSERT_ERR(err,writtenBytes);

        err = writeBuffer(it, &plist->texCoord1,bg2io_TexCoord1Array);
        ASSERT_ERR(err,writtenBytes);

        err = writeBuffer(it, &plist->texCoord2,bg2io_TexCoord2Array);
        ASSERT_ERR(err,writtenBytes);

        err = writeIntdexBuffer(it, &plist->index);
        ASSERT_ERR(err,writtenBytes);
    }

    err = bg2io_writeBlock(it,bg2io_End);
    ASSERT_ERR(err,writtenBytes);
    
    return writtenBytes;
}

Bg2ioSize writeBuffer(Bg2ioBufferIterator *it, Bg2ioFloatArray *arr, int blockType)
{
    Bg2ioSize err;
    Bg2ioSize totalBytes = 0;

    if (arr->length > 0)
    {
        err = bg2io_writeBlock(it, blockType);
        ASSERT_ERR(err, totalBytes);

        err = bg2io_writeFloatArray(it, arr->data, arr->length);
        ASSERT_ERR(err,totalBytes);
    }
    return totalBytes;
}

Bg2ioSize writeIntdexBuffer(Bg2ioBufferIterator *it, Bg2ioIntArray *arr)
{
    Bg2ioSize err;
    Bg2ioSize totalBytes = 0;

    if (arr->length > 0)
    {
        err = bg2io_writeBlock(it, bg2io_IndexArray);
        ASSERT_ERR(err, totalBytes);

        err = bg2io_writeIntArray(it, arr->data, arr->length);
        ASSERT_ERR(err,totalBytes);
    }
    return totalBytes;
}
