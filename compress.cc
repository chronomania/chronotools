#include "compress.hh"
#include "msginsert.hh"

//#define DEBUG_DECOMPRESS
//#define DEBUG_COMPRESS

#define MAX_RECURSION 0         // anything over 1 is mad
//#define LITERAL_FOLLOWUP_ONLY //- speeds up recursion

#define SUPER_POSITIONS //- good good
#define USE_GENLEN      //- always good - but works only if SUPER_POSITIONS defined

class Compressor
{
    typedef const unsigned char* dataptr;
    
    /* Input data */
    dataptr const data;
    const unsigned length;
    const unsigned Segment;
    const unsigned Depth;

    const unsigned max_offset;
    const unsigned min_length;
    const unsigned max_length;

    vector<unsigned> saving_offset;
    vector<unsigned> saving_len;

    unsigned char FirstControlByte;
    unsigned char OtherControlByte;

    mutable bool Only8bits;
#ifdef SUPER_POSITIONS
    mutable bool AllSuperPositions;
#endif

    class result_t
    {
        // Actual result
        vector<unsigned char> result;
        // Current code
        vector<unsigned char> output;
        
        bool sent_control;
    public:
        result_t(): sent_control(false)
        {
            result.reserve(1024);
            output.reserve(1024);
        }
        void AddByte(unsigned char v) { output.push_back(v); }
        void AddWord(unsigned short v) { AddByte(v&255); AddByte(v>>8); }
        
        void SendControl(unsigned char value)
        {
            unsigned endpos = GetSize();
            if(sent_control) endpos += 2;
            result.push_back(endpos & 255);
            result.push_back(endpos >> 8);
#ifdef DEBUG_COMPRESS
            fprintf(stderr, "Put endpos: %04X - and there %02X\n", endpos, value);
#endif
            AddByte(value);
            sent_control = true;
            
            result.insert(result.end(), output.begin(), output.end());
            output.clear();
            output.reserve(1024);
        }
        
        void GrowMad()
        {
            output.resize(output.size()+1024);
            result.resize(result.size()+1024);
        }
        
        bool SentControl() const { return sent_control; }
        unsigned GetSize() const { return result.size() + output.size(); }
        const vector<unsigned char>& GetResult() const { return result; }
    };
    
    void Map_Savings()
    {
        /* FIXME: Rewrite using std::mismatch and co. */
        for(unsigned pos=0; pos < length; ++pos)
        {
            unsigned max_saving = 0, max_saving_offset = 0;
            
            /* Find out the best backward reference */
            for(unsigned offset=max_offset; offset>=1; --offset)
            {
                unsigned matching_length = 0;
                unsigned testpos = pos;
                if(testpos < offset) continue;
                
                while(testpos < length && matching_length < max_length)
                {
                    if(testpos >= length) break;
                    if(data[testpos] != data[testpos-offset]) break;
                    ++matching_length;
                    ++testpos;
                }
                if(matching_length < min_length) continue;
                if(matching_length > max_saving)
                {
                    max_saving        = matching_length;
                    max_saving_offset = offset;
                }
/*              - this optimization doesn't work -
                if(matching_length > 1)
                {
                    if(offset > matching_length)
                        offset -= matching_length;
                }
*/
            }
            saving_len[pos] = max_saving;
            saving_offset[pos] = max_saving_offset;
        }
    }
    
    unsigned char GetControl(const result_t& result) const
    {
        return result.SentControl() ? OtherControlByte : FirstControlByte;
    }
    
    bool IsAcceptableControl(unsigned char c, const result_t& result) const
    {
        if(result.SentControl()) return true;
        
        return (c&1) == (FirstControlByte&1);
    }
    
    void Do_Literal8(unsigned &pos, result_t& result) const
    {
#ifdef DEBUG_COMPRESS
        fprintf(stderr, "literal_8");
#endif
        result.AddByte(0);
        for(unsigned n=0; n<8; ++n)
        {
#ifdef DEBUG_COMPRESS
            fprintf(stderr, " %02X", data[pos]);
#endif
            result.AddByte(data[pos++]);
        }
#ifdef DEBUG_COMPRESS
        fprintf(stderr, "\n");
#endif
    }
    
    struct BitStatus
    {
        unsigned testpos;
        unsigned counter;
        unsigned char bits;
#ifdef USE_GENLEN
        unsigned genlen;
#endif
        
        BitStatus(unsigned t,unsigned c,unsigned char b)
           : testpos(t), counter(c), bits(b)
#ifdef USE_GENLEN
           , genlen(0)
#endif
        {
        }
    };
    bool Do_Bits(unsigned& pos, result_t& result, unsigned bitcount) const
    {
        vector<BitStatus> statuslist;
        statuslist.reserve(8);
        
        statuslist.push_back( BitStatus(pos,1,0xFF) );
        
ReHandle:
        for(unsigned a=0; a<statuslist.size(); ++a)
        {
            unsigned& testpos   = statuslist[a].testpos;
            unsigned& counter   = statuslist[a].counter;
            unsigned char& bits = statuslist[a].bits;
#ifdef USE_GENLEN
            unsigned& genlen    = statuslist[a].genlen;
#endif
            
            //fprintf(stderr, "\n--Status %u/%u:", a, statuslist.size());
            for(;;)
            {
                /* If eaten too much */
                if(testpos > length)
                {
                    statuslist.erase(statuslist.begin() + a);
                    goto ReHandle;
                }
                
                /* If done everything */
                if(counter > bitcount) break;
                
                /* If nothing to eat */
                if(testpos >= length)
                {
                    statuslist.erase(statuslist.begin() + a);
                    goto ReHandle;
                }
                
                if(counter > 8)
                {
                    //fprintf(stderr, "\nBit %u(pos %u): copy", counter, testpos);
                    ++testpos; // just copy...
                    ++counter;
#ifdef USE_GENLEN
                    ++genlen;
#endif
                    continue;
                }

                unsigned char bit = 1 << (counter-1);
                ++counter;
                
                bool accept = saving_len[testpos] >= 3;
                
#ifdef SUPER_POSITIONS
                if(accept
                && (saving_len[testpos] == 3 || AllSuperPositions))
                {
                    // Do both - accept and don't accept!
                    BitStatus what_if_1 = statuslist[a];
                    BitStatus what_if_2 = statuslist[a];
                    
                    //fprintf(stderr, "\nBit %u(pos %u): Accepting (%u)", counter-1, testpos, saving_len[testpos]);
                    
                    // Add "accept" as todo
                    what_if_1.testpos += saving_len[testpos];
#ifdef USE_GENLEN
                    what_if_1.genlen  += 2;
#endif

                    //fprintf(stderr, "\nBit %u(pos %u): Raw", counter-1, testpos);
                    what_if_2.testpos++;
                    what_if_2.bits &= ~bit;
#ifdef USE_GENLEN
                    what_if_2.genlen++;
#endif
                    
                    statuslist[a] = what_if_1;
                    statuslist.push_back(what_if_2);
                    goto ReHandle;
                }
#endif
                
                if(accept)
                {
                    //fprintf(stderr, "\nBit %u(pos %u): Accepting (%u)", counter-1, testpos, saving_len[testpos]);
                    testpos += saving_len[testpos];
#if 0
                    genlen  += 2;
#endif
                }
                else
                {
                    //fprintf(stderr, "\nBit %u(pos %u): Raw", counter-1, testpos);
                    bits &= ~bit;
                    ++testpos;
#ifdef USE_GENLEN
                    ++genlen;
#endif
                }
            }
            if(!bits && bitcount != 8)
            {
                statuslist.erase(statuslist.begin() + a);
                goto ReHandle;
            }
        }
        if(!statuslist.size()) return false;
        
        /* Pick the most attracting status. */
        unsigned char bits = 0;
        double best=0;
        for(unsigned a=0; a<statuslist.size(); ++a)
        {
#ifdef DEBUG_COMPRESS
            fprintf(stderr, "\nChoice %u: pos=%u, bits=%02X",
                a, statuslist[a].testpos,
                   statuslist[a].bits);
#endif
            /* Attractiveness is the ratio of bytes saved */
            double score = (statuslist[a].testpos - pos)
#ifdef USE_GENLEN
             / (double)statuslist[a].genlen
#endif
                ;
            if(score > best)
            {
                best = score;
                bits = statuslist[a].bits;
            }
        }
        result.AddByte(bits);
        
#ifdef DEBUG_COMPRESS
        fprintf(stderr, "\n%u choices - picked %u, bits=%02X; ",
            statuslist.size(), bitcount, bits);
        bool errors = false;
#endif
        for(;;)
        {
            if(bits&1)
            {
                unsigned word = ((saving_len[pos]-3) << Depth)
                              + saving_offset[pos];
                
                result.AddWord(word);
#ifdef DEBUG_COMPRESS
                if(!saving_len[pos]) errors = true;
                
                fprintf(stderr, " %04X(-%u,%u)", word, saving_offset[pos], saving_len[pos]);
#endif
                
                pos += saving_len[pos];
            }
            else
            {
#ifdef DEBUG_COMPRESS
                fprintf(stderr, " %02X", data[pos]);
#endif
                result.AddByte(data[pos]);
                ++pos;
            }
            bits >>= 1;
            if(!--bitcount)break;
        }
#ifdef DEBUG_COMPRESS
        if(errors) fprintf(stderr, "\n -ERROR");
        fprintf(stderr, "\n");
#endif
        return true;
    }
    
    unsigned PickBitCount(const unsigned& pos, const result_t& result) const
    {
        if(Only8bits) return 8;

        unsigned in_total[64],  in_size=0;
        unsigned out_total[64], out_size=0;
        
        for(unsigned testpos=pos, counter=0; ++counter <= 63; )
        {
            if(testpos >= length)
            {
                in_total[counter] = 0;
                out_total[counter] = 1000;
                //^ Ensures this won't be used
                continue;
            }
            if(counter <= 8 && saving_len[testpos])
            {
                out_size += 2;
                in_size += saving_len[testpos];
                testpos += saving_len[testpos];
            }
            else
            {
                out_size += 1;
                in_size  += 1;
                ++testpos;
            }
            in_total[counter]  = in_size;
            out_total[counter] = out_size;
        }
        
        for(unsigned counter=1; counter<=63; ++counter)
        {
            out_total[counter] += 1; // add the "bits" byte
            
            if(counter != 8)
            {
                /* Add 3 extra bytes (endpos=2, control=1) */
                out_total[counter] += 3;
            }
        }

        /*fprintf(stderr, "Ratio list: ");*/
        double bestratio = 0; unsigned bestratiopos = 0;
        for(unsigned counter=1; counter<=63; ++counter)
        {
            double ratio = in_total[counter] / (double)out_total[counter];
            
            if(counter != 8)
            {
                if(!IsAcceptableControl(counter, result))
                {
                    /* This counter doesn't work */
                    ratio = 0;
                }
            }
            
            /*fprintf(stderr, " %.2f", ratio);*/
            
            if(ratio > bestratio)
            {
                bestratio = ratio;
                bestratiopos = counter;
            }
        }
        /*fprintf(stderr, "\n");*/

        return bestratiopos;
    }
    
    void Compress(unsigned pos, result_t& result, unsigned maxrec) const
    {
        //fprintf(stderr, "Entering recursion level %u at pos %u\n", maxrec, pos);
        while(pos < length)
        {
#ifdef DEBUG_COMPRESS
            char Buf[64];
            sprintf(Buf, "pos %u/%u", pos, length);
            fprintf(stderr, "%15s: ", Buf);
#endif
            
            bool forced_literal_8 = true;
            for(unsigned testpos=pos, n=0; n<8; ++n,++testpos)
            {
                if(testpos < length && !saving_len[testpos]) continue;
                forced_literal_8 = false;
                break;
            }
            if(forced_literal_8)
            {
                /* Must do literal8 here - no other choice. */
                Do_Literal8(pos, result);
                continue;
            }

            unsigned bitcount = 8;
            
            if(maxrec > 0)
            {
                double bestratio=0;
                unsigned bestratiopos = 8;
                
            #if 1
                //fprintf(stderr, "[%u] Trying at %u\n", maxrec, pos);

                /* Attempt all 63 */
                for(unsigned counter=0; counter<=63; ++counter)
                {
                    unsigned tmppos = pos;
                    result_t tmpresult = result;
                    
                    if(!counter)
                    {
                        if(tmppos + 8 >= length) continue;
                        Do_Literal8(tmppos, tmpresult);
                    }
                    else
                    {
                        if(counter != 8)
                        {
                            unsigned ControlByte = GetControl(tmpresult);
                            if(!IsAcceptableControl(counter, tmpresult))
                                continue;
                            tmpresult.SendControl(counter | ControlByte);
                        }
                        
                        if(!Do_Bits(tmppos, tmpresult, counter)) continue;
                        
#ifdef LITERAL_FOLLOWUP_ONLY
                        if(counter != 8)
                        {
                            bool literal8_next = true;
                            for(unsigned testpos=tmppos, n=0; n<8; ++n,++testpos)
                            {
                                if(testpos < length && !saving_len[testpos]) continue;
                                literal8_next = false;
                                break;
                            }
                            if(!literal8_next) continue;
                        }
#endif
                    }
                    
                    Compress(tmppos, tmpresult, maxrec-1);
                    double ratio = length / (double)tmpresult.GetSize();
                    if(ratio > bestratio) { bestratio = ratio; bestratiopos = counter; }
                }
            #endif
                bitcount = bestratiopos;
                //fprintf(stderr, "[%u]Picked %u\n", maxrec, bitcount);
            }
            else
            {
                bitcount = PickBitCount(pos, result);
            }
            
        Retry:
            if(!bitcount)
            {
                Do_Literal8(pos, result);
                continue;
            }
            const result_t savedresult = result;
            const unsigned savedpos    = pos;
            
            if(bitcount != 8)
            {
                unsigned ControlByte = GetControl(result);
                if(!IsAcceptableControl(bitcount, result))
                {
                    goto DoesntWork;
                }
                result.SendControl(bitcount | ControlByte);
            }
            
            if(!Do_Bits(pos, result, bitcount))
            {
DoesntWork:
#ifdef DEBUG_COMPRESS
                fprintf(stderr, "Eek - %u doesn't work.\n", bitcount);
#endif
                --bitcount;
                
                result = savedresult;
                pos    = savedpos;
                goto Retry;
            }
        }
        
        unsigned char ControlByte = GetControl(result);
        if(ControlByte & 0x3F)
        {
            //fprintf(stderr, "Panic: Can't send controlbyte %02X at end\n", ControlByte);
            result.GrowMad();
        }
        
        result.SendControl(ControlByte);
        //
        //fprintf(stderr, "Leaving recursion level %u - generated %u bytes\n",
        //    maxrec, result.GetSize());
    }
    
public:    
    Compressor(dataptr d, unsigned l,
               unsigned seg, unsigned bits)
        : data(d), length(l),
          Segment(seg), Depth(bits),
          max_offset((1 << Depth) - 1),
          min_length(3),
          max_length((1 << (16-Depth)) - 1 + min_length),
          saving_offset(length),
          saving_len(length)
    {
        unsigned char ControlByte = 0;
        if(Depth == 11) ControlByte |= 0x40;
        
        FirstControlByte = ControlByte;
        OtherControlByte = ControlByte;
        
        if(seg == 0x7F) FirstControlByte |= 1;
        
        Map_Savings();
    }
    
    const vector<unsigned char> Compress() const
    {
        vector<result_t> results;
        
#ifdef SUPER_POSITIONS
        for(int sup=0; sup<2; ++sup)
        {
            AllSuperPositions = sup;
#endif

#ifdef MAX_RECURSION
        {
          Only8bits = false;
          result_t result;
          MessageWorking();
          Compress(0, result, MAX_RECURSION);
          results.push_back(result);
        }
#endif        
        {
          Only8bits = true;
          result_t result;
          MessageWorking();
          Compress(0, result, 0);
          results.push_back(result);
        }
        
#ifdef SUPER_POSITIONS
        }
#endif
        
        unsigned bestsize = 0, bestsizeindex = 0;
        for(unsigned a=0; a<results.size(); ++a)
            if(!a || results[a].GetSize() < bestsize)
            {
                bestsizeindex = a;
                bestsize = results[a].GetSize();
            }
        return results[bestsizeindex].GetResult();
    }
};



unsigned Uncompress                 /* Return value: compressed size */
    (const unsigned char* Memory,   /* Pointer to the compressed data */
     vector<unsigned char>& Target, /* Where to decompress to */
     const unsigned char* End
    )
{
    Target.clear();
    
    #define NEED_BYTES(n) do { if(Memory+n > End) return 0; } while(0)
    
    const unsigned char* Begin = Memory, *Endpos = NULL;
    unsigned endtmp;
    
    NEED_BYTES(2);
    endtmp = *Memory | (Memory[1] << 8); Memory += 2;
#ifdef DEBUG_DECOMPRESS
    fprintf(stderr, "  $%03X: endpos %04X\n", Memory-Begin-2, endtmp);
#endif
    Endpos = Begin + endtmp + 2;
    
    unsigned char Control = *Endpos;
#ifdef DEBUG_DECOMPRESS
    fprintf(stderr, "Control @ $%03X = %02X\n", Endpos-Begin, Control);
#else
 #if 0
    fprintf(stderr, "GFX type: %02X (seg=%02X)\n",
        Control & 0xC1, 0x7E + (Control&1));
 #endif
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
                NEED_BYTES(1);
                counter = *Memory++;
#ifdef DEBUG_DECOMPRESS
                fprintf(stderr, "  $%03X: counter %02X\n", Memory-Begin-1, counter);
#endif
                counter &= 0x3F;
                if(!counter) goto End;
                
                NEED_BYTES(2);
                endtmp = *Memory | (Memory[1] << 8); Memory += 2;
#ifdef DEBUG_DECOMPRESS
                fprintf(stderr, "  $%03X: endpos %04X\n", Memory-Begin-2, endtmp);
#endif
                Endpos = Begin + endtmp;
            }
            /* Fetch instructions */
            
            NEED_BYTES(1);
            bits = *Memory++;
            if(bits) break;
            
            /* bits $00 means always "copy 8 literal bytes", dependless of counter */

#ifdef DEBUG_DECOMPRESS
            fprintf(stderr, "  $%03X: [8]%02X; %02X %02X %02X %02X %02X %02X %02X %02X -pos now %u\n",
                Memory-Begin-1,
                bits,
                Memory[0],Memory[1],Memory[2],Memory[3],
                Memory[4],Memory[5],Memory[6],Memory[7],
                Target.size()+8);
#endif
            
            NEED_BYTES(8);
            /* Literal 8 bytes */
            for(unsigned n=0; n<8; ++n) Target.push_back(*Memory++);
        }
#ifdef DEBUG_DECOMPRESS
        fprintf(stderr, "  $%03X: (%u)%02X;", Memory-Begin-1, counter, bits);
#endif
        for(;;)
        {
            if(bits&1)
            {
                /* Fetch offset and length, copy. */
                
                NEED_BYTES(2);
                unsigned code;
                code = *Memory | (Memory[1] << 8); Memory += 2;
                unsigned Length = (code >> OffsetBits) + 3;
                unsigned Offset = code & ((1 << OffsetBits) - 1);

#ifdef DEBUG_DECOMPRESS
                fprintf(stderr, " %04X(-%u,%u)", code, Offset, Length);
#endif
                
                unsigned len = Target.size();
                Target.resize(len + Length);
                for(unsigned n=0; n<Length; ++n,++len)
                    Target[len] = Target[len-Offset];
            }
            else
            {
                NEED_BYTES(1);
                /* Literal 1 byte */
                unsigned char c = *Memory++;
#ifdef DEBUG_DECOMPRESS
                fprintf(stderr, " %02X", c);
#endif
                Target.push_back(c);
            }
            bits >>= 1;
            if(!--counter)break;
        }
#ifdef DEBUG_DECOMPRESS
        fprintf(stderr, " - pos now %u\n", Target.size());
#endif
    }
End:
    return Memory - Begin;
}

const vector<unsigned char> Compress
    (const unsigned char* data, unsigned length,
     unsigned Segment,   /* 7E or 7F */
     unsigned Depth      /* 11 or 12 */
     )
{
    Compressor com(data, length, Segment, Depth);
    return com.Compress();
}

const vector<unsigned char> Compress
    (const unsigned char* data, unsigned length,
     unsigned char seg)
{
    vector<unsigned char> result1 = Compress(data, length, seg, 11);
    vector<unsigned char> result2 = Compress(data, length, seg, 12);
    
    return result1.size() < result2.size() ? result1 : result2;
}
