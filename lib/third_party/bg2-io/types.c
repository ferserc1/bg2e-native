
#include "types.h"
#include "errors.h"


int bg2io_allocateFloatArray(Bg2ioFloatArray * array, int length)
{
    if (array == NULL)
    {
        return BG2IO_ERR_INVALID_PTR;
    }
    else if (length <= 0) {
        return BG2IO_ERR_INVALID_LENGTH;
    }

    array->data = (float*) malloc(sizeof(float) * length);
    array->length = length;

    return BG2IO_NO_ERROR;
}

int bg2io_freeFloatArray(Bg2ioFloatArray * array)
{
    if (array == NULL) {
        return BG2IO_ERR_INVALID_PTR;
    }

    if (array->data != NULL)
    {
        free(array->data);
        array->data = NULL;
        array->length = 0;
    }

    return BG2IO_NO_ERROR;
}

int bg2io_allocateIntArray(Bg2ioIntArray * array, int length)
{
    if (array == NULL)
    {
        return BG2IO_ERR_INVALID_PTR;
    }
    else if (length <= 0) {
        return BG2IO_ERR_INVALID_LENGTH;
    }

    array->data = (int*) malloc(sizeof(int) * length);
    array->length = length;

    return BG2IO_NO_ERROR;
}

int bg2io_freeIntArray(Bg2ioIntArray * array)
{
    if (array == NULL) {
        return BG2IO_ERR_INVALID_PTR;
    }

    if (array->data != NULL)
    {
        free(array->data);
        array->data = NULL;
        array->length = 0;
    }

    return BG2IO_NO_ERROR;
}

Bg2ioPolyList * bg2io_createPolyList(void)
{
    Bg2ioPolyList * result = (Bg2ioPolyList*) malloc(sizeof(Bg2ioPolyList));
    BG2IO_POLY_LIST_PTR_INIT(result);
    return result;
}

int bg2io_freePolyList(Bg2ioPolyList * plist)
{
    if (plist == NULL)
    {
        return BG2IO_ERR_INVALID_PTR;
    }
    int error = BG2IO_NO_ERROR;

    if (plist->name != NULL)
    {
        free(plist->name);
        plist->name = NULL;
    }

    if (plist->groupName != NULL)
    {
        free(plist->groupName);
        plist->groupName = NULL;  
    }

    if (plist->matName != NULL)
    {
        free(plist->matName);
        plist->matName = NULL;
    }

    bg2io_freeFloatArray(&plist->vertex);
    bg2io_freeFloatArray(&plist->normal);
    bg2io_freeFloatArray(&plist->texCoord0);
    bg2io_freeFloatArray(&plist->texCoord1);
    bg2io_freeFloatArray(&plist->texCoord2);
    bg2io_freeIntArray(&plist->index);

    return error;
}

Bg2ioPolyListArray * bg2io_createPolyListArray(int length)
{
    Bg2ioPolyListArray * result = NULL;

    if (length > 0)
    {
        result = (Bg2ioPolyListArray*) malloc(sizeof(Bg2ioPolyListArray));
        BG2IO_POLY_LIST_ARRAY_PTR_INIT(result);

        result->data = (Bg2ioPolyList**) malloc(sizeof(Bg2ioPolyList) * length);
        for (int i = 0; i < length; ++i)
        {
            result->data[i] = bg2io_createPolyList();
        }
        result->length = length;
    }

    return result;
}

int bg2io_freePolyListArray(Bg2ioPolyListArray * arr)
{
    if (arr == NULL)
    {
        return BG2IO_ERR_INVALID_PTR;
    }

    if (arr->data != NULL)
    {
        for (int i = 0; i < arr->length; ++i)
        {
            bg2io_freePolyList(arr->data[i]);
        }
        free(arr->data);
        arr->data = NULL;
        arr->length = 0;
    }
    
    return BG2IO_NO_ERROR;
}

Bg2File * bg2io_createBg2File(void)
{
    Bg2File * result = (Bg2File*) malloc(sizeof(Bg2File));
    BG2IO_BG2_FILE_PTR_INIT(result);
    return result;
}

int bg2io_freeBg2File(Bg2File * file)
{
    if (file == NULL)
    {
        return BG2IO_ERR_INVALID_PTR;
    }

    if (file->plists)
    {
        bg2io_freePolyListArray(file->plists);
        file->plists = NULL;
    }

    if (file->name)
    {
        free(file->name);
        file->name = NULL;
    }

    if (file->componentData)
    {
        free(file->componentData);
        file->componentData = NULL;
    }

    if (file->materialData != NULL)
    {
        free(file->materialData);
        file->materialData = NULL;
    }

    if (file->jointData)
    {
        free(file->jointData);
        file->jointData = NULL;
    }

    return BG2IO_NO_ERROR;
}
