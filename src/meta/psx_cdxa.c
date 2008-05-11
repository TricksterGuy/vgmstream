#include "meta.h"
#include "../layout/layout.h"
#include "../util.h"

/* Sony PSX CD-XA */
/* No looped file ! */

off_t init_xa_channel(int channel,VGMSTREAM* vgmstream);

uint8_t AUDIO_CODING_GET_STEREO(uint8_t value) {
	return (uint8_t)(value & 3);
}

uint8_t AUDIO_CODING_GET_FREQ(uint8_t value) {
	return (uint8_t)((value >> 2) & 3);
}

VGMSTREAM * init_vgmstream_cdxa(const char * const filename) {
    VGMSTREAM * vgmstream = NULL;
    STREAMFILE * infile = NULL;

	int channel_count;
	uint8_t bCoding;

    int i;

    /* check extension, case insensitive */
    if (strcasecmp("xa",filename_extension(filename))) goto fail;

    /* try to open the file for header reading */
    infile = open_streamfile(filename);
    if (!infile) goto fail;

    /* check RIFF Header */
    if (!((read_32bitBE(0x00,infile) == 0x52494646) && 
	      (read_32bitBE(0x08,infile) == 0x43445841) && 
		  (read_32bitBE(0x0C,infile) == 0x666D7420)))
        goto fail;

	bCoding = read_8bit(0x3F,infile);

	switch (AUDIO_CODING_GET_STEREO(bCoding)) {
		case 0: channel_count = 1; break;
		case 1: channel_count = 2; break;
		default: channel_count = 0; break;
	}

	/* build the VGMSTREAM */
    vgmstream = allocate_vgmstream(channel_count,0);
    if (!vgmstream) goto fail;

	/* fill in the vital statistics */
	vgmstream->channels = channel_count;

	switch (AUDIO_CODING_GET_FREQ(bCoding)) {
		case 0: vgmstream->sample_rate = 37800; break;
		case 1: vgmstream->sample_rate = 19800; break;
		default: vgmstream->sample_rate = 0; break;
	}

	/* Check for Compression Scheme */
	vgmstream->coding_type = coding_XA;
    vgmstream->num_samples = (int32_t)((((get_streamfile_size(infile) - 0x3C)/2352)*0x1F80)/(2*channel_count));

    vgmstream->layout_type = layout_xa_blocked;
    vgmstream->meta_type = meta_PSX_XA;

	close_streamfile(infile); infile=NULL;

    /* open the file for reading by each channel */
    {
        for (i=0;i<channel_count;i++) {
            vgmstream->ch[i].streamfile = open_streamfile_buffer(filename,0x8000);

            if (!vgmstream->ch[i].streamfile) goto fail;
        }
    }

	xa_block_update(init_xa_channel(0,vgmstream),vgmstream);

	return vgmstream;

    /* clean up anything we may have opened */
fail:
    if (infile) close_streamfile(infile);
    if (vgmstream) close_vgmstream(vgmstream);
    return NULL;
}

// in extension, we can use this fonction to set the channel
// we want to listen to ...
off_t init_xa_channel(int channel,VGMSTREAM* vgmstream) {
	
	off_t block_offset=0x44;

	uint8_t currentChannel;
	uint8_t subAudio;

	vgmstream->xa_channel=channel;

begin:
	currentChannel=read_8bit(block_offset-7,vgmstream->ch[0].streamfile);
	subAudio=read_8bit(block_offset-6,vgmstream->ch[0].streamfile);
	if ((currentChannel!=channel) && (subAudio==0x64)) {
		block_offset+=2352;
		goto begin;
	}
	return block_offset;
}