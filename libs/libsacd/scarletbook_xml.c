#include <stdio.h>
#include <string.h>
#include <charset.h>
#include <version.h>
#include <libxml/xmlreader.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/encoding.h>
#include <libxml/xmlwriter.h>
#include <libxml/xmlmemory.h>
#include "scarletbook.h"
#include "scarletbook_xml.h"

static void streamFile(const char *filename);
void sacdXmlwriterFilename(scarletbook_handle_t *handle, const char *path_file);
void testXmlwriterMemory(const char *file);
void testXmlwriterDoc(const char *file);
void testXmlwriterTree(const char *file);
xmlChar *ConvertInput(const char *in, const char *encoding);


void read_metadata_xml(const char *filename)
{
    /*
     * this initialize the library and check potential ABI mismatches
     * between the version it was compiled for and the actual shared
     * library used.
     */
    LIBXML_TEST_VERSION

    streamFile(filename);

    /*
     * Cleanup function for the XML library.
     */
    xmlCleanupParser();
    /*
     * this is to debug memory for regression tests
     */
    xmlMemoryDump();
}

/***
* processNode : *@reader : the xmlReader
*
* Dump information about the current node
*/
static void processNode(xmlTextReaderPtr reader)
{
    const xmlChar *name, *value;

    name = xmlTextReaderConstName(reader);
    if (name == NULL)
        name = BAD_CAST "--";

    value = xmlTextReaderConstValue(reader);

    printf("%d %d %s %d %d",
           xmlTextReaderDepth(reader),
           xmlTextReaderNodeType(reader),
           name,
           xmlTextReaderIsEmptyElement(reader),
           xmlTextReaderHasValue(reader));
    if (value == NULL)
        printf("\n");
    else
    {
        if (xmlStrlen(value) > 40)
            printf(" %.40s...\n", value);
        else
            printf(" %s\n", value);
    }
}

/**
 * streamFile:
 * @filename: the file name to parse
 *
 * Parse and print information about an XML file.
 */
static void
streamFile(const char *filename)
{
    xmlTextReaderPtr reader;
    int ret;

    reader = xmlReaderForFile(filename, NULL, 0);
    if (reader != NULL)
    {
        ret = xmlTextReaderRead(reader);
        while (ret == 1)
        {
            processNode(reader);
            ret = xmlTextReaderRead(reader);
        }
        xmlFreeTextReader(reader);
        if (ret != 0)
        {
            fprintf(stderr, "%s : failed to parse\n", filename);
        }
    }
    else
    {
        fprintf(stderr, "Unable to open %s\n", filename);
    }
}

//  Write in xml file the metadata found in area
//  input  *handle, const char *filename, int area
//
//
int write_metadata_xml(scarletbook_handle_t *handle, const char *path_file)
{
    /*
     * this initialize the library and check potential ABI mismatches
     * between the version it was compiled for and the actual shared
     * library used.
     */
    LIBXML_TEST_VERSION

    /* first, the file version */
    sacdXmlwriterFilename(handle,path_file);

    /* next, the memory version */
   // testXmlwriterMemory("writer2.tmp");

    /* next, the DOM version */
    //testXmlwriterDoc("writer3.tmp");

    /* next, the tree version */
   // testXmlwriterTree("writer4.tmp");

    /*
     * Cleanup function for the XML library.
     */
    xmlCleanupParser();
    /*
     * this is to debug memory for regression tests
     */
    //xmlMemoryDump();
    return 0;
}

/**
 * testXmlwriterFilename:
 * @uri: the output URI
 *
 * test the xmlWriter interface when writing to a new file *uri
 */
void sacdXmlwriterFilename(scarletbook_handle_t *handle, const char *uri)
{
    int rc;
    xmlTextWriterPtr writer;
    //xmlChar *tmp;
    char string_buf[1000];
    int area_idx;

    /* Create a new XmlWriter for uri, with no compression. */
    writer = xmlNewTextWriterFilename(uri, 0);
    if (writer == NULL)
    {
        printf("testXmlwriterFilename: Error creating the xml writer\n");
        return;
    }

    /* Start the document with the xml default for the version,
     * encoding UTF-8 and the default for the standalone
     * declaration. */
    rc = xmlTextWriterStartDocument(writer, NULL, "UTF-8", NULL);
    if (rc < 0)
    {
        printf("testXmlwriterFilename: Error at xmlTextWriterStartDocument\n");
        return;
    }

    /* Start an element named "AudioMetadata". Since thist is the first
     * element, this will be the root element of the document. */
    rc = xmlTextWriterStartElement(writer, BAD_CAST "AudioMetadata");
    if (rc < 0)
    {
        printf("testXmlwriterFilename: Error at xmlTextWriterStartElement\n");
        return;
    }

    /* Write a comment as child of AudioMetadata.
     * Please observe, that the input to the xmlTextWriter functions
     * HAS to be in UTF-8, even if the output XML is encoded
     * in iso-8859-1 */
    
    snprintf(string_buf, 500, "SACD metadata file (created by sacd_extract, version: %s)", SACD_RIPPER_VERSION_STRING);
    //tmp = ConvertInput(string_buf,MY_ENCODING);
    rc = xmlTextWriterWriteComment(writer, (const xmlChar *)string_buf);
    if (rc < 0)
    {
        printf("testXmlwriterFilename: Error at xmlTextWriterWriteComment\n");
        return;
    }
    //if (tmp != NULL)
    //    xmlFree(tmp);

    /* Start an element named "Album" as child of AudioMetadata . */
    rc = xmlTextWriterStartElement(writer, BAD_CAST "Album");
    if (rc < 0)
    {
        printf("testXmlwriterFilename: Error at xmlTextWriterStartElement\n");
        return;
    }
  
    /* Add an attribute with name "catalog" and value "xxxxxxx" to Album. */
    strncpy(string_buf, handle->master_toc->album_catalog_number, 16);
    string_buf[16] = '\0';
    rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "catalog_number",
                                     BAD_CAST string_buf);
    if (rc < 0)
    {
        printf("testXmlwriterFilename: Error at xmlTextWriterWriteAttribute\n");
        return;
    }
    /* Add an attribute with name "set size" and value "xxxx" to Album. */
    rc = xmlTextWriterWriteFormatAttribute(writer, BAD_CAST "set_size",
                                      "%i", handle->master_toc->album_set_size);
    if (rc < 0)
    {
        printf("testXmlwriterFilename: Error at xmlTextWriterWriteAttribute\n");
        return;
    }
    /* Add an attribute with name "sequence_number" and value "xxxx" to Album. */
    rc = xmlTextWriterWriteFormatAttribute(writer, BAD_CAST "sequence_number",
                                           "%i", handle->master_toc->album_sequence_number);
    if (rc < 0)
    {
        printf("testXmlwriterFilename: Error at xmlTextWriterWriteAttribute\n");
        return;
    }

    /* Add an element with name "title" and value "XXXXXX" to Album. */
    if (handle->master_text.album_title != NULL)
        rc = xmlTextWriterWriteElement(writer, BAD_CAST "Title", BAD_CAST handle->master_text.album_title);
    else
        rc = xmlTextWriterWriteElement(writer, BAD_CAST "Title", BAD_CAST "unknown album title");

    if (rc < 0)
    {
        printf("testXmlwriterFilename: Error at xmlTextWriterWriteAttribute\n");
        return;
    }
    /* Add an element with name "artist" and value "xxxx" to Album. */
    rc = xmlTextWriterWriteElement(writer, BAD_CAST "Artist", BAD_CAST handle->master_text.album_artist);
    if (rc < 0)
    {
        printf("testXmlwriterFilename: Error at xmlTextWriterWriteAttribute\n");
        return;
    }
    /* Add an element with name "publisher" and value "xxxx" to Album. */
    rc = xmlTextWriterWriteElement(writer, BAD_CAST "Publisher",BAD_CAST handle->master_text.album_publisher);
    if (rc < 0)
    {
        printf("testXmlwriterFilename: Error at xmlTextWriterWriteAttribute\n");
        return;
    }
    /* Add an element with name "copyright" and value "xxxx" to Album. */
    rc = xmlTextWriterWriteElement(writer, BAD_CAST "Copyright", BAD_CAST handle->master_text.album_copyright);
    if (rc < 0)
    {
        printf("testXmlwriterFilename: Error at xmlTextWriterWriteAttribute\n");
        return;
    }

    /* Start an element named "Disc" as child of "Album". */
    rc = xmlTextWriterStartElement(writer, BAD_CAST "Disc");
    if (rc < 0)
    {
        printf("testXmlwriterFilename: Error at xmlTextWriterStartElement\n");
        return;
    }
    /* Add an attribute with name "disc_version" and value "XXXXXX" to Disc. */
    rc = xmlTextWriterWriteFormatAttribute(writer, BAD_CAST "disc_version",
                                           "%2i.%02i", handle->master_toc->version.major, handle->master_toc->version.minor);
    if (rc < 0)
    {
        printf("testXmlwriterFilename: Error at xmlTextWriterWriteFormatAttribute\n");
        return;
    }

    /* Add an attribute with name "catalog number" and value "XXXXXX" to Disc. */
    strncpy(string_buf, handle->master_toc->disc_catalog_number, 16);
    string_buf[16] = '\0';
    rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "catalog_number",
                                     BAD_CAST string_buf);
    if (rc < 0)
    {
        printf("testXmlwriterFilename: Error at xmlTextWriterWriteAttribute\n");
        return;
    }
    rc = xmlTextWriterWriteFormatAttribute(writer, BAD_CAST "creation_date",
                                             "%4i-%02i-%02i", handle->master_toc->disc_date_year, handle->master_toc->disc_date_month, handle->master_toc->disc_date_day);
    if (rc < 0)
    {
        printf("testXmlwriterFilename: Error at xmlTextWriterWriteFormatAttribute\n");
        return;
    }
    uint8_t current_charset_nr;
    char *current_charset_name;
    current_charset_nr = handle->master_toc->locales[0].character_set & 0x07;
    current_charset_name = (char *)character_set[current_charset_nr];
    if (handle->master_toc->locales[0].language_code[0] != '\0' && handle->master_toc->locales[0].language_code[1] != '\0')
        snprintf(string_buf,500, "%c%c, code_character_set:[%d], %s", handle->master_toc->locales[0].language_code[0], handle->master_toc->locales[0].language_code[1], handle->master_toc->locales[0].character_set, current_charset_name);
    else
        snprintf(string_buf,500, "unspecified, asume code_character_set:[%d], %s", handle->master_toc->locales[0].character_set, current_charset_name);

    /* Add an attribute with name "locale" and value "xxxxxx" to Disc. */
    rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "locale",
                                     BAD_CAST string_buf);
    if (rc < 0)
    {
        printf("testXmlwriterFilename: Error at xmlTextWriterWriteAttribute\n");
        return;
    }


    genre_table_t *t = &handle->master_toc->disc_genre[0];
    if (t->category)
    {
        /* Add an attribute with name "category" and value "xxxxxx" to Disc. */
        rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "category",
                                            BAD_CAST album_category[t->category]);
        if (rc < 0)
        {
            printf("testXmlwriterFilename: Error at xmlTextWriterWriteAttribute\n");
            return;
        }
        /* Add an attribute with name "genre" and value "xxxxxx" to Disc. */
        rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "genre",
                                            BAD_CAST album_genre[t->genre]);
        if (rc < 0)
        {
            printf("testXmlwriterFilename: Error at xmlTextWriterWriteAttribute\n");
            return;
        }
    }  
    

    /* Add an element with name "title" and value "XXXXXX" to Disc. */
    rc = xmlTextWriterWriteElement(writer, BAD_CAST "Title",
                                   (const xmlChar *)handle->master_text.disc_title);
    if (rc < 0)
    {
        printf("testXmlwriterFilename: Error at xmlTextWriterWriteAttribute\n");
        return;
    }
    /* Add an element with name "artist" and value "xxxx" to Disc. */
    rc = xmlTextWriterWriteElement(writer, BAD_CAST "Artist",
                                   (const xmlChar *)handle->master_text.disc_artist);
    if (rc < 0)
    {
        printf("testXmlwriterFilename: Error at xmlTextWriterWriteAttribute\n");
        return;
    }
    /* Add an element with name "publisher" and value "xxxx" to Disc. */
    rc = xmlTextWriterWriteElement(writer, BAD_CAST "Publisher",
                                   (const xmlChar *)handle->master_text.disc_publisher);
    if (rc < 0)
    {
        printf("testXmlwriterFilename: Error at xmlTextWriterWriteAttribute\n");
        return;
    }
    /* Add an element with name "copyright" and value "xxxx" to Disc. */
    rc = xmlTextWriterWriteElement(writer, BAD_CAST "Copyright",(const xmlChar *)handle->master_text.disc_copyright);
    if (rc < 0)
    {
        printf("testXmlwriterFilename: Error at xmlTextWriterWriteAttribute\n");
        return;
    }
    for (int j = 0; j < handle->area_count; j++)
    {
        if (handle->area_count < 2)
        {
            if (handle->twoch_area_idx != -1)
                    area_idx = handle->twoch_area_idx;
                else
                    area_idx = handle->mulch_area_idx;
        }
        else
         area_idx =j;
            

        /* Add an element with name "Area" and value "xxxx" to Disc. */
        rc = xmlTextWriterStartElement(writer, BAD_CAST "Area");
        if (rc < 0)
        {
            printf("testXmlwriterFilename: Error at xmlTextWriterWriteAttribute\n");
            return;
        }
        /* Add an attribute with name "id" and value "xxxxxx" to Area. */
        rc = xmlTextWriterWriteFormatAttribute(writer, BAD_CAST "id", "%d", area_idx);
        if (rc < 0)
        {
            printf("testXmlwriterFilename: Error at xmlTextWriterWriteAttribute\n");
            return;
        }
        /* Add an attribute with name "version" and value "xxxxxx" to Area. */
        rc = xmlTextWriterWriteFormatAttribute(writer, BAD_CAST "version", "%2i.%02i", handle->area->area_toc->version.major, handle->area->area_toc->version.minor);
        if (rc < 0)
        {
            printf("testXmlwriterFilename: Error at xmlTextWriterWriteAttribute\n");
            return;
        }

    //     fwprintf(stdout, L"\tArea Description: %ls\n", ucs(area->description));

        if (handle->area[area_idx].area_toc->channel_count == 2 && handle->area[area_idx].area_toc->extra_settings == 0)
        {
            snprintf(string_buf, 500,  "2 Channel");
        }
        else if (handle->area[area_idx].area_toc->channel_count == 5 && handle->area[area_idx].area_toc->extra_settings == 3)
        {
            snprintf(string_buf, 500,  "5 Channel");
        }
        else if (handle->area[area_idx].area_toc->channel_count == 6 && handle->area[area_idx].area_toc->extra_settings == 4)
        {
            snprintf(string_buf, 500, "6 Channel");
        }
        else
        {
            snprintf(string_buf, 500,  "Unknown");
        }
        /* Add an attribute with name "speaker_config" and value "xxxxxx" to Area. */
        rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "speaker_configuration", BAD_CAST string_buf);
        if (rc < 0)
        {
            printf("testXmlwriterFilename: Error at xmlTextWriterWriteAttribute\n");
            return;
        }
        /* Add an attribute with name "totaltracks" and value "xxxxxx" to Area. */
        rc = xmlTextWriterWriteFormatAttribute(writer, BAD_CAST "totaltracks", "%d", (int)handle->area[area_idx].area_toc->track_count);
        if (rc < 0)
        {
            printf("testXmlwriterFilename: Error at xmlTextWriterWriteAttribute\n");
            return;
        }
        /* Add an attribute with name "Total play time" and value "xxxxxx" to Area. */
        rc = xmlTextWriterWriteFormatAttribute(writer, BAD_CAST "total_play_time", "%02d:%02d:%02d [mins:secs:frames]", handle->area[area_idx].area_toc->total_playtime.minutes, handle->area[area_idx].area_toc->total_playtime.seconds, handle->area[area_idx].area_toc->total_playtime.frames);
        if (rc < 0)
        {
            printf("testXmlwriterFilename: Error at xmlTextWriterWriteAttribute\n");
            return;
        }
        rc = xmlTextWriterWriteComment(writer, BAD_CAST "[mins:secs:frames]");
        if (rc < 0)
        {
            printf("testXmlwriterFilename: Error at xmlTextWriterWriteComment\n");
            return;
        }

        /*  list all tracks  */

        for (int t = 0; t < (int)handle->area[area_idx].area_toc->track_count; t++)
        {
            area_track_text_t *track_text = &handle->area[area_idx].area_track_text[t];

            /* Add an element "meta" and value "name" to track. */
            rc = xmlTextWriterStartElement(writer, BAD_CAST "track");
            if (rc < 0)
            {
                printf("testXmlwriterFilename: Error at xmlTextWriterStartElement\n");
                return;
            }
            rc = xmlTextWriterWriteFormatAttribute(writer, BAD_CAST "id", "%d", t + 1);
            if (rc < 0)
            {
                printf("testXmlwriterFilename: Error at xmlTextWriterWriteFormatAttribute\n");
                return;
            }

            /* Add an element "meta" and value "name" whith TITLE. */
            rc = xmlTextWriterStartElement(writer, BAD_CAST "meta");
            if (rc < 0)
            {
                printf("testXmlwriterFilename: Error at xmlTextWriterWriteAttribute\n");
                return;
            }
            rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "name",
                                             BAD_CAST "TITLE");
            if (rc < 0)
            {
                printf("testXmlwriterFilename: Error at xmlTextWriterWriteAttribute\n");
                return;
            }
            if (track_text->track_type_title != NULL)
            {
                rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "value",
                                                 BAD_CAST track_text->track_type_title);
            }
            else
            {
                rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "value",
                                                 BAD_CAST "unknown title");
            }
            if (rc < 0)
            {
                printf("testXmlwriterFilename: Error at xmlTextWriterWriteAttribute\n");
                return;
            }
            /* Close the element  */
            rc = xmlTextWriterEndElement(writer);
            if (rc < 0)
            {
                printf("testXmlwriterFilename: Error at xmlTextWriterEndElement\n");
                return;
            }

            /* Add an element "meta" and value "name" to PERFORMER. */
            rc = xmlTextWriterStartElement(writer, BAD_CAST "meta");
            if (rc < 0)
            {
                printf("testXmlwriterFilename: Error at xmlTextWriterWriteAttribute\n");
                return;
            }
            rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "name",
                                             BAD_CAST "PERFORMER");
            if (rc < 0)
            {
                printf("testXmlwriterFilename: Error at xmlTextWriterWriteAttribute\n");
                return;
            }
            if (track_text->track_type_performer != NULL)
            {
                rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "value",
                                                 BAD_CAST track_text->track_type_performer);
            }
            else
            {
                rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "value",
                                                 BAD_CAST "");
            }
            if (rc < 0)
            {
                printf("testXmlwriterFilename: Error at xmlTextWriterWriteAttribute\n");
                return;
            }
            /* Close the element */
            rc = xmlTextWriterEndElement(writer);
            if (rc < 0)
            {
                printf("testXmlwriterFilename: Error at xmlTextWriterEndElement\n");
                return;
            }

            /* Add an element "meta" and value "name" to ALBUM. */
            rc = xmlTextWriterStartElement(writer, BAD_CAST "meta");
            if (rc < 0)
            {
                printf("testXmlwriterFilename: Error at xmlTextWriterWriteAttribute\n");
                return;
            }
            rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "name",
                                             BAD_CAST "ALBUM");
            if (rc < 0)
            {
                printf("testXmlwriterFilename: Error at xmlTextWriterWriteAttribute\n");
                return;
            }
            if (handle->master_text.album_title != NULL)
                rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "value",
                                                 BAD_CAST handle->master_text.album_title);
            else
                rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "value",
                                                 BAD_CAST "unknown album title");
            if (rc < 0)
            {
                printf("testXmlwriterFilename: Error at xmlTextWriterWriteAttribute\n");
                return;
            }
            /* Close the element  */
            rc = xmlTextWriterEndElement(writer);
            if (rc < 0)
            {
                printf("testXmlwriterFilename: Error at xmlTextWriterEndElement\n");
                return;
            }

            /* Add an element "meta" and value "name" to ALBUM ARTIST. */
            rc = xmlTextWriterStartElement(writer, BAD_CAST "meta");
            if (rc < 0)
            {
                printf("testXmlwriterFilename: Error at xmlTextWriterWriteAttribute\n");
                return;
            }
            rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "name",
                                             BAD_CAST "ALBUM ARTIST");
            if (rc < 0)
            {
                printf("testXmlwriterFilename: Error at xmlTextWriterWriteAttribute\n");
                return;
            }
            if (handle->master_text.album_artist != NULL)
                rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "value",
                                                 BAD_CAST handle->master_text.album_artist);
            else
                rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "value",
                                                 BAD_CAST "");

            if (rc < 0)
            {
                printf("testXmlwriterFilename: Error at xmlTextWriterWriteAttribute\n");
                return;
            }
            /* Close the element  */
            rc = xmlTextWriterEndElement(writer);
            if (rc < 0)
            {
                printf("testXmlwriterFilename: Error at xmlTextWriterEndElement\n");
                return;
            }

            /* Add an element "meta" and value "name" to COMPOSER. */
            if (track_text->track_type_composer != NULL)
            {
                rc = xmlTextWriterStartElement(writer, BAD_CAST "meta");
                if (rc < 0)
                {
                    printf("testXmlwriterFilename: Error at xmlTextWriterWriteAttribute\n");
                    return;
                }
                rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "name",
                                                 BAD_CAST "COMPOSER");
                if (rc < 0)
                {
                    printf("testXmlwriterFilename: Error at xmlTextWriterWriteAttribute\n");
                    return;
                }
                rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "value",
                                                 BAD_CAST track_text->track_type_composer);
                if (rc < 0)
                {
                    printf("testXmlwriterFilename: Error at xmlTextWriterWriteAttribute\n");
                    return;
                }
                /* Close the element  */
                rc = xmlTextWriterEndElement(writer);
                if (rc < 0)
                {
                    printf("testXmlwriterFilename: Error at xmlTextWriterEndElement\n");
                    return;
                }
            }

            /* Add an element "meta" and value "name" to ARRANGER. */
            if (track_text->track_type_arranger != NULL)
            {
                rc = xmlTextWriterStartElement(writer, BAD_CAST "meta");
                if (rc < 0)
                {
                    printf("testXmlwriterFilename: Error at xmlTextWriterWriteAttribute\n");
                    return;
                }
                rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "name",
                                                 BAD_CAST "CONDUCTOR");
                if (rc < 0)
                {
                    printf("testXmlwriterFilename: Error at xmlTextWriterWriteAttribute\n");
                    return;
                }
                rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "value",
                                                 BAD_CAST track_text->track_type_arranger);
                if (rc < 0)
                {
                    printf("testXmlwriterFilename: Error at xmlTextWriterWriteAttribute\n");
                    return;
                }
                /* Close the element  */
                rc = xmlTextWriterEndElement(writer);
                if (rc < 0)
                {
                    printf("testXmlwriterFilename: Error at xmlTextWriterEndElement\n");
                    return;
                }
            }

            /* Add an element "meta" and value "name" to SONGWRITER. */
            if (track_text->track_type_songwriter != NULL)
            {
                rc = xmlTextWriterStartElement(writer, BAD_CAST "meta");
                if (rc < 0)
                {
                    printf("testXmlwriterFilename: Error at xmlTextWriterWriteAttribute\n");
                    return;
                }
                rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "name",
                                                 BAD_CAST "SONGWRITER");
                if (rc < 0)
                {
                    printf("testXmlwriterFilename: Error at xmlTextWriterWriteAttribute\n");
                    return;
                }
                rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "value",
                                                 BAD_CAST track_text->track_type_songwriter);
                if (rc < 0)
                {
                    printf("testXmlwriterFilename: Error at xmlTextWriterWriteAttribute\n");
                    return;
                }
                /* Close the element  */
                rc = xmlTextWriterEndElement(writer);
                if (rc < 0)
                {
                    printf("testXmlwriterFilename: Error at xmlTextWriterEndElement\n");
                    return;
                }
            }

            /* Add an element "meta" and value "name" to MESSAGE. */
            if (track_text->track_type_message != NULL)
            {
                rc = xmlTextWriterStartElement(writer, BAD_CAST "meta");
                if (rc < 0)
                {
                    printf("testXmlwriterFilename: Error at xmlTextWriterWriteAttribute\n");
                    return;
                }
                rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "name",
                                                 BAD_CAST "COMMENT");
                if (rc < 0)
                {
                    printf("testXmlwriterFilename: Error at xmlTextWriterWriteAttribute\n");
                    return;
                }
                rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "value",
                                                 BAD_CAST track_text->track_type_message);
                if (rc < 0)
                {
                    printf("testXmlwriterFilename: Error at xmlTextWriterWriteAttribute\n");
                    return;
                }
                /* Close the element  */
                rc = xmlTextWriterEndElement(writer);
                if (rc < 0)
                {
                    printf("testXmlwriterFilename: Error at xmlTextWriterEndElement\n");
                    return;
                }
            }

            /* Add an element "meta" and value "name" to EXTRAMESSAGE.  */
            if (track_text->track_type_extra_message != NULL)
            {
                rc = xmlTextWriterStartElement(writer, BAD_CAST "meta");
                if (rc < 0)
                {
                    printf("testXmlwriterFilename: Error at xmlTextWriterWriteAttribute\n");
                    return;
                }
                rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "name",
                                                 BAD_CAST "EXTRAMESSAGE");
                if (rc < 0)
                {
                    printf("testXmlwriterFilename: Error at xmlTextWriterWriteAttribute\n");
                    return;
                }
                rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "value",
                                                 BAD_CAST track_text->track_type_extra_message);
                if (rc < 0)
                {
                    printf("testXmlwriterFilename: Error at xmlTextWriterWriteAttribute\n");
                    return;
                }
                /* Close the element  */
                rc = xmlTextWriterEndElement(writer);
                if (rc < 0)
                {
                    printf("testXmlwriterFilename: Error at xmlTextWriterEndElement\n");
                    return;
                }
            }

            /* Add an element "meta" and value "name" to TRACKNUMBER. */
            rc = xmlTextWriterStartElement(writer, BAD_CAST "meta");
            if (rc < 0)
            {
                printf("testXmlwriterFilename: Error at xmlTextWriterWriteAttribute\n");
                return;
            }
            rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "name",
                                             BAD_CAST "TRACKNUMBER");
            if (rc < 0)
            {
                printf("testXmlwriterFilename: Error at xmlTextWriterWriteAttribute\n");
                return;
            }
            rc = xmlTextWriterWriteFormatAttribute(writer, BAD_CAST "value", "%d", t + 1);
            if (rc < 0)
            {
                printf("testXmlwriterFilename: Error at xmlTextWriterWriteFormatAttribute\n");
                return;
            }
            /* Close the element  */
            rc = xmlTextWriterEndElement(writer);
            if (rc < 0)
            {
                printf("testXmlwriterFilename: Error at xmlTextWriterEndElement\n");
                return;
            }

            /* Add an element "meta" and value "name" to TOTALTRACKS. */
            rc = xmlTextWriterStartElement(writer, BAD_CAST "meta");
            if (rc < 0)
            {
                printf("testXmlwriterFilename: Error at xmlTextWriterWriteAttribute\n");
                return;
            }
            rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "name",
                                             BAD_CAST "TOTALTRACKS");
            if (rc < 0)
            {
                printf("testXmlwriterFilename: Error at xmlTextWriterWriteAttribute\n");
                return;
            }
            rc = xmlTextWriterWriteFormatAttribute(writer, BAD_CAST "value", "%d", (int)handle->area[area_idx].area_toc->track_count);
            if (rc < 0)
            {
                printf("testXmlwriterFilename: Error at xmlTextWriterWriteFormatAttribute\n");
                return;
            }
            /* Close the element  */
            rc = xmlTextWriterEndElement(writer);
            if (rc < 0)
            {
                printf("testXmlwriterFilename: Error at xmlTextWriterEndElement\n");
                return;
            }

            area_isrc_genre_t *area_isrc_genre = handle->area[area_idx].area_isrc_genre;

            /* Add an element "meta" and value "name" to GENRE. */
            rc = xmlTextWriterStartElement(writer, BAD_CAST "meta");
            if (rc < 0)
            {
                printf("testXmlwriterFilename: Error at xmlTextWriterWriteAttribute\n");
                return;
            }
            rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "name",
                                             BAD_CAST "GENRE");
            if (rc < 0)
            {
                printf("testXmlwriterFilename: Error at xmlTextWriterWriteAttribute\n");
                return;
            }
            rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "value", BAD_CAST album_genre[area_isrc_genre->track_genre[t].genre]);
            if (rc < 0)
            {
                printf("testXmlwriterFilename: Error at xmlTextWriterWriteFormatAttribute\n");
                return;
            }
            /* Close the element  */
            rc = xmlTextWriterEndElement(writer);
            if (rc < 0)
            {
                printf("testXmlwriterFilename: Error at xmlTextWriterEndElement\n");
                return;
            }

            //     //     fwprintf(stdout, L"\tISRC Track [%d]:\n\t  ", i);
            //     //     fwprintf(stdout, L"Country: %ls, ", ucs(substr(isrc->country_code, 0, 2)));
            //     //     fwprintf(stdout, L"Owner: %ls, ", ucs(substr(isrc->owner_code, 0, 3)));
            //     //     fwprintf(stdout, L"Year: %ls, ", ucs(substr(isrc->recording_year, 0, 2)));
            //     //     fwprintf(stdout, L"Designation: %ls\n", ucs(substr(isrc->designation_code, 0, 5)));
            //     // }

            /* Add an element "meta" and value "name" to ISRC. */
            rc = xmlTextWriterStartElement(writer, BAD_CAST "meta");
            if (rc < 0)
            {
                printf("testXmlwriterFilename: Error at xmlTextWriterWriteAttribute\n");
                return;
            }
            rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "name",
                                             BAD_CAST "ISRC");
            if (rc < 0)
            {
                printf("testXmlwriterFilename: Error at xmlTextWriterWriteAttribute\n");
                return;
            }
            if (area_isrc_genre->isrc[t].country_code)
            {
                strncpy(string_buf, area_isrc_genre->isrc[t].country_code, 12);
                string_buf[12] = '\0';
                rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "value", BAD_CAST string_buf);
            }
            else
                rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "value", BAD_CAST "");
            if (rc < 0)
            {
                printf("testXmlwriterFilename: Error at xmlTextWriterWriteFormatAttribute\n");
                return;
            }
            /* Close the element  */
            rc = xmlTextWriterEndElement(writer);
            if (rc < 0)
            {
                printf("testXmlwriterFilename: Error at xmlTextWriterEndElement\n");
                return;
            }

            /* Add an element "meta" and value "name" to DATE. */
            rc = xmlTextWriterStartElement(writer, BAD_CAST "meta");
            if (rc < 0)
            {
                printf("testXmlwriterFilename: Error at xmlTextWriterWriteAttribute\n");
                return;
            }
            rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "name", BAD_CAST "DATE");
            if (rc < 0)
            {
                printf("testXmlwriterFilename: Error at xmlTextWriterWriteAttribute\n");
                return;
            }
            if (handle->area[area_idx].area_isrc_genre->isrc[t].recording_year)
            {
                strncpy(string_buf, handle->area[area_idx].area_isrc_genre->isrc[t].recording_year, 2);
                string_buf[2] = '\0';
                rc = xmlTextWriterWriteFormatAttribute(writer, BAD_CAST "value",
                                                       "%s", string_buf);
            }
            else
                rc = xmlTextWriterWriteFormatAttribute(writer, BAD_CAST "value",
                                                       "%4i", handle->master_toc->disc_date_year);
            if (rc < 0)
            {
                printf("testXmlwriterFilename: Error at xmlTextWriterWriteAttribute\n");
                return;
            }
            /* Close the element  */
            rc = xmlTextWriterEndElement(writer);
            if (rc < 0)
            {
                printf("testXmlwriterFilename: Error at xmlTextWriterEndElement\n");
                return;
            }

            area_tracklist_time_t time_start = handle->area[area_idx].area_tracklist_time->start[t];
            area_tracklist_time_t time_duration = handle->area[area_idx].area_tracklist_time->duration[t];

            /* Add an element "meta" and value "name" to Track_Start_Time_Code. */
            rc = xmlTextWriterStartElement(writer, BAD_CAST "meta");
            if (rc < 0)
            {
                printf("testXmlwriterFilename: Error at xmlTextWriterWriteAttribute\n");
                return;
            }
            rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "name",
                                             BAD_CAST "Track_Start_Time_Code");
            if (rc < 0)
            {
                printf("testXmlwriterFilename: Error at xmlTextWriterWriteAttribute\n");
                return;
            }
            rc = xmlTextWriterWriteFormatAttribute(writer, BAD_CAST "value", "%02d:%02d:%02d", time_start.minutes, time_start.seconds, time_start.frames);
            if (rc < 0)
            {
                printf("testXmlwriterFilename: Error at xmlTextWriterWriteFormatAttribute\n");
                return;
            }
            rc = xmlTextWriterWriteComment(writer, BAD_CAST "[mins:secs:frames]");
            if (rc < 0)
            {
                printf("testXmlwriterFilename: Error at xmlTextWriterWriteComment\n");
                return;
            }
            /* Close the element  */
            rc = xmlTextWriterEndElement(writer);
            if (rc < 0)
            {
                printf("testXmlwriterFilename: Error at xmlTextWriterEndElement\n");
                return;
            }
            /* Add an element "meta" and value "name" to Duration. */
            rc = xmlTextWriterStartElement(writer, BAD_CAST "meta");
            if (rc < 0)
            {
                printf("testXmlwriterFilename: Error at xmlTextWriterWriteAttribute\n");
                return;
            }
            rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "name",
                                             BAD_CAST "Duration");
            if (rc < 0)
            {
                printf("testXmlwriterFilename: Error at xmlTextWriterWriteAttribute\n");
                return;
            }
            rc = xmlTextWriterWriteFormatAttribute(writer, BAD_CAST "value", "%02d:%02d:%02d", time_duration.minutes, time_duration.seconds, time_duration.frames);
            if (rc < 0)
            {
                printf("testXmlwriterFilename: Error at xmlTextWriterWriteFormatAttribute\n");
                return;
            }
            rc = xmlTextWriterWriteComment(writer, BAD_CAST "[mins:secs:frames]");
            if (rc < 0)
            {
                printf("testXmlwriterFilename: Error at xmlTextWriterWriteComment\n");
                return;
            }
            /* Close the element  */
            rc = xmlTextWriterEndElement(writer);
            if (rc < 0)
            {
                printf("testXmlwriterFilename: Error at xmlTextWriterEndElement\n");
                return;
            }

            /* Close the element 'track' */
            rc = xmlTextWriterEndElement(writer);
            if (rc < 0)
            {
                printf("testXmlwriterFilename: Error at xmlTextWriterEndElement\n");
                return;
            }

    } // end for i   track_count

    /* Close the element Area */
    rc = xmlTextWriterEndElement(writer);
    if (rc < 0)
    {
        printf("testXmlwriterFilename: Error at xmlTextWriterEndElement\n");
        return;
    }

    }  // end for area_idx

    /* Close the element named Disc. */
    rc = xmlTextWriterEndElement(writer);
    if (rc < 0)
    {
        printf("testXmlwriterFilename: Error at xmlTextWriterEndElement\n");
        return;
    }

    /* Write an element named "ENTRY_NO" as child of ENTRY. */
    // rc = xmlTextWriterWriteFormatElement(writer, BAD_CAST "ENTRY_NO", "%d",
    //                                      20);
    // if (rc < 0)
    // {
    //     printf("testXmlwriterFilename: Error at xmlTextWriterWriteFormatElement\n");
    //     return;
    // }


    /* Close the element named Album. */
    rc = xmlTextWriterEndElement(writer);
    if (rc < 0)
    {
        printf("testXmlwriterFilename: Error at xmlTextWriterEndElement\n");
        return;
    }


    /* Here we could close the elements Album and Audiometadata using the
     * function xmlTextWriterEndElement, but since we do not want to
     * write any other elements, we simply call xmlTextWriterEndDocument,
     * which will do all the work. */
    rc = xmlTextWriterEndDocument(writer);
    if (rc < 0)
    {
        printf("testXmlwriterFilename: Error at xmlTextWriterEndDocument\n");
        return;
    }

    xmlFreeTextWriter(writer);
}

/**
 * testXmlwriterMemory:
 * @file: the output file
 *
 * test the xmlWriter interface when writing to memory
 */
void testXmlwriterMemory(const char *file)
{
    int rc;
    xmlTextWriterPtr writer;
    xmlBufferPtr buf;
    xmlChar *tmp;
    FILE *fp;

    /* Create a new XML buffer, to which the XML document will be
     * written */
    buf = xmlBufferCreate();
    if (buf == NULL)
    {
        printf("testXmlwriterMemory: Error creating the xml buffer\n");
        return;
    }

    /* Create a new XmlWriter for memory, with no compression.
     * Remark: there is no compression for this kind of xmlTextWriter */
    writer = xmlNewTextWriterMemory(buf, 0);
    if (writer == NULL)
    {
        printf("testXmlwriterMemory: Error creating the xml writer\n");
        return;
    }

    /* Start the document with the xml default for the version,
     * encoding ISO 8859-1 and the default for the standalone
     * declaration. */
    rc = xmlTextWriterStartDocument(writer, NULL, MY_ENCODING, NULL);
    if (rc < 0)
    {
        printf("testXmlwriterMemory: Error at xmlTextWriterStartDocument\n");
        return;
    }

    /* Start an element named "EXAMPLE". Since thist is the first
     * element, this will be the root element of the document. */
    rc = xmlTextWriterStartElement(writer, BAD_CAST "EXAMPLE");
    if (rc < 0)
    {
        printf("testXmlwriterMemory: Error at xmlTextWriterStartElement\n");
        return;
    }

    /* Write a comment as child of EXAMPLE.
     * Please observe, that the input to the xmlTextWriter functions
     * HAS to be in UTF-8, even if the output XML is encoded
     * in iso-8859-1 */
    tmp = ConvertInput("This is a comment with special chars: <\xE4\xF6\xFC>",
                       MY_ENCODING);
    rc = xmlTextWriterWriteComment(writer, tmp);
    if (rc < 0)
    {
        printf("testXmlwriterMemory: Error at xmlTextWriterWriteComment\n");
        return;
    }
    if (tmp != NULL)
        xmlFree(tmp);

    /* Start an element named "ORDER" as child of EXAMPLE. */
    rc = xmlTextWriterStartElement(writer, BAD_CAST "ORDER");
    if (rc < 0)
    {
        printf("testXmlwriterMemory: Error at xmlTextWriterStartElement\n");
        return;
    }

    /* Add an attribute with name "version" and value "1.0" to ORDER. */
    rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "version",
                                     BAD_CAST "1.0");
    if (rc < 0)
    {
        printf("testXmlwriterMemory: Error at xmlTextWriterWriteAttribute\n");
        return;
    }

    /* Add an attribute with name "xml:lang" and value "de" to ORDER. */
    rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "xml:lang",
                                     BAD_CAST "de");
    if (rc < 0)
    {
        printf("testXmlwriterMemory: Error at xmlTextWriterWriteAttribute\n");
        return;
    }

    /* Write a comment as child of ORDER */
    tmp = ConvertInput("<\xE4\xF6\xFC>", MY_ENCODING);
    rc = xmlTextWriterWriteFormatComment(writer,
                                         "This is another comment with special chars: %s",
                                         tmp);
    if (rc < 0)
    {
        printf("testXmlwriterMemory: Error at xmlTextWriterWriteFormatComment\n");
        return;
    }
    if (tmp != NULL)
        xmlFree(tmp);

    /* Start an element named "HEADER" as child of ORDER. */
    rc = xmlTextWriterStartElement(writer, BAD_CAST "HEADER");
    if (rc < 0)
    {
        printf("testXmlwriterMemory: Error at xmlTextWriterStartElement\n");
        return;
    }

    /* Write an element named "X_ORDER_ID" as child of HEADER. */
    rc = xmlTextWriterWriteFormatElement(writer, BAD_CAST "X_ORDER_ID",
                                         "%010d", 53535);
    if (rc < 0)
    {
        printf("testXmlwriterMemory: Error at xmlTextWriterWriteFormatElement\n");
        return;
    }

    /* Write an element named "CUSTOMER_ID" as child of HEADER. */
    rc = xmlTextWriterWriteFormatElement(writer, BAD_CAST "CUSTOMER_ID",
                                         "%d", 1010);
    if (rc < 0)
    {
        printf("testXmlwriterMemory: Error at xmlTextWriterWriteFormatElement\n");
        return;
    }

    /* Write an element named "NAME_1" as child of HEADER. */
    tmp = ConvertInput("M\xFCller", MY_ENCODING);
    rc = xmlTextWriterWriteElement(writer, BAD_CAST "NAME_1", tmp);
    if (rc < 0)
    {
        printf("testXmlwriterMemory: Error at xmlTextWriterWriteElement\n");
        return;
    }
    if (tmp != NULL)
        xmlFree(tmp);

    /* Write an element named "NAME_2" as child of HEADER. */
    tmp = ConvertInput("J\xF6rg", MY_ENCODING);
    rc = xmlTextWriterWriteElement(writer, BAD_CAST "NAME_2", tmp);

    if (rc < 0)
    {
        printf("testXmlwriterMemory: Error at xmlTextWriterWriteElement\n");
        return;
    }
    if (tmp != NULL)
        xmlFree(tmp);

    /* Close the element named HEADER. */
    rc = xmlTextWriterEndElement(writer);
    if (rc < 0)
    {
        printf("testXmlwriterMemory: Error at xmlTextWriterEndElement\n");
        return;
    }

    /* Start an element named "ENTRIES" as child of ORDER. */
    rc = xmlTextWriterStartElement(writer, BAD_CAST "ENTRIES");
    if (rc < 0)
    {
        printf("testXmlwriterMemory: Error at xmlTextWriterStartElement\n");
        return;
    }

    /* Start an element named "ENTRY" as child of ENTRIES. */
    rc = xmlTextWriterStartElement(writer, BAD_CAST "ENTRY");
    if (rc < 0)
    {
        printf("testXmlwriterMemory: Error at xmlTextWriterStartElement\n");
        return;
    }

    /* Write an element named "ARTICLE" as child of ENTRY. */
    rc = xmlTextWriterWriteElement(writer, BAD_CAST "ARTICLE",
                                   BAD_CAST "<Test>");
    if (rc < 0)
    {
        printf("testXmlwriterMemory: Error at xmlTextWriterWriteElement\n");
        return;
    }

    /* Write an element named "ENTRY_NO" as child of ENTRY. */
    rc = xmlTextWriterWriteFormatElement(writer, BAD_CAST "ENTRY_NO", "%d",
                                         10);
    if (rc < 0)
    {
        printf("testXmlwriterMemory: Error at xmlTextWriterWriteFormatElement\n");
        return;
    }

    /* Close the element named ENTRY. */
    rc = xmlTextWriterEndElement(writer);
    if (rc < 0)
    {
        printf("testXmlwriterMemory: Error at xmlTextWriterEndElement\n");
        return;
    }

    /* Start an element named "ENTRY" as child of ENTRIES. */
    rc = xmlTextWriterStartElement(writer, BAD_CAST "ENTRY");
    if (rc < 0)
    {
        printf("testXmlwriterMemory: Error at xmlTextWriterStartElement\n");
        return;
    }

    /* Write an element named "ARTICLE" as child of ENTRY. */
    rc = xmlTextWriterWriteElement(writer, BAD_CAST "ARTICLE",
                                   BAD_CAST "<Test 2>");
    if (rc < 0)
    {
        printf("testXmlwriterMemory: Error at xmlTextWriterWriteElement\n");
        return;
    }

    /* Write an element named "ENTRY_NO" as child of ENTRY. */
    rc = xmlTextWriterWriteFormatElement(writer, BAD_CAST "ENTRY_NO", "%d",
                                         20);
    if (rc < 0)
    {
        printf("testXmlwriterMemory: Error at xmlTextWriterWriteFormatElement\n");
        return;
    }

    /* Close the element named ENTRY. */
    rc = xmlTextWriterEndElement(writer);
    if (rc < 0)
    {
        printf("testXmlwriterMemory: Error at xmlTextWriterEndElement\n");
        return;
    }

    /* Close the element named ENTRIES. */
    rc = xmlTextWriterEndElement(writer);
    if (rc < 0)
    {
        printf("testXmlwriterMemory: Error at xmlTextWriterEndElement\n");
        return;
    }

    /* Start an element named "FOOTER" as child of ORDER. */
    rc = xmlTextWriterStartElement(writer, BAD_CAST "FOOTER");
    if (rc < 0)
    {
        printf("testXmlwriterMemory: Error at xmlTextWriterStartElement\n");
        return;
    }

    /* Write an element named "TEXT" as child of FOOTER. */
    rc = xmlTextWriterWriteElement(writer, BAD_CAST "TEXT",
                                   BAD_CAST "This is a text.");
    if (rc < 0)
    {
        printf("testXmlwriterMemory: Error at xmlTextWriterWriteElement\n");
        return;
    }

    /* Close the element named FOOTER. */
    rc = xmlTextWriterEndElement(writer);
    if (rc < 0)
    {
        printf("testXmlwriterMemory: Error at xmlTextWriterEndElement\n");
        return;
    }

    /* Here we could close the elements ORDER and EXAMPLE using the
     * function xmlTextWriterEndElement, but since we do not want to
     * write any other elements, we simply call xmlTextWriterEndDocument,
     * which will do all the work. */
    rc = xmlTextWriterEndDocument(writer);
    if (rc < 0)
    {
        printf("testXmlwriterMemory: Error at xmlTextWriterEndDocument\n");
        return;
    }

    xmlFreeTextWriter(writer);

    fp = fopen(file, "w");
    if (fp == NULL)
    {
        printf("testXmlwriterMemory: Error at fopen\n");
        return;
    }

    fprintf(fp, "%s", (const char *)buf->content);

    fclose(fp);

    xmlBufferFree(buf);
}

/**
 * testXmlwriterDoc:
 * @file: the output file
 *
 * test the xmlWriter interface when creating a new document
 */
void testXmlwriterDoc(const char *file)
{
    int rc;
    xmlTextWriterPtr writer;
    xmlChar *tmp;
    xmlDocPtr doc;

    /* Create a new XmlWriter for DOM, with no compression. */
    writer = xmlNewTextWriterDoc(&doc, 0);
    if (writer == NULL)
    {
        printf("testXmlwriterDoc: Error creating the xml writer\n");
        return;
    }

    /* Start the document with the xml default for the version,
     * encoding ISO 8859-1 and the default for the standalone
     * declaration. */
    rc = xmlTextWriterStartDocument(writer, NULL, MY_ENCODING, NULL);
    if (rc < 0)
    {
        printf("testXmlwriterDoc: Error at xmlTextWriterStartDocument\n");
        return;
    }

    /* Start an element named "EXAMPLE". Since thist is the first
     * element, this will be the root element of the document. */
    rc = xmlTextWriterStartElement(writer, BAD_CAST "EXAMPLE");
    if (rc < 0)
    {
        printf("testXmlwriterDoc: Error at xmlTextWriterStartElement\n");
        return;
    }

    /* Write a comment as child of EXAMPLE.
     * Please observe, that the input to the xmlTextWriter functions
     * HAS to be in UTF-8, even if the output XML is encoded
     * in iso-8859-1 */
    tmp = ConvertInput("This is a comment with special chars: <\xE4\xF6\xFC>",
                       MY_ENCODING);
    rc = xmlTextWriterWriteComment(writer, tmp);
    if (rc < 0)
    {
        printf("testXmlwriterDoc: Error at xmlTextWriterWriteComment\n");
        return;
    }
    if (tmp != NULL)
        xmlFree(tmp);

    /* Start an element named "ORDER" as child of EXAMPLE. */
    rc = xmlTextWriterStartElement(writer, BAD_CAST "ORDER");
    if (rc < 0)
    {
        printf("testXmlwriterDoc: Error at xmlTextWriterStartElement\n");
        return;
    }

    /* Add an attribute with name "version" and value "1.0" to ORDER. */
    rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "version",
                                     BAD_CAST "1.0");
    if (rc < 0)
    {
        printf("testXmlwriterDoc: Error at xmlTextWriterWriteAttribute\n");
        return;
    }

    /* Add an attribute with name "xml:lang" and value "de" to ORDER. */
    rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "xml:lang",
                                     BAD_CAST "de");
    if (rc < 0)
    {
        printf("testXmlwriterDoc: Error at xmlTextWriterWriteAttribute\n");
        return;
    }

    /* Write a comment as child of ORDER */
    tmp = ConvertInput("<\xE4\xF6\xFC>", MY_ENCODING);
    rc = xmlTextWriterWriteFormatComment(writer,
                                         "This is another comment with special chars: %s",
                                         tmp);
    if (rc < 0)
    {
        printf("testXmlwriterDoc: Error at xmlTextWriterWriteFormatComment\n");
        return;
    }
    if (tmp != NULL)
        xmlFree(tmp);

    /* Start an element named "HEADER" as child of ORDER. */
    rc = xmlTextWriterStartElement(writer, BAD_CAST "HEADER");
    if (rc < 0)
    {
        printf("testXmlwriterDoc: Error at xmlTextWriterStartElement\n");
        return;
    }

    /* Write an element named "X_ORDER_ID" as child of HEADER. */
    rc = xmlTextWriterWriteFormatElement(writer, BAD_CAST "X_ORDER_ID",
                                         "%010d", 53535);
    if (rc < 0)
    {
        printf("testXmlwriterDoc: Error at xmlTextWriterWriteFormatElement\n");
        return;
    }

    /* Write an element named "CUSTOMER_ID" as child of HEADER. */
    rc = xmlTextWriterWriteFormatElement(writer, BAD_CAST "CUSTOMER_ID",
                                         "%d", 1010);
    if (rc < 0)
    {
        printf("testXmlwriterDoc: Error at xmlTextWriterWriteFormatElement\n");
        return;
    }

    /* Write an element named "NAME_1" as child of HEADER. */
    tmp = ConvertInput("M\xFCller", MY_ENCODING);
    rc = xmlTextWriterWriteElement(writer, BAD_CAST "NAME_1", tmp);
    if (rc < 0)
    {
        printf("testXmlwriterDoc: Error at xmlTextWriterWriteElement\n");
        return;
    }
    if (tmp != NULL)
        xmlFree(tmp);

    /* Write an element named "NAME_2" as child of HEADER. */
    tmp = ConvertInput("J\xF6rg", MY_ENCODING);
    rc = xmlTextWriterWriteElement(writer, BAD_CAST "NAME_2", tmp);
    if (rc < 0)
    {
        printf("testXmlwriterDoc: Error at xmlTextWriterWriteElement\n");
        return;
    }
    if (tmp != NULL)
        xmlFree(tmp);

    /* Close the element named HEADER. */
    rc = xmlTextWriterEndElement(writer);
    if (rc < 0)
    {
        printf("testXmlwriterDoc: Error at xmlTextWriterEndElement\n");
        return;
    }

    /* Start an element named "ENTRIES" as child of ORDER. */
    rc = xmlTextWriterStartElement(writer, BAD_CAST "ENTRIES");
    if (rc < 0)
    {
        printf("testXmlwriterDoc: Error at xmlTextWriterStartElement\n");
        return;
    }

    /* Start an element named "ENTRY" as child of ENTRIES. */
    rc = xmlTextWriterStartElement(writer, BAD_CAST "ENTRY");
    if (rc < 0)
    {
        printf("testXmlwriterDoc: Error at xmlTextWriterStartElement\n");
        return;
    }

    /* Write an element named "ARTICLE" as child of ENTRY. */
    rc = xmlTextWriterWriteElement(writer, BAD_CAST "ARTICLE",
                                   BAD_CAST "<Test>");
    if (rc < 0)
    {
        printf("testXmlwriterDoc: Error at xmlTextWriterWriteElement\n");
        return;
    }

    /* Write an element named "ENTRY_NO" as child of ENTRY. */
    rc = xmlTextWriterWriteFormatElement(writer, BAD_CAST "ENTRY_NO", "%d",
                                         10);
    if (rc < 0)
    {
        printf("testXmlwriterDoc: Error at xmlTextWriterWriteFormatElement\n");
        return;
    }

    /* Close the element named ENTRY. */
    rc = xmlTextWriterEndElement(writer);
    if (rc < 0)
    {
        printf("testXmlwriterDoc: Error at xmlTextWriterEndElement\n");
        return;
    }

    /* Start an element named "ENTRY" as child of ENTRIES. */
    rc = xmlTextWriterStartElement(writer, BAD_CAST "ENTRY");
    if (rc < 0)
    {
        printf("testXmlwriterDoc: Error at xmlTextWriterStartElement\n");
        return;
    }

    /* Write an element named "ARTICLE" as child of ENTRY. */
    rc = xmlTextWriterWriteElement(writer, BAD_CAST "ARTICLE",
                                   BAD_CAST "<Test 2>");
    if (rc < 0)
    {
        printf("testXmlwriterDoc: Error at xmlTextWriterWriteElement\n");
        return;
    }

    /* Write an element named "ENTRY_NO" as child of ENTRY. */
    rc = xmlTextWriterWriteFormatElement(writer, BAD_CAST "ENTRY_NO", "%d",
                                         20);
    if (rc < 0)
    {
        printf("testXmlwriterDoc: Error at xmlTextWriterWriteFormatElement\n");
        return;
    }

    /* Close the element named ENTRY. */
    rc = xmlTextWriterEndElement(writer);
    if (rc < 0)
    {
        printf("testXmlwriterDoc: Error at xmlTextWriterEndElement\n");
        return;
    }

    /* Close the element named ENTRIES. */
    rc = xmlTextWriterEndElement(writer);
    if (rc < 0)
    {
        printf("testXmlwriterDoc: Error at xmlTextWriterEndElement\n");
        return;
    }

    /* Start an element named "FOOTER" as child of ORDER. */
    rc = xmlTextWriterStartElement(writer, BAD_CAST "FOOTER");
    if (rc < 0)
    {
        printf("testXmlwriterDoc: Error at xmlTextWriterStartElement\n");
        return;
    }

    /* Write an element named "TEXT" as child of FOOTER. */
    rc = xmlTextWriterWriteElement(writer, BAD_CAST "TEXT",
                                   BAD_CAST "This is a text.");
    if (rc < 0)
    {
        printf("testXmlwriterDoc: Error at xmlTextWriterWriteElement\n");
        return;
    }

    /* Close the element named FOOTER. */
    rc = xmlTextWriterEndElement(writer);
    if (rc < 0)
    {
        printf("testXmlwriterDoc: Error at xmlTextWriterEndElement\n");
        return;
    }

    /* Here we could close the elements ORDER and EXAMPLE using the
     * function xmlTextWriterEndElement, but since we do not want to
     * write any other elements, we simply call xmlTextWriterEndDocument,
     * which will do all the work. */
    rc = xmlTextWriterEndDocument(writer);
    if (rc < 0)
    {
        printf("testXmlwriterDoc: Error at xmlTextWriterEndDocument\n");
        return;
    }

    xmlFreeTextWriter(writer);

    xmlSaveFileEnc(file, doc, MY_ENCODING);

    xmlFreeDoc(doc);
}

/**
 * testXmlwriterTree:
 * @file: the output file
 *
 * test the xmlWriter interface when writing to a subtree
 */
void testXmlwriterTree(const char *file)
{
    int rc;
    xmlTextWriterPtr writer;
    xmlDocPtr doc;
    xmlNodePtr node;
    xmlChar *tmp;

    /* Create a new XML DOM tree, to which the XML document will be
     * written */
    doc = xmlNewDoc(BAD_CAST XML_DEFAULT_VERSION);
    if (doc == NULL)
    {
        printf("testXmlwriterTree: Error creating the xml document tree\n");
        return;
    }

    /* Create a new XML node, to which the XML document will be
     * appended */
    node = xmlNewDocNode(doc, NULL, BAD_CAST "EXAMPLE", NULL);
    if (node == NULL)
    {
        printf("testXmlwriterTree: Error creating the xml node\n");
        return;
    }

    /* Make ELEMENT the root node of the tree */
    xmlDocSetRootElement(doc, node);

    /* Create a new XmlWriter for DOM tree, with no compression. */
    writer = xmlNewTextWriterTree(doc, node, 0);
    if (writer == NULL)
    {
        printf("testXmlwriterTree: Error creating the xml writer\n");
        return;
    }

    /* Start the document with the xml default for the version,
     * encoding ISO 8859-1 and the default for the standalone
     * declaration. */
    rc = xmlTextWriterStartDocument(writer, NULL, MY_ENCODING, NULL);
    if (rc < 0)
    {
        printf("testXmlwriterTree: Error at xmlTextWriterStartDocument\n");
        return;
    }

    /* Write a comment as child of EXAMPLE.
     * Please observe, that the input to the xmlTextWriter functions
     * HAS to be in UTF-8, even if the output XML is encoded
     * in iso-8859-1 */
    tmp = ConvertInput("This is a comment with special chars: <\xE4\xF6\xFC>",
                       MY_ENCODING);
    rc = xmlTextWriterWriteComment(writer, tmp);
    if (rc < 0)
    {
        printf("testXmlwriterTree: Error at xmlTextWriterWriteComment\n");
        return;
    }
    if (tmp != NULL)
        xmlFree(tmp);

    /* Start an element named "ORDER" as child of EXAMPLE. */
    rc = xmlTextWriterStartElement(writer, BAD_CAST "ORDER");
    if (rc < 0)
    {
        printf("testXmlwriterTree: Error at xmlTextWriterStartElement\n");
        return;
    }

    /* Add an attribute with name "version" and value "1.0" to ORDER. */
    rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "version",
                                     BAD_CAST "1.0");
    if (rc < 0)
    {
        printf("testXmlwriterTree: Error at xmlTextWriterWriteAttribute\n");
        return;
    }

    /* Add an attribute with name "xml:lang" and value "de" to ORDER. */
    rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "xml:lang",
                                     BAD_CAST "de");
    if (rc < 0)
    {
        printf("testXmlwriterTree: Error at xmlTextWriterWriteAttribute\n");
        return;
    }

    /* Write a comment as child of ORDER */
    tmp = ConvertInput("<\xE4\xF6\xFC>", MY_ENCODING);
    rc = xmlTextWriterWriteFormatComment(writer,
                                         "This is another comment with special chars: %s",
                                         tmp);
    if (rc < 0)
    {
        printf("testXmlwriterTree: Error at xmlTextWriterWriteFormatComment\n");
        return;
    }
    if (tmp != NULL)
        xmlFree(tmp);

    /* Start an element named "HEADER" as child of ORDER. */
    rc = xmlTextWriterStartElement(writer, BAD_CAST "HEADER");
    if (rc < 0)
    {
        printf("testXmlwriterTree: Error at xmlTextWriterStartElement\n");
        return;
    }

    /* Write an element named "X_ORDER_ID" as child of HEADER. */
    rc = xmlTextWriterWriteFormatElement(writer, BAD_CAST "X_ORDER_ID",
                                         "%010d", 53535);
    if (rc < 0)
    {
        printf("testXmlwriterTree: Error at xmlTextWriterWriteFormatElement\n");
        return;
    }

    /* Write an element named "CUSTOMER_ID" as child of HEADER. */
    rc = xmlTextWriterWriteFormatElement(writer, BAD_CAST "CUSTOMER_ID",
                                         "%d", 1010);
    if (rc < 0)
    {
        printf("testXmlwriterTree: Error at xmlTextWriterWriteFormatElement\n");
        return;
    }

    /* Write an element named "NAME_1" as child of HEADER. */
    tmp = ConvertInput("M\xFCller", MY_ENCODING);
    rc = xmlTextWriterWriteElement(writer, BAD_CAST "NAME_1", tmp);
    if (rc < 0)
    {
        printf("testXmlwriterTree: Error at xmlTextWriterWriteElement\n");
        return;
    }
    if (tmp != NULL)
        xmlFree(tmp);

    /* Write an element named "NAME_2" as child of HEADER. */
    tmp = ConvertInput("J\xF6rg", MY_ENCODING);
    rc = xmlTextWriterWriteElement(writer, BAD_CAST "NAME_2", tmp);
    if (rc < 0)
    {
        printf("testXmlwriterTree: Error at xmlTextWriterWriteElement\n");
        return;
    }
    if (tmp != NULL)
        xmlFree(tmp);

    /* Close the element named HEADER. */
    rc = xmlTextWriterEndElement(writer);
    if (rc < 0)
    {
        printf("testXmlwriterTree: Error at xmlTextWriterEndElement\n");
        return;
    }

    /* Start an element named "ENTRIES" as child of ORDER. */
    rc = xmlTextWriterStartElement(writer, BAD_CAST "ENTRIES");
    if (rc < 0)
    {
        printf("testXmlwriterTree: Error at xmlTextWriterStartElement\n");
        return;
    }

    /* Start an element named "ENTRY" as child of ENTRIES. */
    rc = xmlTextWriterStartElement(writer, BAD_CAST "ENTRY");
    if (rc < 0)
    {
        printf("testXmlwriterTree: Error at xmlTextWriterStartElement\n");
        return;
    }

    /* Write an element named "ARTICLE" as child of ENTRY. */
    rc = xmlTextWriterWriteElement(writer, BAD_CAST "ARTICLE",
                                   BAD_CAST "<Test>");
    if (rc < 0)
    {
        printf("testXmlwriterTree: Error at xmlTextWriterWriteElement\n");
        return;
    }

    /* Write an element named "ENTRY_NO" as child of ENTRY. */
    rc = xmlTextWriterWriteFormatElement(writer, BAD_CAST "ENTRY_NO", "%d",
                                         10);
    if (rc < 0)
    {
        printf("testXmlwriterTree: Error at xmlTextWriterWriteFormatElement\n");
        return;
    }

    /* Close the element named ENTRY. */
    rc = xmlTextWriterEndElement(writer);
    if (rc < 0)
    {
        printf("testXmlwriterTree: Error at xmlTextWriterEndElement\n");
        return;
    }

    /* Start an element named "ENTRY" as child of ENTRIES. */
    rc = xmlTextWriterStartElement(writer, BAD_CAST "ENTRY");
    if (rc < 0)
    {
        printf("testXmlwriterTree: Error at xmlTextWriterStartElement\n");
        return;
    }

    /* Write an element named "ARTICLE" as child of ENTRY. */
    rc = xmlTextWriterWriteElement(writer, BAD_CAST "ARTICLE",
                                   BAD_CAST "<Test 2>");
    if (rc < 0)
    {
        printf("testXmlwriterTree: Error at xmlTextWriterWriteElement\n");
        return;
    }

    /* Write an element named "ENTRY_NO" as child of ENTRY. */
    rc = xmlTextWriterWriteFormatElement(writer, BAD_CAST "ENTRY_NO", "%d",
                                         20);
    if (rc < 0)
    {
        printf("testXmlwriterTree: Error at xmlTextWriterWriteFormatElement\n");
        return;
    }

    /* Close the element named ENTRY. */
    rc = xmlTextWriterEndElement(writer);
    if (rc < 0)
    {
        printf("testXmlwriterTree: Error at xmlTextWriterEndElement\n");
        return;
    }

    /* Close the element named ENTRIES. */
    rc = xmlTextWriterEndElement(writer);
    if (rc < 0)
    {
        printf("testXmlwriterTree: Error at xmlTextWriterEndElement\n");
        return;
    }

    /* Start an element named "FOOTER" as child of ORDER. */
    rc = xmlTextWriterStartElement(writer, BAD_CAST "FOOTER");
    if (rc < 0)
    {
        printf("testXmlwriterTree: Error at xmlTextWriterStartElement\n");
        return;
    }

    /* Write an element named "TEXT" as child of FOOTER. */
    rc = xmlTextWriterWriteElement(writer, BAD_CAST "TEXT",
                                   BAD_CAST "This is a text.");
    if (rc < 0)
    {
        printf("testXmlwriterTree: Error at xmlTextWriterWriteElement\n");
        return;
    }

    /* Close the element named FOOTER. */
    rc = xmlTextWriterEndElement(writer);
    if (rc < 0)
    {
        printf("testXmlwriterTree: Error at xmlTextWriterEndElement\n");
        return;
    }

    /* Here we could close the elements ORDER and EXAMPLE using the
     * function xmlTextWriterEndElement, but since we do not want to
     * write any other elements, we simply call xmlTextWriterEndDocument,
     * which will do all the work. */
    rc = xmlTextWriterEndDocument(writer);
    if (rc < 0)
    {
        printf("testXmlwriterTree: Error at xmlTextWriterEndDocument\n");
        return;
    }

    xmlFreeTextWriter(writer);

    xmlSaveFileEnc(file, doc, MY_ENCODING);

    xmlFreeDoc(doc);
}

/**
 * ConvertInput:
 * @in: string in a given encoding
 * @encoding: the encoding used
 *
 * Converts @in into UTF-8 for processing with libxml2 APIs
 *
 * Returns the converted UTF-8 string, or NULL in case of error.
 */
xmlChar *
ConvertInput(const char *in, const char *encoding)
{
    xmlChar *out;
    int ret;
    int size;
    int out_size;
    int temp;
    xmlCharEncodingHandlerPtr handler;

    if (in == 0)
        return 0;

    handler = xmlFindCharEncodingHandler(encoding);

    if (!handler)
    {
        printf("ConvertInput: no encoding handler found for '%s'\n",
               encoding ? encoding : "");
        return 0;
    }

    size = (int)strlen(in) + 1;
    out_size = size * 2 - 1;
    out = (unsigned char *)xmlMalloc((size_t)out_size);

    if (out != 0)
    {
        temp = size - 1;
        ret = handler->input(out, &out_size, (const xmlChar *)in, &temp);
        if ((ret < 0) || (temp - size + 1))
        {
            if (ret < 0)
            {
                printf("ConvertInput: conversion wasn't successful.\n");
            }
            else
            {
                printf("ConvertInput: conversion wasn't successful. converted: %i octets.\n",
                       temp);
            }

            xmlFree(out);
            out = 0;
        }
        else
        {
            out = (unsigned char *)xmlRealloc(out, out_size + 1);
            out[out_size] = 0; /*null terminating out */
        }
    }
    else
    {
        printf("ConvertInput: no mem\n");
    }

    return out;
}

/**
 * exampleFunc:
 * @filename: a filename or an URL
 *
 * Parse and validate the resource and free the resulting tree
 */
static void exampleFunc(const char *filename)
{
    xmlParserCtxtPtr ctxt; /* the parser context */
    xmlDocPtr doc;         /* the resulting document tree */

    /* create a parser context */
    ctxt = xmlNewParserCtxt();
    if (ctxt == NULL)
    {
        fprintf(stderr, "Failed to allocate parser context\n");
        return;
    }
    /* parse the file, activating the DTD validation option */
    doc = xmlCtxtReadFile(ctxt, filename, NULL, XML_PARSE_DTDVALID);
    /* check if parsing suceeded */
    if (doc == NULL)
    {
        fprintf(stderr, "Failed to parse %s\n", filename);
    }
    else
    {
        /* check if validation suceeded */
        if (ctxt->valid == 0)
            fprintf(stderr, "Failed to validate %s\n", filename);
        /* free up the resulting document */
        xmlFreeDoc(doc);
    }
    /* free up the parser context */
    xmlFreeParserCtxt(ctxt);
}

int test_exampleFunc(int argc, char **argv)
{
    if (argc != 2)
        return (1);

    /*
     * this initialize the library and check potential ABI mismatches
     * between the version it was compiled for and the actual shared
     * library used.
     */
    LIBXML_TEST_VERSION

    exampleFunc(argv[1]);

    /*
     * Cleanup function for the XML library.
     */
    xmlCleanupParser();
    /*
     * this is to debug memory for regression tests
     */
    xmlMemoryDump();
    return (0);
}

/* A simple example how to create DOM. Libxml2 automagically 
 * allocates the necessary amount of memory to it.
*/
int createXML(int argc, char **argv)
{
    xmlDocPtr doc = NULL;                                   /* document pointer */
    xmlNodePtr root_node = NULL, node = NULL, node1 = NULL; /* node pointers */
    char buff[256];
    int i, j;

    LIBXML_TEST_VERSION;

    /* 
     * Creates a new document, a node and set it as a root node
     */
    doc = xmlNewDoc(BAD_CAST "1.0");
    root_node = xmlNewNode(NULL, BAD_CAST "root");
    xmlDocSetRootElement(doc, root_node);

    /*
     * Creates a DTD declaration. Isn't mandatory. 
     */
    xmlCreateIntSubset(doc, BAD_CAST "root", NULL, BAD_CAST "tree2.dtd");

    /* 
     * xmlNewChild() creates a new node, which is "attached" as child node
     * of root_node node. 
     */
    xmlNewChild(root_node, NULL, BAD_CAST "node1",
                BAD_CAST "content of node 1");
    /* 
     * The same as above, but the new child node doesn't have a content 
     */
    xmlNewChild(root_node, NULL, BAD_CAST "node2", NULL);

    /* 
     * xmlNewProp() creates attributes, which is "attached" to an node.
     * It returns xmlAttrPtr, which isn't used here.
     */
    node =
        xmlNewChild(root_node, NULL, BAD_CAST "node3",
                    BAD_CAST "this node has attributes");
    xmlNewProp(node, BAD_CAST "attribute", BAD_CAST "yes");
    xmlNewProp(node, BAD_CAST "foo", BAD_CAST "bar");

    /*
     * Here goes another way to create nodes. xmlNewNode() and xmlNewText
     * creates a node and a text node separately. They are "attached"
     * by xmlAddChild() 
     */
    node = xmlNewNode(NULL, BAD_CAST "node4");
    node1 = xmlNewText(BAD_CAST
                       "other way to create content (which is also a node)");
    xmlAddChild(node, node1);
    xmlAddChild(root_node, node);

    /* 
     * A simple loop that "automates" nodes creation 
     */
    for (i = 5; i < 7; i++)
    {
        sprintf(buff, "node%d", i);
        node = xmlNewChild(root_node, NULL, BAD_CAST buff, NULL);
        for (j = 1; j < 4; j++)
        {
            sprintf(buff, "node%d%d", i, j);
            node1 = xmlNewChild(node, NULL, BAD_CAST buff, NULL);
            xmlNewProp(node1, BAD_CAST "odd", BAD_CAST((j % 2) ? "no" : "yes"));
        }
    }

    /* 
     * Dumping document to stdio or file
     */
    xmlSaveFormatFileEnc(argc > 1 ? argv[1] : "-", doc, "UTF-8", 1);

    /*free the document */
    xmlFreeDoc(doc);

    /*
     *Free the global variables that may
     *have been allocated by the parser.
     */
    xmlCleanupParser();

    /*
     * this is to debug memory for regression tests
     */
    xmlMemoryDump();
    return (0);
}




