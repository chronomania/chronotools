#include "compiler.hh"
#include "ctinsert.hh"
#include "config.hh"
#include "settings.hh"
#include "o65.hh"

namespace
{
    unsigned DrawS_2bit;
    unsigned DrawS_4bit;
    
    #define START_VARS  0x10
    #define END_VARS    0x28
    
    #define TILENUM   0x10 //word
    #define VRAMADDR  0x12 //word

    #define CALCTMP   0x14 //word
    
    // NMI käyttää muuttujia osoitteista:
    //    00:0Dxx
    //    00:00Fx
    //    00:040x
    // joten kunhan et sotke niitä, ei hätää NMI:stä.
    
    // FIXME: Selvitä myös IRQ
    
    void TalletaMuuttujat(SNEScode &code)
    {
        const int MinPEI = START_VARS;
        const int MaxPEI = (END_VARS - 1) & ~1;
        
        // PEI some space
        for(int a=MinPEI; a<=MaxPEI; a+=2) code.EmitCode(0xD4, a); // pei.
    }
    void PalautaMuuttujat(SNEScode &code)
    {
        const int MinPEI = START_VARS;
        const int MaxPEI = (END_VARS - 1) & ~1;
        
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
        // Wastes CALCTMP and flags though.
        
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
                    fprintf(stderr, "STA CALCTMP\n");
#endif
                    code.EmitCode(0x85, CALCTMP);       // STA CALCTMP
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
                    /* nothing to do, CALCTMP already prepared */
                }
                else if(pushnum == lastpush-2)
                {
#if CALC_DEBUG
                    fprintf(stderr, "STX CALCTMP\n");
#endif
                    code.EmitCode(0x86, CALCTMP);       // STX CALCTMP
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
                    fprintf(stderr, "STY CALCTMP; PLY\n");
#endif
                    code.EmitCode(0x84, CALCTMP);       // STY CALCTMP
                    code.EmitCode(0x7A); //PLY
                }
                else
                {
#if CALC_DEBUG
                    fprintf(stderr, "PLX; STX CALCTMP\n");
#endif
                    code.EmitCode(0xFA); //PLX
                    code.EmitCode(0x86, CALCTMP);       // STX CALCTMP
                }
                --pushnum;
                
                if(is_add[pushnum])
                {
#if CALC_DEBUG
                    fprintf(stderr, "CLC; ADC CALCTMP\n");
#endif
                    code.EmitCode(0x18, 0x65, CALCTMP); // ADD CALCTMP
                }
                else
                {
#if CALC_DEBUG
                    fprintf(stderr, "STC; SBC CALCTMP\n");
#endif
                    code.EmitCode(0x38, 0xE5, CALCTMP); // SUB CALCTMP
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
    
    void Init_ItemFunc(SNEScode &code)
    {
        code.Set16bit_M();
        code.Set16bit_X();
        
        // This is what $C2:EF65 does, and we're substituting it
        // Save X somewhere
        code.EmitCode(0x86,0x65); // STX [$00:D+$65]

        TalletaMuuttujat(code);
    }
    
    void Done_ItemFunc(SNEScode &code)
    {
        PalautaMuuttujat(code);
        
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
        
        Init_ItemFunc(code);
        
        /////
        code.Set16bit_M();
        code.EmitCode(0x48);             //PHA
       
        // VRAMADDR = VRAM_Addr
        code.EmitCode(0xA9, VRAM_Addr&255, VRAM_Addr/256);
        code.EmitCode(0x85, VRAMADDR);
        
#if 1
        code.EmitCode(0x8A);             //TXA
        
        /* TÄMÄ ON LÄHEMPÄNÄ OIKEAA RATKAISUA.
         * MUTTA SEN SIJAAN ETTÄ LASKETAAN (A-BaseAddr)/128,
         * PITÄISI LASKEA POSITIO LISTASSA JA OTTAA MODULO
         * LISTAN KORKEUDEN MUKAAN.
         */
        // Laske (A - BaseAddr) / 128 * Length + TileNum
        GenerateCalculation(code, BaseAddr, 7, Length, TileNum);
#else
        /* TÄMÄKÄÄN MENETELMÄ EI TOIMI, JOS RUUTUA
         * SKROLLAA EDESTAKAISIN (YLÖS JA ALAS).
         * RUUDUN SISÄLLÖT ALKAVAT ENNEN PITKÄÄ
         * HAJOTA, KUN RUUDULLA JO OLEVIA TILEJÄ
         * MÄÄRITELLÄÄN UUDESTAAN.
         */
        code.Set16bit_M();
        const unsigned Var_Addr=0xFAE6;   // just a random address.
        const unsigned char Var_Page=0x7F;// hope it isn't important.
        code.EmitCode(0xAF, Var_Addr&255, Var_Addr/256, Var_Page);
        GenerateModulo(code, Lines*Length);
        // Lisätään TileNum-base
        GenerateCalculation(code, 0,0,1, TileNum);
#endif
        
        code.EmitCode(0x85, TILENUM);    //STA TILENUM
        code.EmitCode(0x68); //PLA
        /////
        
        switch(Bitness)
        {
            case 2: result.CallFar(DrawS_2bit | 0xC00000); break;
            case 4: result.CallFar(DrawS_4bit | 0xC00000); break;
        }

#if 1
#else
        code.Set16bit_M();
        code.EmitCode(0xA5, TILENUM);    //LDA TILENUM
        // Vähennetään TileNum-base
        GenerateCalculation(code, TileNum,0,1, 0);
        // Save to global var
        code.EmitCode(0x8F, Var_Addr&255, Var_Addr/256, Var_Page);
#endif
        
        Done_ItemFunc(code);
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
        
        // Tätä kutsutaan offseteilla 4FC6..52C6 80:n stepein
        
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
    {
     const string codefile = WstrToAsc(GetConf("vwf8", "file").SField());
     
     FILE *fp = fopen(codefile.c_str(), "rb");
     if(!fp) return;
     
     fprintf(stderr, "Loading '%s'...\n", codefile.c_str());
     
     O65 vwf8_code;
     
     vwf8_code.Load(fp);
     fclose(fp);
     
     unsigned code_size = vwf8_code.GetCodeSize();
     unsigned code_addr = freespace.FindFromAnyPage(code_size);
     vwf8_code.LocateCode(code_addr);
     
     vwf8_code.LinkSym("WIDTH_SEG",   (widthtab_addr >> 16) | 0xC0);
     vwf8_code.LinkSym("WIDTH_OFFS",  (widthtab_addr & 0xFFFF));
     vwf8_code.LinkSym("TILEDATA_SEG",  (tiletab_addr >> 16) | 0xC0);
     vwf8_code.LinkSym("TILEDATA_OFFS", (tiletab_addr & 0xFFFF));
     
     DrawS_2bit = vwf8_code.GetSymAddress(WstrToAsc(GetConf("vwf8", "b2_draws")));
     DrawS_4bit = vwf8_code.GetSymAddress(WstrToAsc(GetConf("vwf8", "b4_draws")));
     
     vwf8_code.Verify();
     
     {
      SNEScode tmp;
      tmp.resize(code_size);
      const vector<unsigned char>& code = vwf8_code.GetCode();
      for(unsigned a=0; a<code_size; ++a)
          tmp[a] = code[a];
      
      tmp.YourAddressIs(code_addr);
      codes.push_back(tmp);
     }
    }

    ucs4string VWF8_I1 = GetConf("vwf8", "i1").SField();
    ucs4string VWF8_I2 = GetConf("vwf8", "i2").SField();
    ucs4string VWF8_I3 = GetConf("vwf8", "i3").SField();
    ucs4string VWF8_T1 = GetConf("vwf8", "t1").SField();
    
    FunctionList Functions;

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
