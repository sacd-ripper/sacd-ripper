#include <sys/stat.h>
#include <stdio.h>

#ifdef _WIN32
typedef int   mode_t;
 #endif

void debugLog(const char* format, ...);

void fatalError(const char* message);

// substitute various items into a formatted string (similar to printf)
//
// format - the format of the filename
// tracknum - gets substituted for %N in format
// year - gets substituted for %Y in format
// artist - gets substituted for %A in format
// album - gets substituted for %L in format
// title - gets substituted for %T in format
//
// NOTE: caller must free the returned string!
char * parse_format(const char * format, int tracknum, const char * year, const char * artist, const char * album, const char * title);

// construct a filename from various parts
//
// path - the path the file is placed in (don't include a trailing '/')
// dir - the parent directory of the file (don't include a trailing '/')
// file - the filename
// extension - the suffix of a file (don't include a leading '.')
//
// NOTE: caller must free the returned string!
// NOTE: any of the parameters may be NULL to be omitted
char * make_filename(const char * path, const char * dir, const char * file, const char * extension);

void make_playlist(const char* pathAndName, FILE** file);

// reads an entire line from a file and returns it
//
// NOTE: caller must free the returned string!
char * read_line(int fd);

// reads an entire line from a file and turns it into a number
int read_line_num(int fd);

int recursive_mkdir(char* pathAndName, mode_t mode);

int recursive_parent_mkdir(char* pathAndName, mode_t mode);

// searches $PATH for the named program
// returns 1 if found, 0 otherwise
int program_exists(const char * name);

// removes leading and trailing whitespace as defined by isspace()
//
// str - the string to trim
void trim_whitespace(char * str);

// removes all instances of bad characters from the string
//
// str - the string to trim
// bad - the sting containing all the characters to remove
void trim_chars(char * str, const char * bad);

// LNR - It's possible that some files may end up on a MS file system,
// so it's best to disallow MS invalid chars as well. I also disallow
// period (dot) because it screws up my file name database software. YMMV
#define BADCHARS    "./?*|><:\"\\"
