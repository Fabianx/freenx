/*
 * nxvorbis.c - A nx esd proxy for low bandwidth vorbis encoded transmission of data.
 *
 * Copyright (c) 2007 by Fabian Franz <freenx@fabian-franz.de>.
 *
 * License: GPL, v2
 *
 */

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <esd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <time.h>

#include <vorbis/codec.h>
#include <vorbis/vorbisenc.h>

#define READSIZE 1024

/* After how many frames should we resync with server? */
#define DEFAULT_DO_SYNC_SEQ	20

/* #define DEBUG 1 */

int esd_set_socket_buffers( int sock, int src_format,
		            int src_rate, int base_rate );

int do_fwd(int from, int to, void* buf, size_t count)
{
        ssize_t len;

	len=read(from, buf, count);
        return write(to, buf, len);
}

typedef struct {
  ogg_int64_t  bytes;
  ogg_int64_t  b_o_s;
  ogg_int64_t  e_o_s;

  ogg_int64_t  granulepos;

  ogg_int64_t  packetno;

} nx_ogg_packet;

int do_read_complete(int from, void* buf, size_t count);

int do_read_op(int from, ogg_packet* op)
{
	nx_ogg_packet nxop;

	if (do_read_complete(from, &nxop, sizeof(nxop)) != sizeof(nxop))
		return 0;

	op->bytes=nxop.bytes;
	op->b_o_s=nxop.b_o_s;
	op->granulepos=nxop.granulepos;
	op->packetno=nxop.packetno;

	op->packet=malloc(op->bytes+1);

	if (do_read_complete(from, op->packet, op->bytes) != op->bytes)
		return 0;
	return 1;
}

int do_write_op(int to, ogg_packet* op)
{
	nx_ogg_packet nxop;

	nxop.bytes=op->bytes;
	nxop.b_o_s=op->b_o_s;
	nxop.granulepos=op->granulepos;
	nxop.packetno=op->packetno;
	
	if (write(to, &nxop, sizeof(nxop)) != sizeof(nxop))
		return 0;
	if (write(to, op->packet, op->bytes) != op->bytes)
		return 0;
	return 1;
}

void do_sockopts(int sock, int buf_size)
{
	int sz=buf_size;
	setsockopt(sock, SOL_SOCKET, SO_SNDBUF, &sz, sizeof(sz));
	setsockopt(sock, SOL_SOCKET, SO_RCVBUF, &sz, sizeof(sz));
}

int do_read_complete(int from, void* buf, size_t count)
{
	size_t erg, len=0;
	
	do
	{
		erg=read(from, buf+len, count-len);
		if (erg <= 0)
			break;
		len+=erg;
	}
	while (len < count);

	return len;
}


int do_read_samples(int from, float** buffer, int samples, esd_format_t format)
{
	int i,j;
	int samplesize=(format & ESD_BITS16)?16:8;
	int channels=(format & ESD_STEREO)?2:1;

        int sampbyte = samplesize / 8;
        signed char *buf = alloca(samples*sampbyte*channels);
	size_t erg, len=0, count=samples*sampbyte*channels;

	do
	{
		erg=read(from, buf+len, count-len);
		if (erg <= 0)
			return len / (sampbyte*channels);
		len+=erg;
	}
	while (len < count);

	/* Now do the conversion */

	if (samplesize == 16)
	{
		/* FIXME: Big endian */
		for(i = 0; i < samples; i++)
			for(j=0; j < channels; j++)
				buffer[j][i] = ((buf[i*2*channels + 2*j + 1]<<8) | 
							(buf[i*2*channels + 2*j] & 0xff))/32768.0f;
	}
	else 
	{
		fprintf(stderr, "8 Bits unsupported for now.");
		return 0;
#if 0
                unsigned char *bufu = (unsigned char *)buf;
		for(i = 0; i < samples; i++)
			for(j=0; j < channels; j++)
				buffer[j][i]=((int)(bufu[i*channels + j])-128)/128.0f;
#endif
	}
	
	return samples;
}



int do_encode(int client, int server, esd_format_t format, int speed, char* ident)
{
	/* Encoder specific variables */
	ogg_packet       op;
	vorbis_dsp_state vd;
	vorbis_block     vb;
	vorbis_info      vi;
	vorbis_comment   vc; /* struct that stores all the bitstream user comments */

	unsigned int do_sync_seq=DEFAULT_DO_SYNC_SEQ;
	unsigned int seqNr=0;
	
	/* Configuration variables */
	/* FIXME: Depend on NX setting */
	float quality=0.3;
	
	/* Encoder initialisation */
	vorbis_info_init(&vi);
	vorbis_comment_init(&vc);

	vorbis_encode_setup_vbr(&vi, (format & ESD_STEREO)?2:1 , speed, quality);
	vorbis_encode_setup_init(&vi);

	/* Now, set up the analysis engine, stream encoder, and other
	 *            preparation before the encoding begins.
	 *                     
	 */

	vorbis_analysis_init(&vd,&vi);
	vorbis_block_init(&vd,&vb);

	fprintf(stderr, "Analysis ok\n");

	/* Send all three headers */
	{
                ogg_packet header_main;
		ogg_packet header_comments;
		ogg_packet header_codebooks;

		/* Build the packets */
		vorbis_analysis_headerout(&vd,&vc, &header_main,&header_comments,&header_codebooks);
		
		if (!do_write_op(server, &header_main))
			goto out;
		if (!do_write_op(server, &header_comments))
			goto out;
		if (!do_write_op(server, &header_codebooks))
			goto out;
										
	}
	fprintf(stderr, "Header ok\n");
	
	/* Lower the latency */
	
	//do_sockopts(client, frame_size2 * (((format & ESD_BITS16)?16:8) / 8) * 4);
	esd_set_socket_buffers(client, format, speed, 44100);
	do_sockopts(server, 200);

	/* Send do_sync_seq to decoder */
	if (write(server, &do_sync_seq, sizeof(do_sync_seq)) != sizeof(do_sync_seq))
		goto out;

	/* Main encoding loop */

	while (1)
	{
		unsigned int newSeq;
		float **buffer = vorbis_analysis_buffer(&vd, READSIZE);
		long samples_read = do_read_samples(client, buffer, READSIZE, format);

		if (samples_read != READSIZE)
			break;

		/* Tell the library how many samples (per channel) we wrote
		 * into the supplied buffer */
		vorbis_analysis_wrote(&vd, samples_read);

		while(vorbis_analysis_blockout(&vd,&vb)==1)
                {

                        /* Do the main analysis, creating a packet */
                        vorbis_analysis(&vb, NULL);
                        vorbis_bitrate_addblock(&vb);

                        while(vorbis_bitrate_flushpacket(&vd, &op))
                        {

				if (write(server, &seqNr, sizeof(seqNr)) != sizeof(seqNr))
					goto out;
#ifdef DEBUG
				fprintf(stderr, "Encoder SeqNr: %d\n", seqNr);
#endif
		
				if (seqNr % do_sync_seq == 0)
					if (do_read_complete(server, &newSeq, sizeof(newSeq)) != sizeof(newSeq))
						goto out;

				seqNr++;
				if (!do_write_op(server, &op))
					goto out;
                        }
                }


	}

out:
	/* Encoder shutdown */
        vorbis_block_clear(&vb);
        vorbis_dsp_clear(&vd);
        vorbis_info_clear(&vi);

	return 0;
}

#if 0
int do_write_samples(int to, short* sbuf, int frame_size, int bits)
{
	unsigned char buf[2*MAX_FRAME_SIZE+1];
	short *s;
	int i;	
	size_t erg, len=0, count=frame_size*(bits/8);
	
	s=(short*)buf;
	
	/* FIXME: Unsupported */
	if (bits == 8)
	{
		fprintf(stderr, "8 Bits unsupported for now.");
		return 0;
	}

	/* FIXME: Endian? */
	for (i=0;i<frame_size;i++)
		s[i]=(short)sbuf[i];

	if (frame_size > 2*MAX_FRAME_SIZE)
	{
		fprintf(stderr, "Error: frame_size too big!");
		exit(1);
	}
	
	do
	{
		erg=write(to, buf+len, count-len);
		if (erg <= 0)
			return len;
		len+=erg;
	}
	while (len < count);

	/* Now do the conversion */

	return frame_size;
}
#endif

ogg_int16_t convbuffer[4096]; /* take 8k out of the data segment, not the stack */
int convsize=4096;

int do_decode(int client, int server, esd_format_t format, int speed, char* ident)
{
	/* Decoder specific variables */
	ogg_packet       op; /* one raw packet of data for decode */
	vorbis_info      vi; /* struct that stores all the static vorbis bitstream settings */
	vorbis_comment   vc; /* struct that stores all the bitstream user comments */
	vorbis_dsp_state vd; /* central working state for the packet->PCM decoder */
	vorbis_block     vb; /* local working space for packet->PCM decode */
	unsigned int do_sync_seq=DEFAULT_DO_SYNC_SEQ;
	
	/* Configuration variables */
	/* FIXME: Depend on NX setting and read from network */
	
	/* Encoder initialisation */
	vorbis_info_init(&vi);
        vorbis_comment_init(&vc);
	
	/* Read all three headers */
	{
		ogg_packet header_main;
		ogg_packet header_comments;
		ogg_packet header_codebooks;

		/* Build the packets */
		
		if (!do_read_op(client, &header_main))
			goto out;
		if (!do_read_op(client, &header_comments))
			goto out;
		if (!do_read_op(client, &header_codebooks))
			goto out;
		vorbis_synthesis_headerin(&vi,&vc,&header_main);
		vorbis_synthesis_headerin(&vi,&vc,&header_comments);
		vorbis_synthesis_headerin(&vi,&vc,&header_codebooks);
		if (header_main.packet)
			free(header_main.packet);
		if (header_comments.packet)
			free(header_comments.packet);
		if (header_codebooks.packet)
			free(header_codebooks.packet);
	}

	/* Throw the comments plus a few lines about the bitstream we're
	*        decoding */
	{
		char **ptr=vc.user_comments;
		while(*ptr){
			fprintf(stderr,"%s\n",*ptr);
			++ptr;
		}
		fprintf(stderr,"\nBitstream is %d channel, %ldHz\n",vi.channels,vi.rate);
		fprintf(stderr,"Encoded by: %s\n\n",vc.vendor);
	}

	convsize=4096/vi.channels;

	vorbis_synthesis_init(&vd,&vi); /* central decode state */
	vorbis_block_init(&vd,&vb);     

	/* Lower the latency a bit */
	//do_sockopts(client, 200);
	do_sockopts(server, vi.channels * convsize * (((format & ESD_BITS16)?16:8) / 8));
	//esd_set_socket_buffers(server, format, speed, 44100);
	//
	
	/* Get do_sync_seq from encoder */
	if (do_read_complete(client, &do_sync_seq, sizeof(do_sync_seq)) != sizeof(do_sync_seq))
		goto out;

	/* Main decoding loop */

	while (1)
	{
		float **pcm;
		int samples;
		unsigned int seqNr = 0;
		
		if (do_read_complete(client, &seqNr, sizeof(seqNr)) != sizeof(seqNr))
			break;
#ifdef DEBUG	
		fprintf(stderr, "SeqNr: %d\n", seqNr);
#endif

		if (seqNr % do_sync_seq == 0)
			if (write(client, &seqNr, sizeof(seqNr)) != sizeof(seqNr))
				break;
		
		if (!do_read_op(client, &op))
			break;

		if(vorbis_synthesis(&vb,&op)==0) /* test for success! */
			vorbis_synthesis_blockin(&vd,&vb);

		if (op.packet)
			free(op.packet);

		/*
		
		**pcm is a multichannel float vector.  In stereo, for
		example, pcm[0] is left, and pcm[1] is right.  samples is
		the size of each channel.  Convert the float values
		(-1.<=range<=1.) to whatever PCM format and write it out */

		while((samples=vorbis_synthesis_pcmout(&vd,&pcm))>0)
		{
			int j, i;
			int clipflag=0;
			int bout=(samples<convsize?samples:convsize);

			/* convert floats to 16 bit signed ints (host order) and interleave */
			for(i=0;i<vi.channels;i++)
			{
				ogg_int16_t *ptr=convbuffer+i;
				float  *mono=pcm[i];
				for(j=0;j<bout;j++)
				{
#if 1
					int val=mono[j]*32767.f;
#else					/* optional dither */
					int val=mono[j]*32767.f+drand48()-0.5f;
#endif
					/* might as well guard against clipping */
					if(val>32767)
					{
						val=32767;
						clipflag=1;
					}
					if(val<-32768)
					{
						val=-32768;
						clipflag=1;
					}
					*ptr=val;
					ptr+=vi.channels;
				}
			}

			if(clipflag)
				fprintf(stderr,"Clipping in frame %ld\n",(long)(vd.sequence));
			
			write(server, convbuffer,2*vi.channels*bout);

			vorbis_synthesis_read(&vd,bout); /* tell libvorbis how many samples we actually consumed */
		}
	}

out:
	/* Decoder shutdown */
	vorbis_block_clear(&vb);
	vorbis_dsp_clear(&vd);
	vorbis_comment_clear(&vc);
	
	vorbis_info_clear(&vi);  /* must be called last */

	return 0;
}

int do_child(int client, int server, int encode)
{
	esd_format_t format;
	int speed;
	char ident[ESD_NAME_MAX+1];

	read(client, &format, sizeof(format));
	read(client, &speed, sizeof(speed));
	read(client, ident, ESD_NAME_MAX);

	write(server, &format, sizeof(format));
	write(server, &speed, sizeof(speed));
	write(server, ident, ESD_NAME_MAX);

	if (encode)
		return do_encode(client, server, format, speed, ident);
	else
		return do_decode(client, server, format, speed, ident);
	
	/* Should never get here */
	
	return -1;
}

int main(int argc, char** argv)
{
	char buf[255];
	int reply;
	int proto;
	
	int client=6;
	int server=7;
	

        do_fwd(client, server, buf, ESD_KEY_LEN);
        do_fwd(client, server, buf, sizeof(int));

	do_fwd(server, client, &reply, sizeof(reply));
	//reply=1;
	//write(client, &reply, sizeof(reply));

	/* Server will close connection anyway */
	if (reply != 1)
		exit(1);

	while (read(client, &proto, sizeof(proto)) == sizeof(proto))
	{
		write(server, &proto, sizeof(proto));

#ifdef DEBUG
		fprintf(stderr, "proto = %d\n", proto);
#endif

	 	switch(proto)
		{
			case ESD_PROTO_SERVER_INFO:
				do_fwd(client, server, buf, sizeof(int));
				do_fwd(server, client, buf, sizeof(int)+sizeof(esd_format_t)+sizeof(int));
				break;
			case ESD_PROTO_LATENCY:
				do_fwd(server, client, buf, sizeof(int));
				break;
			case ESD_PROTO_STREAM_PLAY:
				do_child(client, server, (argc > 1 && argv[1][0] == 'e'));
				exit(0);
				break;
			default:
				close(client);
				close(server);
				exit(1);
		}
	}

	exit(1);
}
