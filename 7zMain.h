/* 7z.h -- 7z interface
2010-03-11 : Igor Pavlov : Public domain */
#ifndef __7Z_Main
#define __7Z_Main


#include <stdio.h>
#include <string.h>

#include <direct.h>

#include "7z.h"
#include "7zAlloc.h"
#include "7zCrc.h"
#include "7zFile.h"
#include "7zVersion.h"

EXTERN_C_BEGIN

typedef struct
{

  CFileInStream archiveStream;
  CLookToRead lookStream;
  CSzArEx db;
  ISzAlloc allocImp;
  ISzAlloc allocTempImp;

  UInt32 blockIndex; /* it can have any value before first call (if outBuffer = 0) */
  Byte *outBuffer; /* it must be 0 before first call for each new archive. */
  size_t outBufferSize;  /* it can have any value before first call (if outBuffer = 0) */

  CBuf buf;

} szArchive;

typedef struct {
	char * FileName;
	int isDir;
} szretFile;

 // Exported Functions //
int szopen(szArchive *arc, const char *source);
void szClose(szArchive *arc);
void szextract(szArchive *arc, int fileItem, const char *dest);

static int szBuf_EnsureSize(CBuf *dest, size_t size);

static SRes szChar_To_UTF16(CBuf *buf, const char *s, int fileMode);
static SRes szUtf16_To_Char(CBuf *buf, const UInt16 *s, int fileMode);

static WRes szOutFile_OpenUtf16(CSzFile *p, const UInt16 *name);

UInt16* szmakeUInt(CBuf *buf, const char *s);
char* szmakeStr(CBuf *buf, const UInt16 *s);



szretFile szlist(szArchive *arc, int fileNum);


EXTERN_C_END

#endif
