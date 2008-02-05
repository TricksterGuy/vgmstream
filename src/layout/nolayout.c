#include "nolayout.h"
#include "../vgmstream.h"

void render_vgmstream_nolayout(sample * buffer, int32_t sample_count, VGMSTREAM * vgmstream) {
    int samples_written=0;

    const int samples_this_block = vgmstream->num_samples;
    int samples_per_frame = get_vgmstream_samples_per_frame(vgmstream);

    while (samples_written<sample_count) {
        int samples_to_do;
        int chan;
        int samples_left_this_block;

        samples_left_this_block = samples_this_block - vgmstream->samples_into_block;
        samples_to_do = samples_left_this_block;

        /* fun loopy crap */
        /* Why did I think this would be any simpler? */
        if (vgmstream->loop_flag) {
            /* is this the loop end? */
            if (vgmstream->current_sample==vgmstream->loop_end_sample) {
                /* restore! */
                memcpy(vgmstream->ch,vgmstream->loop_ch,sizeof(VGMSTREAMCHANNEL)*vgmstream->channels);
                vgmstream->current_sample=vgmstream->loop_sample;
                vgmstream->samples_into_block=vgmstream->loop_samples_into_block;

                continue;   /* recalculate stuff */
            }


            /* is this the loop start? */
            if (!vgmstream->hit_loop && vgmstream->current_sample==vgmstream->loop_start_sample) {
                /* save! */
                memcpy(vgmstream->loop_ch,vgmstream->ch,sizeof(VGMSTREAMCHANNEL)*vgmstream->channels);

                vgmstream->loop_sample=vgmstream->current_sample;
                vgmstream->loop_samples_into_block=vgmstream->samples_into_block;
                vgmstream->hit_loop=1;
            }

            /* are we going to hit the loop end during this block? */
            if (vgmstream->current_sample+samples_left_this_block > vgmstream->loop_end_sample) {
                /* only do to just before it */
                samples_to_do = vgmstream->loop_end_sample-vgmstream->current_sample;
            }

            /* are we going to hit the loop start during this block? */
            if (!vgmstream->hit_loop && vgmstream->current_sample+samples_left_this_block > vgmstream->loop_start_sample) {
                /* only do to just before it */
                samples_to_do = vgmstream->loop_start_sample-vgmstream->current_sample;
            }

        }

        if (samples_per_frame > 1 && (vgmstream->samples_into_block%samples_per_frame)+samples_to_do>samples_per_frame) samples_to_do=samples_per_frame-(vgmstream->samples_into_block%samples_per_frame);

        if (samples_written+samples_to_do > sample_count)
            samples_to_do=sample_count-samples_written;

        decode_vgmstream(vgmstream, samples_written, samples_to_do, buffer);

        samples_written += samples_to_do;
        vgmstream->current_sample += samples_to_do;
        vgmstream->samples_into_block+=samples_to_do;
    }
}