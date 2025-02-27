/*  $Id$
 *
 *  This file is part of the OpenLink Software Virtuoso Open-Source (VOS)
 *  project.
 *
 *  Copyright (C) 2011-2022 OpenLink Software
 *
 *  The file is a modified version of source file of the Shapelib library.
 *  The original version was obtained via http://shapelib.maptools.org
 *  Authors of the original version are not responsible for possible errors
 *  in this modified version.
 *  The original copyright and licensing info is as follows:
 */

/******************************************************************************
 * Original Id: dbfopen.c,v 1.89 2011-07-24 05:59:25 fwarmerdam Exp $
 *
 * Project:  Shapelib
 * Purpose:  Implementation of .dbf access API documented in dbf_api.html.
 * Author:   Frank Warmerdam, warmerdam@pobox.com
 *
 ******************************************************************************
 * Copyright (c) 1999, Frank Warmerdam
 *
 * This software is available under the following "MIT Style" license,
 * or at the option of the licensee under the LGPL (see LICENSE.LGPL).  This
 * option is discussed in more detail in shapelib.html.
 *
 * --
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 ******************************************************************************/

#include "shapefil.h"

#include <math.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

SHP_CVSID("$Id$")

#ifndef FALSE
#  define FALSE		0
#  define TRUE		1
#endif

/************************************************************************/
/*                             SfRealloc()                              */
/*                                                                      */
/*      A realloc cover function that will access a NULL pointer as     */
/*      a valid input.                                                  */
/************************************************************************/

static void * SfRealloc( void * pMem, int nNewSize )

{
    if( pMem == NULL )
        return( (void *) dk_alloc (nNewSize) );
    else
      {
        dk_free (pMem, -1);
        return( (void *) dk_alloc (nNewSize) );
      }
}

/************************************************************************/
/*                           DBFWriteHeader()                           */
/*                                                                      */
/*      This is called to write out the file header, and field          */
/*      descriptions before writing any actual data records.  This      */
/*      also computes all the DBFDataSet field offset/size/decimals     */
/*      and so forth values.                                            */
/************************************************************************/

static void DBFWriteHeader(DBFHandle psDBF)

{
    unsigned char	abyHeader[XBASE_FLDHDR_SZ];
    int		i;

    if( !psDBF->bNoHeader )
        return;

    psDBF->bNoHeader = FALSE;

/* -------------------------------------------------------------------- */
/*	Initialize the file header information.				*/
/* -------------------------------------------------------------------- */
    for( i = 0; i < XBASE_FLDHDR_SZ; i++ )
        abyHeader[i] = 0;

    abyHeader[0] = 0x03;		/* memo field? - just copying 	*/

    /* write out a dummy date */
    abyHeader[1] = 95;			/* YY */
    abyHeader[2] = 7;			/* MM */
    abyHeader[3] = 26;			/* DD */

    /* record count preset at zero */

    abyHeader[8] = (unsigned char) (psDBF->nHeaderLength % 256);
    abyHeader[9] = (unsigned char) (psDBF->nHeaderLength / 256);
    
    abyHeader[10] = (unsigned char) (psDBF->nRecordLength % 256);
    abyHeader[11] = (unsigned char) (psDBF->nRecordLength / 256);

    abyHeader[29] = (unsigned char) (psDBF->iLanguageDriver);

/* -------------------------------------------------------------------- */
/*      Write the initial 32 byte file header, and all the field        */
/*      descriptions.                                     		*/
/* -------------------------------------------------------------------- */
    psDBF->sHooks.FSeek( psDBF->fp, 0, 0 );
    psDBF->sHooks.FWrite( abyHeader, XBASE_FLDHDR_SZ, 1, psDBF->fp );
    psDBF->sHooks.FWrite( psDBF->pszHeader, XBASE_FLDHDR_SZ, psDBF->nFields, 
                          psDBF->fp );

/* -------------------------------------------------------------------- */
/*      Write out the newline character if there is room for it.        */
/* -------------------------------------------------------------------- */
    if( psDBF->nHeaderLength > 32*psDBF->nFields + 32 )
    {
        char	cNewline;

        cNewline = 0x0d;
        psDBF->sHooks.FWrite( &cNewline, 1, 1, psDBF->fp );
    }
}

/************************************************************************/
/*                           DBFFlushRecord()                           */
/*                                                                      */
/*      Write out the current record if there is one.                   */
/************************************************************************/

static int DBFFlushRecord( DBFHandle psDBF )

{
    SAOffset	nRecordOffset;

    if( psDBF->bCurrentRecordModified && psDBF->nCurrentRecord > -1 )
    {
	psDBF->bCurrentRecordModified = FALSE;

	nRecordOffset = 
            psDBF->nRecordLength * (SAOffset) psDBF->nCurrentRecord 
            + psDBF->nHeaderLength;

	if( psDBF->sHooks.FSeek( psDBF->fp, nRecordOffset, 0 ) != 0 
            || psDBF->sHooks.FWrite( psDBF->pszCurrentRecord, 
                                     psDBF->nRecordLength, 
                                     1, psDBF->fp ) != 1 )
        {
            char szMessage[128];
            sprintf( szMessage, "Failure writing DBF record %d.", 
                     psDBF->nCurrentRecord );
            psDBF->sHooks.Error( szMessage );
            return FALSE;
        }
    }

    return TRUE;
}

/************************************************************************/
/*                           DBFLoadRecord()                            */
/************************************************************************/

static int DBFLoadRecord( DBFHandle psDBF, int iRecord )

{
    if( psDBF->nCurrentRecord != iRecord )
    {
        SAOffset nRecordOffset;

	if( !DBFFlushRecord( psDBF ) )
            return FALSE;

	nRecordOffset = 
            psDBF->nRecordLength * (SAOffset) iRecord + psDBF->nHeaderLength;

	if( psDBF->sHooks.FSeek( psDBF->fp, nRecordOffset, SEEK_SET ) != 0 )
        {
            char szMessage[128];
            sprintf( szMessage, "fseek(%ld) failed on DBF file.\n",
                     (long) nRecordOffset );
            psDBF->sHooks.Error( szMessage );
            return FALSE;
        }

	if( psDBF->sHooks.FRead( psDBF->pszCurrentRecord, 
                                 psDBF->nRecordLength, 1, psDBF->fp ) != 1 )
        {
            char szMessage[128];
            sprintf( szMessage, "fread(%d) failed on DBF file.\n",
                     psDBF->nRecordLength );
            psDBF->sHooks.Error( szMessage );
            return FALSE;
        }

	psDBF->nCurrentRecord = iRecord;
    }

    return TRUE;
}

/************************************************************************/
/*                          DBFUpdateHeader()                           */
/************************************************************************/

void SHPAPI_CALL
DBFUpdateHeader( DBFHandle psDBF )

{
    unsigned char		abyFileHeader[32];

    if( psDBF->bNoHeader )
        DBFWriteHeader( psDBF );

    DBFFlushRecord( psDBF );

    psDBF->sHooks.FSeek( psDBF->fp, 0, 0 );
    psDBF->sHooks.FRead( abyFileHeader, 32, 1, psDBF->fp );
    
    abyFileHeader[4] = (unsigned char) (psDBF->nRecords % 256);
    abyFileHeader[5] = (unsigned char) ((psDBF->nRecords/256) % 256);
    abyFileHeader[6] = (unsigned char) ((psDBF->nRecords/(256*256)) % 256);
    abyFileHeader[7] = (unsigned char) ((psDBF->nRecords/(256*256*256)) % 256);
    
    psDBF->sHooks.FSeek( psDBF->fp, 0, 0 );
    psDBF->sHooks.FWrite( abyFileHeader, 32, 1, psDBF->fp );

    psDBF->sHooks.FFlush( psDBF->fp );
}

/************************************************************************/
/*                              DBFOpen()                               */
/*                                                                      */
/*      Open a .dbf file.                                               */
/************************************************************************/
   
DBFHandle SHPAPI_CALL
DBFOpen( const char * pszFilename, const char * pszAccess )

{
    SAHooks sHooks;

    SASetupDefaultHooks( &sHooks );

    return DBFOpenLL( pszFilename, pszAccess, &sHooks );
}

/************************************************************************/
/*                              DBFOpen()                               */
/*                                                                      */
/*      Open a .dbf file.                                               */
/************************************************************************/
   
DBFHandle SHPAPI_CALL
DBFOpenLL( const char * pszFilename, const char * pszAccess, SAHooks *psHooks )

{
    DBFHandle		psDBF;
    SAFile		pfCPG;
    unsigned char	*pabyBuf;
    int			nFields, nHeadLen, iField, i;
    char		*pszBasename, *pszFullname;
    int                 nBufSize = 500;

/* -------------------------------------------------------------------- */
/*      We only allow the access strings "rb" and "r+".                  */
/* -------------------------------------------------------------------- */
    if( strcmp(pszAccess,"r") != 0 && strcmp(pszAccess,"r+") != 0 
        && strcmp(pszAccess,"rb") != 0 && strcmp(pszAccess,"rb+") != 0
        && strcmp(pszAccess,"r+b") != 0 )
        return( NULL );

    if( strcmp(pszAccess,"r") == 0 )
        pszAccess = "rb";
 
    if( strcmp(pszAccess,"r+") == 0 )
        pszAccess = "rb+";

/* -------------------------------------------------------------------- */
/*	Compute the base (layer) name.  If there is any extension	*/
/*	on the passed in filename we will strip it off.			*/
/* -------------------------------------------------------------------- */
    pszBasename = (char *) dk_alloc(strlen(pszFilename)+5);
    strcpy( pszBasename, pszFilename );
    for( i = strlen(pszBasename)-1; 
	 i > 0 && pszBasename[i] != '.' && pszBasename[i] != '/'
	       && pszBasename[i] != '\\';
	 i-- ) {}

    if( pszBasename[i] == '.' )
        pszBasename[i] = '\0';

    pszFullname = (char *) dk_alloc(strlen(pszBasename) + 5);
    sprintf( pszFullname, "%s.dbf", pszBasename );
        
    psDBF = (DBFHandle) dk_alloc (sizeof(DBFInfo));
    memset (psDBF, 0, sizeof(DBFInfo));
    psDBF->fp = psHooks->FOpen( pszFullname, pszAccess );
    memcpy( &(psDBF->sHooks), psHooks, sizeof(SAHooks) );

    if( psDBF->fp == NULL )
    {
        sprintf( pszFullname, "%s.DBF", pszBasename );
        psDBF->fp = psDBF->sHooks.FOpen(pszFullname, pszAccess );
    }

    sprintf( pszFullname, "%s.cpg", pszBasename );
    pfCPG = psHooks->FOpen( pszFullname, "r" );
    if( pfCPG == NULL )
    {
        sprintf( pszFullname, "%s.CPG", pszBasename );
        pfCPG = psHooks->FOpen( pszFullname, "r" );
    }

    dk_free ( pszBasename, -1 );
    dk_free ( pszFullname, -1 );
    
    if( psDBF->fp == NULL )
    {
        dk_free ( psDBF, -1 );
        if( pfCPG ) psHooks->FClose( pfCPG );
        return( NULL );
    }

    psDBF->bNoHeader = FALSE;
    psDBF->nCurrentRecord = -1;
    psDBF->bCurrentRecordModified = FALSE;

/* -------------------------------------------------------------------- */
/*  Read Table Header info                                              */
/* -------------------------------------------------------------------- */
    pabyBuf = (unsigned char *) dk_alloc(nBufSize);
    if( psDBF->sHooks.FRead( pabyBuf, 32, 1, psDBF->fp ) != 1 )
    {
        psDBF->sHooks.FClose( psDBF->fp );
        if( pfCPG ) psDBF->sHooks.FClose( pfCPG );
        dk_free ( pabyBuf, -1 );
        dk_free ( psDBF, -1 );
        return NULL;
    }

    psDBF->nRecords = 
     pabyBuf[4] + pabyBuf[5]*256 + pabyBuf[6]*256*256 + pabyBuf[7]*256*256*256;

    psDBF->nHeaderLength = nHeadLen = pabyBuf[8] + pabyBuf[9]*256;
    psDBF->nRecordLength = pabyBuf[10] + pabyBuf[11]*256;
    psDBF->iLanguageDriver = pabyBuf[29];

    if (nHeadLen < 32)
    {
        psDBF->sHooks.FClose( psDBF->fp );
        if( pfCPG ) psDBF->sHooks.FClose( pfCPG );
        dk_free ( pabyBuf, -1 );
        dk_free ( psDBF, -1 );
        return NULL;
    }

    psDBF->nFields = nFields = (nHeadLen - 32) / 32;

    psDBF->pszCurrentRecord = (char *) dk_alloc(psDBF->nRecordLength);

/* -------------------------------------------------------------------- */
/*  Figure out the code page from the LDID and CPG                      */
/* -------------------------------------------------------------------- */

    psDBF->pszCodePage = NULL;
    if( pfCPG )
    {
        size_t n;
        memset( pabyBuf, 0, nBufSize);
        psDBF->sHooks.FRead( pabyBuf, nBufSize - 1, 1, pfCPG );
        n = strcspn( (char *) pabyBuf, "\n\r" );
        if( n > 0 )
        {
            pabyBuf[n] = '\0';
            psDBF->pszCodePage = (char *) dk_alloc(n + 1);
            memcpy( psDBF->pszCodePage, pabyBuf, n + 1 );
        }
		psDBF->sHooks.FClose( pfCPG );
    }
    if( psDBF->pszCodePage == NULL && pabyBuf[29] != 0 )
    {
        sprintf( (char *) pabyBuf, "LDID/%d", psDBF->iLanguageDriver );
        psDBF->pszCodePage = (char *) dk_alloc(strlen((char*)pabyBuf) + 1);
        strcpy( psDBF->pszCodePage, (char *) pabyBuf );
    }

/* -------------------------------------------------------------------- */
/*  Read in Field Definitions                                           */
/* -------------------------------------------------------------------- */
    
    pabyBuf = (unsigned char *) SfRealloc(pabyBuf,nHeadLen);
    psDBF->pszHeader = (char *) pabyBuf;

    psDBF->sHooks.FSeek( psDBF->fp, 32, 0 );
    if( psDBF->sHooks.FRead( pabyBuf, nHeadLen-32, 1, psDBF->fp ) != 1 )
    {
        psDBF->sHooks.FClose( psDBF->fp );
        dk_free ( pabyBuf, -1 );
        dk_free ( psDBF->pszCurrentRecord, -1 );
        dk_free ( psDBF, -1 );
        return NULL;
    }

    psDBF->panFieldOffset = (int *) dk_alloc(sizeof(int) * nFields);
    psDBF->panFieldSize = (int *) dk_alloc(sizeof(int) * nFields);
    psDBF->panFieldDecimals = (int *) dk_alloc(sizeof(int) * nFields);
    psDBF->pachFieldType = (char *) dk_alloc(sizeof(char) * nFields);

    for( iField = 0; iField < nFields; iField++ )
    {
	unsigned char		*pabyFInfo;

	pabyFInfo = pabyBuf+iField*32;

	if( pabyFInfo[11] == 'N' || pabyFInfo[11] == 'F' )
	{
	    psDBF->panFieldSize[iField] = pabyFInfo[16];
	    psDBF->panFieldDecimals[iField] = pabyFInfo[17];
	}
	else
	{
	    psDBF->panFieldSize[iField] = pabyFInfo[16];
	    psDBF->panFieldDecimals[iField] = 0;

/*
** The following seemed to be used sometimes to handle files with long
** string fields, but in other cases (such as bug 1202) the decimals field
** just seems to indicate some sort of preferred formatting, not very
** wide fields.  So I have disabled this code.  FrankW.
	    psDBF->panFieldSize[iField] = pabyFInfo[16] + pabyFInfo[17]*256;
	    psDBF->panFieldDecimals[iField] = 0;
*/
	}

	psDBF->pachFieldType[iField] = (char) pabyFInfo[11];
	if( iField == 0 )
	    psDBF->panFieldOffset[iField] = 1;
	else
	    psDBF->panFieldOffset[iField] = 
	      psDBF->panFieldOffset[iField-1] + psDBF->panFieldSize[iField-1];
    }

    return( psDBF );
}

/************************************************************************/
/*                              DBFClose()                              */
/************************************************************************/

void SHPAPI_CALL
DBFClose(DBFHandle psDBF)
{
    if( psDBF == NULL )
        return;

/* -------------------------------------------------------------------- */
/*      Write out header if not already written.                        */
/* -------------------------------------------------------------------- */
    if( psDBF->bNoHeader )
        DBFWriteHeader( psDBF );

    DBFFlushRecord( psDBF );

/* -------------------------------------------------------------------- */
/*      Update last access date, and number of records if we have	*/
/*	write access.                					*/
/* -------------------------------------------------------------------- */
    if( psDBF->bUpdated )
        DBFUpdateHeader( psDBF );

/* -------------------------------------------------------------------- */
/*      Close, and dk_free  resources.                                      */
/* -------------------------------------------------------------------- */
    psDBF->sHooks.FClose( psDBF->fp );

    if( psDBF->panFieldOffset != NULL )
    {
        dk_free ( psDBF->panFieldOffset, -1 );
        dk_free ( psDBF->panFieldSize, -1 );
        dk_free ( psDBF->panFieldDecimals, -1 );
        dk_free ( psDBF->pachFieldType, -1 );
    }

    if( psDBF->pszWorkField != NULL )
        dk_free ( psDBF->pszWorkField, -1 );

    dk_free ( psDBF->pszHeader, -1 );
    dk_free ( psDBF->pszCurrentRecord, -1 );
    if (NULL != psDBF->pszCodePage)
        dk_free ( psDBF->pszCodePage, -1 );

    dk_free ( psDBF, -1 );
}

/************************************************************************/
/*                             DBFCreate()                              */
/*                                                                      */
/* Create a new .dbf file with default code page LDID/87 (0x57)         */
/************************************************************************/

DBFHandle SHPAPI_CALL
DBFCreate( const char * pszFilename )

{
    return DBFCreateEx( pszFilename, "LDID/87" ); // 0x57
}

/************************************************************************/
/*                            DBFCreateEx()                             */
/*                                                                      */
/*      Create a new .dbf file.                                         */
/************************************************************************/

DBFHandle SHPAPI_CALL
DBFCreateEx( const char * pszFilename, const char* pszCodePage )

{
    SAHooks sHooks;

    SASetupDefaultHooks( &sHooks );

    return DBFCreateLL( pszFilename, pszCodePage , &sHooks );
}

/************************************************************************/
/*                             DBFCreate()                              */
/*                                                                      */
/*      Create a new .dbf file.                                         */
/************************************************************************/

DBFHandle SHPAPI_CALL
DBFCreateLL( const char * pszFilename, const char * pszCodePage, SAHooks *psHooks )

{
    DBFHandle	psDBF;
    SAFile	fp;
    char	*pszFullname, *pszBasename;
    int		i, ldid = -1;
    char chZero = '\0';

/* -------------------------------------------------------------------- */
/*	Compute the base (layer) name.  If there is any extension	*/
/*	on the passed in filename we will strip it off.			*/
/* -------------------------------------------------------------------- */
    pszBasename = (char *) dk_alloc(strlen(pszFilename)+5);
    strcpy( pszBasename, pszFilename );
    for( i = strlen(pszBasename)-1; 
	 i > 0 && pszBasename[i] != '.' && pszBasename[i] != '/'
	       && pszBasename[i] != '\\';
	 i-- ) {}

    if( pszBasename[i] == '.' )
        pszBasename[i] = '\0';

    pszFullname = (char *) dk_alloc(strlen(pszBasename) + 5);
    sprintf( pszFullname, "%s.dbf", pszBasename );

/* -------------------------------------------------------------------- */
/*      Create the file.                                                */
/* -------------------------------------------------------------------- */
    fp = psHooks->FOpen( pszFullname, "wb" );
    if( fp == NULL )
        return( NULL );
    
    psHooks->FWrite( &chZero, 1, 1, fp );
    psHooks->FClose( fp );

    fp = psHooks->FOpen( pszFullname, "rb+" );
    if( fp == NULL )
        return( NULL );


    sprintf( pszFullname, "%s.cpg", pszBasename );
    if( pszCodePage != NULL )
    {
        if( strncmp( pszCodePage, "LDID/", 5 ) == 0 )
        {
            ldid = atoi( pszCodePage + 5 );
            if( ldid > 255 )
                ldid = -1; // don't use 0 to indicate out of range as LDID/0 is a valid one
        }
        if( ldid < 0 )
        {
            SAFile fpCPG = psHooks->FOpen( pszFullname, "w" );
            psHooks->FWrite( (char*) pszCodePage, strlen(pszCodePage), 1, fpCPG );
            psHooks->FClose( fpCPG );
        }
    }
    if( pszCodePage == NULL || ldid >= 0 )
    {
        psHooks->Remove( pszFullname );
    }

    dk_free ( pszBasename, -1 );
    dk_free ( pszFullname, -1 );

/* -------------------------------------------------------------------- */
/*	Create the info structure.					*/
/* -------------------------------------------------------------------- */
    psDBF = (DBFHandle) dk_alloc (sizeof(DBFInfo));
    memset (psDBF, 0, sizeof(DBFInfo));
    memcpy( &(psDBF->sHooks), psHooks, sizeof(SAHooks) );
    psDBF->fp = fp;
    psDBF->nRecords = 0;
    psDBF->nFields = 0;
    psDBF->nRecordLength = 1;
    psDBF->nHeaderLength = 33;
    
    psDBF->panFieldOffset = NULL;
    psDBF->panFieldSize = NULL;
    psDBF->panFieldDecimals = NULL;
    psDBF->pachFieldType = NULL;
    psDBF->pszHeader = NULL;

    psDBF->nCurrentRecord = -1;
    psDBF->bCurrentRecordModified = FALSE;
    psDBF->pszCurrentRecord = NULL;

    psDBF->bNoHeader = TRUE;

    psDBF->iLanguageDriver = ldid > 0 ? ldid : 0;
    psDBF->pszCodePage = NULL;
    if( pszCodePage )
    {
        psDBF->pszCodePage = (char * ) dk_alloc( strlen(pszCodePage) + 1 );
        strcpy( psDBF->pszCodePage, pszCodePage );
    }

    return( psDBF );
}

/************************************************************************/
/*                            DBFAddField()                             */
/*                                                                      */
/*      Add a field to a newly created .dbf or to an existing one       */
/************************************************************************/

int SHPAPI_CALL
DBFAddField(DBFHandle psDBF, const char * pszFieldName, 
            DBFFieldType eType, int nWidth, int nDecimals )

{
    char chNativeType = 'C';

    if( eType == FTLogical )
        chNativeType = 'L';
    else if( eType == FTString )
        chNativeType = 'C';
    else
        chNativeType = 'N';

    return DBFAddNativeFieldType( psDBF, pszFieldName, chNativeType, 
                                  nWidth, nDecimals );
}

/************************************************************************/
/*                        DBFGetNullCharacter()                         */
/************************************************************************/

static char DBFGetNullCharacter(char chType)
{
    switch (chType)
    {
      case 'N':
      case 'F':
        return '*';
      case 'D':
        return '0';
      case 'L':
       return '?';
      default:
       return ' ';
    }
}

/************************************************************************/
/*                            DBFAddField()                             */
/*                                                                      */
/*      Add a field to a newly created .dbf file before any records     */
/*      are written.                                                    */
/************************************************************************/

int SHPAPI_CALL
DBFAddNativeFieldType(DBFHandle psDBF, const char * pszFieldName, 
                      char chType, int nWidth, int nDecimals )

{
    char	*pszFInfo;
    int		i;
    int         nOldRecordLength, nOldHeaderLength;
    char        *pszRecord;
    char        chFieldFill;
    SAOffset    nRecordOffset;

    /* make sure that everything is written in .dbf */
    if( !DBFFlushRecord( psDBF ) )
        return -1;

/* -------------------------------------------------------------------- */
/*      Do some checking to ensure we can add records to this file.     */
/* -------------------------------------------------------------------- */
    if( nWidth < 1 )
        return -1;

    if( nWidth > 255 )
        nWidth = 255;

    nOldRecordLength = psDBF->nRecordLength;
    nOldHeaderLength = psDBF->nHeaderLength;

/* -------------------------------------------------------------------- */
/*      SfRealloc all the arrays larger to hold the additional field      */
/*      information.                                                    */
/* -------------------------------------------------------------------- */
    psDBF->nFields++;

    psDBF->panFieldOffset = (int *) 
        SfRealloc( psDBF->panFieldOffset, sizeof(int) * psDBF->nFields );

    psDBF->panFieldSize = (int *) 
        SfRealloc( psDBF->panFieldSize, sizeof(int) * psDBF->nFields );

    psDBF->panFieldDecimals = (int *) 
        SfRealloc( psDBF->panFieldDecimals, sizeof(int) * psDBF->nFields );

    psDBF->pachFieldType = (char *) 
        SfRealloc( psDBF->pachFieldType, sizeof(char) * psDBF->nFields );

/* -------------------------------------------------------------------- */
/*      Assign the new field information fields.                        */
/* -------------------------------------------------------------------- */
    psDBF->panFieldOffset[psDBF->nFields-1] = psDBF->nRecordLength;
    psDBF->nRecordLength += nWidth;
    psDBF->panFieldSize[psDBF->nFields-1] = nWidth;
    psDBF->panFieldDecimals[psDBF->nFields-1] = nDecimals;
    psDBF->pachFieldType[psDBF->nFields-1] = chType;

/* -------------------------------------------------------------------- */
/*      Extend the required header information.                         */
/* -------------------------------------------------------------------- */
    psDBF->nHeaderLength += 32;
    psDBF->bUpdated = FALSE;

    psDBF->pszHeader = (char *) SfRealloc(psDBF->pszHeader,psDBF->nFields*32);

    pszFInfo = psDBF->pszHeader + 32 * (psDBF->nFields-1);

    for( i = 0; i < 32; i++ )
        pszFInfo[i] = '\0';

    if( (int) strlen(pszFieldName) < 10 )
        strncpy( pszFInfo, pszFieldName, strlen(pszFieldName));
    else
        strncpy( pszFInfo, pszFieldName, 10);

    pszFInfo[11] = psDBF->pachFieldType[psDBF->nFields-1];

    if( chType == 'C' )
    {
        pszFInfo[16] = (unsigned char) (nWidth % 256);
        pszFInfo[17] = (unsigned char) (nWidth / 256);
    }
    else
    {
        pszFInfo[16] = (unsigned char) nWidth;
        pszFInfo[17] = (unsigned char) nDecimals;
    }
    
/* -------------------------------------------------------------------- */
/*      Make the current record buffer appropriately larger.            */
/* -------------------------------------------------------------------- */
    psDBF->pszCurrentRecord = (char *) SfRealloc(psDBF->pszCurrentRecord,
                                                 psDBF->nRecordLength);

    /* we're done if dealing with new .dbf */
    if( psDBF->bNoHeader )
        return( psDBF->nFields - 1 );

/* -------------------------------------------------------------------- */
/*      For existing .dbf file, shift records                           */
/* -------------------------------------------------------------------- */

    /* alloc record */
    pszRecord = (char *) dk_alloc(sizeof(char) * psDBF->nRecordLength);

    chFieldFill = DBFGetNullCharacter(chType);

    for (i = psDBF->nRecords-1; i >= 0; --i)
    {
        nRecordOffset = nOldRecordLength * (SAOffset) i + nOldHeaderLength;

        /* load record */
        psDBF->sHooks.FSeek( psDBF->fp, nRecordOffset, 0 );
        psDBF->sHooks.FRead( pszRecord, nOldRecordLength, 1, psDBF->fp );

        /* set new field's value to NULL */
        memset(pszRecord + nOldRecordLength, chFieldFill, nWidth);

        nRecordOffset = psDBF->nRecordLength * (SAOffset) i + psDBF->nHeaderLength;

        /* move record to the new place*/
        psDBF->sHooks.FSeek( psDBF->fp, nRecordOffset, 0 );
        psDBF->sHooks.FWrite( pszRecord, psDBF->nRecordLength, 1, psDBF->fp );
    }

    /* dk_free  record */
    dk_free (pszRecord, -1);

    /* force update of header with new header, record length and new field */
    psDBF->bNoHeader = TRUE;
    DBFUpdateHeader( psDBF );

    psDBF->nCurrentRecord = -1;
    psDBF->bCurrentRecordModified = FALSE;

    return( psDBF->nFields-1 );
}

/************************************************************************/
/*                          DBFReadAttribute()                          */
/*                                                                      */
/*      Read one of the attribute fields of a record.                   */
/************************************************************************/

static void *DBFReadAttribute(DBFHandle psDBF, int hEntity, int iField,
                              char chReqType )

{
    unsigned char	*pabyRec;
    void	*pReturnField = NULL;

/* -------------------------------------------------------------------- */
/*      Verify selection.                                               */
/* -------------------------------------------------------------------- */
    if( hEntity < 0 || hEntity >= psDBF->nRecords )
        return( NULL );

    if( iField < 0 || iField >= psDBF->nFields )
        return( NULL );

/* -------------------------------------------------------------------- */
/*	Have we read the record?					*/
/* -------------------------------------------------------------------- */
    if( !DBFLoadRecord( psDBF, hEntity ) )
        return NULL;

    pabyRec = (unsigned char *) psDBF->pszCurrentRecord;

/* -------------------------------------------------------------------- */
/*      Ensure we have room to extract the target field.                */
/* -------------------------------------------------------------------- */
    if( psDBF->panFieldSize[iField] >= psDBF->nWorkFieldLength )
    {
        psDBF->nWorkFieldLength = psDBF->panFieldSize[iField] + 100;
        if( psDBF->pszWorkField == NULL )
            psDBF->pszWorkField = (char *) dk_alloc(psDBF->nWorkFieldLength);
        else
            psDBF->pszWorkField = (char *) SfRealloc(psDBF->pszWorkField,
                                                   psDBF->nWorkFieldLength);
    }

/* -------------------------------------------------------------------- */
/*	Extract the requested field.					*/
/* -------------------------------------------------------------------- */
    strncpy( psDBF->pszWorkField,
	     ((const char *) pabyRec) + psDBF->panFieldOffset[iField],
	     psDBF->panFieldSize[iField] );
    psDBF->pszWorkField[psDBF->panFieldSize[iField]] = '\0';

    pReturnField = psDBF->pszWorkField;

/* -------------------------------------------------------------------- */
/*      Decode the field.                                               */
/* -------------------------------------------------------------------- */
    if( chReqType == 'N' )
    {
        psDBF->dfDoubleField = psDBF->sHooks.Atof(psDBF->pszWorkField);

	pReturnField = &(psDBF->dfDoubleField);
    }

/* -------------------------------------------------------------------- */
/*      Should we trim white space off the string attribute value?      */
/* -------------------------------------------------------------------- */
#ifdef TRIM_DBF_WHITESPACE
    else
    {
        char	*pchSrc, *pchDst;

        pchDst = pchSrc = psDBF->pszWorkField;
        while( *pchSrc == ' ' )
            pchSrc++;

        while( *pchSrc != '\0' )
            *(pchDst++) = *(pchSrc++);
        *pchDst = '\0';

        while( pchDst != psDBF->pszWorkField && *(--pchDst) == ' ' )
            *pchDst = '\0';
    }
#endif
    
    return( pReturnField );
}

/************************************************************************/
/*                        DBFReadIntAttribute()                         */
/*                                                                      */
/*      Read an integer attribute.                                      */
/************************************************************************/

int SHPAPI_CALL
DBFReadIntegerAttribute( DBFHandle psDBF, int iRecord, int iField )

{
    double	*pdValue;

    pdValue = (double *) DBFReadAttribute( psDBF, iRecord, iField, 'N' );

    if( pdValue == NULL )
        return 0;
    else
        return( (int) *pdValue );
}

/************************************************************************/
/*                        DBFReadDoubleAttribute()                      */
/*                                                                      */
/*      Read a double attribute.                                        */
/************************************************************************/

double SHPAPI_CALL
DBFReadDoubleAttribute( DBFHandle psDBF, int iRecord, int iField )

{
    double	*pdValue;

    pdValue = (double *) DBFReadAttribute( psDBF, iRecord, iField, 'N' );

    if( pdValue == NULL )
        return 0.0;
    else
        return( *pdValue );
}

/************************************************************************/
/*                        DBFReadStringAttribute()                      */
/*                                                                      */
/*      Read a string attribute.                                        */
/************************************************************************/

const char SHPAPI_CALL1(*)
DBFReadStringAttribute( DBFHandle psDBF, int iRecord, int iField )

{
    return( (const char *) DBFReadAttribute( psDBF, iRecord, iField, 'C' ) );
}

/************************************************************************/
/*                        DBFReadLogicalAttribute()                     */
/*                                                                      */
/*      Read a logical attribute.                                       */
/************************************************************************/

const char SHPAPI_CALL1(*)
DBFReadLogicalAttribute( DBFHandle psDBF, int iRecord, int iField )

{
    return( (const char *) DBFReadAttribute( psDBF, iRecord, iField, 'L' ) );
}


/************************************************************************/
/*                         DBFIsValueNULL()                             */
/*                                                                      */
/*      Return TRUE if the passed string is NULL.                       */
/************************************************************************/

static int DBFIsValueNULL( char chType, const char* pszValue )
{
    int i;

    if( pszValue == NULL )
        return TRUE;

    switch(chType)
    {
      case 'N':
      case 'F':
        /*
        ** We accept all asterisks or all blanks as NULL
        ** though according to the spec I think it should be all
        ** asterisks.
        */
        if( pszValue[0] == '*' )
            return TRUE;

        for( i = 0; pszValue[i] != '\0'; i++ )
        {
            if( pszValue[i] != ' ' )
                return FALSE;
        }
        return TRUE;

      case 'D':
        /* NULL date fields have value "00000000" */
        return strncmp(pszValue,"00000000",8) == 0;

      case 'L':
        /* NULL boolean fields have value "?" */
        return pszValue[0] == '?';

      default:
        /* empty string fields are considered NULL */
        return strlen(pszValue) == 0;
    }
}

/************************************************************************/
/*                         DBFIsAttributeNULL()                         */
/*                                                                      */
/*      Return TRUE if value for field is NULL.                         */
/*                                                                      */
/*      Contributed by Jim Matthews.                                    */
/************************************************************************/

int SHPAPI_CALL
DBFIsAttributeNULL( DBFHandle psDBF, int iRecord, int iField )

{
    const char	*pszValue;

    pszValue = DBFReadStringAttribute( psDBF, iRecord, iField );

    if( pszValue == NULL )
        return TRUE;

    return DBFIsValueNULL( psDBF->pachFieldType[iField], pszValue );
}

/************************************************************************/
/*                          DBFGetFieldCount()                          */
/*                                                                      */
/*      Return the number of fields in this table.                      */
/************************************************************************/

int SHPAPI_CALL
DBFGetFieldCount( DBFHandle psDBF )

{
    return( psDBF->nFields );
}

/************************************************************************/
/*                         DBFGetRecordCount()                          */
/*                                                                      */
/*      Return the number of records in this table.                     */
/************************************************************************/

int SHPAPI_CALL
DBFGetRecordCount( DBFHandle psDBF )

{
    return( psDBF->nRecords );
}

/************************************************************************/
/*                          DBFGetFieldInfo()                           */
/*                                                                      */
/*      Return any requested information about the field.               */
/************************************************************************/

DBFFieldType SHPAPI_CALL
DBFGetFieldInfo( DBFHandle psDBF, int iField, char * pszFieldName,
                 int * pnWidth, int * pnDecimals )

{
    if( iField < 0 || iField >= psDBF->nFields )
        return( FTInvalid );

    if( pnWidth != NULL )
        *pnWidth = psDBF->panFieldSize[iField];

    if( pnDecimals != NULL )
        *pnDecimals = psDBF->panFieldDecimals[iField];

    if( pszFieldName != NULL )
    {
	int	i;

	strncpy( pszFieldName, (char *) psDBF->pszHeader+iField*32, 11 );
	pszFieldName[11] = '\0';
	for( i = 10; i > 0 && pszFieldName[i] == ' '; i-- )
	    pszFieldName[i] = '\0';
    }

    if ( psDBF->pachFieldType[iField] == 'L' )
	return( FTLogical);

    else if( psDBF->pachFieldType[iField] == 'N' 
             || psDBF->pachFieldType[iField] == 'F' )
    {
	if( psDBF->panFieldDecimals[iField] > 0 
            || psDBF->panFieldSize[iField] > 10 )
	    return( FTDouble );
	else
	    return( FTInteger );
    }
    else
    {
	return( FTString );
    }
}

/************************************************************************/
/*                         DBFWriteAttribute()                          */
/*									*/
/*	Write an attribute record to the file.				*/
/************************************************************************/

static int DBFWriteAttribute(DBFHandle psDBF, int hEntity, int iField,
			     void * pValue )

{
    int	       	i, j, nRetResult = TRUE;
    unsigned char	*pabyRec;
    char	szSField[400], szFormat[20];

/* -------------------------------------------------------------------- */
/*	Is this a valid record?						*/
/* -------------------------------------------------------------------- */
    if( hEntity < 0 || hEntity > psDBF->nRecords )
        return( FALSE );

    if( psDBF->bNoHeader )
        DBFWriteHeader(psDBF);

/* -------------------------------------------------------------------- */
/*      Is this a brand new record?                                     */
/* -------------------------------------------------------------------- */
    if( hEntity == psDBF->nRecords )
    {
	if( !DBFFlushRecord( psDBF ) )
            return FALSE;

	psDBF->nRecords++;
	for( i = 0; i < psDBF->nRecordLength; i++ )
	    psDBF->pszCurrentRecord[i] = ' ';

	psDBF->nCurrentRecord = hEntity;
    }

/* -------------------------------------------------------------------- */
/*      Is this an existing record, but different than the last one     */
/*      we accessed?                                                    */
/* -------------------------------------------------------------------- */
    if( !DBFLoadRecord( psDBF, hEntity ) )
        return FALSE;

    pabyRec = (unsigned char *) psDBF->pszCurrentRecord;

    psDBF->bCurrentRecordModified = TRUE;
    psDBF->bUpdated = TRUE;

/* -------------------------------------------------------------------- */
/*      Translate NULL value to valid DBF file representation.          */
/*                                                                      */
/*      Contributed by Jim Matthews.                                    */
/* -------------------------------------------------------------------- */
    if( pValue == NULL )
    {
        memset( (char *) (pabyRec+psDBF->panFieldOffset[iField]),
                DBFGetNullCharacter(psDBF->pachFieldType[iField]),
                psDBF->panFieldSize[iField] );
        return TRUE;
    }

/* -------------------------------------------------------------------- */
/*      Assign all the record fields.                                   */
/* -------------------------------------------------------------------- */
    switch( psDBF->pachFieldType[iField] )
    {
      case 'D':
      case 'N':
      case 'F':
	if( psDBF->panFieldDecimals[iField] == 0 )
	{
            int		nWidth = psDBF->panFieldSize[iField];

            if( (int) sizeof(szSField)-2 < nWidth )
                nWidth = sizeof(szSField)-2;

	    sprintf( szFormat, "%%%dd", nWidth );
	    sprintf(szSField, szFormat, (int) *((double *) pValue) );
	    if( (int)strlen(szSField) > psDBF->panFieldSize[iField] )
            {
	        szSField[psDBF->panFieldSize[iField]] = '\0';
                nRetResult = FALSE;
            }

	    strncpy((char *) (pabyRec+psDBF->panFieldOffset[iField]),
		    szSField, strlen(szSField) );
	}
	else
	{
            int		nWidth = psDBF->panFieldSize[iField];

            if( (int) sizeof(szSField)-2 < nWidth )
                nWidth = sizeof(szSField)-2;

	    sprintf( szFormat, "%%%d.%df", 
                     nWidth, psDBF->panFieldDecimals[iField] );
	    sprintf(szSField, szFormat, *((double *) pValue) );
	    if( (int) strlen(szSField) > psDBF->panFieldSize[iField] )
            {
	        szSField[psDBF->panFieldSize[iField]] = '\0';
                nRetResult = FALSE;
            }
	    strncpy((char *) (pabyRec+psDBF->panFieldOffset[iField]),
		    szSField, strlen(szSField) );
	}
	break;

      case 'L':
        if (psDBF->panFieldSize[iField] >= 1  && 
            (*(char*)pValue == 'F' || *(char*)pValue == 'T'))
            *(pabyRec+psDBF->panFieldOffset[iField]) = *(char*)pValue;
        break;

      default:
	if( (int) strlen((char *) pValue) > psDBF->panFieldSize[iField] )
        {
	    j = psDBF->panFieldSize[iField];
            nRetResult = FALSE;
        }
	else
        {
            memset( pabyRec+psDBF->panFieldOffset[iField], ' ',
                    psDBF->panFieldSize[iField] );
	    j = strlen((char *) pValue);
        }

	strncpy((char *) (pabyRec+psDBF->panFieldOffset[iField]),
		(char *) pValue, j );
	break;
    }

    return( nRetResult );
}

/************************************************************************/
/*                     DBFWriteAttributeDirectly()                      */
/*                                                                      */
/*      Write an attribute record to the file, but without any          */
/*      reformatting based on type.  The provided buffer is written     */
/*      as is to the field position in the record.                      */
/************************************************************************/

int SHPAPI_CALL
DBFWriteAttributeDirectly(DBFHandle psDBF, int hEntity, int iField,
                              void * pValue )

{
    int	       		i, j;
    unsigned char	*pabyRec;

/* -------------------------------------------------------------------- */
/*	Is this a valid record?						*/
/* -------------------------------------------------------------------- */
    if( hEntity < 0 || hEntity > psDBF->nRecords )
        return( FALSE );

    if( psDBF->bNoHeader )
        DBFWriteHeader(psDBF);

/* -------------------------------------------------------------------- */
/*      Is this a brand new record?                                     */
/* -------------------------------------------------------------------- */
    if( hEntity == psDBF->nRecords )
    {
	if( !DBFFlushRecord( psDBF ) )
            return FALSE;

	psDBF->nRecords++;
	for( i = 0; i < psDBF->nRecordLength; i++ )
	    psDBF->pszCurrentRecord[i] = ' ';

	psDBF->nCurrentRecord = hEntity;
    }

/* -------------------------------------------------------------------- */
/*      Is this an existing record, but different than the last one     */
/*      we accessed?                                                    */
/* -------------------------------------------------------------------- */
    if( !DBFLoadRecord( psDBF, hEntity ) )
        return FALSE;

    pabyRec = (unsigned char *) psDBF->pszCurrentRecord;

/* -------------------------------------------------------------------- */
/*      Assign all the record fields.                                   */
/* -------------------------------------------------------------------- */
    if( (int)strlen((char *) pValue) > psDBF->panFieldSize[iField] )
        j = psDBF->panFieldSize[iField];
    else
    {
        memset( pabyRec+psDBF->panFieldOffset[iField], ' ',
                psDBF->panFieldSize[iField] );
        j = strlen((char *) pValue);
    }

    strncpy((char *) (pabyRec+psDBF->panFieldOffset[iField]),
            (char *) pValue, j );

    psDBF->bCurrentRecordModified = TRUE;
    psDBF->bUpdated = TRUE;

    return( TRUE );
}

/************************************************************************/
/*                      DBFWriteDoubleAttribute()                       */
/*                                                                      */
/*      Write a double attribute.                                       */
/************************************************************************/

int SHPAPI_CALL
DBFWriteDoubleAttribute( DBFHandle psDBF, int iRecord, int iField,
                         double dValue )

{
    return( DBFWriteAttribute( psDBF, iRecord, iField, (void *) &dValue ) );
}

/************************************************************************/
/*                      DBFWriteIntegerAttribute()                      */
/*                                                                      */
/*      Write a integer attribute.                                      */
/************************************************************************/

int SHPAPI_CALL
DBFWriteIntegerAttribute( DBFHandle psDBF, int iRecord, int iField,
                          int nValue )

{
    double	dValue = nValue;

    return( DBFWriteAttribute( psDBF, iRecord, iField, (void *) &dValue ) );
}

/************************************************************************/
/*                      DBFWriteStringAttribute()                       */
/*                                                                      */
/*      Write a string attribute.                                       */
/************************************************************************/

int SHPAPI_CALL
DBFWriteStringAttribute( DBFHandle psDBF, int iRecord, int iField,
                         const char * pszValue )

{
    return( DBFWriteAttribute( psDBF, iRecord, iField, (void *) pszValue ) );
}

/************************************************************************/
/*                      DBFWriteNULLAttribute()                         */
/*                                                                      */
/*      Write a string attribute.                                       */
/************************************************************************/

int SHPAPI_CALL
DBFWriteNULLAttribute( DBFHandle psDBF, int iRecord, int iField )

{
    return( DBFWriteAttribute( psDBF, iRecord, iField, NULL ) );
}

/************************************************************************/
/*                      DBFWriteLogicalAttribute()                      */
/*                                                                      */
/*      Write a logical attribute.                                      */
/************************************************************************/

int SHPAPI_CALL
DBFWriteLogicalAttribute( DBFHandle psDBF, int iRecord, int iField,
		       const char lValue)

{
    return( DBFWriteAttribute( psDBF, iRecord, iField, (void *) (&lValue) ) );
}

/************************************************************************/
/*                         DBFWriteTuple()                              */
/*									*/
/*	Write an attribute record to the file.				*/
/************************************************************************/

int SHPAPI_CALL
DBFWriteTuple(DBFHandle psDBF, int hEntity, void * pRawTuple )

{
    int	       		i;
    unsigned char	*pabyRec;

/* -------------------------------------------------------------------- */
/*	Is this a valid record?						*/
/* -------------------------------------------------------------------- */
    if( hEntity < 0 || hEntity > psDBF->nRecords )
        return( FALSE );

    if( psDBF->bNoHeader )
        DBFWriteHeader(psDBF);

/* -------------------------------------------------------------------- */
/*      Is this a brand new record?                                     */
/* -------------------------------------------------------------------- */
    if( hEntity == psDBF->nRecords )
    {
	if( !DBFFlushRecord( psDBF ) )
            return FALSE;

	psDBF->nRecords++;
	for( i = 0; i < psDBF->nRecordLength; i++ )
	    psDBF->pszCurrentRecord[i] = ' ';

	psDBF->nCurrentRecord = hEntity;
    }

/* -------------------------------------------------------------------- */
/*      Is this an existing record, but different than the last one     */
/*      we accessed?                                                    */
/* -------------------------------------------------------------------- */
    if( !DBFLoadRecord( psDBF, hEntity ) )
        return FALSE;

    pabyRec = (unsigned char *) psDBF->pszCurrentRecord;

    memcpy ( pabyRec, pRawTuple,  psDBF->nRecordLength );

    psDBF->bCurrentRecordModified = TRUE;
    psDBF->bUpdated = TRUE;

    return( TRUE );
}

/************************************************************************/
/*                            DBFReadTuple()                            */
/*                                                                      */
/*      Read a complete record.  Note that the result is only valid     */
/*      till the next record read for any reason.                       */
/************************************************************************/

const char SHPAPI_CALL1(*)
DBFReadTuple(DBFHandle psDBF, int hEntity )

{
    if( hEntity < 0 || hEntity >= psDBF->nRecords )
        return( NULL );

    if( !DBFLoadRecord( psDBF, hEntity ) )
        return NULL;

    return (const char *) psDBF->pszCurrentRecord;
}

/************************************************************************/
/*                          DBFCloneEmpty()                              */
/*                                                                      */
/*      Read one of the attribute fields of a record.                   */
/************************************************************************/

DBFHandle SHPAPI_CALL
DBFCloneEmpty(DBFHandle psDBF, const char * pszFilename ) 
{
    DBFHandle	newDBF;

   newDBF = DBFCreateEx ( pszFilename, psDBF->pszCodePage );
   if ( newDBF == NULL ) return ( NULL ); 
   
   newDBF->nFields = psDBF->nFields;
   newDBF->nRecordLength = psDBF->nRecordLength;
   newDBF->nHeaderLength = psDBF->nHeaderLength;
    
   newDBF->pszHeader = (char *) dk_alloc ( newDBF->nHeaderLength );
   memcpy ( newDBF->pszHeader, psDBF->pszHeader, newDBF->nHeaderLength );
   
   newDBF->panFieldOffset = (int *) dk_alloc ( sizeof(int) * psDBF->nFields ); 
   memcpy ( newDBF->panFieldOffset, psDBF->panFieldOffset, sizeof(int) * psDBF->nFields );
   newDBF->panFieldSize = (int *) dk_alloc ( sizeof(int) * psDBF->nFields );
   memcpy ( newDBF->panFieldSize, psDBF->panFieldSize, sizeof(int) * psDBF->nFields );
   newDBF->panFieldDecimals = (int *) dk_alloc ( sizeof(int) * psDBF->nFields );
   memcpy ( newDBF->panFieldDecimals, psDBF->panFieldDecimals, sizeof(int) * psDBF->nFields );
   newDBF->pachFieldType = (char *) dk_alloc ( sizeof(char) * psDBF->nFields );
   memcpy ( newDBF->pachFieldType, psDBF->pachFieldType, sizeof(char)*psDBF->nFields );

   newDBF->bNoHeader = TRUE;
   newDBF->bUpdated = TRUE;
   
   DBFWriteHeader ( newDBF );
   DBFClose ( newDBF );
   
   newDBF = DBFOpen ( pszFilename, "rb+" );

   return ( newDBF );
}

/************************************************************************/
/*                       DBFGetNativeFieldType()                        */
/*                                                                      */
/*      Return the DBase field type for the specified field.            */
/*                                                                      */
/*      Value can be one of: 'C' (String), 'D' (Date), 'F' (Float),     */
/*                           'N' (Numeric, with or without decimal),    */
/*                           'L' (Logical),                             */
/*                           'M' (Memo: 10 digits .DBT block ptr)       */
/************************************************************************/

char SHPAPI_CALL
DBFGetNativeFieldType( DBFHandle psDBF, int iField )

{
    if( iField >=0 && iField < psDBF->nFields )
        return psDBF->pachFieldType[iField];

    return  ' ';
}

/************************************************************************/
/*                            str_to_upper()                            */
/************************************************************************/

static void str_to_upper (char *string)
{
    int len;
    short i = -1;

    len = strlen (string);

    while (++i < len)
        if (isalpha(string[i]) && islower(string[i]))
            string[i] = (char) toupper ((int)string[i]);
}

/************************************************************************/
/*                          DBFGetFieldIndex()                          */
/*                                                                      */
/*      Get the index number for a field in a .dbf file.                */
/*                                                                      */
/*      Contributed by Jim Matthews.                                    */
/************************************************************************/

int SHPAPI_CALL
DBFGetFieldIndex(DBFHandle psDBF, const char *pszFieldName)

{
    char          name[12], name1[12], name2[12];
    int           i;

    strncpy(name1, pszFieldName,11);
    name1[11] = '\0';
    str_to_upper(name1);

    for( i = 0; i < DBFGetFieldCount(psDBF); i++ )
    {
        DBFGetFieldInfo( psDBF, i, name, NULL, NULL );
        strncpy(name2,name,11);
        str_to_upper(name2);

        if(!strncmp(name1,name2,10))
            return(i);
    }
    return(-1);
}

/************************************************************************/
/*                         DBFIsRecordDeleted()                         */
/*                                                                      */
/*      Returns TRUE if the indicated record is deleted, otherwise      */
/*      it returns FALSE.                                               */
/************************************************************************/

int SHPAPI_CALL DBFIsRecordDeleted( DBFHandle psDBF, int iShape )

{
/* -------------------------------------------------------------------- */
/*      Verify selection.                                               */
/* -------------------------------------------------------------------- */
    if( iShape < 0 || iShape >= psDBF->nRecords )
        return TRUE;

/* -------------------------------------------------------------------- */
/*	Have we read the record?					*/
/* -------------------------------------------------------------------- */
    if( !DBFLoadRecord( psDBF, iShape ) )
        return FALSE;

/* -------------------------------------------------------------------- */
/*      '*' means deleted.                                              */
/* -------------------------------------------------------------------- */
    return psDBF->pszCurrentRecord[0] == '*';
}

/************************************************************************/
/*                        DBFMarkRecordDeleted()                        */
/************************************************************************/

int SHPAPI_CALL DBFMarkRecordDeleted( DBFHandle psDBF, int iShape, 
                                      int bIsDeleted )

{
    char chNewFlag;

/* -------------------------------------------------------------------- */
/*      Verify selection.                                               */
/* -------------------------------------------------------------------- */
    if( iShape < 0 || iShape >= psDBF->nRecords )
        return FALSE;

/* -------------------------------------------------------------------- */
/*      Is this an existing record, but different than the last one     */
/*      we accessed?                                                    */
/* -------------------------------------------------------------------- */
    if( !DBFLoadRecord( psDBF, iShape ) )
        return FALSE;

/* -------------------------------------------------------------------- */
/*      Assign value, marking record as dirty if it changes.            */
/* -------------------------------------------------------------------- */
    if( bIsDeleted )
        chNewFlag = '*';
    else 
        chNewFlag = ' ';

    if( psDBF->pszCurrentRecord[0] != chNewFlag )
    {
        psDBF->bCurrentRecordModified = TRUE;
        psDBF->bUpdated = TRUE;
        psDBF->pszCurrentRecord[0] = chNewFlag;
    }

    return TRUE;
}

/************************************************************************/
/*                            DBFGetCodePage                            */
/************************************************************************/

const char SHPAPI_CALL1(*)
DBFGetCodePage(DBFHandle psDBF )
{
    if( psDBF == NULL )
        return NULL;
    return psDBF->pszCodePage;
}

/************************************************************************/
/*                          DBFDeleteField()                            */
/*                                                                      */
/*      Remove a field from a .dbf file                                 */
/************************************************************************/

int SHPAPI_CALL
DBFDeleteField(DBFHandle psDBF, int iField)
{
    int nOldRecordLength, nOldHeaderLength;
    int nDeletedFieldOffset, nDeletedFieldSize;
    SAOffset nRecordOffset;
    char* pszRecord;
    int i, iRecord;

    if (iField < 0 || iField >= psDBF->nFields)
        return FALSE;

    /* make sure that everything is written in .dbf */
    if( !DBFFlushRecord( psDBF ) )
        return FALSE;

    /* get information about field to be deleted */
    nOldRecordLength = psDBF->nRecordLength;
    nOldHeaderLength = psDBF->nHeaderLength;
    nDeletedFieldOffset = psDBF->panFieldOffset[iField];
    nDeletedFieldSize = psDBF->panFieldSize[iField];

    /* update fields info */
    for (i = iField + 1; i < psDBF->nFields; i++)
    {
        psDBF->panFieldOffset[i-1] = psDBF->panFieldOffset[i] - nDeletedFieldSize;
        psDBF->panFieldSize[i-1] = psDBF->panFieldSize[i];
        psDBF->panFieldDecimals[i-1] = psDBF->panFieldDecimals[i];
        psDBF->pachFieldType[i-1] = psDBF->pachFieldType[i];
    }

    /* resize fields arrays */
    psDBF->nFields--;

    psDBF->panFieldOffset = (int *) 
        SfRealloc( psDBF->panFieldOffset, sizeof(int) * psDBF->nFields );

    psDBF->panFieldSize = (int *) 
        SfRealloc( psDBF->panFieldSize, sizeof(int) * psDBF->nFields );

    psDBF->panFieldDecimals = (int *) 
        SfRealloc( psDBF->panFieldDecimals, sizeof(int) * psDBF->nFields );

    psDBF->pachFieldType = (char *) 
        SfRealloc( psDBF->pachFieldType, sizeof(char) * psDBF->nFields );

    /* update header information */
    psDBF->nHeaderLength -= 32;
    psDBF->nRecordLength -= nDeletedFieldSize;

    /* overwrite field information in header */
    memmove(psDBF->pszHeader + iField*32,
           psDBF->pszHeader + (iField+1)*32,
           sizeof(char) * (psDBF->nFields - iField)*32);

    psDBF->pszHeader = (char *) SfRealloc(psDBF->pszHeader,psDBF->nFields*32);

    /* update size of current record appropriately */
    psDBF->pszCurrentRecord = (char *) SfRealloc(psDBF->pszCurrentRecord,
                                                 psDBF->nRecordLength);

    /* we're done if we're dealing with not yet created .dbf */
    if ( psDBF->bNoHeader && psDBF->nRecords == 0 )
        return TRUE;

    /* force update of header with new header and record length */
    psDBF->bNoHeader = TRUE;
    DBFUpdateHeader( psDBF );

    /* alloc record */
    pszRecord = (char *) dk_alloc(sizeof(char) * nOldRecordLength);

    /* shift records to their new positions */
    for (iRecord = 0; iRecord < psDBF->nRecords; iRecord++)
    {
        nRecordOffset = 
            nOldRecordLength * (SAOffset) iRecord + nOldHeaderLength;

        /* load record */
        psDBF->sHooks.FSeek( psDBF->fp, nRecordOffset, 0 );
        psDBF->sHooks.FRead( pszRecord, nOldRecordLength, 1, psDBF->fp );

        nRecordOffset = 
            psDBF->nRecordLength * (SAOffset) iRecord + psDBF->nHeaderLength;

        /* move record in two steps */
        psDBF->sHooks.FSeek( psDBF->fp, nRecordOffset, 0 );
        psDBF->sHooks.FWrite( pszRecord, nDeletedFieldOffset, 1, psDBF->fp );
        psDBF->sHooks.FWrite( pszRecord + nDeletedFieldOffset + nDeletedFieldSize,
                              nOldRecordLength - nDeletedFieldOffset - nDeletedFieldSize,
                              1, psDBF->fp );

    }

    /* TODO: truncate file */

    /* dk_free  record */
    dk_free (pszRecord, -1);

    psDBF->nCurrentRecord = -1;
    psDBF->bCurrentRecordModified = FALSE;

    return TRUE;
}

/************************************************************************/
/*                          DBFReorderFields()                          */
/*                                                                      */
/*      Reorder the fields of a .dbf file                               */
/*                                                                      */
/* panMap must be exactly psDBF->nFields long and be a permutation      */
/* of [0, psDBF->nFields-1]. This assumption will not be asserted in the*/
/* code of DBFReorderFields.                                            */
/************************************************************************/

int SHPAPI_CALL
DBFReorderFields( DBFHandle psDBF, int* panMap )
{
    SAOffset nRecordOffset;
    int      i, iRecord;
    int     *panFieldOffsetNew;
    int     *panFieldSizeNew;
    int     *panFieldDecimalsNew;
    char    *pachFieldTypeNew;
    char    *pszHeaderNew;
    char    *pszRecord;
    char    *pszRecordNew;

    if ( psDBF->nFields == 0 )
        return TRUE;

    /* make sure that everything is written in .dbf */
    if( !DBFFlushRecord( psDBF ) )
        return FALSE;

    panFieldOffsetNew = (int *) dk_alloc(sizeof(int) * psDBF->nFields);
    panFieldSizeNew = (int *) dk_alloc(sizeof(int) *  psDBF->nFields);
    panFieldDecimalsNew = (int *) dk_alloc(sizeof(int) *  psDBF->nFields);
    pachFieldTypeNew = (char *) dk_alloc(sizeof(char) *  psDBF->nFields);
    pszHeaderNew = (char*) dk_alloc(sizeof(char) * 32 *  psDBF->nFields);

    /* shuffle fields definitions */
    for(i=0; i < psDBF->nFields; i++)
    {
        panFieldSizeNew[i] = psDBF->panFieldSize[panMap[i]];
        panFieldDecimalsNew[i] = psDBF->panFieldDecimals[panMap[i]];
        pachFieldTypeNew[i] = psDBF->pachFieldType[panMap[i]];
        memcpy(pszHeaderNew + i * 32,
               psDBF->pszHeader + panMap[i] * 32, 32);
    }
    panFieldOffsetNew[0] = 1;
    for(i=1; i < psDBF->nFields; i++)
    {
        panFieldOffsetNew[i] = panFieldOffsetNew[i - 1] + panFieldSizeNew[i - 1];
    }

    dk_free (psDBF->pszHeader, -1);
    psDBF->pszHeader = pszHeaderNew;

    /* we're done if we're dealing with not yet created .dbf */
    if ( !(psDBF->bNoHeader && psDBF->nRecords == 0) )
    {
        /* force update of header with new header and record length */
        psDBF->bNoHeader = TRUE;
        DBFUpdateHeader( psDBF );

        /* alloc record */
        pszRecord = (char *) dk_alloc(sizeof(char) * psDBF->nRecordLength);
        pszRecordNew = (char *) dk_alloc(sizeof(char) * psDBF->nRecordLength);

        /* shuffle fields in records */
        for (iRecord = 0; iRecord < psDBF->nRecords; iRecord++)
        {
            nRecordOffset =
                psDBF->nRecordLength * (SAOffset) iRecord + psDBF->nHeaderLength;

            /* load record */
            psDBF->sHooks.FSeek( psDBF->fp, nRecordOffset, 0 );
            psDBF->sHooks.FRead( pszRecord, psDBF->nRecordLength, 1, psDBF->fp );

            pszRecordNew[0] = pszRecord[0];

            for(i=0; i < psDBF->nFields; i++)
            {
                memcpy(pszRecordNew + panFieldOffsetNew[i],
                       pszRecord + psDBF->panFieldOffset[panMap[i]],
                       psDBF->panFieldSize[panMap[i]]);
            }

            /* write record */
            psDBF->sHooks.FSeek( psDBF->fp, nRecordOffset, 0 );
            psDBF->sHooks.FWrite( pszRecordNew, psDBF->nRecordLength, 1, psDBF->fp );
        }

        /* dk_free  record */
        dk_free (pszRecord, -1);
        dk_free (pszRecordNew, -1);
    }

    dk_free (psDBF->panFieldOffset, -1);
    dk_free (psDBF->panFieldSize, -1);
    dk_free (psDBF->panFieldDecimals, -1);
    dk_free (psDBF->pachFieldType, -1);

    psDBF->panFieldOffset = panFieldOffsetNew;
    psDBF->panFieldSize = panFieldSizeNew;
    psDBF->panFieldDecimals =panFieldDecimalsNew;
    psDBF->pachFieldType = pachFieldTypeNew;

    psDBF->nCurrentRecord = -1;
    psDBF->bCurrentRecordModified = FALSE;

    return TRUE;
}


/************************************************************************/
/*                          DBFAlterFieldDefn()                         */
/*                                                                      */
/*      Alter a field definition in a .dbf file                         */
/************************************************************************/

int SHPAPI_CALL
DBFAlterFieldDefn( DBFHandle psDBF, int iField, const char * pszFieldName,
                    char chType, int nWidth, int nDecimals )
{
    int   i;
    int   iRecord;
    int   nOffset;
    int   nOldWidth;
    int   nOldRecordLength;
    int   nRecordOffset;
    char* pszFInfo;
    char  chOldType;
    int   bIsNULL;
    char chFieldFill;

    if (iField < 0 || iField >= psDBF->nFields)
        return FALSE;

    /* make sure that everything is written in .dbf */
    if( !DBFFlushRecord( psDBF ) )
        return FALSE;

    chFieldFill = DBFGetNullCharacter(chType);

    chOldType = psDBF->pachFieldType[iField];
    nOffset = psDBF->panFieldOffset[iField];
    nOldWidth = psDBF->panFieldSize[iField];
    nOldRecordLength = psDBF->nRecordLength;

/* -------------------------------------------------------------------- */
/*      Do some checking to ensure we can add records to this file.     */
/* -------------------------------------------------------------------- */
    if( nWidth < 1 )
        return -1;

    if( nWidth > 255 )
        nWidth = 255;

/* -------------------------------------------------------------------- */
/*      Assign the new field information fields.                        */
/* -------------------------------------------------------------------- */
    psDBF->panFieldSize[iField] = nWidth;
    psDBF->panFieldDecimals[iField] = nDecimals;
    psDBF->pachFieldType[iField] = chType;

/* -------------------------------------------------------------------- */
/*      Update the header information.                                  */
/* -------------------------------------------------------------------- */
    pszFInfo = psDBF->pszHeader + 32 * iField;

    for( i = 0; i < 32; i++ )
        pszFInfo[i] = '\0';

    if( (int) strlen(pszFieldName) < 10 )
        strncpy( pszFInfo, pszFieldName, strlen(pszFieldName));
    else
        strncpy( pszFInfo, pszFieldName, 10);

    pszFInfo[11] = psDBF->pachFieldType[iField];

    if( chType == 'C' )
    {
        pszFInfo[16] = (unsigned char) (nWidth % 256);
        pszFInfo[17] = (unsigned char) (nWidth / 256);
    }
    else
    {
        pszFInfo[16] = (unsigned char) nWidth;
        pszFInfo[17] = (unsigned char) nDecimals;
    }

/* -------------------------------------------------------------------- */
/*      Update offsets                                                  */
/* -------------------------------------------------------------------- */
    if (nWidth != nOldWidth)
    {
        for (i = iField + 1; i < psDBF->nFields; i++)
             psDBF->panFieldOffset[i] += nWidth - nOldWidth;
        psDBF->nRecordLength += nWidth - nOldWidth;

        psDBF->pszCurrentRecord = (char *) SfRealloc(psDBF->pszCurrentRecord,
                                                     psDBF->nRecordLength);
    }

    /* we're done if we're dealing with not yet created .dbf */
    if ( psDBF->bNoHeader && psDBF->nRecords == 0 )
        return TRUE;

    /* force update of header with new header and record length */
    psDBF->bNoHeader = TRUE;
    DBFUpdateHeader( psDBF );

    if (nWidth < nOldWidth || (nWidth == nOldWidth && chType != chOldType))
    {
        char* pszRecord = (char *) dk_alloc(sizeof(char) * nOldRecordLength);
        char* pszOldField = (char *) dk_alloc(sizeof(char) * (nOldWidth + 1));

        pszOldField[nOldWidth] = 0;

        /* move records to their new positions */
        for (iRecord = 0; iRecord < psDBF->nRecords; iRecord++)
        {
            nRecordOffset =
                nOldRecordLength * (SAOffset) iRecord + psDBF->nHeaderLength;

            /* load record */
            psDBF->sHooks.FSeek( psDBF->fp, nRecordOffset, 0 );
            psDBF->sHooks.FRead( pszRecord, nOldRecordLength, 1, psDBF->fp );

            memcpy(pszOldField, pszRecord + nOffset, nOldWidth);
            bIsNULL = DBFIsValueNULL( chOldType, pszOldField );

            if (nWidth != nOldWidth)
            {
                if ((chOldType == 'N' || chOldType == 'F') && pszOldField[0] == ' ')
                {
                    /* Strip leading spaces when truncating a numeric field */
                    memmove( pszRecord + nOffset,
                            pszRecord + nOffset + nOldWidth - nWidth,
                            nWidth );
                }
                if (nOffset + nOldWidth < nOldRecordLength)
                {
                    memmove( pszRecord + nOffset + nWidth,
                            pszRecord + nOffset + nOldWidth,
                            nOldRecordLength - (nOffset + nOldWidth));
                }
            }

            /* Convert null value to the appropriate value of the new type */
            if (bIsNULL)
            {
                memset( pszRecord + nOffset, chFieldFill, nWidth);
            }

            nRecordOffset =
                psDBF->nRecordLength * (SAOffset) iRecord + psDBF->nHeaderLength;

            /* write record */
            psDBF->sHooks.FSeek( psDBF->fp, nRecordOffset, 0 );
            psDBF->sHooks.FWrite( pszRecord, psDBF->nRecordLength, 1, psDBF->fp );
        }

        dk_free (pszRecord, -1);
        dk_free (pszOldField, -1);
    }
    else if (nWidth > nOldWidth)
    {
        char* pszRecord = (char *) dk_alloc(sizeof(char) * psDBF->nRecordLength);
        char* pszOldField = (char *) dk_alloc(sizeof(char) * (nOldWidth + 1));

        pszOldField[nOldWidth] = 0;

        /* move records to their new positions */
        for (iRecord = psDBF->nRecords - 1; iRecord >= 0; iRecord--)
        {
            nRecordOffset =
                nOldRecordLength * (SAOffset) iRecord + psDBF->nHeaderLength;

            /* load record */
            psDBF->sHooks.FSeek( psDBF->fp, nRecordOffset, 0 );
            psDBF->sHooks.FRead( pszRecord, nOldRecordLength, 1, psDBF->fp );

            memcpy(pszOldField, pszRecord + nOffset, nOldWidth);
            bIsNULL = DBFIsValueNULL( chOldType, pszOldField );

            if (nOffset + nOldWidth < nOldRecordLength)
            {
                memmove( pszRecord + nOffset + nWidth,
                         pszRecord + nOffset + nOldWidth,
                         nOldRecordLength - (nOffset + nOldWidth));
            }

            /* Convert null value to the appropriate value of the new type */
            if (bIsNULL)
            {
                memset( pszRecord + nOffset, chFieldFill, nWidth);
            }
            else
            {
                if ((chOldType == 'N' || chOldType == 'F'))
                {
                    /* Add leading spaces when expanding a numeric field */
                    memmove( pszRecord + nOffset + nWidth - nOldWidth,
                             pszRecord + nOffset, nOldWidth );
                    memset( pszRecord + nOffset, ' ', nWidth - nOldWidth );
                }
                else
                {
                    /* Add trailing spaces */
                    memset(pszRecord + nOffset + nOldWidth, ' ', nWidth - nOldWidth);
                }
            }

            nRecordOffset =
                psDBF->nRecordLength * (SAOffset) iRecord + psDBF->nHeaderLength;

            /* write record */
            psDBF->sHooks.FSeek( psDBF->fp, nRecordOffset, 0 );
            psDBF->sHooks.FWrite( pszRecord, psDBF->nRecordLength, 1, psDBF->fp );
        }

        dk_free (pszRecord, -1);
        dk_free (pszOldField, -1);
    }

    psDBF->nCurrentRecord = -1;
    psDBF->bCurrentRecordModified = FALSE;

    return TRUE;
}
