#include <vector>

//#define DEBUG_DECOMPRESS
//#define DEBUG_COMPRESS

#define MAX_RECURSION 1 // anything over 1 is mad
//#define LITERAL_FOLLOWUP_ONLY //- speeds up recursion
//#define PICK_ONLY_8BITS //- good thing but sometimes invalid

#include "rommap.hh"
#include "compress.hh"

using namespace std;

typedef unsigned short Word;
typedef unsigned char Byte;
typedef unsigned int Ptr;

FILE *scriptout = stdout;

static vector<Byte> Data(65536);

static unsigned Decompress(unsigned addr)
{
    unsigned origsize = 0;
    unsigned resultsize =
        Uncompress(ROM+(addr&0x3FFFFF),
                   &Data[0],
                   Data.size(),
                   &origsize);
    fprintf(stderr, "Original size: %u\n", origsize);
    return resultsize;
}

class Compressor
{
    /* Input data */
    const unsigned char* const data;
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

    bool Do_Bits(unsigned& pos, result_t& result, unsigned bitcount) const
    {
        unsigned char bits = 0;
        for(unsigned testpos=pos, counter=1; counter<=bitcount; ++counter)
        {
            if(testpos >= length) return false;
            
            if(counter <= 8 && saving_len[testpos])
            {
                bits |= 1 << (counter-1);
                testpos += saving_len[testpos];
            }
            else
                ++testpos;
        }
        
        if(!bits && bitcount != 8) return false;
        
        result.AddByte(bits);
        
#ifdef DEBUG_COMPRESS
        fprintf(stderr, "Picked %u, bits=%02X; ", bitcount, bits);
#endif

        for(unsigned counter=1; counter<=bitcount; ++counter)
        {
            if(counter <= 8 && saving_len[pos])
            {
                unsigned word = ((saving_len[pos]-3) << Depth)
                              + saving_offset[pos];
                
                result.AddWord(word);
#ifdef DEBUG_COMPRESS
                fprintf(stderr, " %04X", word);
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
        }
#ifdef DEBUG_COMPRESS
        fprintf(stderr, "\n");
#endif
        return true;
    }
    
    unsigned PickBitCount(const unsigned& pos, const result_t& result) const
    {
#ifdef PICK_ONLY_8BITS
    	return 8;
#endif
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
                if(!result.SentControl())
                {
                    if((counter&1) != (FirstControlByte&1))
                    {
                        /* This counter doesn't work */
                        ratio = 0;
                    }
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
    
    void Compress(unsigned pos, result_t& result, unsigned maxrec=MAX_RECURSION)
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
                            unsigned ControlByte = FirstControlByte;
                            if(tmpresult.SentControl())
                                ControlByte = OtherControlByte;
                            else
                            {
                                if((counter&1) != (FirstControlByte&1))
                                    continue;
                            }
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
            result_t savedresult = result;
            unsigned savedpos    = pos;
            if(bitcount != 8)
            {
                unsigned ControlByte = FirstControlByte;
                if(result.SentControl()) ControlByte = OtherControlByte;
                result.SendControl(bitcount | ControlByte);
            }
            
            if(!Do_Bits(pos, result, bitcount))
            {
                //fprintf(stderr, "Eek.\n");
                --bitcount;
                result = savedresult;
                pos    = savedpos;
                goto Retry;
            }
        }
        
        unsigned ControlByte = FirstControlByte;
        if(result.SentControl()) ControlByte = OtherControlByte;
        
        if(ControlByte & 0x3F)
        {
            fprintf(stderr, "Panic: Can't send controlbyte %02X at end\n", ControlByte);
            result.GrowMad();
        }
        
        result.SendControl(ControlByte);
        //
        //fprintf(stderr, "Leaving recursion level %u - generated %u bytes\n",
        //    maxrec, result.GetSize());
    }
    
public:    
    Compressor(const unsigned char *d, unsigned l,
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
    
    const vector<unsigned char> Compress()
    {
        result_t result;
        Compress(0, result);
        return result.GetResult();
    }
};

static const vector<unsigned char> Compress
    (const unsigned char *data, unsigned length,
     unsigned Segment,   /* 7E or 7F */
     unsigned Depth      /* 11 or 12 */
     )
{
    Compressor com(data, length, Segment, Depth);
    return com.Compress();
}

static void Try(const vector<unsigned char>& Orig, unsigned n,
                unsigned seg, unsigned bits)
{
    fprintf(stderr, "Segment %X, bits %u:\n", seg, bits);
    vector<unsigned char> result = Compress(&Orig[0], n, seg, bits);

    fprintf(stderr, "Generated %u bytes of compressed data.\n", result.size());
    fprintf(stderr, "Attempting decompressing:\n");
    
    unsigned origsize = 0;
    unsigned resultsize =
        Uncompress(&result[0],
                   &Data[0],
                   Data.size(),
                   &origsize);
    
    unsigned errors = 0;
    if(origsize != result.size())
    {
        fprintf(stderr, "- Error: Compressor ate %u bytes, should be %u\n",
            origsize, result.size());
        ++errors;
    }
    if(resultsize != n)
    {
        fprintf(stderr, "- Error: Compressor made %u bytes, should be %u\n",
            resultsize, n);
        ++errors;
    }

    unsigned diffs = 0;
    for(unsigned a=0; a<resultsize; ++a)
    {
        if(a >= n) break;
        if(Orig[a] != Data[a]) ++diffs;
    }
    if(diffs)
    {
        fprintf(stderr, "- Error: %u bytes of difference\n", diffs);
        ++errors;
    }
    if(!errors)
        fprintf(stderr, "- OK!\n");
}

static void RecompressTests(unsigned n)
{
    vector<unsigned char> Orig = Data;
    
    Try(Orig, n, 0x7E, 11);
    Try(Orig, n, 0x7E, 12);
    Try(Orig, n, 0x7F, 11);
    Try(Orig, n, 0x7F, 12);
}

static const vector<unsigned char> Compress(unsigned char seg, unsigned n)
{
    vector<unsigned char> result1 = Compress(&Data[0], n, seg, 11);
    vector<unsigned char> result2 = Compress(&Data[0], n, seg, 12);
    
    return result1.size() < result2.size() ? result1 : result2;
}

int main(void)
{
    FILE *fp = fopen("chronofin-nohdr.smc", "rb");
    LoadROM(fp);
    fclose(fp);
    
    unsigned addr, n;

    addr = 0xC59A56; // players on map
    n = Decompress(addr);
    
    addr = 0xC40000; // map gfx, perhaps
    n = Decompress(addr);
    
    addr = 0xC40EB8; // more map gfx
    n = Decompress(addr);
    
    addr = 0xC6FB00; // tile indices in time gauge ?
    n = Decompress(addr);
    
    addr = 0xC62000; // no idea about this one
    n = Decompress(addr);
    
    addr = 0xC5D80B; // "?" time label
    n = Decompress(addr);
    
    addr = 0xC5DA88; // Time label boxes and "PONTPO"
    n = Decompress(addr);
    
    addr = 0xFD0000; // It's a font
    n = Decompress(addr);
    
    addr = 0xC38000; // time gauge gfx titles
    n = Decompress(addr);
    
    addr = 0xFE6002; // Title screen stuff
    n = Decompress(addr);
    
    fprintf(stderr, "Uncompressed %u bytes.\n", n);
    
    //for(unsigned a=0; a<n; ++a) putchar(Data[a]);
    
    fprintf(stderr, "Attempting recompression.\n");
    
    RecompressTests(n);

    return 0;
}
