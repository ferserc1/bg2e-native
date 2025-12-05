#ifndef BG2IO_TYPES_H
#define BG2IO_TYPES_H

#include <stdlib.h>

#ifdef WIN32

#pragma warning( disable: 4251 )
#pragma warning( disable: 4275 )

#ifdef _WINDLL
#define BG2IO_EXPORT __declspec( dllexport )
#else
#define BG2IO_EXPORT __declspec( dllimport )
#endif

#else

#define BG2IO_EXPORT 

#endif

typedef unsigned char Bg2ioByte;
typedef Bg2ioByte * Bg2ioBytePtr;
typedef long long Bg2ioSize;

/**
 * @brief Represents a data buffer
 *
 */
typedef struct Bg2ioBufferT {
    Bg2ioBytePtr mem;           /* < the memory pointer to the buffer */
    Bg2ioSize length;           /* < is the size of the buffer that is filled with valid data. */
    Bg2ioSize actualLength;     /* < is the actual size of the buffer in memory, which may be larger than length. */
} Bg2ioBuffer;

enum Bg2ioBufferType {
    bg2io_BufferVertex          =  1,
    bg2io_BufferNormal          =  2,
    bg2io_BufferTexCoord0       =  3,
    bg2io_BufferTexCoord1       =  4,
    bg2io_BufferTexCoord2       =  5,
    bg2io_BufferIndex           = 10
};

/**
 * @brief It is important to initialize the Bg2ioBuffer structures before using 
 * them. To do this you can use the following macro, or do it manually.
 * 
 * Bg2ioBuffer b = BG2IO_BUFFER_INIT;
 * 
 * or
 * 
 * Bg2ioBuffer *b = malloc(sizeof(Bg2ioBuffer));
 * BG2IO_BUFFER_PTR_INIT(b);
 * 
 */
#define BG2IO_BUFFER_INIT { NULL, 0L, 0L }
#define BG2IO_BUFFER_PTR_INIT(b) b->mem = NULL; b->length = 0L; b->actualLength = 0L

/**
 * @brief Iterator of a buffer. Used to move through a buffer.
 * 
 */
typedef struct Bg2ioBufferIteratorT {
    Bg2ioBuffer * buffer;       /* < pointer to a Buffer structure in memory. */
    Bg2ioSize current;          /* < current position of the iterator.*/
} Bg2ioBufferIterator;

/**
 * @brief It is important to initialize the Bg2ioBufferIterator structures before using 
 * them. To do this you can use the following macro, or do it manually.
 * 
 * Bg2ioBuffer b = BG2IO_BUFFER_ITERATOR_INIT;
 * 
 * or
 * 
 * Bg2ioBufferIterator *b = malloc(sizeof(Bg2ioBufferIterator));
 * BG2IO_BUFFER_ITERATOR_PTR_INIT(b);
 * 
 */
#define BG2IO_BUFFER_ITERATOR_INIT { NULL, 0L }

/**
 * @brief Initialize a buffer iterator with a buffer pointer
 * 
 */
#define BG2IO_ITERATOR(b) { b, 0L }

/**
 * @brief Store an array of floating point 32 bits
 * 
 */
typedef struct Bg2ioFloatArrayT {
    int length;
    float * data;
} Bg2ioFloatArray;

/**
 * @brief Store an array of integer 32 bits.
 * 
 */
typedef struct Bg2ioIntArrayT {
    int length;
    int * data;
} Bg2ioIntArray;

/**
 * @brief Initialize a float or integer array.
 * 
 */
#define BG2IO_ARRAY_INIT { 0L, NULL }

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Reserves space to accommodate an array of 32-bit floating point elements.
 * 
 * @param array A pointer to the array structure
 * @param length The size of the array to be allocated
 */
int bg2io_allocateFloatArray(Bg2ioFloatArray * array, int length);

/**
 * @brief Release the memory of a float 32-bit array.
 * 
 * @param array  pointer to the array to be released
 */
int bg2io_freeFloatArray(Bg2ioFloatArray * array);

/**
 * @brief Reserves space to accommodate an array of 32-bit integer elements.
 * 
 * @param array A pointer to the array structure
 * @param length The size of the array to be allocated
 */
int bg2io_allocateIntArray(Bg2ioIntArray * array, int length);

/**
 * @brief Release the memory of an integer 32-bit array.
 * 
 * @param array  pointer to the array to be released
 */
int bg2io_freeIntArray(Bg2ioIntArray * array);

/**
 * @brief Store the information of a poly list
 * 
 */
typedef struct Bg2ioPolyListT {
    char * name;
    char * matName;
    char * groupName;
    int visible;
    Bg2ioFloatArray vertex;
    Bg2ioFloatArray normal;
    Bg2ioFloatArray texCoord0;
    Bg2ioFloatArray texCoord1;
    Bg2ioFloatArray texCoord2;
    Bg2ioIntArray index;
} Bg2ioPolyList;

/**
 * @brief Initialize a poly list struct
 * 
 */
#define BG2IO_POLY_LIST_INIT { NULL, NULL, NULL, 1, { NULL, 0 }, { NULL, 0 }, { NULL, 0 }, { NULL, 0 }, { NULL, 0 }, { NULL, 0 } }

/**
 * @brief Initialize a poly list ptr struct
 * 
 */
#define BG2IO_POLY_LIST_PTR_INIT(pl)    pl->name = NULL; pl->matName = NULL; pl->groupName = NULL; pl->visible = 1; \
                                        pl->vertex.data = NULL; pl->vertex.length = 0; \
                                        pl->normal.data = NULL; pl->normal.length = 0; \
                                        pl->texCoord0.data = NULL; pl->texCoord0.length = 0; \
                                        pl->texCoord1.data = NULL; pl->texCoord1.length = 0; \
                                        pl->texCoord2.data = NULL; pl->texCoord2.length = 0; \
                                        pl->index.data = NULL; pl->index.length = 0;

/**
 * @brief create a poly list struct
 * 
 * @return Bg2ioPolyList* a pointer to the poly list
 */
Bg2ioPolyList * bg2io_createPolyList(void);

/**
 * @brief Release a poly list struct
 * 
 * @param plist pointer to the poly list to be released
 */
int bg2io_freePolyList(Bg2ioPolyList * plist);

/**
 * @brief Store an array of poly list elements
 * 
 */
typedef struct Bg2ioPolyListArrayT {
    int length;
    Bg2ioPolyList ** data;
} Bg2ioPolyListArray;

/**
 * @brief Initialzie a poly list array
 * 
 */
#define BG2IO_POLY_LIST_ARRAY_INIT { 0, NULL }

/**
 * @brief Initialize a poly list array pointer
 * 
 */
#define BG2IO_POLY_LIST_ARRAY_PTR_INIT(pl) pl->data = NULL; pl->length = 0

/**
 * @brief Create an array of poly list elements
 * 
 * @param length number of poly list
 * @return Bg2ioPolyListArray* pointer to the poly list array structure
 */
Bg2ioPolyListArray * bg2io_createPolyListArray(int length);

/**
 * @brief Release a poly list array, and all its contents
 * 
 * @param arr pointer to the poly list
 * @return int error code
 */
int bg2io_freePolyListArray(Bg2ioPolyListArray * arr);

/**
 * @brief Store the header metadat of a bg2 file 
 * 
 */
typedef struct Bg2FileHeaderT {
    unsigned char endianess;
    unsigned char majorVersion;
    unsigned char minorVersion;
    unsigned char revision;
    int numberOfPolyList;
} Bg2FileHeader;

/**
 * @brief Stores the information from a bg2 file
 * 
 */
typedef struct Bg2FileT {
    Bg2FileHeader header;
    Bg2ioPolyListArray * plists;
    char * name;
    char * componentData;
    char * materialData;
    char * jointData;
} Bg2File;

/**
 * @brief Initialize a Bg2File struct
 * 
 */
#define BG2IO_BG2_FILE_INIT { { 0, { 0, 0, 0, 0 }}, NULL, NULL, NULL, NULL, NULL }

/**
 * @brief Initialize a Bg2File struct pointer
 * 
 */
#define BG2IO_BG2_FILE_PTR_INIT(f) f->header.endianess = 0; f->header.majorVersion = 0; f->header.minorVersion = 0; f->header.revision = 0; f->header.numberOfPolyList = 0; \
                                   f->plists = NULL; f->name = NULL; f->componentData = NULL; f->materialData = NULL; f->jointData = NULL

/**
 * @brief Create an empty Bg2File struct
 * 
 * @return Bg2File* 
 */
Bg2File * bg2io_createBg2File(void);

/**
 * @brief Release a bg2 file struct and all its contents
 * 
 * @return int BG2IO_NO_ERROR if success, BG2IO_ERR_INVALID_PTR if the pointer to the
 * structure is NULL.
 */
int bg2io_freeBg2File(Bg2File * file);

#ifdef __cplusplus
}
#endif

#endif
