#include "chk.h"

using namespace NetworkCoding;

uint8_t Checksum::get(const uint8_t *const buffer, const uint16_t size)
{
    uint16_t tmp = 0;
    uint16_t pos = 0;
    while (size - pos > 0)
    {
        if (size - pos > 128)
        {
            tmp += buffer[pos + 0];
            tmp += buffer[pos + 1];
            tmp += buffer[pos + 2];
            tmp += buffer[pos + 3];
            tmp += buffer[pos + 4];
            tmp += buffer[pos + 5];
            tmp += buffer[pos + 6];
            tmp += buffer[pos + 7];
            tmp += buffer[pos + 8];
            tmp += buffer[pos + 9];
            tmp += buffer[pos + 10];
            tmp += buffer[pos + 11];
            tmp += buffer[pos + 12];
            tmp += buffer[pos + 13];
            tmp += buffer[pos + 14];
            tmp += buffer[pos + 15];
            tmp += buffer[pos + 16];
            tmp += buffer[pos + 17];
            tmp += buffer[pos + 18];
            tmp += buffer[pos + 19];
            tmp += buffer[pos + 20];
            tmp += buffer[pos + 21];
            tmp += buffer[pos + 22];
            tmp += buffer[pos + 23];
            tmp += buffer[pos + 24];
            tmp += buffer[pos + 25];
            tmp += buffer[pos + 26];
            tmp += buffer[pos + 27];
            tmp += buffer[pos + 28];
            tmp += buffer[pos + 29];
            tmp += buffer[pos + 30];
            tmp += buffer[pos + 31];
            tmp += buffer[pos + 32];
            tmp += buffer[pos + 33];
            tmp += buffer[pos + 34];
            tmp += buffer[pos + 35];
            tmp += buffer[pos + 36];
            tmp += buffer[pos + 37];
            tmp += buffer[pos + 38];
            tmp += buffer[pos + 39];
            tmp += buffer[pos + 40];
            tmp += buffer[pos + 41];
            tmp += buffer[pos + 42];
            tmp += buffer[pos + 43];
            tmp += buffer[pos + 44];
            tmp += buffer[pos + 45];
            tmp += buffer[pos + 46];
            tmp += buffer[pos + 47];
            tmp += buffer[pos + 48];
            tmp += buffer[pos + 49];
            tmp += buffer[pos + 50];
            tmp += buffer[pos + 51];
            tmp += buffer[pos + 52];
            tmp += buffer[pos + 53];
            tmp += buffer[pos + 54];
            tmp += buffer[pos + 55];
            tmp += buffer[pos + 56];
            tmp += buffer[pos + 57];
            tmp += buffer[pos + 58];
            tmp += buffer[pos + 59];
            tmp += buffer[pos + 60];
            tmp += buffer[pos + 61];
            tmp += buffer[pos + 62];
            tmp += buffer[pos + 63];
            tmp += buffer[pos + 64];
            tmp += buffer[pos + 65];
            tmp += buffer[pos + 66];
            tmp += buffer[pos + 67];
            tmp += buffer[pos + 68];
            tmp += buffer[pos + 69];
            tmp += buffer[pos + 70];
            tmp += buffer[pos + 71];
            tmp += buffer[pos + 72];
            tmp += buffer[pos + 73];
            tmp += buffer[pos + 74];
            tmp += buffer[pos + 75];
            tmp += buffer[pos + 76];
            tmp += buffer[pos + 77];
            tmp += buffer[pos + 78];
            tmp += buffer[pos + 79];
            tmp += buffer[pos + 80];
            tmp += buffer[pos + 81];
            tmp += buffer[pos + 82];
            tmp += buffer[pos + 83];
            tmp += buffer[pos + 84];
            tmp += buffer[pos + 85];
            tmp += buffer[pos + 86];
            tmp += buffer[pos + 87];
            tmp += buffer[pos + 88];
            tmp += buffer[pos + 89];
            tmp += buffer[pos + 90];
            tmp += buffer[pos + 91];
            tmp += buffer[pos + 92];
            tmp += buffer[pos + 93];
            tmp += buffer[pos + 94];
            tmp += buffer[pos + 95];
            tmp += buffer[pos + 96];
            tmp += buffer[pos + 97];
            tmp += buffer[pos + 98];
            tmp += buffer[pos + 99];
            tmp += buffer[pos + 100];
            tmp += buffer[pos + 101];
            tmp += buffer[pos + 102];
            tmp += buffer[pos + 103];
            tmp += buffer[pos + 104];
            tmp += buffer[pos + 105];
            tmp += buffer[pos + 106];
            tmp += buffer[pos + 107];
            tmp += buffer[pos + 108];
            tmp += buffer[pos + 109];
            tmp += buffer[pos + 110];
            tmp += buffer[pos + 111];
            tmp += buffer[pos + 112];
            tmp += buffer[pos + 113];
            tmp += buffer[pos + 114];
            tmp += buffer[pos + 115];
            tmp += buffer[pos + 116];
            tmp += buffer[pos + 117];
            tmp += buffer[pos + 118];
            tmp += buffer[pos + 119];
            tmp += buffer[pos + 120];
            tmp += buffer[pos + 121];
            tmp += buffer[pos + 122];
            tmp += buffer[pos + 123];
            tmp += buffer[pos + 124];
            tmp += buffer[pos + 125];
            tmp += buffer[pos + 126];
            tmp += buffer[pos + 127];
            pos += 128;
        }
        else if (size - pos > 64)
        {
            tmp += buffer[pos + 0];
            tmp += buffer[pos + 1];
            tmp += buffer[pos + 2];
            tmp += buffer[pos + 3];
            tmp += buffer[pos + 4];
            tmp += buffer[pos + 5];
            tmp += buffer[pos + 6];
            tmp += buffer[pos + 7];
            tmp += buffer[pos + 8];
            tmp += buffer[pos + 9];
            tmp += buffer[pos + 10];
            tmp += buffer[pos + 11];
            tmp += buffer[pos + 12];
            tmp += buffer[pos + 13];
            tmp += buffer[pos + 14];
            tmp += buffer[pos + 15];
            tmp += buffer[pos + 16];
            tmp += buffer[pos + 17];
            tmp += buffer[pos + 18];
            tmp += buffer[pos + 19];
            tmp += buffer[pos + 20];
            tmp += buffer[pos + 21];
            tmp += buffer[pos + 22];
            tmp += buffer[pos + 23];
            tmp += buffer[pos + 24];
            tmp += buffer[pos + 25];
            tmp += buffer[pos + 26];
            tmp += buffer[pos + 27];
            tmp += buffer[pos + 28];
            tmp += buffer[pos + 29];
            tmp += buffer[pos + 30];
            tmp += buffer[pos + 31];
            tmp += buffer[pos + 32];
            tmp += buffer[pos + 33];
            tmp += buffer[pos + 34];
            tmp += buffer[pos + 35];
            tmp += buffer[pos + 36];
            tmp += buffer[pos + 37];
            tmp += buffer[pos + 38];
            tmp += buffer[pos + 39];
            tmp += buffer[pos + 40];
            tmp += buffer[pos + 41];
            tmp += buffer[pos + 42];
            tmp += buffer[pos + 43];
            tmp += buffer[pos + 44];
            tmp += buffer[pos + 45];
            tmp += buffer[pos + 46];
            tmp += buffer[pos + 47];
            tmp += buffer[pos + 48];
            tmp += buffer[pos + 49];
            tmp += buffer[pos + 50];
            tmp += buffer[pos + 51];
            tmp += buffer[pos + 52];
            tmp += buffer[pos + 53];
            tmp += buffer[pos + 54];
            tmp += buffer[pos + 55];
            tmp += buffer[pos + 56];
            tmp += buffer[pos + 57];
            tmp += buffer[pos + 58];
            tmp += buffer[pos + 59];
            tmp += buffer[pos + 60];
            tmp += buffer[pos + 61];
            tmp += buffer[pos + 62];
            tmp += buffer[pos + 63];
            pos += 64;
        }
        else if (size - pos > 32)
        {
            tmp += buffer[pos + 0];
            tmp += buffer[pos + 1];
            tmp += buffer[pos + 2];
            tmp += buffer[pos + 3];
            tmp += buffer[pos + 4];
            tmp += buffer[pos + 5];
            tmp += buffer[pos + 6];
            tmp += buffer[pos + 7];
            tmp += buffer[pos + 8];
            tmp += buffer[pos + 9];
            tmp += buffer[pos + 10];
            tmp += buffer[pos + 11];
            tmp += buffer[pos + 12];
            tmp += buffer[pos + 13];
            tmp += buffer[pos + 14];
            tmp += buffer[pos + 15];
            tmp += buffer[pos + 16];
            tmp += buffer[pos + 17];
            tmp += buffer[pos + 18];
            tmp += buffer[pos + 19];
            tmp += buffer[pos + 20];
            tmp += buffer[pos + 21];
            tmp += buffer[pos + 22];
            tmp += buffer[pos + 23];
            tmp += buffer[pos + 24];
            tmp += buffer[pos + 25];
            tmp += buffer[pos + 26];
            tmp += buffer[pos + 27];
            tmp += buffer[pos + 28];
            tmp += buffer[pos + 29];
            tmp += buffer[pos + 30];
            tmp += buffer[pos + 31];
            pos += 32;
        }
        else if (size - pos > 16)
        {
            tmp += buffer[pos + 0];
            tmp += buffer[pos + 1];
            tmp += buffer[pos + 2];
            tmp += buffer[pos + 3];
            tmp += buffer[pos + 4];
            tmp += buffer[pos + 5];
            tmp += buffer[pos + 6];
            tmp += buffer[pos + 7];
            tmp += buffer[pos + 8];
            tmp += buffer[pos + 9];
            tmp += buffer[pos + 10];
            tmp += buffer[pos + 11];
            tmp += buffer[pos + 12];
            tmp += buffer[pos + 13];
            tmp += buffer[pos + 14];
            tmp += buffer[pos + 15];
            pos += 16;
        }
        else if (size - pos > 8)
        {
            tmp += buffer[pos + 0];
            tmp += buffer[pos + 1];
            tmp += buffer[pos + 2];
            tmp += buffer[pos + 3];
            tmp += buffer[pos + 4];
            tmp += buffer[pos + 5];
            tmp += buffer[pos + 6];
            tmp += buffer[pos + 7];
            pos += 8;
        }
        else
        {
            for (; pos < size; pos++)
            {
                tmp += buffer[pos];
            }
            pos = size;
        }
    }
    tmp = (tmp >> 8) + (tmp & 0xff);
    tmp += (tmp >> 8);

    return (uint8_t)(~tmp);
}