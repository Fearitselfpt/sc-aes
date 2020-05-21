
//	************************************************************
//	*  aes11.cpp : program de implementare a algoritmului aes  *
//  *  Created by Ricardo Belinha             20/03/2020       *
//	************************************************************

#include "aes.h"
#include "string.h"
#include <stdio.h>


//	====================================================================

//	subrutine

//	--------------------------------------------------------------------

uint32 SubWord(uint32 T) {
    // returns an unsigned int as result of applying the sandbox to T
    return ((SBOX[(uint8) (T >> 24)] << 24)^(SBOX[(uint8) (T >> 16)] << 16)^(SBOX[(uint8) (T >> 8)] << 8)^(SBOX[(uint8) (T)]));
}

//	--------------------------------------------------------------------

uint32 RotWord(uint32 T) {
    // returns the rotated unsigned int T
    return (T << 8) | (T >> 24);
}

//	--------------------------------------------------------------------

void KeyExpansion(uint8 key[], uint32 W[], int NK) {
    // fill out with NK keys the array W[] starting with the key key[]
    uint32 temp;
    int i;
    for (i = 0; i < NK; i++) {
        GET_UINT32(W[i], key, 4 * i);
    }
    for (i = NK; i < NB * (NR + 1); i++) {
        temp = W[i - 1];
        if ((i % NK) == 0) {
            //temp=SubWord(RotWord(temp))^RCON[i/NK];
            temp = SubWord(RotWord(temp))^RCON[(i - 4) / NK];
        } else if ((NK > 6) && (i % NK == 4)) {
            temp = SubWord(temp);
        }
        W[i] = W[i - NK]^temp;
    }
}

//	--------------------------------------------------------------------

uint8 xtime(uint8 aa) {
    // returns the result of multiplying the byte aa by x
    //multiply the byte aa by x = 1.x = 00000010 = 2
    //if the most sign bit in aa is 1 (x^7)
    //we get x^2 = x^4 + x^3 + x + 1 = 0x1b
    if (aa & 10000000)
        return (aa << 1)^0x1b;
    else
        return (aa << 1);
}

//	--------------------------------------------------------------------

void init_xtim() {
    // fill out the array XTIME[256][8]
    for (int j = 0; j < 256; j++) {
        XTIM[j][0] = j;
    }
    for (int k = 1; k < 8; k++)
        for (int j = 0; j < 256; j++) {
            XTIM[j][k] = xtime(XTIM[j][k - 1]);
        }
}

//	--------------------------------------------------------------------

uint8 dot(uint8 aa, uint8 bb) {
    // returns the result of the dot operation aa * bb
    int index = 1;
    uint8 cc = 0;
    for (int j = 0; j < 8; j++) {
        if (index & bb)
            cc = cc^XTIM[aa][j];
        index *= 2;
    }
    return cc;
}

//	--------------------------------------------------------------------

void init_state(uint8 in[], uint8 stat[]) {
    // initializes stat[] with the values from in[]
    for (int i = 0; i < NB * 4; i++)
        stat[i] = in[i];
}

//	--------------------------------------------------------------------

void display_b(uint8 stat[]) {
    for (int j = 0; j < NB * 4; j++) {
        printf("%3x", stat[j]);
    }
    printf("\n");
    printf("***********************************************\n");
}

//	--------------------------------------------------------------------

void AddRoundKey(uint8 stat[], uint32 W[], int round) {
    // add the key from W[] for the round round in stat[]
    uint8 TEMP[4];
    for (int j = 0; j < NB; j++) {
        PUT_UINT32(W[ NB * round + j], TEMP, 0);
        for (int k = 0; k < 4; k++)
            stat[j * 4 + k] = stat[j * 4 + k]^TEMP[k];
    }
}

//	--------------------------------------------------------------------

void SubBytes(uint8 stat[]) {
    // aply the sandbox
    for (int j = 0; j < NB * 4; j++)
        stat[j] = SBOX[stat[j]];
}

//	--------------------------------------------------------------------

void ShiftRows(uint8 stat[]) {
    // shift the lines in stat[] according to the AES specification
    //stat looks like
    //0 4 8 12
    //1 5 9 13
    //2 6 10 14
    //3 7 11 15

    uint8 TEMP;

    TEMP = stat[1];
    stat[1] = stat[5];
    stat[5] = stat[9];
    stat[9] = stat[13];
    stat[13] = TEMP;

    TEMP = stat[2];
    stat[2] = stat[10];
    stat[10] = TEMP;
    TEMP = stat[6];
    stat[6] = stat[14];
    stat[14] = TEMP;

    TEMP = stat[15];
    stat[15] = stat[11];
    stat[11] = stat[7];
    stat[7] = stat[3];
    stat[3] = TEMP;
}

//	--------------------------------------------------------------------

void MixColumns(uint8 stat[]) {
    // mix the columns in stat[] according to the AES specification
    //multiply matrix  02 03 01 01
    // 01 02 03 01
    // 01 01 02 03
    // 03 01 01 02
    //with stat
    int j;
    uint8 temp[NB * 4];
    for (j = 0; j < NB * 4; j++)
        temp[j] = stat[j];
    for (j = 0; j < NB; j++) {
        stat[j * 4 + 0] = dot(0x02, temp[j * 4]) ^ dot(0x03, temp[j * 4 + 1]) ^ temp[j * 4 + 2] ^ temp[j * 4 + 3];
        stat[j * 4 + 1] = temp[j * 4] ^ dot(0x02, temp[j * 4 + 1]) ^ dot(0x03, temp[j * 4 + 2]) ^ temp[j * 4 + 3];
        stat[j * 4 + 2] = temp[j * 4] ^ temp[j * 4 + 1] ^ dot(0x02, temp[j * 4 + 2]) ^ dot(0x03, temp[j * 4 + 3]);
        stat[j * 4 + 3] = dot(0x03, temp[j * 4]) ^ temp[j * 4 + 1] ^ temp[j * 4 + 2] ^ dot(0x02, temp[j * 4 + 3]);
    }
}

//	--------------------------------------------------------------------

void InvSubBytes(uint8 stat[]) {
    // apply the inverse sandbox
    for (int j = 0; j < NB * 4; j++)
        stat[j] = ISBOX[stat[j]];
}

//	--------------------------------------------------------------------

void InvShiftRows(uint8 stat[]) {
    // shift the lines in stat[] according to the AES specification
    //stat looks like
    //0 4 8 12
    //1 5 9 13
    //2 6 10 14
    //3 7 11 15

    uint8 TEMP;

    TEMP = stat[13];
    stat[13] = stat[9];
    stat[9] = stat[5];
    stat[5] = stat[1];
    stat[1] = TEMP;

    TEMP = stat[10];
    stat[10] = stat[2];
    stat[2] = TEMP;
    TEMP = stat[14];
    stat[14] = stat[6];
    stat[6] = TEMP;

    TEMP = stat[3];
    stat[3] = stat[7];
    stat[7] = stat[11];
    stat[11] = stat[15];
    stat[15] = TEMP;
}

//	--------------------------------------------------------------------

void InvMixColumns(uint8 stat[]) {
    // mix the columns in stat[] according to the AES specification
    //multiply matrix  0e 0b 0d 09
    // 09 0e 0b 0d
    // 0d 09 0e 0b
    // 0b 0d 09 0e
    //with stat
    int j;
    uint8 temp[NB * 4];
    for (j = 0; j < NB * 4; j++)
        temp[j] = stat[j];
    for (j = 0; j < NB; j++) {
        stat[j * 4 + 0] = dot(0x0e, temp[j * 4]) ^ dot(0x0b, temp[j * 4 + 1]) ^ dot(0x0d, temp[j * 4 + 2]) ^ dot(0x09, temp[j * 4 + 3]);
        stat[j * 4 + 1] = dot(0x09, temp[j * 4]) ^ dot(0x0e, temp[j * 4 + 1]) ^ dot(0x0b, temp[j * 4 + 2]) ^ dot(0x0d, temp[j * 4 + 3]);
        stat[j * 4 + 2] = dot(0x0d, temp[j * 4]) ^ dot(0x09, temp[j * 4 + 1]) ^ dot(0x0e, temp[j * 4 + 2]) ^ dot(0x0b, temp[j * 4 + 3]);
        stat[j * 4 + 3] = dot(0x0b, temp[j * 4]) ^ dot(0x0d, temp[j * 4 + 1]) ^ dot(0x09, temp[j * 4 + 2]) ^ dot(0x0e, temp[j * 4 + 3]);
    }
}

//	====================================================================

int main(int argc, char* argv[]) {
    	int i, NK[3];
	uint8 state[NB*4];
	uint32 WW[NB * (1 + MAX_NUM_ROUNDS)];

	//	----------------------------------------------------------------
	//	init phase
	//	----------------------------------------------------------------

	static uint8 key4[16] =
	{
		0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6,
		0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c
	};

	static uint8 key6[24] =
	{
		0x8e, 0x73, 0xb0, 0xf7, 0xda, 0x0e, 0x64, 0x52,
		0xc8, 0x10, 0xf3, 0x2b, 0x80, 0x90, 0x79, 0xe5,
		0x62, 0xf8, 0xea, 0xd2, 0x52, 0x2c, 0x6b, 0x7b
	};

	static uint8 key8[32] =
	{
		0x60, 0x3d, 0xeb, 0x10, 0x15, 0xca, 0x71, 0xbe,
		0x2b, 0x73, 0xae, 0xf0, 0x85, 0x7d, 0x77, 0x81,
		0x1f, 0x35, 0x2c, 0x07, 0x3b, 0x61, 0x08, 0xd7,
		0x2d, 0x98, 0x10, 0xa3, 0x09, 0x14, 0xdf, 0xf4
	};

	init_xtim();

	//	----------------------------------------------------------------

	//	APPENDIX A - key schedules for 128, 192, and 256 bit keys

	for (int ii=2; ii>=0; --ii)
	{
		NK[ii] = 4 + ii*2;
		NR = NK[ii] + 6;

		if (ii==0)
			KeyExpansion(key4, WW, NK[ii]);
		else if (ii==1)
			KeyExpansion(key6, WW, NK[ii]);
		else
			KeyExpansion(key8, WW, NK[ii]);
	}

	//	----------------------------------------------------------------

	//	APPENDIX B - cipher phase for test_appb - 128 bits key from key4

	NR = NK[0] + 6;
	init_state(test_appb, state);
	display_b(state);

	AddRoundKey(state, WW, 0);	// WW contains the key schedule for key4
	printf("addroundKey: \n");
	display_b(state);

	for (i=1; i<NR; i++)
	{
		SubBytes(state);
		printf("subBytes:\n");
		display_b(state);
		ShiftRows(state);
		printf("shiftRows: \n");
		display_b(state);
		MixColumns(state);
		printf("MixColumns: \n");
		display_b(state);
		AddRoundKey(state, WW, i);
		printf("addroundKey: \n");
		display_b(state);
	}

	SubBytes(state);
	printf("subBytes:\n");
	display_b(state);
	ShiftRows(state);
	printf("shiftRows: \n");
	display_b(state);
	AddRoundKey(state, WW, NR);

	printf("cipher phase for test_appb:\n");
	display_b(state);
	getchar();

	//	----------------------------------------------------------------

	//	APPENDIX C - cipher, decipher phase for test_appc using all keys

	for (int ii=0; ii<3; ii++)
	{
		NR = NK[ii] + 6;
		KeyExpansion(keys_appc, WW, NK[ii]);

		////////////////////////////////////////////////////////////
		//	cipher phase - like APPENDIX B, from init_state() to the end

		if(NR==10)
			printf("AES-128\n\n");
		else if(NR==12)
			printf("AES-192\n\n");
		else printf("AES-256\n\n");

		printf("cipher phase\n" );
		init_state(test_appc, ciph_appc[ii]);
		display_b(ciph_appc[ii]);

		AddRoundKey(ciph_appc[ii], WW, 0);	// WW contains the key schedule for key4
		printf("addroundKey: \n");
		display_b(ciph_appc[ii]);

		for (i=1; i<NR; i++)
		{
			SubBytes(ciph_appc[ii]);
			printf("subBytes:\n");
			display_b(ciph_appc[ii]);

			ShiftRows(ciph_appc[ii]);
			printf("shiftRows: \n");
			display_b(ciph_appc[ii]);

			MixColumns(ciph_appc[ii]);
			printf("MixColumns: \n");
			display_b(ciph_appc[ii]);

			AddRoundKey(ciph_appc[ii], WW, i);
			printf("addroundKey: \n");
			display_b(ciph_appc[ii]);
		}

		SubBytes(ciph_appc[ii]);
		printf("subBytes:\n");
		display_b(ciph_appc[ii]);
		ShiftRows(ciph_appc[ii]);
		printf("shiftRows: \n");
		display_b(ciph_appc[ii]);
		AddRoundKey(ciph_appc[ii], WW, NR);

		printf("output cipher phase:\n");
		display_b(ciph_appc[ii]);
		getchar();

		////////////////////////////////////////////////////////
		//	decipher phase (inverse cipher only) - similar to APPENDIX B
		printf("decipher phase\n");
		init_state(ciph_appc[ii],decipher[ii]);
		display_b(decipher[ii]);

		AddRoundKey(decipher[ii], WW, NR);	// WW contains the key schedule for key4
		printf("addroundKey: \n");
		display_b(decipher[ii]);

		for (i=NR-1; i>=1; i--)
		{
			InvShiftRows(decipher[ii]);
			printf("invShiftRows: \n");
			display_b(decipher[ii]);

			InvSubBytes(decipher[ii]);
			printf("invSubBytes:\n");
			display_b(decipher[ii]);

			AddRoundKey(decipher[ii], WW, i);
			printf("addroundKey: \n");
			display_b(decipher[ii]);

			InvMixColumns(decipher[ii]);
			printf("invMixColumns: \n");
			display_b(decipher[ii]);

		}

		InvShiftRows(decipher[ii]);
		printf("invSiftRows: \n");
		display_b(decipher[ii]);

		InvSubBytes(decipher[ii]);
		printf("invSubBytes:\n");
		display_b(decipher[ii]);

		AddRoundKey(decipher[ii], WW, 0);
		printf("output decipher phase:\n");
		display_b(decipher[ii]);
		getchar();

	}

	return 0;
}

//	********************************************************************
