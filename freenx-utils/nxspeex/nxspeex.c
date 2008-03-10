/*
 * nxspeex.c - A nx esd proxy for low bandwidth speex encoded transmission of data.
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

#include <speex/speex.h>
#include <speex/speex_stereo.h>
#include <speex/speex_callbacks.h>
#include <speex/speex_preprocess.h>

#define MAX_FRAME_SIZE 2000
#define MAX_FRAME_BYTES 2000

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


int do_read_samples(int from, short* sbuf, int frame_size, int bits)
{
	unsigned char buf[2*MAX_FRAME_SIZE+1];
	short *s;
	int i;	
	size_t erg, len=0, count=frame_size*(bits/8);

	if (frame_size > 2*MAX_FRAME_SIZE)
	{
		fprintf(stderr, "Error: frame_size too big!");
		exit(1);
	}
	
	do
	{
		erg=read(from, buf+len, count-len);
		if (erg <= 0)
			return len;
		len+=erg;
	}
	while (len < count);

	/* Now do the conversion */

	s=(short*)buf;

	/* FIXME: Unsupported */
	if (bits == 8)
	{
		fprintf(stderr, "8 Bits unsupported for now.");
		return 0;
	}

	/* FIXME: Endian? */
	for (i=0;i<frame_size;i++)
		sbuf[i]=(short)s[i];

	return frame_size;
}

int do_encode(int client, int server, esd_format_t format, int speed, char* ident)
{
	/* Encoder specific variables */
	void *enc_state;
	SpeexBits bits;
	const SpeexMode* mode = NULL;
	SpeexPreprocessState *preprocess = NULL;
	int frame_size, frame_size2;
	unsigned int seqNr = 0;
	unsigned int do_sync_seq=DEFAULT_DO_SYNC_SEQ;
	
	/* Configuration variables */
	/* FIXME: Depend on NX setting */
	int modeID = SPEEX_MODEID_UWB;
	int complexity=3;
	int denoise_enabled=1;
	
	/* Encoder initialisation */
	speex_bits_init(&bits);
	mode = speex_lib_get_mode (modeID);
	enc_state=speex_encoder_init(mode);
	
	speex_encoder_ctl(enc_state, SPEEX_SET_SAMPLING_RATE, &speed);
	speex_encoder_ctl(enc_state, SPEEX_SET_COMPLEXITY, &complexity);
	speex_encoder_ctl(enc_state, SPEEX_GET_FRAME_SIZE, &frame_size);

	frame_size2=frame_size*((format & ESD_STEREO)?2:1);
	
	/* Lower the latency */
	
	//do_sockopts(client, frame_size2 * (((format & ESD_BITS16)?16:8) / 8) * 4);
	esd_set_socket_buffers(client, format, speed, 44100);
	do_sockopts(server, 200);

	if (denoise_enabled)
	{
		preprocess = speex_preprocess_state_init(frame_size, speed);
		speex_preprocess_ctl(preprocess, SPEEX_PREPROCESS_SET_DENOISE, &denoise_enabled);
	}

	/* Send do_sync_seq to decoder */
	if (write(server, &do_sync_seq, sizeof(do_sync_seq)) != sizeof(do_sync_seq))
		goto out;

	/* Main encoding loop */

	while (1)
	{
		int nbBytes;
		unsigned int newSeq;
		short input[MAX_FRAME_SIZE+1];
		char output[MAX_FRAME_BYTES+1];

		if (do_read_samples(client, input, frame_size2, ((format & ESD_BITS16)?16:8)) != frame_size2)
			break;
		
		speex_bits_reset(&bits); 
		if (format & ESD_STEREO)
			speex_encode_stereo_int(input, frame_size, &bits);

		if (preprocess)
			speex_preprocess(preprocess, input, NULL);
			
		speex_encode_int(enc_state, input, &bits);
		nbBytes = speex_bits_write(&bits, output, MAX_FRAME_BYTES);
	
		if (write(server, &seqNr, sizeof(seqNr)) != sizeof(seqNr))
			break;
#ifdef DEBUG
		fprintf(stderr, "Encoder SeqNr: %d\n", seqNr);
#endif
		
		if (seqNr % do_sync_seq == 0)
			if (do_read_complete(server, &newSeq, sizeof(newSeq)) != sizeof(newSeq))
				break;

		seqNr++;
		if (write(server, &nbBytes, sizeof(nbBytes)) != sizeof(nbBytes))
			break;
		if (write(server, output, nbBytes) != nbBytes)
			break;
	}

out:
	/* Encoder shutdown */
	speex_bits_destroy(&bits); 

	speex_encoder_destroy(enc_state);

	return 0;
}

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

int do_decode(int client, int server, esd_format_t format, int speed, char* ident)
{
	/* Decoder specific variables */
	void *dec_state;
	SpeexBits bits;
	const SpeexMode* mode = NULL;
	int frame_size, frame_size2;
	SpeexCallback callback;
	SpeexStereoState stereo = SPEEX_STEREO_STATE_INIT;
	unsigned int do_sync_seq;
	
	/* Configuration variables */
	/* FIXME: Depend on NX setting and read from network */
	int modeID = SPEEX_MODEID_UWB;
	
	/* Encoder initialisation */
	speex_bits_init(&bits);
	mode = speex_lib_get_mode (modeID);
	dec_state=speex_decoder_init(mode);
	
	speex_decoder_ctl(dec_state, SPEEX_SET_SAMPLING_RATE, &speed);

	if (format & ESD_STEREO)
	{
		callback.callback_id = SPEEX_INBAND_STEREO;
		callback.func = speex_std_stereo_request_handler;
		callback.data = &stereo;
		speex_decoder_ctl(dec_state, SPEEX_SET_HANDLER, &callback);
	}
	speex_decoder_ctl(dec_state, SPEEX_GET_FRAME_SIZE, &frame_size);
	
	frame_size2=frame_size*((format & ESD_STEREO)?2:1);

	/* Lower the latency a bit */
	//do_sockopts(client, 200);
	do_sockopts(server, frame_size2 * (((format & ESD_BITS16)?16:8) / 8));
	//esd_set_socket_buffers(server, format, speed, 44100);
	//
	
	/* Get do_sync_seq from encoder */
	if (do_read_complete(client, &do_sync_seq, sizeof(do_sync_seq)) != sizeof(do_sync_seq))
		goto out;

	/* Main decoding loop */

	while (1)
	{
		unsigned int seqNr = 0;
		int nbBytes;
		char input[MAX_FRAME_BYTES+1];
		short output[MAX_FRAME_SIZE+1];
		
		if (do_read_complete(client, &seqNr, sizeof(seqNr)) != sizeof(seqNr))
			break;
#ifdef DEBUG	
		fprintf(stderr, "SeqNr: %d\n", seqNr);
#endif

		if (seqNr % do_sync_seq == 0)
			if (write(client, &seqNr, sizeof(seqNr)) != sizeof(seqNr))
				break;
		
		if (do_read_complete(client, &nbBytes, sizeof(nbBytes)) != sizeof(nbBytes))
			break;
		if (do_read_complete(client, input, nbBytes) != nbBytes)
			break;

		speex_bits_read_from(&bits, input, nbBytes); 
		speex_decode_int(dec_state, &bits, output);
		
		if (format & ESD_STEREO)
			speex_decode_stereo_int(output, frame_size, &stereo);
	
		if (do_write_samples(server, output, frame_size2, ((format & ESD_BITS16)?16:8)) != frame_size2)
			break;
	}

out:
	/* Decoder shutdown */
	speex_bits_destroy(&bits); 

	speex_decoder_destroy(dec_state);

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
