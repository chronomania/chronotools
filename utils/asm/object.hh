#ifndef bqt65asmObjectHH
#define bqt65asmObjectHH

#include <map>
#include <list>
#include <vector>
#include <string>

class Object
{
public:
    enum SegmentSelection
    {
        CODE,
        DATA,
        ZERO,
        BSS
    };

    class Segment
    {
    public:
        typedef std::map<std::string, unsigned> LabelList;
        typedef std::map<unsigned, LabelList> LabelMap;
    private:
        LabelMap labels;
        std::vector<unsigned char> content;

        template<typename T>
        class Relocdata
        {
        public:
            typedef T                        Type;
            typedef Object::SegmentSelection SegType;
            typedef std::string              VarType;
            
            typedef std::pair<SegType, T> FixupType;
            typedef std::pair<T, VarType> RelocType;
            
            typedef std::vector<FixupType> FixupList;
            typedef std::vector<RelocType> RelocList;
            
            FixupList Fixups;
            RelocList Relocs;
        public:
            void AddFixup(SegType s, T addr) { Fixups.push_back(FixupType(s, addr)); }
            void AddReloc(T addr, const VarType& s) { Relocs.push_back(RelocType(addr, s)); }
        };

    public:
        // too lazy to make handler-functions for all these
        
        typedef std::pair<unsigned,unsigned> r2_t;
        
        typedef Relocdata<unsigned> R16_t;    R16_t R16;       // addr
        typedef Relocdata<unsigned> R16lo_t;  R16lo_t R16lo;   // addr
        typedef Relocdata<r2_t>     R16hi_t;  R16hi_t R16hi;   // addr,lowpart
        typedef Relocdata<r2_t>     R24seg_t; R24seg_t R24seg; // addr,offspart
        typedef Relocdata<unsigned> R24_t;    R24_t R24;       // addr

    public:
        void AddByte(unsigned char byte);
        
        void SetByte(unsigned offset, unsigned char byte);
        
        unsigned GetPos() const { return content.size(); }
        
        unsigned GetBase() const { return 0; }
        unsigned GetSize() const { return content.size(); }
        
        const std::vector<unsigned char>& GetContent() const { return content; }
        
        const LabelMap& GetLabels() const { return labels; }
        const LabelList& GetLabels(unsigned level) { return labels[level]; }

        void ClearLabels(unsigned level);
        
        void DefineLabel(unsigned level, const std::string& name);
        
        bool FindLabel(const std::string& name, unsigned level,
                       unsigned& result) const;
        
        bool FindLabel(const std::string& name) const;
        
        void UndefineLabel(const std::string& name);
        
        void DumpLabels() const;
    };

private:
    class Fixup
    {
        // This record saves a references to a label.
    
        SegmentSelection homeseg;
        unsigned homeoffset;

        char type;        // "prefix"
        long value;       // what's added to it

        // If unresolved, this is nonempty:
        std::string ref;
        
        // If resolved, ref is empty and these are seg:
        SegmentSelection targetseg;
        unsigned targetoffset;
    public:
        Fixup(SegmentSelection s, unsigned o,
              char t, const std::string& r, long v)
          : homeseg(s), homeoffset(o),
            type(t), value(v), ref(r) { }
    
        bool IsResolved() const { return ref.empty(); }

        const std::string& GetName() const { return ref; }
        char GetType() const { return type; }
        long GetValue() const { return value; }
        
        SegmentSelection GetHomeSeg() const { return homeseg; }
        SegmentSelection GetTargetSeg() const { return targetseg; }
        unsigned GetHomeOffset() const { return homeoffset; }
        unsigned GetTargetOffset() const { return targetoffset; }

        void Resolved(SegmentSelection s, unsigned o);

        void Dump() const;
    };
    
    std::list<Fixup> Fixups;

    Segment CodeSeg, DataSeg, ZeroSeg, BssSeg;
    
    unsigned CurScope;
    
    SegmentSelection CurSegment;
    
    void DumpFixups() const;
    void DumpLabels() const;

private:
    Segment& GetSeg();
    
    bool FindLabel(const std::string& s) const;

    bool FindLabel(const std::string& name, unsigned level,
                   SegmentSelection& seg, unsigned& result) const;

    void CheckFixups();
    
public:
    Object(): CurScope(0), CurSegment(CODE)
    {
    }
    
    void StartScope();
    void EndScope();
    
    void GenerateByte(unsigned char byte) { GetSeg().AddByte(byte); }

    void AddFixup(char prefix, const std::string& ref, long value);
    
    void DefineLabel(const std::string& label);
    void DefineLabel(const std::string& label, unsigned level);
    void UndefineLabel(const std::string& label);
    
    void SelectTEXT() { CurSegment = CODE; }
    void SelectDATA() { CurSegment = DATA; }
    void SelectZERO() { CurSegment = ZERO; }
    void SelectBSS() { CurSegment = BSS; }
    
    void Link();
    
    void Dump();
    
    void WriteOut(std::FILE* fp);
};

#endif
