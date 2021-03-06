#include "romaddr.hh"
#include <stdio.h>

unsigned char ROM2SNESpage(unsigned /*char*/ page)
{
    //return 0x80 + page;

    if(page < 0x40) return page | 0xC0;  // C0..FF
    if(page < 0x7E) return page;         // 40..7D

    /* Pages 00..3F have only their high part mirrored */
    /* Pages 7E..7F can not be used (they're RAM) */

    return page & 0x3F;
}

unsigned char SNES2ROMpage(unsigned /*char*/ page)
{
    //return page - 0x80;
    if(page == 0 || page >= 0x80) return page & 0x3F;
    return (page & 0x3F) + 0x40;
}

static unsigned ROM2SNESaddr_(unsigned addr)
{
    return (addr & 0xFFFF) | (ROM2SNESpage(addr >> 16) << 16);
}

static unsigned SNES2ROMaddr_(unsigned addr)
{
    return (addr & 0xFFFF) | (SNES2ROMpage(addr >> 16) << 16);
}

unsigned ROM2SNESaddr(unsigned addr)
{
    unsigned ret = ROM2SNESaddr_(addr);
    //printf("ROM %X -> SNES %X\n", addr, ret);
    return ret;
}

unsigned SNES2ROMaddr(unsigned addr)
{
    unsigned ret = SNES2ROMaddr_(addr);
    //printf("SNES %X -> ROM %X\n", addr, ret);
    return ret;
}


bool IsSNESbased(unsigned addr)
{
    static const bool is[16] =
    { false,false,false,false,
      true,true,true,false,    // 4..6
      false,false,false,false,
      true,true,true,true };   // C..F
    return is[ (addr / 0x100000) & 0xF ];
}


/* ROM type detection. This is copied from snes9x-1.5. */

#define ROM_NAME_LEN 23

static bool AllASCII (const unsigned char *b, unsigned size)
{
    for (unsigned i = 0; i < size; i++)
    {
        if (b[i] < 32 || b[i] > 126)
            return false;
    }
    return true;
}

static int ScoreHiROM(const unsigned char* ROM, unsigned ROMsize)
{
    int score = 0;
    int o = 0xFF00;

    if(ROM[o + 0xd5] & 0x1)
        score+=2;

    //Mode23 is SA-1
    if(ROM[o + 0xd5] == 0x23)
        score-=2;

    if(ROM[o+0xd4] == 0x20)
        score +=2;

    if ((ROM[o + 0xdc] + (ROM[o + 0xdd] << 8) +
        ROM[o + 0xde] + (ROM[o + 0xdf] << 8)) == 0xffff)
    {
        score += 2;
        if(0!=(ROM[o + 0xde] + (ROM[o + 0xdf] << 8)))
            score++;
    }

    if (ROM[o + 0xda] == 0x33)
        score += 2;
    if ((ROM[o + 0xd5] & 0xf) < 4)
        score += 2;
    if (!(ROM[o + 0xfd] & 0x80))
        score -= 6;
    if ((ROM[o + 0xfc]|(ROM[o + 0xfd]<<8))>0xFFB0)
        score -= 2; //reduced after looking at a scan by Cowering
    if (ROMsize > 1024 * 1024 * 3)
        score += 4;
    if ((1 << (ROM[o + 0xd7] - 7)) > 48)
        score -= 1;
    if (!AllASCII (&ROM[o + 0xb0], 6))
        score -= 1;
    if (!AllASCII (&ROM[o + 0xc0], ROM_NAME_LEN - 1))
        score -= 1;

    return (score);
}

static int ScoreLoROM (const unsigned char* ROM, unsigned ROMsize)
{
    int score = 0;
    int o = 0x7F00;

    if(!(ROM[o + 0xd5] & 0x1))
        score+=3;

    //Mode23 is SA-1
    if(ROM[o + 0xd5] == 0x23)
        score+=2;

    if ((ROM[o + 0xdc] + (ROM[o + 0xdd] << 8) +
        ROM[o + 0xde] + (ROM[o + 0xdf] << 8)) == 0xffff)
    {
        score += 2;
        if(0!=(ROM[o + 0xde] + (ROM[o + 0xdf] << 8)))
            score++;
    }

    if (ROM[o + 0xda] == 0x33)
        score += 2;
    if ((ROM[o + 0xd5] & 0xf) < 4)
        score += 2;
    if (ROMsize <= 1024 * 1024 * 16)
        score += 2;
    if (!(ROM[o + 0xfd] & 0x80))
        score -= 6;
    if ((ROM[o + 0xfc]|(ROM[o + 0xfd]<<8))>0xFFB0)
        score -= 2;//reduced per Cowering suggestion
    if ((1 << (ROM[o + 0xd7] - 7)) > 48)
        score -= 1;
    if (!AllASCII (&ROM[o + 0xb0], 6))
        score -= 1;
    if (!AllASCII (&ROM[o + 0xc0], ROM_NAME_LEN - 1))
        score -= 1;

    return (score);
}

bool GuessROMtype(const unsigned char* ROM, unsigned ROMsize)
{
    int HiROMscore = ScoreHiROM(ROM, ROMsize);
    int LoROMscore = ScoreLoROM(ROM, ROMsize);

    return HiROMscore >= LoROMscore;
}

unsigned GuessROMheaderOffset(const unsigned char* ROM, unsigned ROMsize)
{
    return GuessROMtype(ROM, ROMsize) ? 0xFF00 : 0x7F00;
}

unsigned GetPageSize()
{
    return 0x10000;
}
