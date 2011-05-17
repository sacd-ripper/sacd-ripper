#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <limits.h>
#include <ctype.h>
#include <errno.h>
#include <stdarg.h>

#include "util.h"

void debugLog(const char* format, ...)
{
    va_list   ap;
    const char* p = format;

    FILE      * logFile = NULL;

    logFile = fopen("LOG_FILE", "a");
    if (logFile == NULL)
        return;

    va_start(ap, format);

    while (*p && *(p + 1))
    {
        if (*p == '%')
        {
            if (*(p + 1) == 'd')
            {
                fprintf(logFile, "%d", va_arg(ap, int));
#ifdef DEBUG
                printf("%d", va_arg(ap, int));
#endif
            }
            else if (*(p + 1) == 'x')
            {
                fprintf(logFile, "%x", va_arg(ap, unsigned int));
#ifdef DEBUG
                printf("%x", va_arg(ap, unsigned int));
#endif
            }
            else if (*(p + 1) == 'X')
            {
                fprintf(logFile, "%X", va_arg(ap, unsigned int));
#ifdef DEBUG
                printf("%X", va_arg(ap, unsigned int));
#endif
            }
            else if (*(p + 1) == 'o')
            {
                fprintf(logFile, "%o", va_arg(ap, unsigned int));
#ifdef DEBUG
                printf("%o", va_arg(ap, unsigned int));
#endif
            }
            else if (*(p + 1) == 'u')
            {
                fprintf(logFile, "%u", va_arg(ap, unsigned int));
#ifdef DEBUG
                printf("%u", va_arg(ap, unsigned int));
#endif
            }
            else if (*(p + 1) == '.' && *(p + 2) && *(p + 2) >= '0' && *(p + 2) <= '9' &&
                     *(p + 3) && *(p + 3) == 'f')
            {
                fprintf(logFile, "%.2lf", va_arg(ap, double));
#ifdef DEBUG
                printf("%.2lf", va_arg(ap, double));
#endif
                p += 2;
            }
            else if (*(p + 1) == 'f')
            {
                fprintf(logFile, "%.2lf", va_arg(ap, double));
#ifdef DEBUG
                printf("%.2lf", va_arg(ap, double));
#endif
            }
            else if (*(p + 1) == '.' && *(p + 2) && *(p + 2) >= '0' && *(p + 2) <= '9' &&
                     *(p + 3) && *(p + 3) == 'l' && *(p + 4) && *(p + 4) == 'f')
            {
                fprintf(logFile, "%.2lf", va_arg(ap, double));
#ifdef DEBUG
                printf("%.2lf", va_arg(ap, double));
#endif
                p += 3;
            }
            else if (*(p + 1) == 'l' && *(p + 2) && *(p + 2) == 'f')
            {
                fprintf(logFile, "%.2lf", va_arg(ap, double));
#ifdef DEBUG
                printf("%.2lf", va_arg(ap, double));
#endif
            }
            else if (*(p + 1) == 'c')
            {
                fprintf(logFile, "%c", (char) va_arg(ap, int));
#ifdef DEBUG
                printf("%c", (char) va_arg(ap, int));
#endif
            }
            else if (*(p + 1) == 's')
            {
                char* str = va_arg(ap, char*);
                if (str == NULL)
                {
                    fprintf(logFile, "NULL");
#ifdef DEBUG
                    printf("NULL");
#endif
                }
                else
                {
                    fprintf(logFile, "%s", str);
#ifdef DEBUG
                    printf("%s", str);
#endif
                }
            }
            else if (*(p + 1) == '%')
            {
                fprintf(logFile, "%%");
#ifdef DEBUG
                printf("%%");
#endif
            }
            else
            {
                //~ fprintf(logFile, "<<%d %d %d %d %d %d %d %d>>\n",
                //~ *(p + 1) == '.', *(p + 2), *(p + 2) >= 0, *(p + 2) <= 9,
                //~ *(p + 3), *(p + 3) == 'l', *(p + 4), *(p + 4) == 'f');
                fprintf(logFile, "unsupported format flag '%c'", *(p + 1));
#ifdef DEBUG
                printf("unsupported format flag '%c'", *(p + 1));
#endif
                break;
            }
            p++;
        }
        else             /* last character on the line */
        {
            fprintf(logFile, "%c", *p);
#ifdef DEBUG
            printf("%c", *p);
#endif
        }

        p++;
    }
    if (*p)
    {
        fprintf(logFile, "%c", *p);
#ifdef DEBUG
        printf("%c", *p);
#endif
    }

    fclose(logFile);
}

void fatalError(const char* message)
{
    fprintf(stderr, "Fatal error: %s\n", message);
    exit(-1);
}

// construct a filename from various parts
//
// path - the path the file is placed in (don't include a trailing '/')
// dir - the parent directory of the file (don't include a trailing '/')
// file - the filename
// extension - the suffix of a file (don't include a leading '.')
//
// NOTE: caller must free the returned string!
// NOTE: any of the parameters may be NULL to be omitted
char * make_filename(const char * path, const char * dir, const char * file, const char * extension)
{
    int  len   = 1;
    char * ret = NULL;
    int  pos   = 0;

    if (path)
    {
        len += strlen(path) + 1;
    }
    if (dir)
    {
        len += strlen(dir) + 1;
    }
    if (file)
    {
        len += strlen(file);
    }
    if (extension)
    {
        len += strlen(extension) + 1;
    }

    ret = malloc(sizeof(char) * len);
    if (ret == NULL)
        fatalError("malloc(sizeof(char) * len) failed. Out of memory.");

    if (path)
    {
        strncpy(&ret[pos], path, strlen(path));
        pos     += strlen(path);
        ret[pos] = '/';
        pos++;
    }
    if (dir)
    {
        strncpy(&ret[pos], dir, strlen(dir));
        pos     += strlen(dir);
        ret[pos] = '/';
        pos++;
    }
    if (file)
    {
        strncpy(&ret[pos], file, strlen(file));
        pos += strlen(file);
    }
    if (extension)
    {
        ret[pos] = '.';
        pos++;
        strncpy(&ret[pos], extension, strlen(extension));
        pos += strlen(extension);
    }
    ret[pos] = '\0';

    return ret;
}

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
char * parse_format(const char * format, int tracknum, const char * year, const char * artist, const char * album, const char * title)
{
    unsigned i     = 0;
    int      len   = 0;
    char     * ret = NULL;
    int      pos   = 0;

    for (i = 0; i < strlen(format); i++)
    {
        if ((format[i] == '%') && (i + 1 < strlen(format)))
        {
            switch (format[i + 1])
            {
            case 'A':
                if (artist)
                    len += strlen(artist);
                break;
            case 'L':
                if (album)
                    len += strlen(album);
                break;
            case 'N':
                if ((tracknum > 0) && (tracknum < 100))
                    len += 2;
                break;
            case 'Y':
                if (year)
                    len += strlen(year);
                break;
            case 'T':
                if (title)
                    len += strlen(title);
                break;
            case '%':
                len += 1;
                break;
            }

            i++;             // skip the character after the %
        }
        else
        {
            len++;
        }
    }

    ret = malloc(sizeof(char) * (len + 1));
    if (ret == NULL)
        fatalError("malloc(sizeof(char) * (len+1)) failed. Out of memory.");

    for (i = 0; i < strlen(format); i++)
    {
        if ((format[i] == '%') && (i + 1 < strlen(format)))
        {
            switch (format[i + 1])
            {
            case 'A':
                if (artist)
                {
                    strncpy(&ret[pos], artist, strlen(artist));
                    pos += strlen(artist);
                }
                break;
            case 'L':
                if (album)
                {
                    strncpy(&ret[pos], album, strlen(album));
                    pos += strlen(album);
                }
                break;
            case 'N':
                if ((tracknum > 0) && (tracknum < 100))
                {
                    ret[pos]     = '0' + ((int) tracknum / 10);
                    ret[pos + 1] = '0' + (tracknum % 10);
                    pos         += 2;
                }
                break;
            case 'Y':
                if (year)
                {
                    strncpy(&ret[pos], year, strlen(year));
                    pos += strlen(year);
                }
                break;
            case 'T':
                if (title)
                {
                    strncpy(&ret[pos], title, strlen(title));
                    pos += strlen(title);
                }
                break;
            case '%':
                ret[pos] = '%';
                pos     += 1;
            }

            i++;             // skip the character after the %
        }
        else
        {
            ret[pos] = format[i];
            pos++;
        }
    }
    ret[pos] = '\0';

    return ret;
}

// searches $PATH for the named program
// returns 1 if found, 0 otherwise
int program_exists(const char * name)
{
    unsigned    i;
    unsigned    numpaths = 1;
    char        * path;
    char        * strings;
    char        ** paths;
    struct stat s;
    int         ret = 0;
    char        * filename;

    path    = getenv("PATH");
    strings = malloc(sizeof(char) * (strlen(path) + 1));
    if (strings == NULL)
        fatalError("malloc(sizeof(char) * (strlen(path)+1)) failed. Out of memory.");
    strncpy(strings, path, strlen(path) + 1);

    for (i = 0; i < strlen(path); i++)
    {
        if (strings[i] == ':')
        {
            numpaths++;
        }
    }
    paths = malloc(sizeof(char *) * numpaths);
    if (paths == NULL)
        fatalError("malloc(sizeof(char *) * numpaths) failed. Out of memory.");
    numpaths = 1;
    paths[0] = &strings[0];
    for (i = 0; i < strlen(path); i++)
    {
        if (strings[i] == ':')
        {
            strings[i]      = '\0';
            paths[numpaths] = &strings[i + 1];
            numpaths++;
        }
    }

    for (i = 0; i < numpaths; i++)
    {
        filename = make_filename(paths[i], NULL, name, NULL);
        if (stat(filename, &s) == 0)
        {
            ret = 1;
            free(filename);
            break;
        }
        free(filename);
    }

    free(strings);
    free(paths);

    return ret;
}

// Uses mkdir() for every component of the path
// and returns if any of those fails with anything other than EEXIST.
int recursive_mkdir(char* path_and_name, mode_t mode)
{
    int  count;
    int  path_and_name_length = strlen(path_and_name);
    int  rc;
    char charReplaced;

    for (count = 0; count < path_and_name_length; count++)
    {
        if (path_and_name[count] == '/')
        {
            charReplaced             = path_and_name[count + 1];
            path_and_name[count + 1] = '\0';

            rc = mkdir(path_and_name, mode);

            path_and_name[count + 1] = charReplaced;

            if (rc != 0 && !(errno == EEXIST || errno == EISDIR))
                return rc;
        }
    }

    // in case the path doesn't have a trailing slash:
    return mkdir(path_and_name, mode);
}

// Uses mkdir() for every component of the path except the last one,
// and returns if any of those fails with anything other than EEXIST.
int recursive_parent_mkdir(char* path_and_name, mode_t mode)
{
    int count;
    int have_component = 0;
    int rc             = 1; // guaranteed fail unless mkdir is called

    // find the last component and cut it off
    for (count = strlen(path_and_name) - 1; count >= 0; count--)
    {
        if (path_and_name[count] != '/')
            have_component = 1;

        if (path_and_name[count] == '/' && have_component)
        {
            path_and_name[count] = 0;
            rc                   = mkdir(path_and_name, mode);
            path_and_name[count] = '/';
        }
    }

    return rc;
}

// removes all instances of bad characters from the string
//
// str - the string to trim
// bad - the sting containing all the characters to remove
void trim_chars(char * str, const char * bad)
{
    int      i;
    int      pos;
    int      len = strlen(str);
    unsigned b;

    for (b = 0; b < strlen(bad); b++)
    {
        pos = 0;
        for (i = 0; i < len + 1; i++)
        {
            if (str[i] != bad[b])
            {
                str[pos] = str[i];
                pos++;
            }
        }
    }
}

// removes leading and trailing whitespace as defined by isspace()
//
// str - the string to trim
void trim_whitespace(char * str)
{
    int i;
    int pos = 0;
    int len = strlen(str);

    // trim leading space
    for (i = 0; i < len + 1; i++)
    {
        if (!isspace(str[i]) || (pos > 0))
        {
            str[pos] = str[i];
            pos++;
        }
    }

    // trim trailing space
    len = strlen(str);
    for (i = len - 1; i >= 0; i--)
    {
        if (!isspace(str[i]))
        {
            break;
        }
        str[i] = '\0';
    }
}
