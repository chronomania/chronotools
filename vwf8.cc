#include "compiler.hh"
#include "ctinsert.hh"
#include "config.hh"

namespace
{
    wstring VWF8_DrawS[5]; // Draw string
    wstring VWF8_Draw1[5]; // Draw char
    wstring VWF8_Check[5]; // Check for ready tile
    wstring VWF8_Next[5];  // Prepare for next tile
    wstring VWF8_Write[5]; // Write out
    
    #define OSOITE   0x10 //long
    #define TILEDEST 0x13 //long
    #define MEMADDR  0x16 //long
    
    #define TILEBASE 0x19 //long
    #define MEMTMP   0x1C //long
    #define JMPTMP   0x1F //word
    
    #define LASKURI  0x21 //byte

    #define PIXOFFS  0x22 //word
    #define PITUUS   0x24 //byte

    //globaali
    #define TILENUM  0x25 //word
    #define VRAMADDR 0x27 //word
    
    // TILEBUF must be LAST!
    #define TILEBUF  0x29 //size above

    // NMI k‰ytt‰‰ muuttujia osoitteista:
    //    00:0Dxx
    //    00:00Fx
    //    00:040x
    // joten kunhan et sotke niit‰, olet turvassa
    
    unsigned TILEBASE_OFFS, TILEBASE_SEG;
    unsigned WIDTH_OFFS, WIDTH_SEG;
    
    void TalletaMuuttujat(unsigned Bitness, SNEScode &code)
    {
        const unsigned TileDataSize = 2 * 8 * Bitness;
        const unsigned Vapaa = TILEBUF + TileDataSize;
        
        const int MinPEI = 0x10;
        const int MaxPEI = (Vapaa - 1) & ~1;
        
        // PEI some space
        for(int a=MinPEI; a<=MaxPEI; a+=2) code.EmitCode(0xD4, a); // pei.
    }
    void PalautaMuuttujat(unsigned Bitness, SNEScode &code)
    {
        const unsigned TileDataSize = 2 * 8 * Bitness;
        const unsigned Vapaa = TILEBUF + TileDataSize;
        
        const int MinPEI = 0x10;
        const int MaxPEI = (Vapaa - 1) & ~1;
        
        // Release the borrowed space
        code.Set16bit_X(); //shorten code...
        code.Set16bit_M();
        for(int a=MaxPEI; a>=MinPEI; a-=2) code.EmitCode(0x68, 0x85, a); // pla, sta.
    }

#define CALC_DEBUG 0
    /* Return value: number of bits set */
    int BuildShiftTask(int c, vector<int> &bitsigns, unsigned highest_bit=999)
    {
        if(!c) return 0;
        
        int abs_c = c<0 ? -c : c;
        if(highest_bit==999)
        {
            for(highest_bit=0; highest_bit<bitsigns.size(); ++highest_bit)
            {
                int bitvalue = 1 << highest_bit;
                if(bitvalue >= abs_c) break;
            }
        }
        
        if(abs_c >= (2 << highest_bit)) return 999;
        
        for(unsigned bit=highest_bit; bit-->0; )
            bitsigns[bit] = 0;
        
#if CALC_DEBUG && 0
        fprintf(stderr, "Highest bit=%u, thinking %d...\n", highest_bit, c);
#endif
        
        int bestscore = 999, best_c=0;
        vector<int> bestbits = bitsigns;
        for(int value=-1; value<=1; ++value)
        {
            int bitvalue = (1 << highest_bit) * value;
            int tmp_c = c - bitvalue;
            
            bitsigns[highest_bit] = value;
            
            if(!highest_bit)
            {
                if(tmp_c != 0) continue;
            }

            int score = 0;
            if(value != 0) ++score;
            
            if(highest_bit)
            {
                score += BuildShiftTask(tmp_c, bitsigns, highest_bit-1);
            }
            if(score < bestscore)
            {
                bestbits  = bitsigns;
                bestscore = score;
                best_c    = tmp_c;
            }
        }
        bitsigns = bestbits;
        c        = best_c;
        
#if CALC_DEBUG && 0
        if(bestscore < 999)
        {
            fprintf(stderr, "Highest bit=%u, I think ", highest_bit);
            for(unsigned bit=bitsigns.size(); bit-->0; )
                fprintf(stderr, "%c", "N-P"[bitsigns[bit]+1]);
            fprintf(stderr, " (%d) - best score %d\n", c, bestscore);
        }
#endif
        return bestscore;
    }

    void GenerateCalculation(SNEScode &code, unsigned a, unsigned b, unsigned c, unsigned d)
    {
        // Generates code corresponding to this:
        //         A -= a
        //         A >>= b
        //         A *= c
        //         A += d
        // Must keep all other registers intact.
        // Wastes JMPTMP and flags though.
        
        code.Set16bit_M();
        
        if(c == 0)
        {
            // if multiplying by 0, skip something
            
#if CALC_DEBUG
            fprintf(stderr, "LDA #$0000\n");
#endif
            code.EmitCode(0xA9, 0,0);
            
            if(d == 1)
            {
#if CALC_DEBUG
                fprintf(stderr, "INC A\n");
#endif
                code.EmitCode(0x1A);  // INC A
            }
            else if(d > 0)
            {
#if CALC_DEBUG
                fprintf(stderr, "CLC; ADC A, #$%04X\n", d);
#endif
                code.EmitCode(0x18,0x69, d&255, d/256); // ADD A, d
            }
            return;
        }
        
        /* Disable this if you don't like its side effects:
         * (x >> 7)*2 is not really the same as (x >> 6)
         */
        while(b > 0 && !(c%2)) { --b; c >>= 1; }
        
        unsigned const_sub_pos = 0;
        
        while(const_sub_pos < b && !(a&1))
        {
            a >>= 1;
            ++const_sub_pos;
        }
        
        for(unsigned bit=0; ; ++bit)
        {
            if(bit == const_sub_pos)
            {
                if(a == 1)
                {
                    code.EmitCode(0x3A); // DEC A
#if CALC_DEBUG
                    fprintf(stderr, "DEC A\n");
#endif
                }
                else if(a > 0)
                {
                    code.EmitCode(0x38,0xE9, a&255, a/256); // SUB A, a
#if CALC_DEBUG
                    fprintf(stderr, "STC; SBC A, #$%04X\n", a);
#endif
                }
            }
            if(bit == b)break;
        
#if CALC_DEBUG
            fprintf(stderr, "LSR A\n");
#endif
            code.EmitCode(0x4A);
        }
        
        unsigned last_shift_count = 0;
        while(!(c&1))
        {
            ++last_shift_count;
            c >>= 1;
        }

        vector<int> bitsigns(16);
        for(unsigned a=0; a<bitsigns.size(); ++a)
            bitsigns[a] = (c & (1 << a)) ? 1 : 0;

        BuildShiftTask(c, bitsigns);
        
        unsigned pushnum = 0;
        unsigned lastpush = 0;
        unsigned highest_bit = 0;
        for(unsigned bit=0; bit<bitsigns.size(); ++bit)
        {
            if(!bitsigns[bit])continue;
            ++lastpush;
            highest_bit = bit;
        }
        unsigned const_add_pos = last_shift_count;
        if(d > 0)
            while(const_add_pos > 0 && !(d & 1))
            {
                --const_add_pos;
                d >>= 1;
            }
        
        vector<bool> is_add(lastpush);
        bool x_pushed1 = false;
        
        for(unsigned bit=0; bit <= highest_bit; ++bit)
        {
            if(bit > 0)
            {
#if CALC_DEBUG
                fprintf(stderr, "ASL A\n");
#endif
                code.EmitCode(0x0A);
            }
            if(bitsigns[bit] != 0)
            {
                is_add[pushnum] = bitsigns[bit] == 1;

                ++pushnum;
                // Add A ASL'd by this value
                
                if(pushnum == lastpush)
                {
                    // FIXME: check is_add!
                    // nothing to be done
                    continue;
                }
                if(pushnum == lastpush-1)
                {
#if CALC_DEBUG
                    fprintf(stderr, "STA JMPTMP\n");
#endif
                    code.EmitCode(0x85, JMPTMP);       // STA JMPTMP
                    continue;
                }
                if(pushnum == lastpush-2)
                {
                    code.Set16bit_X();
                    
                    if(!x_pushed1)
                    {
#if CALC_DEBUG
                        fprintf(stderr, "PHX\n");
#endif
                        code.EmitCode(0xDA); //PHX
                    }
#if CALC_DEBUG
                    fprintf(stderr, "TAX\n");
#endif
                    code.EmitCode(0xAA); //TAX
                    continue;
                }
                if(pushnum == lastpush-3)
                {
                    code.Set16bit_X();
#if CALC_DEBUG
                    fprintf(stderr, "PHY; TAY\n");
#endif
                    code.EmitCode(0x5A); //PHY
                    code.EmitCode(0xA8); //TAY

                    continue;
                }

#if CALC_DEBUG
                fprintf(stderr, "PHX\n");
#endif
                code.EmitCode(0xDA); //PHX
                x_pushed1 = true;
#if CALC_DEBUG
                fprintf(stderr, "PHA  # bit %u\n", bit);
#endif
                code.EmitCode(0x48); // PHA
            }
        }
        if(pushnum)
        {
            while(pushnum > 0)
            {
                if(pushnum == lastpush)
                {
                    /* nothing to do, the value is already in A */
                    --pushnum;
                    if(!is_add[pushnum])
                    {
                        fprintf(stderr, "ERROR: Negation requested, not implemented\n");
                        /* FIXME: negate A */
                    }
                    continue;
                }
                else if(pushnum == lastpush-1)
                {
                    /* nothing to do, JMPTMP already prepared */
                }
                else if(pushnum == lastpush-2)
                {
#if CALC_DEBUG
                    fprintf(stderr, "STX JMPTMP\n");
#endif
                    code.EmitCode(0x86, JMPTMP);       // STX JMPTMP
                    if(!x_pushed1)
                    {
#if CALC_DEBUG
                        fprintf(stderr, "PLX\n");
#endif
                        code.EmitCode(0xFA); //PLX
                    }
                }
                else if(pushnum == lastpush-3)
                {
#if CALC_DEBUG
                    fprintf(stderr, "STY JMPTMP; PLY\n");
#endif
                    code.EmitCode(0x84, JMPTMP);       // STY JMPTMP
                    code.EmitCode(0x7A); //PLY
                }
                else
                {
#if CALC_DEBUG
                    fprintf(stderr, "PLX; STX JMPTMP\n");
#endif
                    code.EmitCode(0xFA); //PLX
                    code.EmitCode(0x86, JMPTMP);       // STX JMPTMP
                }
                --pushnum;
                
                if(is_add[pushnum])
                {
#if CALC_DEBUG
                    fprintf(stderr, "CLC; ADC JMPTMP\n");
#endif
                    code.EmitCode(0x18, 0x65, JMPTMP); // ADD JMPTMP
                }
                else
                {
#if CALC_DEBUG
                    fprintf(stderr, "STC; SBC JMPTMP\n");
#endif
                    code.EmitCode(0x38, 0xE5, JMPTMP); // SUB JMPTMP
                }
            }
            if(x_pushed1)
            {
#if CALC_DEBUG
                fprintf(stderr, "PLX\n");
#endif
                code.EmitCode(0xFA); //PLX
            }
        }
        for(unsigned l=0; ; ++l)
        {
            if(l == const_add_pos)
            {
                if(d == 1)
                {
#if CALC_DEBUG
                    fprintf(stderr, "INC A\n");
#endif
                    code.EmitCode(0x1A);  // INC A
                }
                else if(d > 0)
                {
#if CALC_DEBUG
                    fprintf(stderr, "CLC; ADC A, #$%04X\n", d);
#endif
                    code.EmitCode(0x18,0x69, d&255, d/256); // ADD A, d
                }
            }
            if(l == last_shift_count) break;
#if CALC_DEBUG
            fprintf(stderr, "ASL A\n");
#endif
            code.EmitCode(0x0A);
        }
    }
    
    const SubRoutine GetPIIRRA_MJONO(unsigned Bitness)
    {
        const unsigned TileDataSize = 2 * 8 * Bitness;
    /*
        Input:
            Ah:Y = address of the string
            Al   = maximum length of the string (nul terminates earlier)
            DB:X = where to draw
        Magic:
           $7000 = address of tile table in VRAM
    */
    
        SubRoutine result;
        SNEScode &code = result.code;
        
        SNEScode::RelativeBranch Loop = code.PrepareRelativeBranch();
        SNEScode::RelativeBranch End = code.PrepareRelativeBranch();
        
        code.Set8bit_M();
        code.EmitCode(0x85, PITUUS);     // STA PITUUS
        code.EmitCode(0xEB);             // XBA
        code.EmitCode(0x85, OSOITE+2);   // OSOITE.seg = Ah
        code.EmitCode(0x8B, 0x68);       // PHB, PLA
        code.EmitCode(0x85, TILEDEST+2); // TILEDEST.seg = DB
        
        // Merkkijonon alussa pixoffs on aina 0
        code.EmitCode(0x64, PIXOFFS);    // STZ PIXOFFS
        
        code.Set16bit_X();
        code.Set16bit_M();

        code.EmitCode(0x84, OSOITE);        // STY OSOITE.ofs
        
        code.EmitCode(0x86, TILEDEST);      // STX TILEDEST.ofs

        // Aluksi nollataan se kuva kokonaan.
        for(unsigned ind=0x0; ind<TileDataSize; ind+=2)
            code.EmitCode(0x64, TILEBUF+ind); // STZ TILEBUF[ind]

        // Muodostetaan pointteri tilebufferiin.
        code.Set16bit_M();
        // Tilebufin segmentti on $00
        code.EmitCode(0x64, MEMADDR+1);        // STZ MEMADDR+1 (seg<-high)
        // Tilebufin offset on D+TILEBUF
        code.EmitCode(0x7B);                   // TDC
        code.EmitCode(0x18, 0x69, TILEBUF,0);  // ADD A, &TILEBUF
        code.EmitCode(0x85, MEMADDR);          // STA MEMADDR
        
        Loop.ToHere();
        
        code.Set8bit_X(); //shorten code...
        code.Set8bit_M();
        code.EmitCode(0xA9, 0x00, 0xEB); // Ah = $00
        code.EmitCode(0xA7, OSOITE);     // Al = [long[$00:D+OSOITE]]
        code.EmitCode(0xF0, 0);          // BEQ loppu
        End.FromHere();
        
#if 1
        // Go and draw the character
        result.CallSub(VWF8_Draw1[Bitness]);
#endif
        
        code.Set16bit_M();
        code.EmitCode(0xE6, OSOITE);     // INC OSOITE
        code.Set8bit_M();
        code.EmitCode(0xC6, PITUUS);     // DEC PITUUS
        code.EmitCode(0xD0, 0);          // BNE looppi
        Loop.FromHere();
        
        End.ToHere();
        
        result.CallSub(VWF8_Write[Bitness]);
        
        code.EmitCode(0x6B); // rtl
        
        Loop.Proceed();
        End.Proceed();
        
        return result;
    }
    const SubRoutine GetPIIRRA_YKSI(unsigned Bitness)
    {
        SubRoutine result;
        SNEScode &code = result.code;
        
        SNEScode::RelativeBranch Loop = code.PrepareRelativeBranch();
        
        code.Set16bit_X();
        code.Set16bit_M();
        code.EmitCode(0x48);                   // PHA
        code.EmitCode(0x0A,0x0A,0x0A,0x0A);    // ASL A, 4
        code.EmitCode(0x18, 0x69,
                      TILEBASE_OFFS&255,
                      TILEBASE_OFFS>>8);       // ADD A, TILEBASE_OFFS
        code.EmitCode(0x85, TILEBASE);         // STA TILEBASE.ofs
        
        code.Set16bit_M();                      // MEMTMP=MEMADDR
        code.EmitCode(0xA5, MEMADDR);
        code.EmitCode(0x85, MEMTMP);
        code.Set8bit_M();
        code.EmitCode(0xA5, MEMADDR+2);
        code.EmitCode(0x85, MEMTMP+2);
        
        code.Set8bit_M();
        code.EmitCode(0xA9, TILEBASE_SEG);
        code.EmitCode(0x85, TILEBASE+2);       // TILEBASE.seg = TILEBASE_SEG
        
        code.Set16bit_X();
        code.EmitCode(0xA0, 8*Bitness, 0x00);  // LDY <miss‰ on seuraava tile>
        
        code.Set8bit_M();
        code.EmitCode(0xA9, 0x08);             // LDA 0x08
        code.EmitCode(0x85, LASKURI);          // STA LASKURI
        Loop.ToHere();
        
        for(unsigned bitnum=0; bitnum<2; ++bitnum)
        {
            SNEScode::RelativeLongBranch Switch = code.PrepareRelativeLongBranch();
            
            code.Set16bit_M();
            code.Set16bit_X();
            
            // Get the address of the switch
            code.EmitCode(0x62, 0,0);              // PER SWITCHI
            Switch.FromHere();
            
            // Load the offset            
            code.EmitCode(0xA5, PIXOFFS);          // LDA PIXOFFS
            code.EmitCode(0x29, 0xFF, 0x00);       // AND A, $00FF
            code.EmitCode(0x85, JMPTMP);           // STA JMPTMP
            
            // Add them together
            code.EmitCode(0x68);                   // PLA - switchin osoite
            code.EmitCode(0x18, 0x65, JMPTMP);     // ADD A, JMPTMP
            
            // Save it to memory
            code.EmitCode(0x3A);                   // DEC A  (RTS wants -1)
            code.EmitCode(0x85, JMPTMP);           // STA JMPTMP
            // Now get the value to be shifted
            code.EmitCode(0xA7, TILEBASE);         // LDA [long[TILEBASE]]
            code.EmitCode(0x29, 0xFF, 0x00);       // AND A, $00FF
            code.EmitCode(0xE6, TILEBASE);         // INC TILEBASE
            // Proceed with jump.
            code.EmitCode(0xD4, JMPTMP, 0x60);     // PEI JMPTMP, RTS = JMP JMPTMP
            
            Switch.ToHere();
            code.Set16bit_M(); // Ensure we're not getting spurious REPs/SEPs
            code.Set16bit_X();
            
            code.EmitCode(0x0A,0x0A,0x0A,0x0A);    // 8*ASL
            code.EmitCode(0x0A,0x0A,0x0A,0x0A);

            // Now update the two tiles.
            code.Set8bit_M();
            code.EmitCode(0x17, MEMTMP);           // ORA [long[MEMTMP]+y]
            code.EmitCode(0x97, MEMTMP);           // STA [long[MEMTMP]+y]
            code.EmitCode(0xEB);                   // XBA
            code.EmitCode(0x07, MEMTMP);           // ORA [long[MEMTMP]]
            code.EmitCode(0x87, MEMTMP);           // STA [long[MEMTMP]]
            code.Set16bit_M();
            
            code.EmitCode(0xE6, MEMTMP);           // INC MEMTMP
            
            Switch.Proceed();
        }
#if 0
        /* Wonder what's wrong, but this breaks things */
        for(unsigned bitnum=2; bitnum<Bitness; ++bitnum)
        {
            code.Set16bit_M();
            
            // Skip over other bits
            code.EmitCode(0xE6, MEMTMP);           // INC MEMTMP
        }
#endif

        code.Set8bit_M();
        code.EmitCode(0xC6, LASKURI);          // DEC LASKURI
        code.EmitCode(0xD0, 0);                // BNE LOOP
        Loop.FromHere();
        
        code.Set8bit_M();
        code.Set16bit_X();
        code.EmitCode(0xFA);                   // PLX
        // X:ss‰ on nyt se merkki taas.
        code.EmitCode(0xBF,
                      WIDTH_OFFS&255,
                      WIDTH_OFFS>>8,
                      WIDTH_SEG);              // LDA WIDTH_SEG:(WIDTH_OFFS+X)
        code.EmitCode(0x18, 0x65, PIXOFFS);    // ADD A, PIXOFFS
        code.EmitCode(0x85, PIXOFFS);          // STA PIXOFFS
        
        result.CallSub(VWF8_Check[Bitness]);
        
        code.EmitCode(0x6B);                   // RTL
        
        Loop.Proceed();
       
        return result;
    }
    const SubRoutine GetPPU_WRITE_IF(unsigned Bitness)
    {
        SubRoutine result;
        SNEScode &code = result.code;
        
        SNEScode::RelativeBranch End = code.PrepareRelativeBranch();
        
        code.Set8bit_M();
        code.EmitCode(0xA5, PIXOFFS);          // LDA PIXOFFS
        code.EmitCode(0xC9, 0x08);             // CMP A, $08
        code.EmitCode(0x90, 0);                // BCC - Jump if <
        End.FromHere();
        
        code.EmitCode(0x29, 0x07);             // AND A, 0x07
        code.EmitCode(0x85, PIXOFFS);          // STA PIXOFFS (pixoffs -= 8)
        
        // Kirjoita merkki ulos
        result.CallSub(VWF8_Write[Bitness]);
        
        // Alusta muistiosoite taas        
        result.CallSub(VWF8_Next[Bitness]);
        
        End.ToHere();
        
        code.EmitCode(0x6B);                   // RTL
        
        End.Proceed();
       
        return result;
    }
    const SubRoutine GetPPU_WRITE(unsigned Bitness)
    {
        SubRoutine result;
        SNEScode &code = result.code;
        
        code.Set16bit_M();
        
#if 1 /* do anything at all */

#if 1 /* if redefine tiles */
        code.EmitCode(0xA5, TILENUM);          // LDA TILENUM
        
        // Bitness=2: 2 4 8       = 3 shifts
        // Bitness=4: 2 4 8 16 32 = 5 shifts
        for(unsigned shiftn=2; shiftn < 8*Bitness; shiftn+=shiftn)
            code.EmitCode(0x0A);               // SHL A
        
        // One character size is 8*Bitness
        // The "2" here is because write "this" and "next".
        unsigned n = 8*Bitness;
#if 1 /* ppu method */
        code.EmitCode(0x18, 0x65, VRAMADDR);   // ADD A, VRAMADDR

        code.EmitCode(0x8F, 0x16, 0x21, 0x00); // STA $00:$2116 (PPU:lle kirjoitusosoite)
        
        for(unsigned ind=0; ind < n; ind+=2)
            code.EmitCode(0xA5,TILEBUF+ind,0x8F,0x18,0x21,0x00); // LDA, STA $00:$2118
#else /* dma method */
        code.EmitCode(0x0A);  // SHL A
        code.EmitCode(0x18, 0x65, VRAMADDR);   // ADD A, VRAMADDR

        // K‰ytet‰‰n DMA:ta ja kopsataan ramiin vaan
        code.Set16bit_X();
        code.EmitCode(0x8B);              // PHB
        code.EmitCode(0xA8);              // TAY
        
        code.EmitCode(0xA9, n-1, 00);     // LDA $001F
        code.EmitCode(0xA2, TILEBUF,0x00);// LDX TILEBUF
        code.EmitCode(0x54, 0x7E, 0x00);  // MVN $00 -> $7E
        code.EmitCode(0xAB);              // PLB
#endif

#endif

        code.Set16bit_M();
        code.EmitCode(0xA5, TILENUM);          // LDA TILENUM
        code.EmitCode(0x29, 0xFF, 0x03);       // AND A, $03FF
        
        code.Set8bit_M();
        code.EmitCode(0xEB); //XBA
        code.EmitCode(0x05, 0x7E);             // ORA ATTR - t‰m‰ tulee pelilt‰
        code.EmitCode(0xEB); //XBA
        
        code.Set16bit_M();
        code.EmitCode(0x87, TILEDEST);         // STA [long[TILEDEST]]
        code.EmitCode(0xE6, TILEDEST);         // INC TILEDEST
        code.EmitCode(0xE6, TILEDEST);         // INC TILEDEST
#endif

        code.EmitCode(0xE6, TILENUM);          // INC TILENUM

        code.EmitCode(0x6B);                   // RTL

        return result;
    }
    const SubRoutine GetNEW_TILE(unsigned Bitness)
    {
        SubRoutine result;
        SNEScode &code = result.code;
        
        code.Set16bit_M();
        unsigned n = 8*Bitness; // Yhden tilen koko
        for(unsigned ind=0; ind<n; ind+=2)
        {
            // Seuraava nykyiseksi, ja seuraava tyhj‰ksi.
            code.EmitCode(0xA5, TILEBUF+ind+n); // LDA TILEBUF[ind+16]
            code.EmitCode(0x85, TILEBUF+ind);   // STA TILEBUF[ind]
            code.EmitCode(0x64, TILEBUF+ind+n); // STZ TILEBUF[ind+16]
        }

        code.EmitCode(0x6B);                   // RTL
       
        return result;
    }
    const SubRoutine GetEquipLeftItemFunc()
    {
        unsigned Bitness = 2;
        // In: A = item number
        
        SubRoutine result;
        SNEScode &code = result.code;
        
        // Get pointer to item name
        SNEScode::FarToNearCall call = code.PrepareFarToNearCall();
        call.Proceed(0xC2F2E2);
        code.BitnessUnknown();
        
        code.Set8bit_M();
        code.Set16bit_X();
        
        code.EmitCode(0x3A); // DEC A
        code.EmitCode(0xC8); // INC Y
        
        // This is what $C2:EF65 does, and we're substituting it
        code.EmitCode(0x8B,0x08); // PHB,PHP
        code.EmitCode(0x86,0x65); // STX [$00:D+$65]
        
        TalletaMuuttujat(Bitness, code);
        
        // Tilenumeron pit‰isi olla vissiin 12 * pystyrivi.
        //    Esimerkkioffsetteja:
        //         X=4FC6
        //         X=5046
        //         X=50C6
        //         X=5146
        //         X=51C6
        //         X=5246
        //         X=52C6
        
        /////
        code.Set16bit_M();
        code.EmitCode(0x48); //PHA
       // unsigned n = 0x0000;
       // code.EmitCode(0xA9, n&255, n>>8);
       
        code.EmitCode(0xA9, 0x00, 0x70); // LDA $7000
        code.EmitCode(0x85, VRAMADDR);   // STA VRAMADDR
        
        code.EmitCode(0x8A);                //TXA

        // Laske (A - 0x4FC6) / 128 * 10 + 0x0000
        GenerateCalculation(code, 0x4FC6, 7, 10, 0x0000);
        
        code.EmitCode(0x85, TILENUM);      //STA TILENUM
        code.EmitCode(0x68); //PLA
        /////
        
        result.CallSub(VWF8_DrawS[Bitness]);
        
        PalautaMuuttujat(Bitness, code);
        
        code.Set8bit_M();

        // This is what $C2:EF65 does
        // Normalizes the colour, I guess? leaves bits $20,$04
        code.EmitCode(0xA9,0xDC,0x14,0x7E); // LDA A,NOT $DC; TRB [$00:D+$7E]
        code.EmitCode(0x28,0xAB); // PLP,PLB
        
        // Then return.
        code.EmitCode(0x6B);                   // RTL
        
        code.AddCallFrom(0xC2A5AA);
        
        return result;
    }
    const SubRoutine GetSomeItemFunc()
    {
        unsigned Bitness = 4;
        // In: A = item number
        
        SubRoutine result;
        SNEScode &code = result.code;
        
        // Get pointer to item name
        SNEScode::FarToNearCall call = code.PrepareFarToNearCall();
        call.Proceed(0xC2F2E2);
        code.BitnessUnknown();
        
        code.Set16bit_M();
        code.Set16bit_X();
        
        // This is what $C2:EF65 does, and we're substituting it
        code.EmitCode(0x8B,0x08); // PHB,PHP
        code.EmitCode(0x86,0x65); // STX [$00:D+$65]
        
        /* Equip-overview-ruudulla DMA:
            $7E:2E80 -> VRAM:5840 * 1600
            $7E:3E80 -> VRAM:6040 * 1600
           Equip-ruudulla (tarkempi):
            $7E:2E80 vain, sama kuin yll‰
            Paitsi kun tulee teksti‰:
            $7E:5E00 -> VRAM:7800 * 1024
           VRAM:
            bg0: $5800 / $0000
            bg1: $6000 / $0000
            bg2: $6800 / $7000
            bg3: $0000 / $0000
        */
        
        //code.EmitCode(0x48,0x8A); //PHA,TXA
        //code.EmitCode(0x18,0x69,0x00,0x10); // ADD A,$1000
        //code.EmitCode(0xAA,0x68); //TAX,PLA
        
        //  Item-ruudun positio layer 1:ss‰
        //  (tai ainakin minne halutaan kirjoitettavan):
        // $30A2
        // $3122
        // $31A2
        // $3222
        
        TalletaMuuttujat(Bitness, code);
        
        code.Set16bit_M();
        code.Set16bit_X();
        code.EmitCode(0xDA); //PHX
        code.EmitCode(0x48); //PHA
        
        
        /////
        code.Set16bit_M();
        code.EmitCode(0x48); //PHA
       // unsigned n = 0x0000;
       // code.EmitCode(0xA9, n&255, n>>8);
       
        code.EmitCode(0xA9, 0x00, 0x00); // LDA $0000
        code.EmitCode(0x85, VRAMADDR);   // STA VRAMADDR
        
        code.EmitCode(0x8A);                //TXA

        // Laske (A - 0x30A2) / 128 * 11 + 0x120
        GenerateCalculation(code, 0x30A2, 7, 11, 0x120);
        
        code.EmitCode(0x85, TILENUM);      //STA TILENUM
        code.EmitCode(0x68); //PLA
        /////
        
        result.CallSub(VWF8_DrawS[Bitness]);

        // Funktion pit‰‰ palauttaa alkuper‰inen X + pituus*2.
        code.Set16bit_M();
        code.Set16bit_X();
        code.EmitCode(0x68); //PLA
        code.EmitCode(0xFA); //PLX
        code.EmitCode(0x86, JMPTMP);       // STX JMPTMP
        code.EmitCode(0x29, 0xFF, 0x00);   // AND A, $00FF
        code.EmitCode(0x0A);               // ASL A, 1
        code.EmitCode(0x18, 0x65, JMPTMP); // ADD JMPTMP
        code.EmitCode(0xAA); //TAX
        
        PalautaMuuttujat(Bitness, code);
        
        code.Set8bit_M();

        // This is what $C2:EF65 does
        // Normalizes the colour, I guess? leaves bits $20,$04
        code.EmitCode(0xA9,0xDC,0x14,0x7E); // LDA A,NOT $DC; TRB [$00:D+$7E]
        code.EmitCode(0x28,0xAB); // PLP,PLB
        
        code.Set16bit_M(); // must do this at end
        
        // Then return.
        code.EmitCode(0x6B);                   // RTL
        
        code.AddCallFrom(0xC2F2DC);
        
        return result;
    }
    const SubRoutine GetItemListFunc()
    {
        unsigned Bitness = 4;
        
        SubRoutine result;
        SNEScode &code = result.code;
        
        // This is what $C2:EF65 does, and we're substituting it
        code.EmitCode(0x8B,0x08); // PHB,PHP
        code.EmitCode(0x86,0x65); // STX [$00:D+$65]
        
        TalletaMuuttujat(Bitness, code);
        
        code.Set16bit_M();
        code.Set16bit_X();
        code.EmitCode(0xDA); //PHX
        code.EmitCode(0x48); //PHA
        
        
        code.Set16bit_M();
        code.EmitCode(0x48); //PHA
       
        code.EmitCode(0xA9, 0x00, 0x00); // LDA $0000
        code.EmitCode(0x85, VRAMADDR);   // STA VRAMADDR
        
        code.EmitCode(0x8A);                //TXA

        GenerateCalculation(code, 0x30A2, 7, 11, 0x000);
        
        code.EmitCode(0x85, TILENUM);      //STA TILENUM
        code.EmitCode(0x68); //PLA
        
        result.CallSub(VWF8_DrawS[Bitness]);

        // Funktion pit‰‰ palauttaa alkuper‰inen X + pituus*2.
        code.Set16bit_M();
        code.Set16bit_X();
        code.EmitCode(0x68); //PLA
        code.EmitCode(0xFA); //PLX
        code.EmitCode(0x86, JMPTMP);       // STX JMPTMP
        code.EmitCode(0x29, 0xFF, 0x00);   // AND A, $00FF
        code.EmitCode(0x0A);               // ASL A, 1
        code.EmitCode(0x18, 0x65, JMPTMP); // ADD JMPTMP
        code.EmitCode(0xAA); //TAX
        
        PalautaMuuttujat(Bitness, code);
        
        code.Set8bit_M();

        // This is what $C2:EF65 does
        // Normalizes the colour, I guess? leaves bits $20,$04
        code.EmitCode(0xA9,0xDC,0x14,0x7E); // LDA A,NOT $DC; TRB [$00:D+$7E]
        code.EmitCode(0x28,0xAB); // PLP,PLB
        
        code.Set16bit_M(); // must do this at end
        
        code.EmitCode(0xA5, 0x7D); // LDA [$00:D+$7D] - must do this too.
        
        // Then return.
        code.EmitCode(0x6B);                   // RTL
        
        code.AddCallFrom(0xC2B061);
        
        return result;
    }
}

void insertor::GenerateVWF8code(unsigned widthtab_addr, unsigned tiletab_addr)
{
    WIDTH_SEG     = (widthtab_addr >> 16) | 0xC0;
    TILEBASE_SEG  = (tiletab_addr >> 16) | 0xC0;
    WIDTH_OFFS    = (widthtab_addr & 0xFFFF);
    TILEBASE_OFFS = (tiletab_addr & 0xFFFF);
    
    VWF8_DrawS[2] = GetConf("vwf8", "b2_draws").SField();
    VWF8_Draw1[2] = GetConf("vwf8", "b2_draw1").SField();
    VWF8_Check[2] = GetConf("vwf8", "b2_check").SField();
    VWF8_Next [2] = GetConf("vwf8", "b2_next").SField();
    VWF8_Write[2] = GetConf("vwf8", "b2_write").SField();
    VWF8_DrawS[4] = GetConf("vwf8", "b4_draws").SField();
    VWF8_Draw1[4] = GetConf("vwf8", "b4_draw1").SField();
    VWF8_Check[4] = GetConf("vwf8", "b4_check").SField();
    VWF8_Next [4] = GetConf("vwf8", "b4_next").SField();
    VWF8_Write[4] = GetConf("vwf8", "b4_write").SField();
    
    wstring VWF8_I1 = GetConf("vwf8", "i1").SField();
    wstring VWF8_I2 = GetConf("vwf8", "i2").SField();
    wstring VWF8_I3 = GetConf("vwf8", "i3").SField();
    
    FunctionList Functions;

    for(unsigned Bitness=2; Bitness<=4; Bitness+=2)
    {
        Functions.Define(VWF8_DrawS[Bitness], GetPIIRRA_MJONO(Bitness));
        Functions.Define(VWF8_Draw1[Bitness], GetPIIRRA_YKSI(Bitness));
        Functions.Define(VWF8_Next[Bitness],  GetNEW_TILE(Bitness));
        Functions.Define(VWF8_Check[Bitness], GetPPU_WRITE_IF(Bitness));
        Functions.Define(VWF8_Write[Bitness], GetPPU_WRITE(Bitness));
    }
    Functions.Define(VWF8_I1,    GetEquipLeftItemFunc());
    Functions.Define(VWF8_I2,    GetSomeItemFunc());
    Functions.Define(VWF8_I3,    GetItemListFunc());
    
    Functions.RequireFunction(VWF8_I1);
    Functions.RequireFunction(VWF8_I2);
    Functions.RequireFunction(VWF8_I3);
    
    vector<SNEScode> codeblobs;
    vector<wstring>  funcnames;
    
    for(FunctionList::functions_t::const_iterator
        i = Functions.functions.begin();
        i != Functions.functions.end();
        ++i)
    {
        // Omit nonrequired functions.
        if(!i->second.second) continue;
        
        codeblobs.push_back(i->second.first.code);
        funcnames.push_back(i->first);
    }
        
    vector<freespacerec> blocks(codeblobs.size());
    for(unsigned a=0; a<codeblobs.size(); ++a)
        blocks[a].len = codeblobs[a].size();
    
    freespace.OrganizeToAnyPage(blocks);
    
    for(unsigned a=0; a<codeblobs.size(); ++a)
    {
        unsigned addr = blocks[a].pos;
        codeblobs[a].YourAddressIs(addr);
        fprintf(stderr, "  Function %s (%u bytes) will be placed at %02X:%04X\n",
            WstrToAsc(funcnames[a]).c_str(),
            codeblobs[a].size(),
            0xC0 | (addr>>16),
            addr & 0xFFFF);
    }
    
    // All of them are now placed somewhere.
    // Link them!
    
    for(unsigned a=0; a<codeblobs.size(); ++a)
    {
        FunctionList::functions_t::const_iterator i = Functions.functions.find(funcnames[a]);
        
        const SubRoutine::requires_t &req = i->second.first.requires;
        for(SubRoutine::requires_t::const_iterator
            j = req.begin();
            j != req.end();
            ++j)
        {
            // Find the address of the function we're requiring
            unsigned req_addr = NOWHERE;
            for(unsigned b=0; b<funcnames.size(); ++b)
                if(funcnames[b] == j->first)
                {
                    req_addr = codeblobs[b].GetAddress() | 0xC00000;
                    break;
                }
            
            for(set<unsigned>::const_iterator
                k = j->second.begin();
                k != j->second.end();
                ++k)
            {
                codeblobs[a][*k + 0] = req_addr & 255;
                codeblobs[a][*k + 1] = (req_addr >> 8) & 255;
                codeblobs[a][*k + 2] = req_addr >> 16;
            }
        }
    }
    
    // They have now been linked.
    
    codes.insert(codes.begin(), codeblobs.begin(), codeblobs.end());
}
