#include "finite_field.h"
#include "common.h"

using namespace NetworkCoding;

NetworkCoding::FiniteField *NetworkCoding::FiniteField::_instance = new NetworkCoding::FiniteField;

void FiniteField::init()
{
    try
    {
        _mul_table = new byte *[256];
    }
    catch (std::exception &ex)
    {
        exit(EXIT_FAILURE);
    }

    try
    {
        _inv_table = new byte[256];
    }
    catch (std::exception &ex)
    {
        exit(EXIT_FAILURE);
    }

    for (unsigned int i = 0; i < 256; i++)
    {
        try
        {
            _mul_table[i] = new byte[256 - i];
        }
        catch (std::exception &ex)
        {
            exit(EXIT_FAILURE);
        }

        for (unsigned int j = i; j < 256; j++)
        {
            _mul_table[i][j - i] = _mul(i, j);
            if (_mul_table[i][j - i] == 1)
            {
                _inv_table[i] = j;
                _inv_table[j] = i;
            }
        }
    }
}

byte FiniteField::_mul(byte a, byte b)
{
    byte p = 0; /* the product of the multiplication */
    while (b)
    {
        if (b & 1)  /* if b is odd, then add the corresponding a to p (final product = sum of all a's corresponding to odd b's) */
            p ^= a; /* since we're in GF(2^m), addition is an XOR */

        if (a & 0x80)             /* GF modulo: if a >= 128, then it will overflow when shifted left, so reduce */
            a = (a << 1) ^ 0x11b; /* XOR with the primitive polynomial x^8 + x^4 + x^3 + x + 1 -- you can change it but it must be irreducible */
        else
            a <<= 1; /* equivalent to a*2 */
        b >>= 1;     /* equivalent to b // 2 */
    }
    return p;
}

byte FiniteField::add(byte a, byte b)
{
    return a ^ b;
}

byte FiniteField::sub(byte a, byte b)
{
    return a ^ b;
}
#ifndef INLINE_MUL_INV
byte FiniteField::mul(byte a, byte b)
{
    return _mul_table[a < b ? a : b][a < b ? b - a : a - b];
}

byte FiniteField::inv(byte a)
{
    return _inv_table[a];
}
#endif
