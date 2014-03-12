typedef union {
    uint8_t BYTE;
    struct {
        unsigned b00: 1; unsigned b01: 1; unsigned b02: 1; unsigned b03: 1;
        unsigned b04: 1; unsigned b05: 1; unsigned b06: 1; unsigned b07: 1;
    };
} byte_t;

typedef union {
    uint16_t WORD;
    struct {
        unsigned b00: 1; unsigned b01: 1; unsigned b02: 1; unsigned b03: 1;
        unsigned b04: 1; unsigned b05: 1; unsigned b06: 1; unsigned b07: 1;
        unsigned b08: 1; unsigned b09: 1; unsigned b0A: 1; unsigned b0B: 1;
        unsigned b0C: 1; unsigned b0D: 1; unsigned b0E: 1; unsigned b0F: 1;
    };
} word_t;

typedef union {
    uint32_t DWORD;
    struct {
        unsigned b00: 1; unsigned b01: 1; unsigned b02: 1; unsigned b03: 1;
        unsigned b04: 1; unsigned b05: 1; unsigned b06: 1; unsigned b07: 1;
        unsigned b08: 1; unsigned b09: 1; unsigned b0A: 1; unsigned b0B: 1;
        unsigned b0C: 1; unsigned b0D: 1; unsigned b0E: 1; unsigned b0F: 1;
        unsigned b10: 1; unsigned b11: 1; unsigned b12: 1; unsigned b13: 1;
        unsigned b14: 1; unsigned b15: 1; unsigned b16: 1; unsigned b17: 1;
        unsigned b18: 1; unsigned b19: 1; unsigned b1A: 1; unsigned b1B: 1;
        unsigned b1C: 1; unsigned b1D: 1; unsigned b1E: 1; unsigned b1F: 1;
    };
} dword_t;

#define R2(n)     n,     n + 2*64,     n + 1*64,     n + 3*64
#define R4(n) R2(n), R2(n + 2*16), R2(n + 1*16), R2(n + 3*16)
#define R6(n) R4(n), R4(n + 2*4 ), R4(n + 1*4 ), R4(n + 3*4 )
static const unsigned char BitReverseTable256[256] = { R6(0), R6(2), R6(1), R6(3) };
#define BitwiseReverse(v) (\
    v = (BitReverseTable256[v & 0xff] << 24)\
      | (BitReverseTable256[(v >> 8) & 0xff] << 16)\
      | (BitReverseTable256[(v >> 16) & 0xff] << 8)\
      | (BitReverseTable256[(v >> 24) & 0xff])\
)

#define BitwiseXNOR8(byte) !(\
    (byte).b00 ^ (byte).b01 ^ (byte).b02 ^ (byte).b03 ^\
    (byte).b04 ^ (byte).b05 ^ (byte).b06 ^ (byte).b07\
)

#define BitwiseXNOR16(word) !(\
    (word).b00 ^ (word).b01 ^ (word).b02 ^ (word).b03 ^\
    (word).b04 ^ (word).b05 ^ (word).b06 ^ (word).b07 ^\
    (word).b08 ^ (word).b09 ^ (word).b0A ^ (word).b0B ^\
    (word).b0C ^ (word).b0D ^ (word).b0E ^ (word).b0F\
)

#define BitwiseXNOR32(dword) !(\
    (dword).b00 ^ (dword).b01 ^ (dword).b02 ^ (dword).b03 ^\
    (dword).b04 ^ (dword).b05 ^ (dword).b06 ^ (dword).b07 ^\
    (dword).b08 ^ (dword).b09 ^ (dword).b0A ^ (dword).b0B ^\
    (dword).b0C ^ (dword).b0D ^ (dword).b0E ^ (dword).b0F ^\
    (dword).b10 ^ (dword).b11 ^ (dword).b12 ^ (dword).b13 ^\
    (dword).b14 ^ (dword).b15 ^ (dword).b16 ^ (dword).b17 ^\
    (dword).b18 ^ (dword).b19 ^ (dword).b1A ^ (dword).b1B ^\
    (dword).b1C ^ (dword).b1D ^ (dword).b1E ^ (dword).b1F\
)

uint32_t __tmp$lsb;
#define LSB(data) (\
    __tmp$lsb = (data),\
    __tmp$lsb >>= 1,\
    __tmp$lsb <<= 1,\
    __tmp$lsb ^= (data),\
    __tmp$lsb\
)

uint32_t __tmp$msb;
#define MSB(data) (\
    __tmp$msb = (data),\
    BitwiseReverse(__tmp$msb),\
    __tmp$msb = LSB(__tmp$msb),\
    __tmp$msb\
)

unsigned char hctoc(const char hc);
