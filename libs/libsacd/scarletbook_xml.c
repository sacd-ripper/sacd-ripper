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
        rc = xmlTextWriterWriteElement(writer, BAD_CAST "Title", BAD_CAST "");

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
                rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "value", BAD_CAST handle->master_text.album_title);
            else
                rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "value", BAD_CAST "");

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
                rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "value", BAD_CAST "");

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
            if (area_isrc_genre->isrc[t].country_code[0])
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
            if (handle->area[area_idx].area_isrc_genre->isrc[t].recording_year[0])
            {
                strncpy(string_buf, handle->area[area_idx].area_isrc_genre->isrc[t].recording_year, 2);
                string_buf[2] = '\0';
                rc = xmlTextWriterWriteFormatAttribute(writer, BAD_CAST "value","%s", string_buf);
            }
            else
                rc = xmlTextWriterWriteFormatAttribute(writer, BAD_CAST "value","%4i", handle->master_toc->disc_date_year);
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

