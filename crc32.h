#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned long crc32_t;
#define crc32_startvalue 0xFFFFFFFF
extern crc32_t crc32_update(crc32_t c, unsigned char b);

#ifdef __cplusplus
}
#endif
