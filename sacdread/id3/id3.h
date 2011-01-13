/*
 * id3 include stuff
 */

/* ====================================================================
Copyright (c) 2006, Tangent Org
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

    * Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.

    * Redistributions in binary form must reproduce the above
copyright notice, this list of conditions and the following disclaimer
in the documentation and/or other materials provided with the
distribution.

    * Neither the name of TangentOrg nor the names of its
contributors may be used to endorse or promote products derived from
this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * ====================================================================
 */


#ifndef __ID3_H__
#define __ID3_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include <strings.h>
//#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <assert.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <ctype.h>

#define VERSION_SIZE 8

typedef unsigned int id3flags;
typedef struct ID3 ID3;

struct ID3 {
	char version[VERSION_SIZE]; //It will never be this large!
	char *filename; // Filename is only added when available, will be null terminated
	char *title;
	size_t title_size;
	char *artist;
	size_t artist_size;
	char *album;
	size_t album_size;
	char *year;
	size_t year_size;
	char *comment;
	size_t comment_size;
	char *track;
	size_t track_size;
	char *genre;
	size_t genre_size;
	char *encoder;
	size_t encoder_size;
	char *blob;
	char signature[33];
	void (*processor)(ID3 *info, const char *name, const char *ptr, size_t length);
        char *data;
	size_t data_size;      // Length of the actual tag
	void *passable;        // Void pointer you can use in your processor() callback function
	unsigned char *buffer; // Block of memory used to hold tags
	unsigned char *ptr;    // Pointer in block of memory used to hold tags
	size_t length;         // Size of the data in ptr
	size_t size;           // Size of the ptr
	size_t tag_length;     // Length of the actual tag
	char user_memory;      // If the user supplied memory to use for ptr
	char allocated;        // If the ID3 object was allocated
	id3flags mask;
	id3flags mask_found;
};

#define GENRE_MAX 148
#define ID3_INIT_SIZE 8192

#define isframeid(a) (isupper(a) || isdigit(a))
#define HUGE_STRING_LEN 8192

ID3 * create_ID3(ID3 *);
int destroy_ID3(ID3 *blob);
int parse_file_ID3(ID3 *info, char *filename);
int parse_blob_ID3(ID3 *info, unsigned char *blob, size_t blob_length);
void set_memory_ID3(ID3 *info, unsigned char *ptr, size_t size) ;
void set_flags_ID3(ID3 *info, id3flags mask);
int ID3_to_file(char *filename, char *newfile);


// Error Codes
typedef enum {
  ID3_OK= 0,
  ID3_ERR_EMPTY_FILE= 1,
  ID3_ERR_NO_TAG= 2,
  ID3_ERR_UNSUPPORTED_FORMAT= 3,
} id3_return;

typedef enum {
  ID3_VERSION_1= 1,
  ID3_VERSION_2= 2,
} id3_version;

#define TITLE_TAG        1              /* Title of the track*/
#define ARTIST_TAG       2              /* Performing Artist */
#define ALBUM_TAG        4              /* Album */
#define YEAR_TAG         8              /* Year for the track */
#define COMMENT_TAG     16              /* Comment */
#define TRACK_TAG       32              /* Track on the Album */
#define GENRE_TAG       64              /* Music Genre */
#define ENCODER_TAG    128              /* Encoder for the track */
#define VERSION_TAG    256              /* Version of the Tag */
#define KEEP_TAG       512              /* Encoder for the track */
#define SIGNATURE_TAG 1024              /* Create MD5 signature of the file */
#define KEEP_BLOB     2048              /* Keep the blob that the file used */
#define VERSION1_TAG        (TITLE_TAG|ARTIST_TAG|ALBUM_TAG|YEAR_TAG|COMMENT_TAG|TRACK_TAG|GENRE_TAG)        /* Encoder for the track */
#define ALL_TAG        (TITLE_TAG|ARTIST_TAG|ALBUM_TAG|YEAR_TAG|COMMENT_TAG|TRACK_TAG|GENRE_TAG|ENCODER_TAG|VERSION_TAG|SIGNATURE_TAG)        /* Encoder for the track */

#define DEBUG 1
#ifdef DEBUG
#define DEBUG_ENTER(A) const char *DEBUG_FUNCTION=A;printf("Entering %s\n", DEBUG_FUNCTION);fflush(stdout);
#define DEBUG_RETURN_MESSAGE(A,B) {printf("Leaving %s: ", DEBUG_FUNCTION);printf(A);printf("\n");fflush(stdout);return B;}
#define DEBUG_RETURN_VOID printf("Leaving %s\n", DEBUG_FUNCTION);fflush(stdout);
#define DEBUG_PRINT(A) printf("\t%s: ", DEBUG_FUNCTION); printf(A);printf("\n");fflush(stdout);
#define DEBUG_RETURN(A) {printf("Leaving %s\n", DEBUG_FUNCTION);fflush(stdout);return A;}
#define DEBUG_ASSERT(A) assert(A);
#define WATCHPOINT printf("WATCHPOINT %s %d\n", __FILE__, __LINE__);fflush(stdout);
#else
#define DEBUG_ENTER(a);
#define DEBUG_RETURN(a); 
#define DEBUG_RETURN_MESSAGE(A,B);
#define DEBUG_RETURN_VOID;
#define DEBUG_PRINT(A);
#define DEBUG_ASSERT(A);
#define WATCHPOINT;
#endif

#endif
