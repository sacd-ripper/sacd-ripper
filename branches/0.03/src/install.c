#include <sysutil/msg.h>
#include <sysutil/sysutil.h>

#include <self.h>

#include "rsxutil.h"
#include "exit_handler.h"

static const char FILE_SACD_PLUGIN_SOURCE[]	= "/dev_flash/vsh/module/sacd_plugin.sprx";
static const char FILE_SAC_MODULE_SOURCE[] 	= "/dev_flash/vsh/module/SacModule.spu.isoself";
static const char FILE_SAC_MODULE[] 	    = "/dev_hdd0/game/SACDRIP01/USRDIR/sac_module.spu.elf";
static const char FILE_DECODER[] 		    = "/dev_hdd0/game/SACDRIP01/USRDIR/decoder.spu.elf";

static int install_sac_module(void) {
    FILE *in = NULL;
    FILE *out = NULL;
    self_ehdr_t self;
    app_info_t app_info;
    elf_ehdr_t elf;
    elf_phdr_t *phdr = NULL;
    elf_shdr_t *shdr = NULL;
    section_info_t *section_info = NULL;
    sceversion_info_t sceversion_info;
    control_info_t *control_info = NULL;
    metadata_info_t metadata_info;
    metadata_header_t metadata_header;
    metadata_section_header_t *section_headers = NULL;
    uint8_t *keys = NULL;
    signature_info_t signature_info;
    signature_t signature;
    self_section_t *sections = NULL;
    int num_sections = 0, ret;
    int i;
    uint8_t *isoself_data;
    size_t isoself_size;

    in = fopen(FILE_SAC_MODULE_SOURCE, "rb");
    if (in == NULL) {
        return -1;
    }

    ret = self_read_headers(in, &self, &app_info, &elf, &phdr, &shdr,
            &section_info, &sceversion_info, &control_info);
    
    if (ret != 0)
    	goto fail;
            
    ret = self_read_metadata(in, &self, &app_info, &metadata_info,
            &metadata_header, &section_headers, &keys,
            &signature_info, &signature);

    if (ret != 0)
    	goto fail;

    num_sections = self_load_sections(in, &self, &elf, &phdr,
            &metadata_header, &section_headers, &keys, &sections);

    if (num_sections <= 0)
    	goto fail;
    
    out = fopen(FILE_SAC_MODULE, "wb");
    if (out == NULL) {
        return -1;
    }

    for (i = 0; i < num_sections; i++) {
        fseek(out, (long) sections[i].offset, SEEK_SET);
        if (fwrite(sections[i].data, 1, (size_t) sections[i].size, out) != sections[i].size) {
            return -1;
        }
    }
    fclose(out);

    isoself_size = find_isoself(sections, num_sections, "decoder.spu", &isoself_data);
    if (isoself_size) {
        out = fopen("decoder.spu.elf", "wb");
        fwrite(isoself_data, isoself_size, 1, out);
        fclose(out);
    }

    fclose(in);
    self_free_sections(&sections, num_sections);

    return 0;

fail:

    fclose(in);
    self_free_sections(&sections, num_sections);

    return -1;
}

static int install_decoder_module(void) {
    FILE *in = NULL;
    FILE *out = NULL;
    self_ehdr_t self;
    app_info_t app_info;
    elf_ehdr_t elf;
    elf_phdr_t *phdr = NULL;
    elf_shdr_t *shdr = NULL;
    section_info_t *section_info = NULL;
    sceversion_info_t sceversion_info;
    control_info_t *control_info = NULL;
    metadata_info_t metadata_info;
    metadata_header_t metadata_header;
    metadata_section_header_t *section_headers = NULL;
    uint8_t *keys = NULL;
    signature_info_t signature_info;
    signature_t signature;
    self_section_t *sections = NULL;
    int num_sections = 0, ret;
    uint8_t *isoself_data;
    size_t isoself_size;

    in = fopen(FILE_SACD_PLUGIN_SOURCE, "rb");
    if (in == NULL) {
        return -1;
    }

    ret = self_read_headers(in, &self, &app_info, &elf, &phdr, &shdr,
            &section_info, &sceversion_info, &control_info);
    
    if (ret != 0)
    	goto fail;
            
    ret = self_read_metadata(in, &self, &app_info, &metadata_info,
            &metadata_header, &section_headers, &keys,
            &signature_info, &signature);

    if (ret != 0)
    	goto fail;

    num_sections = self_load_sections(in, &self, &elf, &phdr,
            &metadata_header, &section_headers, &keys, &sections);

    isoself_size = find_isoself(sections, num_sections, "decoder.spu", &isoself_data);
    if (isoself_size) {
        out = fopen(FILE_DECODER, "wb");
        if (out == NULL) {
        	return -1;
        }
        fwrite(isoself_data, isoself_size, 1, out);
        fclose(out);
    }

    fclose(in);

    self_free_sections(&sections, num_sections);

	if (!isoself_size)
		return -1;

    return 0;

fail:

    fclose(in);
    self_free_sections(&sections, num_sections);

    return -1;
}

int dialog_action;

static void dialog_handler(msgButton button, void *user_data)
{
	switch(button) {
		case MSG_DIALOG_BTN_OK:
			dialog_action = 1;
			break;
		case MSG_DIALOG_BTN_NO:
		case MSG_DIALOG_BTN_ESCAPE:
			dialog_action = 2;
			break;
		case MSG_DIALOG_BTN_NONE:
			dialog_action = -1;
			break;
		default:
			break;
	}
}

static int has_all_modules_installed(void) {
	FILE *in = NULL;
	
    in = fopen(FILE_DECODER, "rb");
    if (in == NULL) {
        fclose(in);
        return -1;
    }

    in = fopen(FILE_SAC_MODULE, "rb");
    if (in == NULL) {
        fclose(in);
        return -1;
    }
    
    return 0;
}

int install_modules(void) {
	msgType dialogType;
	int installed;

	if (has_all_modules_installed() != 0) {

		do {
	
			dialogType = (MSG_DIALOG_NORMAL | MSG_DIALOG_BTN_TYPE_OK);
			msgDialogOpen2(dialogType, "The SACD authentication and DST decoder modules need to be extracted.\nMake sure you have the PS3 keys (app-pub-355, iso-iv-355, etc..) in the root of an external USB/flash disc.", dialog_handler, NULL, NULL);
			dialog_action = 0;
			while (!dialog_action && !user_requested_exit()) {
				sysUtilCheckCallback();
				flip();
			}
			msgDialogAbort();
			
			if (user_requested_exit())
				return 1;

			if (dialog_action != 1)
				return -1;

			installed = (install_sac_module() == 0 && install_decoder_module() == 0);

			if (installed) {

				dialogType = (MSG_DIALOG_NORMAL | MSG_DIALOG_BTN_TYPE_OK | MSG_DIALOG_DISABLE_CANCEL_ON);
				msgDialogOpen2(dialogType, "The modules were successfully extracted.", dialog_handler, NULL, NULL);
				dialog_action = 0;
				while (!dialog_action && !user_requested_exit()) {
					sysUtilCheckCallback();
					flip();
				}
				msgDialogAbort(); 

			} else {

				dialogType = (MSG_DIALOG_NORMAL | MSG_DIALOG_BTN_TYPE_OK);
				msgDialogOpen2(dialogType, "ERROR: The keys were not found or the modules could not be extracted.\nPlease try again.", dialog_handler, NULL, NULL);
				dialog_action = 0;
				while (!dialog_action && !user_requested_exit()) {
					sysUtilCheckCallback();
					flip();
				}
				msgDialogAbort(); 

				if (dialog_action != 1)
					return -1;

			}
			
		} while (!installed && !user_requested_exit());
	}
	
	return 0;
}
