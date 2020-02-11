#ifndef SCARLETBOOK_XML_H_INCLUDED
#define SCARLETBOOK_XML_H_INCLUDED



#define MY_ENCODING "UTF-8" //"ISO-8859-1" 

#ifdef __cplusplus
extern "C"
{
#endif

void read_metadata_xml(const char *filename);

int write_metadata_xml(scarletbook_handle_t *handle, const char *metadata_file_path_unique);

#ifdef __cplusplus
};
#endif

#endif /* SCARLETBOOK_XML_H_INCLUDED */