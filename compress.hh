/* Chrono Trigger data uncompressor
 * Copyright (C) 1992,2003 Bisqwit (http://iki.fi/bisqwit/)
 */
static unsigned Uncompress   /* Return value: uncompressed size */
    (const unsigned char *Memory, /* Pointer to the compressed data */
     unsigned char *Target,       /* Where to decompress to */
     unsigned Maxsize,            /* Capacity of Target */
     unsigned *SizePtr = NULL     /* Where to store compressed size */
    )
{
    
#ifdef DEBUG_DECOMPRESS
    #define EatB(target, comment) do { \
        unsigned char value = *Memory; \
        fprintf(stderr, "Read   %02X at $%03X (%s)\n", value, Memory-Begin, comment); \
        target = value; \
        ++Memory; \
    } while(0)
#else
    #define EatB(target, comment) target = *Memory++
#endif

#ifdef DEBUG_DECOMPRESS
    #define EatW(target, comment) do { \
        unsigned value = *Memory | (Memory[1]<<8); \
        fprintf(stderr, "Read %04X at $%03X (%s)\n", value, Memory-Begin, comment); \
        target = value; \
        Memory+=2; \
    } while(0)
#else
    #define EatW(target, comment) do { \
        target = *Memory | (Memory[1]<<8); \
        Memory += 2; \
    } while(0)       
#endif
    
    const unsigned char *Begin = Memory, *Endpos = NULL;
    unsigned len = 0;
    
    { unsigned tmp; EatW(tmp, "endpos"); Endpos = Begin + tmp + 2; }
    
    unsigned char Control = *Endpos;
#ifdef DEBUG_DECOMPRESS
    fprintf(stderr, "Control @ $%03X = %02X\n", Endpos-Begin, Control);
#else
    fprintf(stderr, "GFX type: %02X\n", Control & 0xC1);
#endif
    
    /* If Control&1, data is in 7F; otherwise 7E */
    /* The game uses this to select between
     * the MVN 7F 7F and MVN 7E 7E versions
     * of the decompressor. */
    
    unsigned OffsetBits = (Control & 0xC0) ? 11 : 12;
    /* ^This setting determines the limit of offsets: 2048 or 4096. */
    /* And limit of lengths (2^(16-offsetbits)). */
    
    for(;;)
    {
        unsigned char counter = 8, bits = 0;
        for(;;)
        {
            while(Memory == Endpos)
            {
                EatB(counter, "counter");
                counter &= 0x3F;
                if(!counter) goto End;
                { unsigned tmp; EatW(tmp, "endpos"); Endpos = Begin + tmp; }
            }
            /* Fetch instructions */
            EatB(bits, "bits"); if(bits) break;
            
            /* Literal 8 bytes */
            for(unsigned n=0; n<8 && len < Maxsize; ++n) EatB(Target[len++], "literal8");
            if(len >= Maxsize) goto End;
        }
#ifdef DEBUG_DECOMPRESS
        fprintf(stderr, "counter=%u\n", counter);
#endif
        for(;;)
        {
            if(len >= Maxsize) goto End;
            if(bits&1)
            {
                /* Fetch offset and length, copy. */
                
                unsigned Offset; EatW(Offset, "offset&length");
                unsigned Length = (Offset >> OffsetBits) + 3;
                Offset &= ((1 << OffsetBits) - 1);
                
                for(unsigned n=0; n<Length && len < Maxsize; ++n,++len)
                    Target[len] = Target[len-Offset];
            }
            else
            {
                /* Literal 1 byte */
                EatB(Target[len++], "literal1");
            }
            bits >>= 1;
            if(!--counter)break;
        }
    }
    #undef EatB
    #undef EatW
End:
    if(SizePtr)
    {
        *SizePtr = Memory - Begin;
    }
    return len;
}
