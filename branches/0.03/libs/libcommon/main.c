
#include <stdlib.h>
#include "util.h"

static char safe_chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+._-";

void sanitize_filename(char *f, short slash)
{
    if (!f)
        return;

    for (; *f; f++)
    {
        if ((slash && *f == '/') || !strchr(safe_chars, *f))
            *f = '_';
    }
}

int main(int argc, char* argv[])
{
//"%A - %L" format, playlist

    char *c = strdup("albumtitle  sfsdf &*%&$%#$@#*^$@#*^sdf");

    char *albumdir      = parse_format("%A - %L", 0, "albumyear", "albumartist <>//\/", "albumtitle  sfsdf &*%&$%#$@#*^$@#*^sdf", NULL);
    char *musicfilename = parse_format("%N - %A - %T", 1, "albumyear", "trackartist", "albumtitle", "tracktitle");
    char *wavfilename   = make_filename("/usb000", albumdir, musicfilename, "dff");


    sanitize_filename(c, 1);

    printf("Ripping track %d to \"%s\"\n", 1, wavfilename);

    free(albumdir);
    free(musicfilename);
    free(wavfilename);



    return 0;
}

