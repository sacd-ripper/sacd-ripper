/*
 **  id3.c
 **  $Id: id3.c,v 1.22 2005/03/28 23:25:03 brian Exp $
 **  This is something I really did not want to write.
 **
 **  It was pretty annoying that I had to write this
 **  but I could not find one single ID3 library
 **  that had a BSD license and was written in C.
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

#include "id3.h"
#include <config.h>
#include "md5.h"

void md5_signature(const unsigned char *buf, int length, char *result);

#ifdef _MSC_VER
 #define snprintf _snprintf
#endif /* _MSC_VER */

const char *id3_genres[GENRE_MAX] =
{
  "Blues", "Classic Rock", "Country", "Dance", "Disco", "Funk", "Grunge", "Hip-Hop",
  "Jazz", "Metal", "New Age", "Oldies", "Other", "Pop", "R&B", "Rap", "Reggae",
  "Rock", "Techno", "Industrial", "Alternative", "Ska", "Death Metal", "Pranks", "Soundtrack",
  "Euro-Techno", "Ambient", "Trip-Hop", "Vocal", "Jazz+Funk", "Fusion", "Trance", "Classical",
  "Instrumental", "Acid", "House", "Game", "Sound Clip", "Gospel", "Noise", "Alt",
  "Bass", "Soul", "Punk", "Space", "Meditative", "Instrumental Pop",
  "Instrumental Rock", "Ethnic", "Gothic", "Darkwave", "Techno-Industrial", "Electronic",
  "Pop-Folk", "Eurodance", "Dream", "Southern Rock", "Comedy", "Cult",
  "Gangsta Rap", "Top 40", "Christian Rap", "Pop/Funk", "Jungle", "Native American",
  "Cabaret", "New Wave", "Psychedelic", "Rave", "Showtunes", "Trailer", "Lo-Fi", "Tribal",
  "Acid Punk", "Acid Jazz", "Polka", "Retro", "Musical", "Rock & Roll", "Hard Rock", "Folk",
  "Folk/Rock", "National Folk", "Swing", "Fast-Fusion", "Bebob", "Latin", "Revival",
  "Celtic", "Bluegrass", "Avantgarde", "Gothic Rock", "Progressive Rock",
  "Psychedelic Rock", "Symphonic Rock", "Slow Rock", "Big Band", "Chorus", "Easy Listening",
  "Acoustic", "Humour", "Speech", "Chanson", "Opera", "Chamber Music", "Sonata", "Symphony",
  "Booty Bass", "Primus", "Porn Groove", "Satire", "Slow Jam", "Club", "Tango",
  "Samba", "Folklore", "Ballad", "Power Ballad", "Rhythmic Soul", "Freestyle", "Duet",
  "Punk Rock", "Drum Solo", "A Cappella", "Euro-House", "Dance Hall", "Goa",
  "Drum & Bass", "Club-House", "Hardcore", "Terror", "Indie", "BritPop", "Negerpunk",
  "Polsk Punk", "Beat", "Christian Gangsta Rap", "Heavy Metal", "Black Metal", "Crossover",
  "Contemporary Christian", "Christian Rock", "Merengue", "Salsa", "Thrash Metal",
  "Anime", "JPop", "Synthpop"
};

static void print_char (unsigned char *foo, int length) 
{
  int x=0;
  printf("String %d\n", length);
  for (; x < length; x++ )
    printf("%d: %c:%d\n", x, foo[x], foo[x]);
  putchar('\n');
}

static const char * genre_string(int genre) 
{
  if (genre < GENRE_MAX && genre > -1)
    return id3_genres[genre];

  return NULL;
}

static void clean_string(unsigned char *string, int length) 
{
  unsigned char *ptr= string;

  for(; (ptr - string) < length; ptr++) 
    if (!isprint(*ptr)) 
      *ptr= ' ';

}

static char * add_tag(ID3 *info, const char *tag, size_t length) 
{
  unsigned char *begin;

  // If the current tag is too big for our block of memory
  if ((length + info->length) > info->size) 
  {
    if ( info->user_memory) 
    {
      // This because we didn't allocate the memory in the first place, and this should be an error
      return NULL;
    }
    else if ( info->size == 0 ) 
    {
      info->buffer= (unsigned char *)malloc(ID3_INIT_SIZE);
      info->size= ID3_INIT_SIZE;
      info->ptr= info->buffer;
    }
    else if ( length > info->tag_length  ) 
    {
      info->buffer= realloc(info->buffer, info->tag_length);
      info->ptr= info->buffer + info->length;
      info->size= info->tag_length;
    }
    else
    {
      /*
        This should never, ever happen
        since we don't support compressed tags just yet
      */
      return NULL;
    }
  }
  begin= info->ptr;
  memcpy(info->ptr, tag, length);	
  clean_string(info->ptr, length);
  info->length += length + 1;
  info->ptr += length;
  *info->ptr++= '\0';

  // Now return the position of the last string
  return (char *)begin;
}

static void select_tag(ID3 *info, const char *name, const char *ptr, size_t length) 
{
  if (!strcmp(name, "title"))
  {
    info->title= add_tag(info, ptr, length);
    info->title_size= length;
  }
  else if (!strcmp(name, "artist"))
  {
    info->artist= add_tag(info, ptr, length);
    info->artist_size= length;
  }
  else if (!strcmp(name, "album"))
  {
    info->album= add_tag(info, ptr, length);
    info->album_size= length;
  }
  else if (!strcmp(name, "year"))
  {
    info->year= add_tag(info, ptr, length);
    info->year_size= length;
  }
  else if (!strcmp(name, "comment"))
  {
    info->comment= add_tag(info, ptr, length);
    info->comment_size= length;
  }
  else if (!strcmp(name, "track"))
  {
    info->track= add_tag(info, ptr, length);
    info->track_size= length;
  }
  else if (!strcmp(name, "genre"))
  {
    info->genre= add_tag(info, ptr, length);
    info->genre_size= length;
  }
  else if (!strcmp(name, "encoder"))
  {
    info->encoder= add_tag(info, ptr, length);
    info->encoder_size= length;
  }
}


static size_t id3_size(const unsigned char* buffer) 
{
  size_t size = 0;
  size=  (buffer[3] & 0x7f) + 
    ((buffer[2] & 0x7f) << 7) +
    ((buffer[1] & 0x7f) << 14) +
    ((buffer[0] & 0x7f) << 21);

  return size;
}

static size_t id3_size2(const unsigned char * buffer) 
{
  size_t size = 0;
  size=  (buffer[2] & 0x7f) + 
    ((buffer[1] & 0x7f) << 7) +
    ((buffer[0] & 0x7f) << 14);

  return size;
}

static size_t get_framesize(const unsigned char *buffer) 
{
  return ((buffer[6] << 8) + (buffer[7]));
}

static int get_id3v1_tag (ID3 *info, unsigned char *blob, size_t blob_length) 
{
  unsigned char *ptr= (unsigned char *)blob;
  const char *genre= NULL;

  ptr += (blob_length - 128);

  if (!memcmp(ptr, "TAG", 3)) 
  {
    /* Paranoid, not all systems are made equally */
    ptr +=3;

    if (info->mask & TITLE_TAG)
      info->processor(info, "title", (char *)ptr, 30);
    ptr +=30;

    if (info->mask & ARTIST_TAG)
      info->processor(info, "artist", (char *)ptr, 30);
    ptr +=30;

    if (info->mask & ALBUM_TAG)
      info->processor(info, "album", (char *)ptr, 30);
    ptr +=30;

    if (info->mask & YEAR_TAG)
      info->processor(info, "year", (char *)ptr, 4);
    ptr +=4;

    if (info->mask & COMMENT_TAG)
      info->processor(info, "comment", (char *)ptr, 30);
    ptr +=30;

    if (info->mask & GENRE_TAG)
      genre= genre_string((int)ptr[0]);

    if (genre)
      info->processor(info, "genre", (char *)genre, strlen(genre));

    sprintf(info->version, "1");

    return 0;
  }

  return ID3_ERR_NO_TAG;
}

static int id_2_2(ID3 *info, unsigned char *blob) 
{
  unsigned char *ptr= blob;

  while (info->tag_length > (size_t) (ptr - blob)) 
  {
    size_t size= id3_size2(ptr+3);
    char *compare_ptr= (char *)ptr;

    if (size <= 0)
    {
      return 0;
    }

    if (size + (ptr - blob) > info->tag_length) 
      break;

    if (!strncmp("TP1", compare_ptr, 3) && (info->mask & ARTIST_TAG))
    {
      info->processor(info, "artist", compare_ptr + 6, size);
      info->mask_found |= ARTIST_TAG;
    }
    else if (!strncmp("TT2", compare_ptr, 3) && (info->mask & TITLE_TAG)) 
    {
      info->processor(info, "title", compare_ptr + 6, size);
      info->mask_found |= TITLE_TAG;
    }
    else if (!strncmp("TAL", compare_ptr, 3) && (info->mask & ALBUM_TAG)) 
    {
      info->processor(info, "album", compare_ptr + 6, size);
      info->mask_found |= ALBUM_TAG;
    }
    else if (!strncmp("TRK", compare_ptr, 3) && (info->mask & TRACK_TAG)) 
    {
      info->processor(info, "track", compare_ptr + 6, size);
      info->mask_found |= TRACK_TAG;
    }
    else if (!strncmp("TEN", compare_ptr, 3) && (info->mask & ENCODER_TAG)) 
    {
      info->processor(info, "encoder", compare_ptr + 6, size);
      info->mask_found |= ENCODER_TAG;
    }
    else if (!strncmp("TYE", compare_ptr, 3) && (info->mask & YEAR_TAG)) 
    {
      info->processor(info, "year", compare_ptr + 6, size);
      info->mask_found |= YEAR_TAG;
    }
    else if (!strncmp("COM", compare_ptr, 3) && (info->mask & COMMENT_TAG)) 
    {
      info->processor(info, "comment", compare_ptr + 6, size);
      info->mask_found |= COMMENT_TAG;
    }
    else if (!strncmp("TCO", compare_ptr, 3) && (info->mask & GENRE_TAG)) 
    {
      info->processor(info, "genre", compare_ptr + 6, size);
      info->mask_found |= GENRE_TAG;
    } 
    else
    {
      char temp[4];
      memset(temp, 0, 4);
      memcpy(temp, compare_ptr, 3);
      info->processor(info, temp, compare_ptr + 6, size);
    }

    if (info->mask_found == info->mask)
      return 0;

    ptr += size + 6;
  }

  return 0;
}

static int id_2_3(ID3 *info, unsigned char *blob) 
{
  unsigned char *ptr= blob;

  while (info->tag_length > (size_t) (ptr - blob)) 
  {
    size_t size= get_framesize(ptr);
    char *compare_ptr= (char *)ptr;

    if (size + (ptr - blob) > info->tag_length) 
      break;

    if (size <= 0)
      return 0;

    if (!strncmp("TPE1", compare_ptr, 4) && (info->mask & ARTIST_TAG)) 
    {
      info->processor(info, "artist", compare_ptr +10, size);
      info->mask_found |= ARTIST_TAG;
    }
    else if (!strncmp("TIT2", compare_ptr, 4) && (info->mask & TITLE_TAG)) 
    {
      info->processor(info, "title", compare_ptr +10, size);
      info->mask_found |= TITLE_TAG;
    }
    else if (!strncmp("TALB", compare_ptr, 4) && (info->mask & ALBUM_TAG)) 
    {
      info->processor(info, "album", compare_ptr +10, size);
      info->mask_found |= ARTIST_TAG;
    }
    else if (!strncmp("TRCK",compare_ptr,4) && (info->mask & TRACK_TAG)) 
    {
      info->processor(info, "track", compare_ptr +10, size);
      info->mask_found |= TRACK_TAG;
    }
    else if (!strncmp("TYER",compare_ptr,4) && (info->mask & YEAR_TAG)) 
    {
      info->processor(info, "year", compare_ptr +10, size);
      info->mask_found |= YEAR_TAG;
    }
    else if (!strncmp("TENC",compare_ptr,4) && (info->mask & ENCODER_TAG)) 
    {
      info->processor(info, "encoder", compare_ptr +10, size);
      info->mask_found |= ENCODER_TAG;
    }
    else if (!strncmp("COMM",compare_ptr,4) && (info->mask & COMMENT_TAG)) 
    {
      info->processor(info, "commment", compare_ptr +10, size);
      info->mask_found |= COMMENT_TAG;
    }
    else
    {
      char temp[5];
      memset(temp, 0, 5);
      memcpy(temp, compare_ptr, 4);
      info->processor(info, temp, compare_ptr + 10, size);
    }
    if (info->mask_found == info->mask)
      return 0;

    ptr += 10 + size;
  }

  return 0;
}

static int process_extended_header(ID3 *info, unsigned char *blob) 
{
  unsigned long CRC= 0;
  unsigned long paddingsize= 0;

  /*
    Shortcut. The extended header size will be either 6 or 10 bytes. 
    If it's ten bytes, it means that there's CRC data (though we check
    the flag anyway). I'm gonna save it, though I'll be damned if I 
    know what to do with it.
  */
  if ((blob[3] == 0x0A) && (blob[4])) 
    CRC= (blob[10] << 24) + (blob[11] << 16) +
      (blob[12] << 8) + (blob[13]);

  paddingsize= (blob[6] << 24) + (blob[7] << 16) +
    (blob[8] << 8) + (blob[9]);

  /*subtract the size of the padding from the size of the tag */
  info->tag_length -= paddingsize;

  /*continue decoding the frames */
  return id_2_3(info, blob);
}

static int get_id3v2_tag (ID3 *info, unsigned char *blob, size_t blob_length) 
{
  int unsynchronized= 0;
  int hasExtendedHeader= 0;
  unsigned char *ptr_buffer= blob;

  if (!strncmp((char *)ptr_buffer, "ID3", 3)) 
  {
    info->tag_length= id3_size(ptr_buffer+6);
    snprintf(info->version, VERSION_SIZE, "2.%d.%d", blob[3], blob[4]);

    if ((blob[5] & 0x40) >> 6) 
      hasExtendedHeader= 1;

    if ((blob[5] & 0x80) >> 7) 
      unsynchronized= 1;

    /* We can not handle experimental tags */
    if ((blob[5] & 0x20) >> 5) 
      return 1;


#ifdef NOT_SUPPORTED
    if (unsynchronized) 
    {
      register int i,j; /*index*/
      /*
        Replace every instance of '0xFF 0x00'
        with '0xFF'
      */
      for(i=10; i < info->tag_length; i++) 
        if (blob[i] == 0xFF && blob[i+1] == 0x00) 
          for(j=i+1; i < info->tag_length; i++) 
            blob[j]= blob[j+1];

    }
#endif

    /* Move past the above */
    ptr_buffer += 10;
    /*If the tag has an extended header, parse it*/
    if (hasExtendedHeader) 
      return process_extended_header(info, blob);
    else if (blob[3] == 2) 
      return id_2_2(info, ptr_buffer);
    else if (blob[3] == 3) 
      return id_2_3(info, ptr_buffer);
    // Yes, I will need to fix this at some point
    else if (blob[3] == 4) 
      return id_2_3(info, ptr_buffer);
    else 
      return ID3_ERR_UNSUPPORTED_FORMAT;
  }

  return 1;
}

#if 0
int ID3_to_file(char *filename, char *newfile) 
{
  int rc= ID3_OK;
  caddr_t blob, tag;
  struct stat blob_buf;
  int fd= -1;
  unsigned char *ptr= NULL;

  size_t tag_length= 0;

  if (stat(filename, &blob_buf)) 
    return -1;


  if ((fd= open(filename, O_RDONLY)) ==  -1) 
    return -1;

  blob= mmap(NULL, blob_buf.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
  if ((blob == NULL) || (blob ==(caddr_t)-1)) 
    return ID3_ERR_EMPTY_FILE;

  close(fd);

  // Minimal size blob
  if (blob_buf.st_size  < 10)
  {
    munmap(blob, blob_buf.st_size);
    return ID3_ERR_NO_TAG;
  }



  if ((blob_buf.st_size > 128)) 
  {
    ptr= (unsigned char *)blob + (blob_buf.st_size - 128);
    //  Set ptr back to NULL if it is not pointing to a tag
    if (memcmp(ptr, "TAG", 3)) 
      ptr= NULL;
  }

  if (!strncmp(blob, "ID3", 3)) 
  {
    tag_length= id3_size((unsigned char *)blob+6);
  }

  if (tag_length || ptr)
  {
    FILE *new= NULL;
    size_t total= tag_length + (ptr ? 128 : 0);

    if(!(new= fopen(newfile,"w+")))
      return -1;
    ftruncate(fileno(new), total);
    tag= mmap(NULL, total, PROT_READ | PROT_WRITE, MAP_SHARED, fileno(new), 0);
    if ((tag == NULL) || (tag ==(caddr_t)-1)) 
      return ID3_ERR_EMPTY_FILE;

    fclose(new);

    if (tag_length)
      memcpy(tag, blob, tag_length);

    if (ptr)
    {
      memcpy(tag + tag_length, ptr, 128);
    }
    munmap(tag, total);
  }

  munmap(blob, blob_buf.st_size);

  return rc;
}
#endif

#if 0
int parse_file_ID3(ID3 *info, char *filename) 
{
  int rc= ID3_OK;
  caddr_t blob;
  int fd= -1;
  size_t blocksize= (size_t)getpagesize();
  struct stat sb;

  if ((fd= open(filename, O_RDONLY)) ==  -1) 
    return -1;

  (void)fstat(fd, &sb);
  // We set blocksize to the smaller of the two unless we are doing
  // a signature. If we need to get the signature or the KEEP_BLOB flag has
  // been set then we just grab the enire thing.
  if (info->mask & SIGNATURE_TAG || info->mask & KEEP_BLOB)
    blocksize= sb.st_size;
  else
    blocksize= sb.st_size < blocksize ? sb.st_size : blocksize;

  blob= mmap(NULL, blocksize, PROT_READ, MAP_PRIVATE, fd, 0);
  if (blob ==(caddr_t)-1) 
  {
    close(fd);
    return ID3_ERR_EMPTY_FILE;
  }

  // We try to minimize the amount of the blob we are about to put into
  // memory. So if we don't find a tag and we haven't allocated the entire
  // blob lets peel out the size of the header and see if we have the entire
  // block. If we don't we will have to unmap and remap to the right size.
  if (blocksize > 3 && !strncmp(blob, "ID3", 3) && !(info->mask & KEEP_BLOB))
  {
    size_t length= id3_size((unsigned char *)blob+6);
    // This is only needed on very large tags
    if (length > blocksize)
    {
      munmap(blob, blocksize);
      blocksize= length;
      blob= mmap(NULL, blocksize, PROT_READ, MAP_PRIVATE, fd, 0);
      if ((blob == NULL) || (blob ==(caddr_t)-1)) 
        return ID3_ERR_EMPTY_FILE;
    }
  }

  close(fd); //FIXME

  rc= parse_blob_ID3(info, (unsigned char *)blob, blocksize);

  info->filename= filename;
  if (info->mask & KEEP_BLOB)
  {
    info->data= blob;
    info->data_size= blocksize;
  } 
  else
  {
    munmap(blob, blocksize);
    info->data_size=0; //Make sure this is set to zero
  }

  return rc;
}
#endif

int parse_blob_ID3(ID3 *info, unsigned char *blob, size_t blob_length) 
{
  int rc= ID3_ERR_NO_TAG;

  // Sometimes people encode both headers, so we test for both

  // Why 127 instead of 128? The file may only have a tag in it
  if ((blob_length > 127) && !get_id3v1_tag(info, blob, blob_length)) 
    rc= ID3_OK;

  if (!get_id3v2_tag(info, blob, blob_length))
    rc= ID3_OK;

  if (info->mask & KEEP_BLOB || info->mask & SIGNATURE_TAG)
    md5_signature(blob, blob_length, info->signature);

  return rc;
}

int destroy_ID3(ID3 *info) 
{
  DEBUG_ENTER("destroy_ID3");
  if (info && !info->user_memory)
    free(info->buffer);
#if 0
  if (info->mask & KEEP_BLOB)
    munmap(info->data, info->data_size);
#endif
  if (info->allocated)
    free(info);

  DEBUG_RETURN(0); //Eventually we should do more here
}

void set_flags_ID3(ID3 *info, id3flags mask) 
{
  info->mask= mask;
}

void set_memory_ID3(ID3 *info, unsigned char *ptr, size_t size) 
{
  if (info->size)
    return;
  info->user_memory= 1;
  info->buffer= ptr;
  info->ptr= ptr;
  info->size= size;
}

ID3 * create_ID3(ID3 *info) 
{
  //At the moment ID3 is just a structure
  if (info) 
  {
    id3flags mask= info->mask;
    void *func= info->processor;
    size_t size= info->size;
    unsigned char *buffer= info->buffer;
    int user_memory= info->user_memory;
    memset(info, 0, sizeof(ID3));
    info->size= size;
    info->buffer= buffer;
    info->mask= mask;
    info->user_memory= user_memory;
    info->allocated= info->allocated;
    info->processor= func ? func : select_tag;
    info->ptr= info->buffer;
  } 
  else 
  {
    info= (ID3 *)malloc(sizeof(ID3));
    if (info) 
      memset(info, 0, sizeof(ID3));
    else {
      assert(0);
      return NULL;
    }

    info->allocated= 1;
    info->processor= select_tag;
    info->mask= ALL_TAG;
  }

  return info;
}
