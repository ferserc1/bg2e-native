
#include "buffer-io.h"
#include "errors.h"
#include "utils.h"
#include "buffer-memory.h"

#include <string.h>

Bg2ioSize getStringSize(const char * str)
{
    Bg2ioSize size = 0;
    char ch = str[size];
    while (ch != '\0')
    {
        ch = str[++size];
    }
    return size;
}

Bg2ioSize checkIterator(Bg2ioBufferIterator *it, Bg2ioSize requiredBytes)
{
    if (it == NULL || it->buffer == NULL || it->buffer->mem == NULL)
	{
		return BG2IO_ERR_INVALID_PTR;
	}
	else if (it->current >= it->buffer->length)
	{
		return BG2IO_ERR_ITERATOR_OUT_OF_BOUNDS;
	}
	else if ((it->current + requiredBytes) > it->buffer->length)
	{
		return BG2IO_ERR_INSUFFICIENT_LENGTH;
	}
    else
    {
        return BG2IO_NO_ERROR;
    }
}

void checkAppendDataLength(Bg2ioBufferIterator *it, Bg2ioSize requiredSize)
{
    Bg2ioSize requiredTotalLength = it->current + requiredSize;
    if (requiredTotalLength > it->buffer->length)
    {
        bg2io_reserveBuffer(it->buffer, requiredTotalLength);
    }
}

int bg2io_isValidBlock(int block)
{
    switch (block) {
    case bg2io_Header:
    case bg2io_PolyList:
    case bg2io_VertexArray:
    case bg2io_NormalArray:
    case bg2io_TexCoord0Array:
    case bg2io_TexCoord1Array:
    case bg2io_TexCoord2Array:
    case bg2io_TexCoord3Array:
    case bg2io_TexCoord4Array:
    case bg2io_IndexArray:
    case bg2io_Materials:
    case bg2io_PlistName:
    case bg2io_MatName:
    case bg2io_ShadowProjector:
    case bg2io_Joint:
    case bg2io_End:
    case bg2io_Components:
        return 1;
    default:
        return 0;
    }
}

Bg2ioSize bg2io_readByte(Bg2ioBufferIterator *it, unsigned char *out)
{
    Bg2ioSize error = checkIterator(it, sizeof(Bg2ioByte));
    if (error != BG2IO_NO_ERROR)
    {
        return error;
    }
    else if (out == NULL)
    {
        return BG2IO_ERR_INVALID_OUT_PARAM_PTR;
    }

	long long cur = it->current;
	*out = it->buffer->mem[cur];
	it->current += sizeof(Bg2ioByte);
    return it->buffer->length - it->current; 
}

unsigned int bg2io_readBlock(Bg2ioBufferIterator *it)
{
    int block = 0;
    Bg2ioSize error = bg2io_readInteger(it, &block);
    if (error < 0)
    {
        return (unsigned int) error;
    }

    if (bg2io_isValidBlock(block))
    {
        return block;
    }
    else
    {
        it->current -= sizeof(int);
        return BG2IO_ERR_INVALID_BLOCK;
    }
}

Bg2ioSize bg2io_readInteger(Bg2ioBufferIterator *it, int *out)
{
    Bg2ioSize readed = checkIterator(it, sizeof(int));
    if (readed != BG2IO_NO_ERROR)
    {
        return readed;
    }
    else if (out == NULL)
    {
        return BG2IO_ERR_INVALID_OUT_PARAM_PTR;
    }

    if (bg2io_isBigEndian())
    {
        memcpy((void *)out, it->buffer->mem, sizeof(int));
    }
    else
    {
        union {
            int integer;
            char byte[4];
        } r_value;
        long long cur = it->current;
        r_value.byte[3] = it->buffer->mem[cur];
        r_value.byte[2] = it->buffer->mem[cur + 1];
        r_value.byte[1] = it->buffer->mem[cur + 2];
        r_value.byte[0] = it->buffer->mem[cur + 3];
        *out = r_value.integer;
    }
    it->current += sizeof(int);
    readed = it->buffer->length - it->current;
    return readed;
}

Bg2ioSize bg2io_readFloat(Bg2ioBufferIterator *it, float *out)
{
    Bg2ioSize readed = checkIterator(it, sizeof(int));
    if (readed != BG2IO_NO_ERROR)
    {
        return readed;
    }
    else if (out == NULL)
    {
        return BG2IO_ERR_INVALID_OUT_PARAM_PTR;
    }

     if (bg2io_isBigEndian())
    {
        memcpy((void *)out, it->buffer->mem, sizeof(float));
    }
    else
    {
        union {
            float number;
            char byte[4];
        } r_value;
        long long cur = it->current;
        r_value.byte[3] = it->buffer->mem[cur];
        r_value.byte[2] = it->buffer->mem[cur + 1];
        r_value.byte[1] = it->buffer->mem[cur + 2];
        r_value.byte[0] = it->buffer->mem[cur + 3];
        *out = r_value.number;
    }
    it->current += sizeof(int);
    readed = it->buffer->length - it->current;
    return readed;
}

Bg2ioSize bg2io_readString(Bg2ioBufferIterator *it, char **out)
{
    int stringSize = 0;
    Bg2ioSize remaining = bg2io_readInteger(it, &stringSize);
    if (remaining < 0)
    {
        return remaining;
    }

    unsigned char * readedBytes = (unsigned char*) malloc(sizeof(unsigned char) * (stringSize + 1));
    int i;
    for (i = 0; i < stringSize; ++i)
    {
        remaining = bg2io_readByte(it, &readedBytes[i]);
    }
    readedBytes[i] = '\0';
    *out = (char*) readedBytes;
    return stringSize;
}

Bg2ioSize bg2io_readFloatArray(Bg2ioBufferIterator *it, float **out)
{
    int arraySize = 0;
    Bg2ioSize remaining = bg2io_readInteger(it, &arraySize);
    if (remaining < 0)
    {
        return remaining;
    }

    if (arraySize > 0) 
    {    
        float * readedFloats = (float*) malloc(sizeof(float) * arraySize);
        int i;
        for (i = 0; i < arraySize; ++i)
        {
            remaining = bg2io_readFloat(it, &readedFloats[i]);
        }
        *out = readedFloats;
    }
    return arraySize;
}

Bg2ioSize bg2io_readIntArray(Bg2ioBufferIterator *it, int **out)
{
    int arraySize = 0;
    Bg2ioSize remaining = bg2io_readInteger(it, &arraySize);
    if (remaining < 0)
    {
        return remaining;
    }

    if (arraySize > 0)
    {
        int * readedInts = (int*) malloc(sizeof(float) * arraySize);
        int i;
        for (i = 0; i < arraySize; ++i)
        {
            remaining = bg2io_readInteger(it, &readedInts[i]);
        }
        *out = readedInts;
    }
    return arraySize;
}

Bg2ioSize bg2io_writeByte(Bg2ioBufferIterator *it, const unsigned char in)
{
    if (it == NULL || it->buffer == NULL)
    {
        return BG2IO_ERR_INVALID_PTR;
    }
    else if (it->current > it->buffer->length)
    {
        return BG2IO_ERR_ITERATOR_OUT_OF_BOUNDS;
    }

    Bg2ioSize increment = sizeof(unsigned char);
    checkAppendDataLength(it, increment);
    it->buffer->mem[it->current] = in;
    it->current += increment;
    return increment;
}

Bg2ioSize bg2io_writeBlock(Bg2ioBufferIterator *it, int block)
{
    if (it == NULL || it->buffer == NULL)
    {
        return BG2IO_ERR_INVALID_PTR;
    }
    else if (it->current > it->buffer->length)
    {
        return BG2IO_ERR_ITERATOR_OUT_OF_BOUNDS;
    }
    else if (bg2io_isValidBlock(block) == 0)
    {
        return BG2IO_ERR_INVALID_BLOCK;
    }

    Bg2ioSize size = bg2io_writeInteger(it, block);
    return size;
}

Bg2ioSize bg2io_writeInteger(Bg2ioBufferIterator *it, int in)
{
    if (it == NULL || it->buffer == NULL)
    {
        return BG2IO_ERR_INVALID_PTR;
    }
    else if (it->current > it->buffer->length) {
        return BG2IO_ERR_ITERATOR_OUT_OF_BOUNDS;
    }

    Bg2ioSize increment = sizeof(int);
    checkAppendDataLength(it, increment);
    
    if (bg2io_isBigEndian())
    {
        memcpy((void*)&it->buffer->mem[it->current], (void*) &in, increment);
    }
    else
    {
        union {
            int integer;
            unsigned char bytes[4];
        } r_value;
        r_value.integer = in;
        it->buffer->mem[it->current] = r_value.bytes[3];
        it->buffer->mem[it->current + 1] = r_value.bytes[2];
        it->buffer->mem[it->current + 2] = r_value.bytes[1];
        it->buffer->mem[it->current + 3] = r_value.bytes[0];
    }

    it->current += increment;
    return increment;
}

Bg2ioSize bg2io_writeFloat(Bg2ioBufferIterator *it, float in)
{
    if (it == NULL || it->buffer == NULL)
    {
        return BG2IO_ERR_INVALID_PTR;
    }
    else if (it->current > it->buffer->length) {
        return BG2IO_ERR_ITERATOR_OUT_OF_BOUNDS;
    }

    Bg2ioSize increment = sizeof(float);
    checkAppendDataLength(it, increment);
    
    if (bg2io_isBigEndian())
    {
        memcpy((void*)&it->buffer->mem[it->current], (void*) &in, increment);
    }
    else
    {
        union {
            float number;
            unsigned char bytes[4];
        } r_value;
        r_value.number = in;
        it->buffer->mem[it->current] = r_value.bytes[3];
        it->buffer->mem[it->current + 1] = r_value.bytes[2];
        it->buffer->mem[it->current + 2] = r_value.bytes[1];
        it->buffer->mem[it->current + 3] = r_value.bytes[0];
    }

    it->current += increment;
    return increment;
}

Bg2ioSize bg2io_writeString(Bg2ioBufferIterator *it, const char * in)
{
    if (it == NULL || it->buffer == NULL)
    {
        return BG2IO_ERR_INVALID_PTR;
    }
    else if (it->current > it->buffer->length)
    {
        return BG2IO_ERR_ITERATOR_OUT_OF_BOUNDS;
    }

    Bg2ioSize stringSize = getStringSize(in);
    Bg2ioSize totalSize = stringSize;
    Bg2ioSize written = bg2io_writeInteger(it, (int) totalSize);

    for (int i = 0; i < stringSize; ++i)
    {
        written += bg2io_writeByte(it, in[i]);
    }

    return written;
}

Bg2ioSize bg2io_writeFloatArray(Bg2ioBufferIterator *it, const float * in, Bg2ioSize length)
{
    if (it == NULL || it->buffer == NULL)
    {
        return BG2IO_ERR_INVALID_PTR;
    }
    else if (it->current > it->buffer->length)
    {
        return BG2IO_ERR_ITERATOR_OUT_OF_BOUNDS;
    }

    Bg2ioSize written = bg2io_writeInteger(it, (int) length);

    for (int i = 0; i < length; ++i)
    {
        written += bg2io_writeFloat(it, in[i]);
    }

    return written;
}

Bg2ioSize bg2io_writeIntArray(Bg2ioBufferIterator *it, const int * in, Bg2ioSize length)
{
    if (it == NULL || it->buffer == NULL)
    {
        return BG2IO_ERR_INVALID_PTR;
    }
    else if (it->current > it->buffer->length)
    {
        return BG2IO_ERR_ITERATOR_OUT_OF_BOUNDS;
    }

    Bg2ioSize written = bg2io_writeInteger(it, (int) length);

    for (int i = 0; i < length; ++i)
    {
        written += bg2io_writeInteger(it, in[i]);
    }

    return written;
}

