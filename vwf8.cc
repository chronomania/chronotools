#include "compiler.hh"
#include "ctinsert.hh"
#include "config.hh"
#include "settings.hh"

namespace
{
    wstring VWF8_DrawS[5]; // Draw string
    wstring VWF8_Write[5]; // Write out
    
    #define OSOITE   0x10 //long
    
    #define TILEBASE 0x13 //long
    #define JMPTMP   0x16 //word
    
    #define PIXOFFS  0x18 //word
    #define LENGTH   0x1A //byte
    
    #define END_X    0x1B //word

    //globaalit
    #define TILENUM  0x1D //word
    #define VRAMADDR 0x1F //word
    
    // TILEBUF must be LAST!
    #define TILEBUF  0x21 //size above

    // NMI k‰ytt‰‰ muuttujia osoitteista:
    //    00:0Dxx
    //    00:00Fx
    //    00:040x
    // joten kunhan et sotke niit‰, ei h‰t‰‰ NMI:st‰.
    
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
        
        /* This should be moved to another file, but what to do with the VAR it uses? */
        
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
    
    void GenerateModulo(SNEScode &code, unsigned char n)
    {
        // Calculates A %= n
#if CALC_DEBUG
        fprintf(stderr, "STA $00:4204\n");
#endif
        code.Set16bit_M();
        code.EmitCode(0x8F, 0x04, 0x42, 0x00); // Dividend
#if CALC_DEBUG
        fprintf(stderr, "LDA $%02X; STA $00:4206\n");
#endif
        code.Set8bit_M();
        code.EmitCode(0xA9, n);
        code.EmitCode(0x8F, 0x06, 0x42, 0x00); // Divisor
        // Then waste 16 machine cycles
        // Load the result
#if CALC_DEBUG
        fprintf(stderr, "15*NOP; LDA $00:4216\n", n);
#endif
        code.EmitCode(0xEA, 0xEA, 0xEA, 0xEA); // 15 cycles
        code.EmitCode(0xEA, 0xEA, 0xEA, 0xEA); // (rep/sep
        code.EmitCode(0xEA, 0xEA, 0xEA, 0xEA); //  takes
        code.EmitCode(0xEA, 0xEA, 0xEA);       // at least 1)
        code.Set16bit_M();
        code.EmitCode(0xAF, 0x16, 0x42, 0x00); // Remainder
    }
    
    void PrepareEndX(SNEScode &code)
    {
        // END_X = X + LENGTH*2
        // -- The value where X should be after call
        
        code.Set8bit_M();
        code.EmitCode(0xA5, LENGTH);           //LDA LENGTH
        code.Set16bit_M();
        // FIXME: If this has problems with colour or
        //        sprite priority, add ORA ATTR (see GetWrite)
        code.EmitCode(0x29, 0xFF, 0x00);       //AND A, $00FF
        code.EmitCode(0x0A);                   //ASL A
        code.EmitCode(0x85, END_X);            //STA END_X
        
        code.EmitCode(0x8A);                   //TXA
        code.EmitCode(0x18, 0x65, END_X);      //ADD A, END_X
        
        code.EmitCode(0x85, END_X);            //STA END_X
    }
    
    void ProceedEndX(SNEScode &code)
    {
        /* Now we should fill empty space */
        SNEScode::RelativeBranch Loop = code.PrepareRelativeBranch();
        SNEScode::RelativeBranch Done = code.PrepareRelativeBranch();
        
        code.Set16bit_M();
        code.Set16bit_X();
        
        Loop.ToHere();
        
        code.EmitCode(0x8A);                   // TXA
        code.EmitCode(0xC5, END_X);            // CMP A, END_X
        code.EmitCode(0xB0, 0);                // BCS done (if >= )
        Done.FromHere();
        
        code.EmitCode(0xA9, 0xFF, 0x00);       // LDA $00FF
        
        code.EmitCode(0x9D, 0x00, 0x00);       // STA DB:($0000+X)
        code.EmitCode(0xE8, 0xE8);             // INX, INX

        code.EmitCode(0x80, 0);                // BRA loop
        Loop.FromHere();
        
        Done.ToHere();
        
        Loop.Proceed();
        Done.Proceed();
    }
    
    void NextTile(SNEScode &code, unsigned Bitness)
    {
        code.Set16bit_M();
        unsigned n = 8*Bitness; // Yhden tilen koko
        for(unsigned ind=0; ind<n; ind+=2)
        {
            // Seuraava nykyiseksi, ja seuraava tyhj‰ksi.
            code.EmitCode(0xA5, TILEBUF+ind+n); // LDA TILEBUF[ind+16]
            code.EmitCode(0x85, TILEBUF+ind);   // STA TILEBUF[ind]
            code.EmitCode(0x64, TILEBUF+ind+n); // STZ TILEBUF[ind+16]
        }
    }
    
    void CheckNext(SubRoutine &result, unsigned Bitness)
    {
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
        
        NextTile(code, Bitness);

        End.ToHere();
        End.Proceed();
    }

    void DoDrawChar(SubRoutine &result, unsigned Bitness)
    {
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
        
        code.Set8bit_M();
        code.EmitCode(0xA9, TILEBASE_SEG);
        code.EmitCode(0x85, TILEBASE+2);       // TILEBASE.seg = TILEBASE_SEG
        
        code.Set16bit_X();
        code.EmitCode(0xA0, 0x00, 0x00);       // LDY $0000 - pointteri tileen
        
        code.Set8bit_M();
        
        code.EmitCode(0xA9, 0x00);             // LDA $00
        code.EmitCode(0x8B,0x48,0xAB);         // PHB, PHA, PLB
        
        Loop.ToHere();
        
        unsigned ymax = 0;
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
            // Note: Y was set before the loop.
            code.Set8bit_M();
            // FIXME: This is a severe mistake
            // if D != 0, and we haven't verified it!
            code.EmitCode(0x19, TILEBUF+8*Bitness,0);// ORA [DB:y+NEXT_TILEBUF]
            code.EmitCode(0x99, TILEBUF+8*Bitness,0);// STA [DB:y+NEXT_TILEBUF]
            code.EmitCode(0xEB);                     // XBA
            code.EmitCode(0x19, TILEBUF,0);          // ORA [DB:y+TILEBUF]
            code.EmitCode(0x99, TILEBUF,0);          // STA [DB:y+TILEBUF]
            
            code.EmitCode(0xC8);                     // INC Y
            
            code.Set16bit_M();
            
            Switch.Proceed();
            
            ymax += 8;
        }
#if 0
        /* Wonder why, but this breaks things */
        for(unsigned bitnum=2; bitnum<Bitness; ++bitnum)
        {
            // Skip over other bits
            code.EmitCode(0xC8);               // INC Y
            ymax += 8;
        }
#endif

        code.EmitCode(0xC0, ymax&255, ymax/256); // CMP Y, ymax
        code.EmitCode(0x90, 0);                  // BCC loop   (if < )
        Loop.FromHere();
        
        code.EmitCode(0xAB);                   // PLB

        code.Set16bit_M();
        code.Set16bit_X();
        code.EmitCode(0x68);                   // PLA
        // A:ss‰ on nyt se merkki taas.
        
        code.EmitCode(0xDA, 0xAA);             // PHX, TAX
        
        code.Set8bit_M();

        code.EmitCode(0xBF,
                      WIDTH_OFFS&255,
                      WIDTH_OFFS>>8,
                      WIDTH_SEG);              // LDA WIDTH_SEG:(WIDTH_OFFS+X)
        code.EmitCode(0x18, 0x65, PIXOFFS);    // ADD A, PIXOFFS
        code.EmitCode(0x85, PIXOFFS);          // STA PIXOFFS
        
        code.EmitCode(0xFA);                   // PLX

        CheckNext(result, Bitness);
        
        Loop.Proceed();
    }
    
    const SubRoutine GetDrawS(unsigned Bitness)
    {
        const unsigned TileDataSize = 2 * 8 * Bitness;
    /*
        Input:
            Ah:Y = address of the string
            Al   = maximum length of the string (nul terminates earlier)
            DB:X = where to draw
    */
    
        SubRoutine result;
        SNEScode &code = result.code;
        
        SNEScode::RelativeLongBranch Loop = code.PrepareRelativeLongBranch();
        SNEScode::RelativeLongBranch Zero = code.PrepareRelativeLongBranch();
        
        code.Set8bit_M();
        code.EmitCode(0x85, LENGTH);     // STA LENGTH
        code.EmitCode(0xEB);             // XBA
        code.EmitCode(0x85, OSOITE+2);   // OSOITE.seg = Ah
        
        code.Set16bit_X();
        code.Set16bit_M();
        
        PrepareEndX(code);

        code.EmitCode(0x84, OSOITE);        // STY OSOITE.ofs
        
        // Merkkijonon alussa pixoffs on aina 0
        code.EmitCode(0x64, PIXOFFS);       // STZ PIXOFFS
        
        // Aluksi nollataan se kuva kokonaan.
        for(unsigned ind=0x0; ind<TileDataSize; ind+=2)
            code.EmitCode(0x64, TILEBUF+ind); // STZ TILEBUF[ind]

        Loop.ToHere();
        
        code.Set8bit_M();
        code.EmitCode(0xA9, 0x00, 0xEB); // Ah = $00
        code.EmitCode(0xA7, OSOITE);     // Al = [long[$00:D+OSOITE]]
        code.EmitCode(0xD0, 3);          // BNE - continue
        code.EmitCode(0x82, 0,0);        // BRL - zero, quit
        Zero.FromHere();
        
#if 1
        // Go and draw the character
        DoDrawChar(result, Bitness);
#endif
        
        code.Set16bit_M();
        code.EmitCode(0xE6, OSOITE);     // INC OSOITE
        code.Set8bit_M();
        code.EmitCode(0xC6, LENGTH);     // DEC LENGTH
        code.EmitCode(0xF0, 3);          // BEQ - end
        code.EmitCode(0x82, 0,0);        // BRL - loop
        Loop.FromHere();
        
        Zero.ToHere();
        
        /* Finish the last tile */
        result.CallSub(VWF8_Write[Bitness]);
        
        ProceedEndX(code);
        
        code.EmitCode(0x6B); // rtl

        Loop.Proceed();
        Zero.Proceed();
        
        return result;
    }
    
    const SubRoutine GetWrite(unsigned Bitness)
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
        /* FIXME: Jos meinaat k‰ytt‰‰, s‰‰st‰ X */
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
        code.Set16bit_X();

        code.EmitCode(0x9D, 0x00,0x00);        // STA DB:($0000+X)
        code.EmitCode(0xE8, 0xE8);             // INX, INX
#endif
        code.EmitCode(0xE6, TILENUM);          // INC TILENUM

        code.EmitCode(0x6B);                   // RTL

        return result;
    }
    
    void Init_ItemFunc(unsigned Bitness, SNEScode &code)
    {
        code.Set16bit_M();
        code.Set16bit_X();
        
        // This is what $C2:EF65 does, and we're substituting it
        // Save X somewhere
        code.EmitCode(0x86,0x65); // STX [$00:D+$65]

        TalletaMuuttujat(Bitness, code);
    }
    
    void Done_ItemFunc(unsigned Bitness, SNEScode &code)
    {
        PalautaMuuttujat(Bitness, code);
        
        code.Set8bit_M();

        // This is what $C2:EF65 does
        // Normalizes the colour, I guess?
        code.EmitCode(0xA9,0xDC,0x14,0x7E); // LDA A,NOT $24; TRB [$00:D+$7E]

        code.Set16bit_M();
    }
    
    void Generic_ItemFunc(SubRoutine &result,
                          unsigned VRAM_Addr, unsigned Bitness,
                          unsigned BaseAddr,
                          unsigned Lines, unsigned Length,
                          unsigned TileNum)
    {
        SNEScode &code = result.code;
        
        Init_ItemFunc(Bitness, code);
        
        /////
        code.Set16bit_M();
        code.EmitCode(0x48);             //PHA
       
        // VRAMADDR = VRAM_Addr
        code.EmitCode(0xA9, VRAM_Addr&255, VRAM_Addr/256);
        code.EmitCode(0x85, VRAMADDR);
        
#if 1
        code.EmitCode(0x8A);             //TXA
        
        /* TƒMƒ ON LƒHEMPƒNƒ OIKEAA RATKAISUA.
         * MUTTA SEN SIJAAN ETTƒ LASKETAAN (A-BaseAddr)/128,
         * PITƒISI LASKEA POSITIO LISTASSA JA OTTAA MODULO
         * LISTAN KORKEUDEN MUKAAN.
         */
        // Laske (A - BaseAddr) / 128 * Length + TileNum
        GenerateCalculation(code, BaseAddr, 7, Length, TileNum);
#else
        /* TƒMƒKƒƒN MENETELMƒ EI TOIMI, JOS RUUTUA
         * SKROLLAA EDESTAKAISIN (YL÷S JA ALAS).
         * RUUDUN SISƒLL÷T ALKAVAT ENNEN PITKƒƒ
         * HAJOTA, KUN RUUDULLA JO OLEVIA TILEJƒ
         * MƒƒRITELLƒƒN UUDESTAAN.
         */
        code.Set16bit_M();
        const unsigned Var_Addr=0xFAE6;   // just a random address.
        const unsigned char Var_Page=0x7F;// hope it isn't important.
        code.EmitCode(0xAF, Var_Addr&255, Var_Addr/256, Var_Page);
        GenerateModulo(code, Lines*Length);
        // Lis‰t‰‰n TileNum-base
        GenerateCalculation(code, 0,0,1, TileNum);
#endif
        
        code.EmitCode(0x85, TILENUM);    //STA TILENUM
        code.EmitCode(0x68); //PLA
        /////
        
        result.CallSub(VWF8_DrawS[Bitness]);

#if 1
#else
        code.Set16bit_M();
        code.EmitCode(0xA5, TILENUM);    //LDA TILENUM
        // V‰hennet‰‰n TileNum-base
        GenerateCalculation(code, TileNum,0,1, 0);
        // Save to global var
        code.EmitCode(0x8F, Var_Addr&255, Var_Addr/256, Var_Page);
#endif
        
        Done_ItemFunc(Bitness, code);
    }
    
    const SubRoutine GetEquipLeftItemFunc()
    {
        const unsigned Bitness = 2;
        // In: A = item number
        
        SubRoutine result;
        SNEScode &code = result.code;
        
        // Get pointer to item name
        SNEScode::FarToNearCall call = code.PrepareFarToNearCall();
        call.Proceed(GetItemNameFunctionAddr | 0xC00000);
        code.BitnessUnknown();
        
        code.Set8bit_M();
        code.Set16bit_X();
        
        code.EmitCode(0x3A); // DEC A
        code.EmitCode(0xC8); // INC Y
        
        // T‰t‰ kutsutaan offseteilla 4FC6..52C6 80:n stepein
        
        // FIXME: Scrolling bug (8 lines)! (Equip selection view)
        Generic_ItemFunc(result, 0x7000, Bitness, 0x4FC6, 9,10, 0x030);
        
        // Then return.
        code.EmitCode(0x6B);             // RTL
        
        code.AddCallFrom(0xC2A5AA);
        
        return result;
    }
    const SubRoutine GetEquipRightItemFunc()
    {
        // Also used in the SHOP
        const unsigned Bitness = 4;
        // In: A = item number
        
        SubRoutine result;
        SNEScode &code = result.code;
        
        // Get pointer to item name
        SNEScode::FarToNearCall call = code.PrepareFarToNearCall();
        call.Proceed(GetItemNameFunctionAddr | 0xC00000);
        code.BitnessUnknown();
        
        // FIXME: Scrolling bug! (Shop)
        Generic_ItemFunc(result, 0x0000, Bitness, 0x2F4A, 11,11, 0x030);
        
        // Then return.
        code.EmitCode(0x6B);                   // RTL
        
        code.AddCallFrom(0xC2F2DC);
        
        return result;
    }
    const SubRoutine GetItemListFunc()
    {
        const unsigned Bitness = 4;
        // In: A = item number
        
        SubRoutine result;
        SNEScode &code = result.code;
        
        // Get pointer to item name
        SNEScode::FarToNearCall call = code.PrepareFarToNearCall();
        call.Proceed(GetItemNameFunctionAddr | 0xC00000);
        code.BitnessUnknown();
        
        // FIXME: Scrolling bug (10 lines)! (Item browsing)
        Generic_ItemFunc(result, 0x0000, Bitness, 0x2F4A, 11,11, 0x02A);

        // Then return.
        code.EmitCode(0x6B);                   // RTL
        
        code.AddCallFrom(0xC2B053);
        
        return result;
    }
    const SubRoutine GetTech1Func()
    {
        const unsigned Bitness = 4;
        
        // In: 
        //    Ah:Y = address of the string
        //    Al   = maximum length of the string (nul terminates earlier)
        //    DB:X = where to draw
        
        SubRoutine result;
        SNEScode &code = result.code;
        
        code.EmitCode(0xA9, 0x0B, 0xCC); // we overwrote it, do it again.
        
        // FIXME: Scrolling bug (8 lines)! (Status screen)
        Generic_ItemFunc(result, 0x0000, Bitness, 0x2F4A, 9,11, 0x02A);

        // Then return.
        code.EmitCode(0x6B);                   // RTL
        
        code.AddCallFrom(0xC2BDE3);
        
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
    VWF8_Write[2] = GetConf("vwf8", "b2_write").SField();
    VWF8_DrawS[4] = GetConf("vwf8", "b4_draws").SField();
    VWF8_Write[4] = GetConf("vwf8", "b4_write").SField();
    
    wstring VWF8_I1 = GetConf("vwf8", "i1").SField();
    wstring VWF8_I2 = GetConf("vwf8", "i2").SField();
    wstring VWF8_I3 = GetConf("vwf8", "i3").SField();
    wstring VWF8_T1 = GetConf("vwf8", "t1").SField();
    
    FunctionList Functions;

    for(unsigned Bitness=2; Bitness<=4; Bitness+=2)
    {
        Functions.Define(VWF8_DrawS[Bitness], GetDrawS(Bitness));
        Functions.Define(VWF8_Write[Bitness], GetWrite(Bitness));
    }
    Functions.Define(VWF8_I1,    GetEquipLeftItemFunc());
    Functions.Define(VWF8_I2,    GetEquipRightItemFunc());
    Functions.Define(VWF8_I3,    GetItemListFunc());
    Functions.Define(VWF8_T1,    GetTech1Func());
    
    Functions.RequireFunction(VWF8_I1);
    Functions.RequireFunction(VWF8_I2);
    Functions.RequireFunction(VWF8_I3);
    Functions.RequireFunction(VWF8_T1);
    
    LinkAndLocate(Functions);
}
