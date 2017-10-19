#ifndef CHK_H
#define CHK_H

#include <stdint.h>

namespace NetworkCoding
{
class Checksum
{
  public:
    static uint8_t get(const uint8_t *const buffer, const uint16_t size);
};
}
#endif
