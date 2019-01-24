/* 7zMain.c - Test application for 7z Decoder
2010-10-28 : Igor Pavlov : Public domain */

#include <stdio.h>
#include <string.h>

#include <direct.h>

#include "7zMain.h"
#include "7z.h"
#include "7zAlloc.h"
#include "7zCrc.h"
#include "7zFile.h"
#include "7zVersion.h"

const TCHAR szModuleName[] = TEXT( "FreeStyleDashPlugin.xex" );

static ISzAlloc g_Alloc = { SzAlloc, SzFree };

static int szBuf_EnsureSize(CBuf *dest, size_t size)
{
  if (dest->size >= size)
    return 1;
  Buf_Free(dest, &g_Alloc);
  return Buf_Create(dest, size, &g_Alloc);
}

static SRes szUtf16_To_Char(CBuf *buf, const UInt16 *s, int fileMode)
{
  int len = 0;
  for (len = 0; s[len] != '\0'; len++);

  #ifdef _WIN32
  {
    int size = len * 3 + 100;
    if (!szBuf_EnsureSize(buf, size))
      return SZ_ERROR_MEM;
    {
      BOOL defUsed;
      int numChars = WideCharToMultiByte(CP_ACP, 0, (LPCWSTR)s, len, (char *)buf->data, size, NULL, &defUsed);
      if (numChars == 0 || numChars >= size)
        return SZ_ERROR_FAIL;
      buf->data[numChars] = 0;
      return SZ_OK;
    }
  }
  #else
  fileMode = fileMode;
  return Utf16_To_Utf8Buf(buf, s, len);
  #endif
}

static SRes szChar_To_UTF16(CBuf *buf, const char *s, int fileMode)
{
  int len = strlen(s);

  #ifdef _WIN32
  {
    int size = len *3 + 100;
    if (!szBuf_EnsureSize(buf, size))
      return SZ_ERROR_MEM;
    {
      MultiByteToWideChar(CP_ACP, 0, s, -1, (LPWSTR)buf->data, size);
      return SZ_OK;
    }
  }
  #else
  fileMode = fileMode;
  return Utf16_To_Utf8Buf(buf, s, len);
  #endif
}


static WRes szOutFile_OpenUtf16(CSzFile *p, const UInt16 *name)
{
  CBuf buf;
  WRes res;
  Buf_Init(&buf);
  RINOK(szUtf16_To_Char(&buf, name, 1));
  res = OutFile_Open(p, (const char *)buf.data);
  Buf_Free(&buf, &g_Alloc);
  return res;
}

 UInt16* szmakeUInt(CBuf *buf, const char *s)
{
  SRes res;
  UInt16* out = (UInt16*)"";

  res = szChar_To_UTF16(buf, s, 0);
  if (res == SZ_OK)
	  out = (UInt16*)buf->data;
  return out;
}

char* szmakeStr(CBuf *buf, const UInt16 *s)
{
  SRes res;
  char* out = "";

  res = szUtf16_To_Char(buf, s, 0);
  if (res == SZ_OK)
	  out = (char*)buf->data;
  return out;
}

int szopen(szArchive* arc, const char* source)
{
  SRes res;

  arc->allocImp.Alloc = SzAlloc;
  arc->allocImp.Alloc = SzAlloc;
  arc->allocImp.Free = SzFree;

  arc->allocTempImp.Alloc = SzAllocTemp;
  arc->allocTempImp.Free = SzFreeTemp;

  if (InFile_Open(&arc->archiveStream.file, source))
  {
    return 1;
  }

  FileInStream_CreateVTable(&arc->archiveStream);
  LookToRead_CreateVTable(&arc->lookStream, False);
  
  arc->lookStream.realStream = &arc->archiveStream.s;
  LookToRead_Init(&arc->lookStream);

  CrcGenerateTable();

  SzArEx_Init(&arc->db);
  res = SzArEx_Open(&arc->db, &arc->lookStream.s, &arc->allocImp, &arc->allocTempImp);
  if (res == SZ_OK)
  {
	  /*
      if you need cache, use these 3 variables.
      if you use external function, you can make these variable as static.
      */
      arc->blockIndex = 0xFFFFFFFF; /* it can have any value before first call (if outBuffer = 0) */
      arc->outBuffer = 0; /* it must be 0 before first call for each new archive. */
      arc->outBufferSize = 0;  /* it can have any value before first call (if outBuffer = 0) */
	  return arc->db.db.NumFiles;
	
  }
  return 0;
}

szretFile szlist(szArchive* arc, int fileNum) {
	
	szretFile retVal;

	UInt16 *temp = NULL;
	size_t tempSize = 0;
	
	const CSzFileItem *f = arc->db.db.Files + fileNum;
    size_t len;
    Buf_Init(&arc->buf);    
    len = SzArEx_GetFileNameUtf16(&arc->db, fileNum, NULL);

    if (len > tempSize)
    {
		SzFree(NULL, temp);
        tempSize = len;
        temp = (UInt16 *)SzAlloc(NULL, tempSize * sizeof(temp[0]));
        if (temp == 0)
        {
			retVal.FileName = "";
			retVal.isDir = 0;
			return retVal;
        }
    }

    SzArEx_GetFileNameUtf16(&arc->db, fileNum, temp);

	if (f->IsDir) {
		retVal.isDir = 1;
	} else {
		retVal.isDir = 0;
	}

	retVal.FileName = szmakeStr(&arc->buf, temp);
	SzFree(NULL, temp);
	return retVal;
}

void szextract(szArchive *arc, int fileItem, const char* dest)
{
    size_t offset = 0;
    size_t outSizeProcessed = 0;
	size_t processedSize;
	CSzFile outFile;
	UInt16* destPath;
   
	SRes res;

	res = SzArEx_Extract(&arc->db, &arc->lookStream.s, fileItem, &arc->blockIndex,
		&arc->outBuffer, &arc->outBufferSize, &offset, &outSizeProcessed, &arc->allocImp, &arc->allocTempImp);

    
	Buf_Init(&arc->buf);
	destPath = szmakeUInt(&arc->buf, dest);
	if (!szOutFile_OpenUtf16(&outFile, destPath))
    {
		processedSize = outSizeProcessed;
        File_Write(&outFile, arc->outBuffer + offset, &processedSize);
        File_Close(&outFile);
    }
}

void szClose(szArchive *arc) {
	
	Buf_Free(&arc->buf, &g_Alloc);
	IAlloc_Free(&arc->allocImp, arc->outBuffer);
	SzArEx_Free(&arc->db, &arc->allocImp);
	File_Close(&arc->archiveStream.file);
}
