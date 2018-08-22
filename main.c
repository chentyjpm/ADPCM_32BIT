#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "adpcm.h"

#define MAXSIZE 44100*60

int size = MAXSIZE;

int main(int argc, char **argv)
{
    FILE *infile, *outfile;

    adpcm_state_t enc_state;
    adpcm_state_t dec_state;

    enc_state.index = 0;
    enc_state.valprev = 0;
    dec_state.index = 0;
    dec_state.valprev = 0;

    int32_t *srcpcm;
    int8_t *adpcmout;
    int32_t *decpcmout;

    srcpcm = malloc(MAXSIZE*4);
    adpcmout = malloc(MAXSIZE*4);
    decpcmout = malloc(MAXSIZE*4);


	infile = fopen(argv[1], "rb");
	if(infile == NULL){
		perror(argv[1]);
		exit(1);
	}

	size = fread(srcpcm, 4, size, infile);

	if(size > MAXSIZE)
        size = MAXSIZE;

    printf("DATSIE %d \r\n",size);

    adpcm_coder(srcpcm, adpcmout, size, &enc_state);

    adpcm_decoder(adpcmout, decpcmout, size, &dec_state);

    printf("ENCDAT ");
    for (int i = 0 ; i < 50; i++)
        printf("%02x ", adpcmout[i]);
    printf("\n\r");

    printf("DECDAT ");
    for (int i = 0 ; i < 50; i++)
        printf("%08x ", decpcmout[i]);
    printf("\n\r");

    printf("SRCDAT ");
    for (int i = 0 ; i < 50; i++)
        printf("%08x ", srcpcm[i]);
    printf("\n\r");

    printf("Finish\n");

     printf("SAVE\n");
	outfile = fopen("out.snd", "wb");
	if(outfile == NULL){
        printf("OFILE ERR\n");
		exit(1);
	}

	fwrite(decpcmout, 4, size, outfile);

	free(srcpcm);
    free(adpcmout);
    free(decpcmout);

	fclose(outfile);
    fclose(infile);

    return 0;
}
