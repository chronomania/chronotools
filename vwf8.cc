#include "compiler.hh"
#include "ctinsert.hh"
#include "config.hh"

namespace
{
    wstring VWF8_DrawS; // Draw string
    wstring VWF8_Draw1; // Draw char
    wstring VWF8_Check; // Check for ready tile
    wstring VWF8_Next;  // Prepare for next tile
    wstring VWF8_Write; // Write out
    
    #define BITNESS  4
    
    #define OSOITE   0x10 //long
    #define TILEDEST 0x13 //long
    #define MEMADDR  0x16 //long
    
    #define TILEBASE 0x19 //long
    #define MEMTMP   0x1C //long
    #define JMPTMP   0x1F //word
    
    #define LASKURI  0x21 //byte
    #define TILEDST2 0x22 //long

    #define PIXOFFS  0x25 //word  // ei tarvitse palauttaa tätä wordia
    #define PITUUS   0x27 //byte  // ei tarvitse palauttaa tätä tavua

    //globaali
    #define TILENUM  0x28 //word
    
    #define TILEDATASIZE    (2*8*BITNESS)

    #define TILEBUF  0x2A //2 * 8byte * BITNESS
    
    #define VAPAA    (0x2A + TILEDATASIZE)


    /* ATTRS: What attributes should be set on */
    /* $2000 works good for the equip-list on left */
    
#if 0    
    /* For equip-list on left: */
    #define VRAM_BASE 0x7000
    #define TILENUM_BASE 0x0000
    #define ATTRS 0x2000
#endif
    
    #define VRAM_BASE 0x0000
    #define TILENUM_BASE 0x0100
    #define ATTRS 0x0000
    // $4000=x-flip
    // $8000=y-flip
    
    
    // NMI käyttää muuttujia osoitteista:
    //    00:0Dxx
    //    00:00Fx
    //    00:040x
    // joten kunhan et sotke niitä, olet turvassa
    
    unsigned TILEBASE_OFFS, TILEBASE_SEG;
    unsigned WIDTH_OFFS, WIDTH_SEG;
    
    const SubRoutine GetPIIRRA_MJONO()
    {
    /*
        Input:
            Ah:Y = address of the string
            Al   = maximum length of the string (nul terminates earlier)
            DB:X = where to draw
        Magic:
           $7000 = address of tile table in VRAM
    */

        const int MinPEI = 0x10;
        const int MaxPEI = (VAPAA - 1) & ~1;
        
        SubRoutine result;
        SNEScode &code = result.code;
        
        SNEScode::RelativeBranch Loop = code.PrepareRelativeBranch();
        SNEScode::RelativeBranch End = code.PrepareRelativeBranch();
        
        // PEI some space
        for(int a=MinPEI; a<=MaxPEI; a+=2) code.EmitCode(0xD4, a); // pei.
        
        code.Set8bit_M();
        code.EmitCode(0x85, PITUUS);     // STA PITUUS
        code.EmitCode(0xEB);             // XBA
        code.EmitCode(0x85, OSOITE+2);   // OSOITE.seg = Ah
        code.EmitCode(0x8B, 0x68);       // PHB, PLA
        code.EmitCode(0x85, TILEDEST+2); // TILEDEST.seg = DB
        
        code.EmitCode(0xA9, 0x7D);
        code.EmitCode(0x85, TILEDST2+2); // TILEDST2.seg = 0x7D
        
        // Tilen alussa taas (0pix).
        code.EmitCode(0x64, PIXOFFS);    // STZ PIXOFFS
        
        /////
        code.Set16bit_M();
        code.EmitCode(0xA9, TILENUM_BASE&255, TILENUM_BASE/256);
        code.EmitCode(0x85, TILENUM);    //STA TILENUM
        /////
        
        code.Set16bit_X();
        code.Set16bit_M();

        code.EmitCode(0x84, OSOITE);        // STY OSOITE.ofs
        
        code.EmitCode(0x86, TILEDEST);      // STX TILEDEST.ofs

        code.EmitCode(0x8A);                // TXA
        code.EmitCode(0x38,0xE9,0xC0,0xFF); // SUB A, 0xFFC0
        code.EmitCode(0x85, TILEDST2);      // STA TILEDST2.ofs

        // Aluksi nollataan se kuva kokonaan.
        for(unsigned ind=0x0; ind<TILEDATASIZE; ind+=2)
            code.EmitCode(0x64, TILEBUF+ind); // STZ TILEBUF[ind]

        // Muodostetaan pointteri tilebufferiin.
        code.Set8bit_M();
        // Tilebufin segmentti on $00
        code.EmitCode(0x64, MEMADDR+2);        // STZ MEMADDR+2
        // Tilebufin offset on D+TILEBUF
        code.EmitCode(0x7B);                   // TDC
        code.EmitCode(0x18, 0x69, TILEBUF);    // ADD A, &TILEBUF
        code.Set16bit_M();
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
        result.CallSub(VWF8_Draw1);
#endif
        
        code.Set16bit_M();
        code.EmitCode(0xE6, OSOITE);     // INC OSOITE
        code.Set8bit_M();
        code.EmitCode(0xC6, PITUUS);     // DEC PITUUS
        code.EmitCode(0xD0, 0);          // BNE looppi
        Loop.FromHere();
        
        End.ToHere();
        
        result.CallSub(VWF8_Write);
        
        // Release the borrowed space
        code.Set16bit_X(); //shorten code...
        code.Set16bit_M();
        for(int a=MaxPEI; a>=MinPEI; a-=2) code.EmitCode(0x68, 0x85, a); // pla, sta.
        code.EmitCode(0x6B); // rtl
        
        Loop.Proceed();
        End.Proceed();
        
        return result;
    }
    const SubRoutine GetPIIRRA_YKSI()
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
        code.EmitCode(0xA0, 8*BITNESS, 0x00);  // LDY <missä on seuraava tile>
        
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
        for(unsigned bitnum=2; bitnum<BITNESS; ++bitnum)
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
        // X:ssä on nyt se merkki taas.
        code.EmitCode(0xBF,
                      WIDTH_OFFS&255,
                      WIDTH_OFFS>>8,
                      WIDTH_SEG);              // LDA WIDTH_SEG:(WIDTH_OFFS+X)
        code.EmitCode(0x18, 0x65, PIXOFFS);    // ADD A, PIXOFFS
        code.EmitCode(0x85, PIXOFFS);          // STA PIXOFFS
        
        result.CallSub(VWF8_Check);
        
        code.EmitCode(0x6B);                   // RTL
        
        Loop.Proceed();
       
        return result;
    }
    const SubRoutine GetPPU_WRITE_IF()
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
        result.CallSub(VWF8_Write);
        
        // Alusta muistiosoite taas        
        result.CallSub(VWF8_Next);
        
        End.ToHere();
        
        code.EmitCode(0x6B);                   // RTL
        
        End.Proceed();
       
        return result;
    }
    const SubRoutine GetPPU_WRITE()
    {
        SubRoutine result;
        SNEScode &code = result.code;
        
        code.Set16bit_M();
        
#if 1 /* do anything at all */

#if 1 /* if redefine tiles */
        code.EmitCode(0xA5, TILENUM);          // LDA TILENUM
        
        // bitness=2: 2 4 8       = 3 shifts
        // bitness=4: 2 4 8 16 32 = 5 shifts
        for(unsigned shiftn=2; shiftn < 8*BITNESS; shiftn+=shiftn)
            code.EmitCode(0x0A);               // SHL A
        
        // One character size is 8*BITNESS
        // The "2" here is because write "this" and "next".
        unsigned n = 8*BITNESS;
#if 1 /* ppu method */
        code.EmitCode(0x18, 0x69,VRAM_BASE&255, VRAM_BASE/256);

        code.EmitCode(0x8F, 0x16, 0x21, 0x00); // STA $00:$2116 (PPU:lle kirjoitusosoite)
        
        for(unsigned ind=0; ind < n; ind+=2)
            code.EmitCode(0xA5,TILEBUF+ind,0x8F,0x18,0x21,0x00); // LDA, STA $00:$2118
#else /* dma method */
        code.EmitCode(0x0A);  // SHL A
        code.EmitCode(0x18, 0x69,0x00,0x70);   // ADD A, 0x7000

        // Käytetään DMA:ta ja kopsataan ramiin vaan
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
        //palettisäätöjä: ORA $imm16
        code.EmitCode(0x09, ATTRS&255, ATTRS/256);
        code.EmitCode(0x87, TILEDEST);         // STA [long[TILEDEST]]
        code.EmitCode(0xE6, TILEDEST);         // INC TILEDEST
        code.EmitCode(0xE6, TILEDEST);         // INC TILEDEST
#endif

#if 0 /* use tiledst2 */
        code.EmitCode(0x87, TILEDST2);         // STA [long[TILEDST2]]
        code.EmitCode(0xE6, TILEDST2);         // INC TILEDST2
        code.EmitCode(0xE6, TILEDST2);         // INC TILEDST2
#endif

        code.EmitCode(0xE6, TILENUM);          // INC TILENUM

        code.EmitCode(0x6B);                   // RTL

        return result;
    }
    const SubRoutine GetNEW_TILE()
    {
        SubRoutine result;
        SNEScode &code = result.code;
        
        code.Set16bit_M();
        unsigned n = 8*BITNESS; // Yhden tilen koko
        for(unsigned ind=0; ind<n; ind+=2)
        {
            // Seuraava nykyiseksi, ja seuraava tyhjäksi.
            code.EmitCode(0xA5, TILEBUF+ind+n); // LDA TILEBUF[ind+16]
            code.EmitCode(0x85, TILEBUF+ind);   // STA TILEBUF[ind]
            code.EmitCode(0x64, TILEBUF+ind+n); // STZ TILEBUF[ind+16]
        }

        code.EmitCode(0x6B);                   // RTL
       
        return result;
    }
    const SubRoutine GetEquipLeftItemFunc()
    {
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
        
        result.CallSub(VWF8_DrawS);
        
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
            $7E:2E80 vain, sama kuin yllä
            Paitsi kun tulee tekstiä:
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
        
        //  Item-ruudun position layer 1:ssä
        //  (tai ainakin minne halutaan kirjoitettavan):
        // $30A2
        // $3122
        // $31A2
        // $3222
        
        
#if 1
        result.CallSub(VWF8_DrawS);
#endif
        
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
}

void insertor::GenerateVWF8code(unsigned widthtab_addr, unsigned tiletab_addr)
{
    WIDTH_SEG     = (widthtab_addr >> 16) | 0xC0;
    TILEBASE_SEG  = (tiletab_addr >> 16) | 0xC0;
    WIDTH_OFFS    = (widthtab_addr & 0xFFFF);
    TILEBASE_OFFS = (tiletab_addr & 0xFFFF);
    
    VWF8_DrawS = GetConf("vwf8", "p").SField();
    VWF8_Draw1 = GetConf("vwf8", "d").SField();
    VWF8_Check = GetConf("vwf8", "c").SField();
    VWF8_Next  = GetConf("vwf8", "a").SField();
    VWF8_Write = GetConf("vwf8", "w").SField();
    
    wstring VWF8_I1 = GetConf("vwf8", "i1").SField();
    wstring VWF8_I2 = GetConf("vwf8", "i2").SField();
    
    FunctionList Functions;

    Functions.Define(VWF8_DrawS, GetPIIRRA_MJONO());
    Functions.Define(VWF8_Draw1, GetPIIRRA_YKSI());
    Functions.Define(VWF8_Next, GetNEW_TILE());
    Functions.Define(VWF8_Check, GetPPU_WRITE_IF());
    Functions.Define(VWF8_Write, GetPPU_WRITE());
    Functions.Define(VWF8_I1, GetEquipLeftItemFunc());
    Functions.Define(VWF8_I2, GetSomeItemFunc());
    
    Functions.RequireFunction(VWF8_I1);
    Functions.RequireFunction(VWF8_I2);
    
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
