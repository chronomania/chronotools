#include <algorithm>
#include <cstring>

#include "compress.hh"

//#define DEBUG_DECOMPRESS
//#define DEBUG_COMPRESS

#define MAX_RECURSION_DEPTH  2
#define DEEP_RUN_FORWARD  2048

/******

 Compression notes:
 
  At any given moment, there are the following options:
   
    - Copy 8 bytes as-is. 
      - Does not affect the counter.
      - Only available when (remaining_bytes) >= 8.
    - Assign a new counter between 1-63.
      - Can be done many times in row.
      - Of course, it's not wise to do it many times in row.
      - Counter value may not exceed (remaining_bytes+3).
      - If the counter value < 8, the high bits of "bits"
        must be set to 1.
    - Run the counterloop.
        - Assign counter value 0
      - Only available when (remaining_bytes) == 0.

  Inside the counterloop, there are the following options:
    - Copy data from backlog. (Only available on 8 first iterations.)
      - Only possible when remaining_bytes >= (counter+3)
      - One of the first 8 iterations of the loop must be a copy-data.
    - Copy 1 byte literal.
      - Only possible when remaining_bytes >= (counter).
    It is not possible to interrupt the loop.
 */

namespace
{
    template<typename T>
    static void copy_n(const T* in, unsigned size, T* out)
    {
        for(; size > 0; --size)
            *out++ = *in++;
    }
    
    template<typename in_it1, typename Size, typename in_it2>
    static const std::pair<in_it1, in_it2>
    mismatch_n(in_it1 in1, Size size, in_it2 in2)
    {
        // There doesn't seem to be mismatch_n at all.
        return std::mismatch(in1, in1+size, in2);
    }

    class SavingMap
    {
        struct CacheRecord
        {
            unsigned len;  // 0=nowhere, 3..maxlen = length.
            unsigned offs; // 0=nowhere, 1..maxoffs = relative offset
        };
    public:
        SavingMap(const unsigned char* data, unsigned length,
                  unsigned maxlen, unsigned maxoffs)
            : Data(data), Length(length),
              MaxLength(maxlen), MaxOffs(maxoffs)
        {
            Cache.clear(); Cache.resize(length);
            for(unsigned pos=0; pos<length; ++pos) CachePos(pos);
        }
        
        typedef CacheRecord QueryResult;
        
        const QueryResult& Query(unsigned pos) const
        {
            return Cache[pos];
        }
    private:
        void CachePos(unsigned pos) const
        {
            const unsigned min_offs = pos - std::min(pos, MaxOffs);
            const unsigned max_offs = pos;
            const unsigned output_min = 3;
            const unsigned output_max = std::min(Length-pos, MaxLength+3);
            const unsigned char* out_offs  = Data+pos;
            
            unsigned best_len = 0;
            unsigned best_offs= 0;
            
            for(unsigned offs = min_offs; offs < max_offs; ++offs)
            {
                const unsigned matching_length =
                    mismatch_n(Data+offs, output_max, out_offs).second - out_offs;
                
                if(matching_length < output_min) continue;
                
                if(matching_length > best_len)
                {
                    best_len = matching_length;
                    best_offs= pos - offs;
                }
            }
            
            //fprintf(stderr, "CachePos(%X (mi%X,ma%X)): len(%u),offs(%u)\n",
            //    pos, min_offs, max_offs, best_len, best_offs);
            
            Cache[pos].len    = best_len;
            Cache[pos].offs   = best_offs;
        }

    private:
        const unsigned char*const  Data;
        const unsigned Length;
        const unsigned MaxLength;
        const unsigned MaxOffs;
        mutable std::vector<CacheRecord> Cache;
    };
    
    class Compressor
    {
        class ResultStream: public vector<unsigned char>
        {
        public:
            ResultStream()
            {
                BeginPacket();
            }
            
            void SendControl(unsigned char value)
            {
                unsigned endpos = (*this).size();
#ifdef DEBUG_COMPRESS
                fprintf(stderr, "@%04X Sending control %02X. Packet begin=%04X\n",
                    endpos, value, packetbegin);
#endif
                if(packetbegin == 0) endpos -= 2;
                (*this)[packetbegin+0] = endpos & 0xFF;
                (*this)[packetbegin+1] = endpos >> 8;

                (*this).push_back(value);
                if(value & 0x3F) BeginPacket();
            }
        private:
            void BeginPacket()
            {
                packetbegin = (*this).size();
                (*this).reserve(packetbegin + 1024);
                
                (*this).push_back(0);
                (*this).push_back(0);
            }
        private:
            unsigned packetbegin;
        };

    public:
        Compressor(const unsigned char*const data, const unsigned length,
                   const unsigned DepthBits)
            : Data(data), Length(length),
              OffsBits(DepthBits),
              ControlTemplate(DepthBits == 11 ? 0xC0 : 0x00),
              Savings(Data,
                      length,
                      (0xFFFF >> DepthBits),
                      (0xFFFF & ((1 << DepthBits) - 1))
                     )
        {
        }
        
        const vector<unsigned char> Compress(unsigned MaxRecursion)
        {
            ResultStream result;
            
            // For each byte in source.
            for(unsigned pos=0; pos<Length; )
            {
                // Select one of the following choices:
                // - Assign a value for counter and run counter
                // - Copy 8 bytes raw.
                unsigned best_counter = PickCounter(pos, MaxRecursion);

                if(best_counter != DefaultLength)
                {
                    result.SendControl(ControlTemplate | best_counter);
                }

                unsigned bitpos = result.size();
                
                unsigned char bits = 0xFF;
                for(unsigned a=0; a<best_counter; ++a)
                {
                    if(a >= 8)
                    {
#ifdef DEBUG_COMPRESS
                        fprintf(stderr, " %02X", Data[pos]);
#endif
                        result.push_back(Data[pos++]);
                        continue;
                    }
                    SavingMap::QueryResult res = Savings.Query(pos);
                    if(res.len == 0)
                    {
#ifdef DEBUG_COMPRESS
                        fprintf(stderr, " %02X", Data[pos]);
#endif
                        result.push_back(Data[pos++]);
                        bits &= ~(1 << a);
                    }
                    else
                    {
                        unsigned word = 0;
                        word |= (res.offs);
                        word |= (res.len - 3) << OffsBits;
                        
#ifdef DEBUG_COMPRESS
                        fprintf(stderr, " %04X", word);
#endif
                        result.push_back(word & 255);
                        result.push_back(word >> 8);
                        pos += res.len;
                    }
                }
#ifdef DEBUG_COMPRESS
                fprintf(stderr, "; %02X\n", bits);
#endif
                
                result.insert(result.begin()+bitpos, 1, bits);
/*
                for(unsigned a=0; a<result.size(); ++a)
                    fprintf(stderr, "%c%02X", (a&15)?' ':'\n', result[a]);
                fprintf(stderr, "\n");
*/
            }
            
            // SendControl applies from now on, not to previous bytes.
            result.SendControl(ControlTemplate | 0x00); // Send end code.
            
            return result;
        }

    private:
        class PickCacheType
        {
        public:
            struct record
            {
                unsigned n_source;
                unsigned n_result;
                unsigned pick;
                bool cached;
            public:
                record(): cached(false) { }
            };

            void Resize(unsigned size) { data.resize(size); }
            const record& operator[] (unsigned n) const { return data[n]; }
            record& operator[] (unsigned n) { return data[n]; }
        private:
            std::vector<PickCacheType::record> data;
        } PickCache[MAX_RECURSION_DEPTH];
        
        /* For various number of recursions. */
        unsigned PickCounter(unsigned pos,
                             unsigned& n_source,
                             unsigned& n_result,
                             unsigned MaxRecursion)
        {
            if(MaxRecursion < MAX_RECURSION_DEPTH)
            {
                PickCache[MaxRecursion].Resize(Length);
                const PickCacheType::record& tmp = PickCache[MaxRecursion][pos];
                if(tmp.cached)
                {
                    n_source = tmp.n_source;
                    n_result = tmp.n_result;
                    return tmp.pick;
                }
            }
            double best_goodness   = 0;
            unsigned best_counter  = 0;
            unsigned best_n_source = 0;
            unsigned best_n_result = 0;
            
            unsigned testpos       = pos;
            unsigned this_n_result = 1; // The "bits" will be 1 byte.
            
            unsigned char bits = 0xFF;
            
            // for each bit, try and see what works.
            for(unsigned counter=0x01; counter<=0x3F; ++counter)
            {
                if(counter == 1)
                    this_n_result += 3; /* Introduce a header. */
                else if(counter == DefaultLength)
                    this_n_result -= 3; /* No header when counter=8. */
                else if(counter == DefaultLength+1)
                    this_n_result += 3; /* Header is back. */
                
                if(testpos >= Length) { /* No more bytes. */ break; }
                
                if(counter > 8)
                {
                    // Ate 1, encoded 1.
                    ++this_n_result;
                    ++testpos;
                }
                else
                {
                    SavingMap::QueryResult res = Savings.Query(testpos);
                    if(res.len == 0)
                    {
                        // Ate 1, encoded 1.
                        ++this_n_result;
                        ++testpos;
                        
                        bits &= ~(1 << (counter-1));
                    }
                    else
                    {
                        // Ate n, encoded 2.
                        this_n_result += 2;
                        testpos += res.len;
                    }
                }
                
                // Calculate the value of this choice.
                unsigned evaluate_n_result = this_n_result;
                unsigned evaluate_n_source = testpos-pos; // how many bytes eaten.
                
                if(testpos < Length)
                {
                    if(MaxRecursion > 1)
                    {
                        unsigned s=0, r=0;
                        // Recurse to get a better estimate of the goodness of this choice.
                        PickCounter(testpos, s, r, MaxRecursion-1);
                        // ignore the consequent counter.
                        evaluate_n_source += s;
                        evaluate_n_result += r;
                    }
                    else if(MaxRecursion == 1)
                    {
                        unsigned s=0, r=0;
                        unsigned subpos = testpos;
                        for(unsigned n=0; n<DEEP_RUN_FORWARD && subpos < Length; ++n)
                        {
                            unsigned subs = 0, subr = 0;
                            PickCounter(subpos, subs, subr, 0);
                            subpos += subs;
                            s += subs;
                            r += subr;
                        }
                        // ignore the consequent counter.
                        evaluate_n_source += s;
                        evaluate_n_result += r;
                    }
                }
                
                double goodness = evaluate_n_source / (double)evaluate_n_result;
                if(goodness > best_goodness)
                {
                    best_goodness = goodness;
                    best_counter  = counter;
                    best_n_source = evaluate_n_source;
                    best_n_result = evaluate_n_result;
                }
                if(bits == 0x00) { /* No more tests: they wouldn't work. */ break; }
            }
            
            if(MaxRecursion < MAX_RECURSION_DEPTH)
            {
                PickCacheType::record& tmp = PickCache[MaxRecursion][pos];

                tmp.n_source = best_n_source;
                tmp.n_result = best_n_result;
                tmp.pick = best_counter;
                tmp.cached = true;
            }

            n_source = best_n_source;
            n_result = best_n_result;
            return best_counter;
        }
        
        unsigned PickCounter(unsigned pos, unsigned MaxRecursion)
        {
            unsigned n_source=0;
            unsigned n_result=0;
            return PickCounter(pos, n_source, n_result, MaxRecursion);
        }
        
    private:
        static const unsigned DefaultLength = 8;
        
        const unsigned char* Data;
        const unsigned Length;
        const unsigned OffsBits;
        const unsigned char ControlTemplate;
        
        const SavingMap Savings;
    };
}


const vector<unsigned char> Compress
    (const unsigned char* data, unsigned length,
     unsigned Depth      /* 11 or 12 */
     )
{
    Compressor com(data, length, Depth);
    vector<unsigned char> result;
    bool first=true;
    for(unsigned maxrec=0; maxrec<MAX_RECURSION_DEPTH; ++maxrec)
    {
        vector<unsigned char> tmp = com.Compress(maxrec);
        if(first || tmp.size() < result.size()) result = tmp;
        first=false;
    }
    return result;
}

const vector<unsigned char> Compress(const unsigned char* data, unsigned length)
{
    vector<unsigned char> result1 = Compress(data, length, 11);
    return result1;
    vector<unsigned char> result2 = Compress(data, length, 12);
    
    return result1.size() <= result2.size() ? result1 : result2;
}

unsigned Uncompress                 /* Return value: compressed size */
    (const unsigned char* Memory,   /* Pointer to the compressed data */
     vector<unsigned char>& Target, /* Where to decompress to */
     const unsigned char* End
    )
{
    Target.clear();
    
    unsigned bytes_left = End-Memory;
    #define NEED_BYTES(n) \
        do { if(bytes_left < n) return 0; } while(0)
    #define ATE_BYTES(n) \
        do { Memory+=(n); bytes_left-=(n); } while(0)
    
    #define READ_WORD(dest) \
        do { NEED_BYTES(2); (dest)=(Memory[0]) | (Memory[1] << 8); ATE_BYTES(2); } while(0)
    #define READ_BYTE(dest) \
        do { NEED_BYTES(1); (dest)=(Memory[0]); ATE_BYTES(1); } while(0)
    
    #define READ_ENDPOS(dest) \
        do { unsigned endtmp; READ_WORD(endtmp); dest = Begin + endtmp; } while(0)

    #define GENERATE_FROM(source, n) \
    { \
        unsigned len = Target.size(); \
        Target.resize(len+n); \
        copy_n(source, n, &Target[len]); \
    }
    
    const unsigned char* const Begin = Memory;
    const unsigned char* Endpos = NULL;
    
    READ_ENDPOS(Endpos); Endpos += 2;

    unsigned char Control = *Endpos;
#ifdef DEBUG_DECOMPRESS
    fprintf(stderr, "Control @ $%03X = %02X\n", Endpos-Begin, Control);
#endif
    
    unsigned OffsetBits = (Control & 0xC0) ? 11 : 12;
    /* ^This setting determines the limit of offsets: 2048 or 4096. */
    /* And limit of lengths (2^(16-offsetbits)). */
    
    const unsigned char counter_default = 8;
    
    unsigned char counter = counter_default;
    for(;;)
    {
        while(Memory == Endpos)
        {
            READ_BYTE(counter);
                
#ifdef DEBUG_DECOMPRESS
            fprintf(stderr, "  $%03X: counter %02X\n", Memory-Begin-1, counter);
#endif
            counter &= 0x3F;
            if(!counter) goto End;
            
            READ_ENDPOS(Endpos);
#ifdef DEBUG_DECOMPRESS
            fprintf(stderr, "        endpos %03X\n", Endpos-Begin);
#endif
        }

        /* Fetch instructions */
        unsigned char bits;
        READ_BYTE(bits);    
        
        if(bits == 0)
        {
            /* bits $00 means always "copy 8 literal bytes", regardless of counter */
            /* In that case, counter is not decremented. */

#ifdef DEBUG_DECOMPRESS
            fprintf(stderr, "  $%03X: [8]%02X; %02X %02X %02X %02X %02X %02X %02X %02X -pos now %u\n",
                Memory-Begin-1,
                bits,
                Memory[0],Memory[1],Memory[2],Memory[3],
                Memory[4],Memory[5],Memory[6],Memory[7],
                Target.size()+8);
#endif
            
            /* Literal 8 bytes */
            NEED_BYTES(8);
            GENERATE_FROM(Memory, 8);
            ATE_BYTES(8);
        }
        else
        {
#ifdef DEBUG_DECOMPRESS
            fprintf(stderr, "  $%03X: (%u)%02X;", Memory-Begin-1, counter, bits);
    #endif
            for(; counter-- > 0; bits >>= 1)
            {
                if(bits&1)
                {
                    /* Fetch offset and length, copy. */
                    unsigned code;
                    READ_WORD(code);
                    
                    unsigned Length = (code >> OffsetBits) + 3;
                    unsigned Offset = code & ((1 << OffsetBits) - 1);

#ifdef DEBUG_DECOMPRESS
                    fprintf(stderr, " %04X(-%u,%u)", code, Offset, Length);
    #endif
                    
                    GENERATE_FROM(&Target[len-Offset], Length); // len is defined in the macro.
                }
                else
                {
                    /* Literal 1 byte */
                    unsigned char c;
                    READ_BYTE(c);
#ifdef DEBUG_DECOMPRESS
                    fprintf(stderr, " %02X", c);
    #endif
                    GENERATE_FROM(&c, 1);
                }
            }
#ifdef DEBUG_DECOMPRESS
            fprintf(stderr, " - pos now %u\n", Target.size());
    #endif
            counter = counter_default;
        }
    }
End:
    return Memory - Begin;
}
