#include <vector>
#include <string>
#include <set>

typedef std::string CaseLabel;
typedef long        CaseValue;

struct CaseItem
{
    std::set<CaseValue> values;
    CaseLabel           target;
};
typedef std::vector<CaseItem> CaseItemList;

class CaseGenerator
{
public:
    virtual ~CaseGenerator();
    
    void Generate(const CaseItemList& source, const CaseLabel& over);
    
    virtual CaseValue GetMinValue()    const = 0;
    virtual CaseValue GetMaxValue()    const = 0;
    virtual CaseValue GetDefaultCase() const = 0;
    
    virtual void EmitJump(const std::string& target) = 0;
    virtual void EmitCompareEQ(CaseValue value, const std::string& target) = 0;
    virtual void EmitCompareNE(CaseValue value, const std::string& target) = 0;
    virtual void EmitCompareLT(CaseValue value, const std::string& target) = 0;
    virtual void EmitCompareGT(CaseValue value, const std::string& target) = 0;
    virtual void EmitCompareLE(CaseValue value, const std::string& target) = 0;
    virtual void EmitCompareGE(CaseValue value, const std::string& target) = 0;
    virtual void EmitSubtract(CaseValue value) = 0;
    virtual void EmitJumpTable(const std::vector<std::string>& table) = 0;
    
    // Starts a logical block of code.
    // Return value: a label to put at the end of block.
    virtual const std::string EmitBlock() = 0;
    
    // Ends a logical block.
    // Puts the acquired label into this position.
    virtual void EmitEndBlock(const std::string& label) = 0;
private:
    struct IntCaseItem
    {
        struct IntCaseItem *left;   // left son in binary tree
        struct IntCaseItem *right;  // right son in binary tree; also node chain
        struct IntCaseItem *parent; // parent of node in binary tree
        CaseValue low, high;
        CaseLabel target;
        
        bool Includes(CaseValue v) const { return v >= low && v <= high; }
        
        bool HasLowerBound(const CaseGenerator&) const;
        bool HasUpperBound(const CaseGenerator&) const;
        bool IsBounded(const CaseGenerator&) const;
        
        bool IsRange() const { return low != high; }
        bool operator< (const IntCaseItem& b) const { return low < b.low; }
    };

    CaseLabel defaultlabel;
    std::vector<IntCaseItem> items;
    
    void InitializePointers();
    void BalanceNodes(IntCaseItem** Head,
                      IntCaseItem* Parent);
    void EmitCaseTree(const IntCaseItem& node);
    void EmitLastEQlow(const IntCaseItem& node);
    
    void CreateTree();
    void CreateTable(CaseValue minval, CaseValue range);
    
    void Generate();
};
