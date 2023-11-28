/*
 * saes_1900630.c
 *
 *  Created on: Nov 20, 2023
 *      Author: hp
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define Decrypt 0
#define Encrypt 1

int k[3] = {0, 0, 0};

//RotNib() is “rotate the nibbles”, which is equivalent to swapping the nibbles)
//EX:1000 1111 ----> 1111 1000
int RotNib(int wx)
{
	int swapped_number  = ((wx & 0x0F) << 4) | ((wx & 0xF0) >> 4);

	return swapped_number;
}

//SubNib8() is “apply S-Box substitution on nibbles using encryption S-Box”
int SubNib8(int input, unsigned char flag)
{
	int left, right, output;
	unsigned char row, col;
	int s_box[4][4] = {
			{0x9, 0x4, 0xA, 0xB},
			{0xD, 0x1, 0x8, 0x5},
			{0x6, 0x2, 0x0, 0x3},
			{0xC, 0xE, 0xF, 0x7}
	};

	int inverse_sBox[4][4] = {
			{0xA, 0x5, 0x9, 0xB},
			{0x1, 0x7, 0x8, 0xF},
			{0x6, 0x0, 0x2, 0x3},
			{0xC, 0x4, 0xD, 0xE}
	};

	left = (input & 0xF0) >> 4;
	right = (input & 0x0F);


	//for the MS 4-bits Ex:1011
	row = (left & 0xC) >> 2;   // (upper nibble = 10)
	col = (left & 0x3);        //(lower nibble = 11)

	if(flag == Encrypt)
	{
		left = s_box[row][col];
	}
	else if(flag == Decrypt)
	{
		left = inverse_sBox[row][col];
	}

	//for the LS 4-bits Ex:1011
	row = (right & 0xC) >> 2;   // (upper nibble = 10)
	col = (right & 0x3);        //(lower nibble = 11)

	if(flag == Encrypt)
	{
		right = s_box[row][col];
	}
	else if(flag == Decrypt)
	{
		right = inverse_sBox[row][col];
	}

	// output after substitution
	output = (left << 4) | right;

	return output;
}

//SubNib16() is “apply S-Box substitution on nibbles using encryption S-Box” for
//16 bits input
int SubNib16(int input, unsigned char flag)
{
	int left, right, output;

	left = (input & 0xFF00) >> 8;
	right = (input & 0x00FF);

	left = SubNib8(left, flag);
	right = SubNib8(right, flag);

	// output after substitution
	output = (left << 8) | right;

	return output;
}


// Function that generate the key
void keyGenerator(int key)
{
	int w[5];

	w[0] = (int)(key >> 8);                          //this represent w0
	w[1] = key & 0xFF;                               //this represent w1
	w[2] = w[0] ^ 0b10000000 ^ SubNib8(RotNib(w[1]), Encrypt); //this represent w2
	w[3] = w[2] ^ w[1];                              //this represent w3
	w[4] = w[2] ^ 0b00110000 ^ SubNib8(RotNib(w[3]), Encrypt);
	w[5] = w[4] ^ w[3];


	k[0] = (w[0] << 8) | w[1];                       //this represent k0
	k[1] = (w[2] << 8) | w[3];                       //this represent k1
	k[2] = (w[4] << 8) | w[5];                       //this represent k2

}

//Shift Row: Swap 2nd nibble and 4th nibble
int shiftRow(int input)
{
	int output, secondNib, fourthNib;

	secondNib = (input & 0x0F00) >> 8;
	fourthNib = (input & 0x000F);
	input = input & 0xF0F0;

	output = input |( (fourthNib << 8) & 0x0F00)| secondNib;

	return output;
}

// Function to perform multiplication in GF(2^4)
char mult_GF2_4(char a, char b)
{
	char result = 0;

	for (int i = 0; i < 4; ++i)
	{
		if (b & 1)
		{
			result ^= a; // XOR operation
		}

		int highBitSet = a & 0b1000;
		a <<= 1;

		if (highBitSet)
		{
			a ^= 0b10011;
		}

		b >>= 1;
	}

	return result;
}

/* Apply the matrix multiplication with the constant matrix, Me, using GF(24).
 * For GF(24), the addition operation is simply an XOR, and for the
 * multiplication operation you can use a lookup table.
 */
int mixCol(int input, unsigned char flag)
{
	int output;

	int M1[2][2]={
			{1, 4},
			{4, 1}
	};

	int M2[2][2]={
			{9, 2},
			{2, 9}
	};

	//converting input into a 2X2 matrix
	int S1[2][2] = {
			{((input & 0xF000) >> 12), ((input & 0x00F0) >> 4)},
			{((input & 0x0F00) >> 8), (input & 0x000F)        }
	};

	int S2[2][2];

	for (int i = 0; i < 2; ++i)
	{
		for (int j = 0; j < 2; ++j)
		{
			S2[i][j] = 0;

			for (int k = 0; k < 2; ++k)
			{
				if(flag == Encrypt)
				{
					S2[i][j] ^= mult_GF2_4(M1[i][k], S1[k][j]);
				}
				else if(flag == Decrypt)
				{
					S2[i][j] ^= mult_GF2_4(M2[i][k], S1[k][j]);
				}
			}
		}
	}

	output = (S2[0][0] << 12) | (S2[1][0] << 8) | (S2[0][1] << 4) | S2[1][1];

	return output;
}

// Function to convert a hexadecimal string to binary and return as unsigned short
int hexStringToBinary(const char* hexString) {
	int binaryResult = 0;

	// Iterate through each character in the string
	for (int i = 0; hexString[i] != '\0'; ++i) {
		binaryResult <<= 4; // Left shift the current value by 4 bits

		// Convert hexadecimal character to integer
		if (hexString[i] >= '0' && hexString[i] <= '9') {
			binaryResult += hexString[i] - '0';
		} else if (hexString[i] >= 'A' && hexString[i] <= 'F') {
			binaryResult += hexString[i] - 'A' + 10;
		} else if (hexString[i] >= 'a' && hexString[i] <= 'f') {
			binaryResult += hexString[i] - 'a' + 10;
		} else {
			// Handle invalid characters in the string
			fprintf(stderr, "Invalid hexadecimal character: %c\n", hexString[i]);
			return 0; // or any other error handling strategy
		}
	}

	return binaryResult;
}
/******************************Encryption**************************************/
int AddRoundKey(int plaintext, int keyNo)
{
	return plaintext ^ k[keyNo];
}

int Round_1(int input)
{
	int output;

	/*Nibble Substitution (S-boxes). Each nibble in the input is used in the
	 *Encryption S-Box to generate an output nibble. */
	output = SubNib16(input, Encrypt);
	output = shiftRow(output);
	output = mixCol(output, Encrypt);
	output = AddRoundKey(output, 1);

	return output;
}

int Final_Round(int input)
{
	int output;
	output = SubNib16(input, Encrypt);
	output = shiftRow(output);
	output = AddRoundKey(output, 2);

	return output;
}

int ENC(int key, int P)
{
	keyGenerator(key);
	return Final_Round(Round_1(AddRoundKey(P, 0)));
}
/******************************Decryption**************************************/

int DEC(int key, int C)
{

	int output;
	keyGenerator(key);
	output = AddRoundKey(C, 2);
	output = shiftRow(output);
	output = SubNib16(output, Decrypt);
	output = AddRoundKey(output, 1);
	output = mixCol(output, Decrypt);
	output = shiftRow(output);
	output = SubNib16(output, Decrypt);
	output = AddRoundKey(output, 0);

	return output;
}

/******************************************************************************/


int main(int argc, char* argv [])
{
	char* op = argv[1];
	char* keey = argv[2];
	char* Data = argv[3];

	unsigned short text = hexStringToBinary(Data);
	unsigned short key = hexStringToBinary(keey);

	if (strcmp(op, "ENC") == 0)
	{
		unsigned short cipher = ENC(key, text);
		printf("0x%04X\n", cipher);
	}

	if(strcmp(op, "DEC") == 0)
	{
		unsigned short plaintext = DEC(key, text);
		printf("0x%04X\n", plaintext);
	}

	return 0;
}

