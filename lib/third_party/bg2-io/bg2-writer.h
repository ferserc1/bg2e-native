#ifndef BG2_WRITER_H
#define BG2_WRITER_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif
Bg2ioSize bg2io_calculateBufferSize(Bg2File * file);

int bg2io_writeFileToBuffer(Bg2File * file, Bg2ioBuffer *dest);

#ifdef __cplusplus
}
#endif

#endif
