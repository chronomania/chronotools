#include <map>
#include <list>
#include <vector>
#include <string>

class Object
{
    typedef std::map<std::string, unsigned> LabelList;
    
    class Segment
    {
        // level
        std::map<unsigned, LabelList> labels;
        std::vector<unsigned char> content;

    public:
        void AddByte(unsigned char byte);
        
        unsigned GetPos() const { return content.size(); }

        const LabelList& GetLabels(unsigned level) { return labels[level]; }

        void ClearLabels(unsigned level);
        
        void DefineLabel(unsigned level, const std::string& name);
        
        bool FindLabel(const std::string& name, unsigned level,
                       unsigned& result) const;
        
        bool FindLabel(const std::string& name) const;
    };
    
    enum SegmentSelection
    {
        CODE,
        DATA,
        ZERO,
        BSS
    };
    
    class Fixup
    {
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

        void Resolved(SegmentSelection s, unsigned o);

        void Dump() const;
    };
    
    std::list<Fixup> Fixups;

    Segment CodeSeg, DataSeg, ZeroSeg, BssSeg;
    
    unsigned CurScope;
    
    SegmentSelection CurSegment;
    
    void DumpFixups() const;
    
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
    
    void SelectTEXT() { CurSegment = CODE; }
    void SelectDATA() { CurSegment = DATA; }
    void SelectZERO() { CurSegment = ZERO; }
    void SelectBSS() { CurSegment = BSS; }
    
    void Link();
    
    void Dump();
};
