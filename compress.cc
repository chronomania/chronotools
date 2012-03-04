#include <algorithm>
#include <cstring>

#include "compress.hh"

//#define DEBUG_DECOMPRESS
//#define DEBUG_COMPRESS

#define MAX_RECURSION_DEPTH  2
#define DEEP_RUN_FORWARD  2048

//#define MAX_RECURSION_DEPTH  2
//#define DEEP_RUN_FORWARD    16

//#define MAX_RECURSION_DEPTH  0
//#define DEEP_RUN_FORWARD     1

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
    static void copy_n(const T* in, size_t size, T* out)
    {
        for(; size > 0; --size)
            *out++ = *in++;
    }
    
    static size_t MatchingLength
        (const unsigned char* in1,
         size_t size,
         const unsigned char* in2)
    {
        // Trivial implementation would be:
        //   return std::mismatch(in1,in1+size, in2).second-in2;
        // However this function attempts to gain something
        // by comparing sizeof(unsigned) bytes at time instead
        // of 1 byte at time.
        size_t len = 0;
        const size_t intsize = sizeof(unsigned);
        const unsigned*const p1 = reinterpret_cast<const unsigned*>(in1);
        const unsigned*const p2 = reinterpret_cast<const unsigned*>(in2);
        while(size >= intsize && p1[len] == p2[len]) { ++len; size -= intsize; }
        len *= intsize;
        while(size > 0 && in1[len] == in2[len]) { --size; ++len; }
        return len;
    }

    class SavingMap
    {
        struct CacheRecord
        {
            size_t len;  // 0=nowhere, 3..maxlen = length.
            size_t offs; // 0=nowhere, 1..maxoffs = relative offset
        };
    public:
        SavingMap(const unsigned char* data, size_t length,
                  size_t maxlen, size_t maxoffs)
            : Data(data), Length(length),
              MaxLength(maxlen), MaxOffs(maxoffs)
        {
            Cache.clear(); Cache.resize(length);
            for(size_t pos=0; pos<length; ++pos) CachePos(pos);
        }
        
        typedef CacheRecord QueryResult;
        
        const QueryResult& Query(size_t pos) const
        {
            return Cache[pos];
        }
    private:
        void CachePos(size_t pos) const
        {
            const size_t min_offs = pos - std::min(pos, MaxOffs);
            const size_t max_offs = pos;
            const size_t output_min = 3;
            const size_t output_max = std::min(Length-pos, MaxLength+3);
            const unsigned char* out_offs  = Data+pos;
            
            size_t best_len = 0;
            size_t best_offs= 0;
            
            for(size_t offs = min_offs; offs < max_offs; ++offs)
            {
                const size_t len = MatchingLength(Data+offs, output_max, out_offs);
                
                if(len < output_min) continue;
                
                if(len > best_len)
                {
                    best_len = len;
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
        const size_t Length;
        const size_t MaxLength;
        const size_t MaxOffs;
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
                size_t endpos = (*this).size();
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
            size_t packetbegin;
        };

    public:
        Compressor(const unsigned char*const data, const size_t length,
                   const size_t DepthBits)
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
            for(size_t pos=0; pos<Length; )
            {
                // Select one of the following choices:
                // - Assign a value for counter and run counter
                // - Copy 8 bytes raw.
                size_t best_counter = PickCounter(pos, MaxRecursion);

                if(best_counter != DefaultLength)
                {
                    result.SendControl(ControlTemplate | best_counter);
                }

                size_t bitpos = result.size();
                
                unsigned char bits = 0xFF;
                for(size_t a=0; a<best_counter; ++a)
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
                for(size_t a=0; a<result.size(); ++a)
                    fprintf(stderr, "%c%02X", (a&15)?' ':'\n', result[a]);
                fprintf(stderr, "\n");
*/
            }
            
            // SendControl applies from now on, not to previous bytes.
            result.SendControl(ControlTemplate | 0x00); // Send end code.
            
            return result;
        }

    private:
#if MAX_RECURSION_DEPTH > 0
        class PickCacheType
        {
        public:
            struct record
            {
                size_t n_source;
                size_t n_result;
                size_t pick;
                bool cached;
            public:
                record(): cached(false) { }
            };

            void Resize(size_t size) { data.resize(size); }
            const record& operator[] (size_t n) const { return data[n]; }
            record& operator[] (size_t n) { return data[n]; }
        private:
            std::vector<PickCacheType::record> data;
        } PickCache[MAX_RECURSION_DEPTH];
#endif
        
        /* For various number of recursions. */
        size_t PickCounter(size_t pos,
                           size_t& n_source,
                           size_t& n_result,
                           size_t MaxRecursion)
        {
#if MAX_RECURSION_DEPTH > 0
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
#endif
            double best_goodness   = 0;
            size_t best_counter  = 0;
            size_t best_n_source = 0;
            size_t best_n_result = 0;
            
            size_t testpos       = pos;
            size_t this_n_result = 1; // The "bits" will be 1 byte.
            
            unsigned char bits = 0xFF;
            
            // for each bit, try and see what works.
            for(size_t counter=0x01; counter<=0x3F; ++counter)
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
                size_t evaluate_n_result = this_n_result;
                size_t evaluate_n_source = testpos-pos; // how many bytes eaten.
                
#if MAX_RECURSION_DEPTH > 0
                if(testpos < Length)
                {
                    if(MaxRecursion > 1)
                    {
                        size_t s=0, r=0;
                        // Recurse to get a better estimate of the goodness of this choice.
                        PickCounter(testpos, s, r, MaxRecursion-1);
                        // ignore the consequent counter.
                        evaluate_n_source += s;
                        evaluate_n_result += r;
                    }
                    else if(MaxRecursion == 1)
                    {
                        size_t s=0, r=0;
                        size_t subpos = testpos;
                        for(size_t n=0; n<DEEP_RUN_FORWARD && subpos < Length; ++n)
                        {
                            size_t subs = 0, subr = 0;
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
#endif
                
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
            
#if MAX_RECURSION_DEPTH > 0
            if(MaxRecursion < MAX_RECURSION_DEPTH)
            {
                PickCacheType::record& tmp = PickCache[MaxRecursion][pos];

                tmp.n_source = best_n_source;
                tmp.n_result = best_n_result;
                tmp.pick = best_counter;
                tmp.cached = true;
            }
#endif

            n_source = best_n_source;
            n_result = best_n_result;
            return best_counter;
        }
        
        size_t PickCounter(size_t pos, size_t MaxRecursion)
        {
            size_t n_source=0;
            size_t n_result=0;
            return PickCounter(pos, n_source, n_result, MaxRecursion);
        }
        
    private:
        static const size_t DefaultLength = 8;
        
        const unsigned char* Data;
        const size_t Length;
        const size_t OffsBits;
        const unsigned char ControlTemplate;
        
        const SavingMap Savings;
    };
}


const vector<unsigned char> Compress
    (const unsigned char* data, size_t length,
     unsigned Depth      /* 11 or 12 */
     )
{
    Compressor com(data, length, Depth);
    vector<unsigned char> result;
    bool first=true;
    for(size_t maxrec=0; maxrec<MAX_RECURSION_DEPTH; ++maxrec)
    {
        vector<unsigned char> tmp = com.Compress(maxrec);
        if(first || tmp.size() < result.size()) result = tmp;
        first=false;
    }
    return result;
}

const vector<unsigned char> Compress(const unsigned char* data, size_t length)
{
    vector<unsigned char> result1 = Compress(data, length, 11);
    return result1;
    vector<unsigned char> result2 = Compress(data, length, 12);
    
    return result1.size() <= result2.size() ? result1 : result2;
}

size_t Uncompress                 /* Return value: compressed size */
    (const unsigned char* Memory,   /* Pointer to the compressed data */
     vector<unsigned char>& Target, /* Where to decompress to */
     const unsigned char* End
    )
{
    Target.clear();
    
    size_t bytes_left = End-Memory;
    #define NEED_BYTES(n) \
        do { if(bytes_left < n) return 0; } while(0)
    #define ATE_BYTES(n) \
        do { Memory+=(n); bytes_left-=(n); } while(0)
    
    #define READ_WORD(dest) \
        do { NEED_BYTES(2); (dest)=(Memory[0]) | (Memory[1] << 8); ATE_BYTES(2); } while(0)
    #define READ_BYTE(dest) \
        do { NEED_BYTES(1); (dest)=(Memory[0]); ATE_BYTES(1); } while(0)
    
    #define READ_ENDPOS(dest) \
        do { size_t endtmp; READ_WORD(endtmp); dest = Begin + endtmp; } while(0)

    #define GENERATE_FROM(source, n) \
    { \
        size_t len = Target.size(); \
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
                    
                    size_t Length = (code >> OffsetBits) + 3;
                    size_t Offset = code & ((1 << OffsetBits) - 1);

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
