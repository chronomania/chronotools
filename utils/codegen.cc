#include <vector>
#include <list>
#include <set>
#include <map>

#include "codegen.hh"

#include "autoptr"

typedef autoptr<struct CodeNode> CodeNodePtr;

#define COMBINE_NODES  1
#define REORDER_NODES  1
#define NEGATE_JUMPS   1
#define DEBUG_NODES    0
#define ELIMINATE_DEAD 1

static FILE *out = NULL;

struct FlagState
{
    FlagAssumedStateXY XY;
    FlagAssumedStateA  A;
    
    bool IsCompatible(const FlagState& b) const
    {
        return((A == UnknownA || b.A == UnknownA || A == b.A)
            && (XY == UnknownXY || b.XY == UnknownXY || XY == b.XY));
    }
};

struct DefList: public ptrable
{
    std::map<std::string, unsigned> Defines;
    std::map<std::string, unsigned> NonDefines;
    
    autoptr<struct DefList> Alternate;
    
public:
    DefList(): Alternate(NULL)
    {
    }
        
    bool operator==(const DefList& b) const
    {
        if(Defines != b.Defines) return false;
        if(NonDefines != b.NonDefines) return false;
        if(!Alternate != !b.Alternate) return false;
        if(Alternate && *Alternate != *b.Alternate) return false;
        return true;
    }
    bool operator!=(const DefList& b) const { return !operator==(b); }
    bool operator< (const DefList& b) const
    {
        if(Defines != b.Defines) return Defines < b.Defines;
        if(NonDefines != b.NonDefines) return NonDefines < b.NonDefines;
        return false;
    }
    
    void Begin()
    {
        std::string rule = GetRule();
        if(!rule.empty())
        {
            if(!Alternate
            && NonDefines.empty()
            && Defines.size() == 1)
            {
                fprintf(out, "#ifdef %s\n", Defines.begin()->first.c_str());
            }
            else if(!Alternate
            && Defines.empty()
            && NonDefines.size() == 1)
            {
                fprintf(out, "#ifndef %s\n", NonDefines.begin()->first.c_str());
            }
            else
                fprintf(out, "#if %s\n", rule.c_str());
        }
    }
    void End()
    {
        std::string rule = GetRule();
        if(!rule.empty())
            fprintf(out, "#endif\n");
    }
    
    void Assimilate(DefList& b);
    
    void ListDefs(std::set<struct DefList>& b);
    
    void Tidy()
    {
        std::map<std::string, unsigned>::iterator i, j;
        for(j = Defines.begin(); j != Defines.end(); j = i)
        {
            i = j; ++i;
            if(!j->second) Defines.erase(j); else j->second=1;
        }
        for(j = NonDefines.begin(); j != NonDefines.end(); j = i)
        {
            i = j; ++i;
            if(!j->second) NonDefines.erase(j); else j->second=1;
        }
        if(Alternate) Alternate->Tidy();
        while(Defines.empty() && NonDefines.empty() && Alternate)
        {
            Defines    = Alternate->Defines;
            NonDefines = Alternate->NonDefines;
            Alternate  = Alternate->Alternate;
        }
    }
private:
    const std::string GetRule() const
    {
        std::map<std::string, unsigned>::const_iterator i;
        std::string result;
        
        bool ands = false;
        for(i=Defines.begin(); i!=Defines.end(); ++i)
            //if(i->second)
            {
                if(!result.empty()) { ands = true; result += "&&"; }
                result += "defined(";
                result += i->first;
                if(i->second != 1) { char Buf[32];sprintf(Buf,"/*%u*/",i->second);result+=Buf;}
                result += ")";
            }
        for(i=NonDefines.begin(); i!=NonDefines.end(); ++i)
            //if(i->second)
            {
                if(!result.empty()) { ands = true; result += "&&"; }
                result += "!defined(";
                result += i->first;
                if(i->second != 1) { char Buf[32];sprintf(Buf,"/*%u*/",i->second);result+=Buf;}
                result += ")";
            }
        if(Alternate)
        {
            std::string alt = Alternate->GetRule();
            if(!alt.empty())
            {
                if(ands) result = std::string("(") + result + ")";
                result = result + " || " + alt;
            }
            else
                result = result + " || false";
        }
        return result;
    }
};

void DefList::ListDefs(std::set<DefList>& b)
{
    Tidy();
    DefList tmp;
    tmp.Defines    = Defines;
    tmp.NonDefines = NonDefines;
    tmp.Alternate = NULL;
    b.insert(tmp);
    if(Alternate) Alternate->ListDefs(b);
}

void DefList::Assimilate(DefList& b)
{
    std::set<DefList> list;
    ListDefs(list);
    b.ListDefs(list);
    
    struct DefList* target = this;
    for(std::set<DefList>::const_iterator i = list.begin(); ; )
    {
        target->Defines    = i->Defines;
        target->NonDefines = i->NonDefines;
        target->Alternate  = NULL;
        
        if(++i == list.end()) break;
        target->Alternate = new DefList;
        target = target->Alternate;
    }
}


struct AutoRefererLink: public CodeNodePtr
{
    CodeNodePtr   owner;
    
    AutoRefererLink(): owner(NULL)
    {
    }
    ~AutoRefererLink()
    {
        Unlink();
    }
    AutoRefererLink& operator= (CodeNode *value)
    {
        Unlink();
        CodeNodePtr::operator= (value);
        Link();
        return *this;
    }
    void SetOwner(CodeNode *p) { owner = p; }
private:
    void Link();
    void Unlink();
    
    AutoRefererLink(const AutoRefererLink& b);
    
    void operator= (const AutoRefererLink& b);
};

struct Jump
{
    bool Is_Call;
    bool Is_C;
    bool Is_SET;
    
    //  Is_C Is_SET Is_Call
    //     x x      1       jsr
    //     0 0      0       bne
    //     0 1      0       beq
    //     1 0      0       bcc
    //     1 1      0       bcs

    AutoRefererLink target_ins;
    std::string target_label;
public:
    Jump()
    : Is_Call(false),
      Is_C(false),
      Is_SET(false)
    {
    }
    
    void Negate()
    {
        Is_SET = !Is_SET;
    }
};

struct CodeNode: public ptrable
{
    std::set<std::string> labels;
    std::set<std::string> important_labels;
    
    std::set<CodeNodePtr> referers;
    
    FlagState assumes;
    enum { t_undefined, t_code, t_jump } type;
    std::string code;
    
    Jump jump;
    
    AutoRefererLink next_ins;
    std::string     next_label;
    
    DefList defines;
public:
    CodeNode()
    : type(t_undefined)
    {
        next_ins.SetOwner(this);
        jump.target_ins.SetOwner(this);
    }
    
    ~CodeNode()
    {
        Terminate();
    }
    
    void SetCode(const std::string& s)
    {
        type = t_code;
        code = s;
    }
    
    void SetCall(const std::string& target)
    {
        type = t_jump;
        jump.Is_Call      = true;
        jump.target_label = target;
    }
    void SetJump(const std::string& op, const std::string& target)
    {
        type = t_jump;
        jump.Is_Call = false;
        jump.Is_C    = (op=="bcc" || op=="bcs");
        jump.Is_SET  = (op=="bcs" || op=="beq");
        jump.target_label = target;
    }
    
    void SetNextLabel(const std::string& s)
    {
        if(type == t_undefined)
        {
            type = t_code;
            code = "";
        }
        next_label = s;
    }
    
    void OptimizeWithNextNode(CodeNode* next)
    {
        if(type != t_jump) return;
        if(jump.Is_Call) return;
        
        if(jump.target_ins == next)
        {
            // Example:          Becomes:
            //   beq Y:bra X       bne X:bra Y
            // Y:                Y:
            // Where the "bra Y" will vanish to nonexistance.
            
            CodeNode *old_next = next_ins;
            
            next_ins    = NULL;
            jump.Negate();
            jump.target_ins   = old_next;
            jump.target_label = next_label;
            next_ins   = next;
            next_label = "";
        }
    }
    
    void KnownAs(const std::string& s, bool Keep)
    {
        labels.insert(s);
        if(Keep) important_labels.insert(s);
    }
    
    void KnowLabel(const std::string& s, CodeNode* ptr)
    {
        if(next_label == s)
        {
            next_label = "";
            next_ins   = ptr;
        }
        if(type == t_jump)
        {
            if(jump.target_label == s)
            {
                jump.target_label = "";
                jump.target_ins   = ptr;
            }
        }
    }
    
    bool IsFunctionallyEqual(const CodeNode& b, bool firstrun=true) const
    {
        /* NOT THREAD-SAFE! */
        /* This is for loop detection */
        typedef std::map<const CodeNode*, unsigned> loopdetectmap;
        static loopdetectmap left_done, right_done;
        if(firstrun)
        {
            left_done.clear();
            right_done.clear();
        }
        else
        {
            loopdetectmap::const_iterator
               L = left_done.find(this),
               R = right_done.find(&b);
            if(L != left_done.end())
            {
                if(R == right_done.end()) return false;
                return R->second == L->second;
            }
            if(R != right_done.end()) return false;
            left_done[this] = left_done.size();
            right_done[&b]  = right_done.size();
        }
        
        if(type != b.type) return false;
        
        if(!assumes.IsCompatible(b.assumes))
        {
            return false;
        }
        switch(type)
        {
            case t_code:
                if(code != b.code) return false;
                break;
            case t_jump:
            {
                if(!jump.target_ins != !b.jump.target_ins) return false;
                
                if(jump.target_ins)
                {
                    if(!jump.target_ins->IsFunctionallyEqual(*b.jump.target_ins, false))
                        return false;
                }
                else
                {
                    if(jump.target_label != b.jump.target_label)
                    {
                        return false;
                    }
                }
                break;
            }
            default:
                return false;
        }
        if(!next_ins != !b.next_ins) return false;
        if(next_ins)
        {
            if(!next_ins->IsFunctionallyEqual(*b.next_ins, false))
                return false;
        }
        return next_label == b.next_label;
    }
    
    void LabelLinks(CodeNode* ptr, const std::string& name)
    {
        if(next_ins == ptr) next_label = name;
        if(type == t_jump)
        {
            if(jump.target_ins == ptr) jump.target_label = name;
        }
    }
    
    void RedirectReferers(CodeNode* newlink)
    {
        typedef std::set<CodeNodePtr>::iterator ref_it;
        for(ref_it next, i = referers.begin();
                   i != referers.end();
                   i = next)
        {
            next = i; ++next;
            (*i)->UpdateLinks(this, newlink);
        }
        if(Dead()) Terminate();
    }
    
    void UpdateLinks(CodeNode* oldlink, CodeNode* newlink)
    {
/*
        std::printf("next_ins=%p, old=%p, new=%p\n",
            (const void *)next_ins,
            oldlink,
            newlink);
*/
        if(next_ins == oldlink)
        {
            next_ins = newlink;
        }
        
        if(type == t_jump)
        {
            if(jump.target_ins == oldlink)
            {
                jump.target_ins = newlink;
            }
        }
    }
    
    void Assimilate(CodeNode& b)
    {
        typedef std::set<std::string>::const_iterator lab_it;
        
        /* Assimilate labels */
        for(lab_it i = b.important_labels.begin();
                   i != b.important_labels.end();
                   ++i)
        {
            important_labels.insert(*i);
        }
        b.important_labels.clear();
        
        if(defines != b.defines)
        {
            defines.Assimilate(b.defines);
        }
        
        std::set<const CodeNode *> seen;
        CodeNode *l1=next_ins,  *l2=b.next_ins;
        while(l1 && l2)
        {
            if(seen.find(l1) != seen.end()) break;
            seen.insert(l1);
            
            l1->defines.Assimilate(l2->defines);
            
            l1 = l1->next_ins;
            l2 = l2->next_ins;
        }

        //code += ";assimilated("+b.code+")";
    }
    
    void AssimilateChain(AutoRefererLink& link)
    {
        std::list<CodeNodePtr> AssimilateList;
        while(link && link->IsNop()) 
        {
            //link->code += ";assimilated by ("+code+")";
            AssimilateList.push_back(link);
            if(!link->next_ins) break;
            
            CodeNodePtr next = link->next_ins;
            
            link->RedirectReferers(link->next_ins);
            
            link = next;
        }
        for(std::list<CodeNodePtr>::const_iterator
            i = AssimilateList.begin();
            i != AssimilateList.end();
            ++i)
        {
            link->Assimilate(**i);
        }
    }
    
    void ThreadJumps()
    {
        AssimilateChain(next_ins);
        if(type == t_jump) AssimilateChain(jump.target_ins);
        
        if(IsNop())
        {
            // oops, we are to be assimilated...
            // by whom?
            if(next_ins)
            {
                // haa!
                next_ins->Assimilate(*this);
            }
        }
    }
    
    bool IsNop() const
    {
        if(type == t_code && code[0] == ';') return true;
        return type == t_undefined;
    }
    
    bool Dead() const
    {
        return referers.empty()
            && important_labels.empty();
    }
    
    void Terminate()
    {
        // Ensure nobody will be referred by this one.
        
        /*
        std::fprintf(out, ";%p killed (", this);
        Dump();
        std::fprintf(out, ")\n");
        */
        
        next_ins        = NULL;
        jump.target_ins = NULL;
    }
    
    unsigned InsLen() const
    {
        if(code == "rts" || code == "asl"
        || code == "sec" || code == "clc") return 1;
        return 2; // good guess
    }
    
    void Dump()
    {
        typedef std::set<CodeNodePtr>::const_iterator ref_it;
        typedef std::set<std::string>::const_iterator lab_it;
        
        for(ref_it j = referers.begin();
                   j != referers.end();
                   ++j)
        {
            std::fprintf(out, "[%p]", (const void *)*j);
        }
        for(lab_it j = important_labels.begin();
                   j != important_labels.end();
                   ++j)
        {
            std::fprintf(out, "%s: ", j->c_str());
        }
        switch(type)
        {
            case CodeNode::t_code:
                std::fprintf(out, "%s", code.c_str());
                break;
            case CodeNode::t_jump:
            {
                const char *ins = "???";
                if(jump.Is_Call) ins = "jsr";
                else ins = jump.Is_C
                        ? jump.Is_SET ? "bcs" : "bcc"
                        : jump.Is_SET ? "beq" : "bne";
                std::fprintf(out, "%s ", ins);
                if(!jump.target_label.empty())
                    std::fprintf(out, "%s", jump.target_label.c_str());
                else
                    std::fprintf(out, "%p", (const void *)jump.target_ins);
                break;
            }
            default:
                break;
        }
        if(!next_label.empty())
            std::fprintf(out, " -> %s", next_label.c_str());
        else if(next_ins)
            std::fprintf(out, " -> %p", (const void *)next_ins);
    }
private:
    CodeNode(const CodeNode &b);
    void operator= (const CodeNode &b);
};

void AutoRefererLink::Unlink()
{
    CodeNode *link = *this;
    if(link)
    {    
        if(link->referers.find(owner)
        == link->referers.end())
        {
            std::fprintf(out, ";Unlink error\n");
        }
        link->referers.erase(owner);
    }
}
void AutoRefererLink::Link()
{
    CodeNode *link = *this;
    if(link)
    {
        if(link->referers.find(owner)
        != link->referers.end())
        {
            std::fprintf(out, ";Link error\n");
        }
        link->referers.insert(owner);
    }
}


static CodeNode* CurNode = NULL;
static std::list<CodeNodePtr> AllNodes;
static bool NodeFinished = false;
static std::set<std::string> KeepList;
static FlagState assumes;

static DefList defines;
static std::list<std::pair<std::string, bool> > defhistory;

static void GenerateRootNode()
{
    CurNode = new CodeNode;
    AllNodes.push_back(CurNode);
    NodeFinished = false;
}

static CodeNode* GetCurrentNode()
{
    if(!CurNode)
    {
        GenerateRootNode();
    }
    if(NodeFinished)
    {
        CodeNode* oldnode = CurNode;
        //if(CurNode) CurNode->assumes = assumes;
        
        GenerateRootNode();
        
        oldnode->next_ins = CurNode;
        
        NodeFinished = false;
    }
    
    if(CurNode) CurNode->assumes = assumes;
    if(CurNode) CurNode->defines = defines;
    return CurNode;
}

static void CloseChain()
{
    //if(CurNode) CurNode->assumes = assumes;
    CurNode = NULL;
}

static void FinishNode()
{
    if(CurNode && CurNode->type != CodeNode::t_undefined)
    {
        NodeFinished = true;
    }
}



static void CheckKeepList()
{
    typedef std::list<CodeNodePtr>::iterator vec_it;
    typedef std::set<std::string>::const_iterator lab_it;
    
    for(vec_it i = AllNodes.begin();
               i != AllNodes.end();
               ++i)
    {
        for(lab_it j = (*i)->labels.begin();
                   j != (*i)->labels.end();
                   ++j)
        {
            if(KeepList.find(*j) != KeepList.end())
                (*i)->important_labels.insert(*j);
        }
    }
    KeepList.clear();
}

static void LinkNodes()
{
    typedef std::list<CodeNodePtr>::iterator vec_it;
    typedef std::set<std::string>::const_iterator lab_it;
    
    for(vec_it i = AllNodes.begin();
               i != AllNodes.end();
               ++i)
    {
        for(lab_it j = (*i)->labels.begin();
                   j != (*i)->labels.end();
                   ++j)
        {
            for(vec_it k = AllNodes.begin();
                       k != AllNodes.end();
                       ++k)
            {
                (*k)->KnowLabel(*j, *i);
            }
        }
        (*i)->labels.clear();
    }
}

static void ThreadJumps()
{
    typedef std::list<CodeNodePtr>::iterator vec_it;
    for(vec_it i = AllNodes.begin();
               i != AllNodes.end();
               ++i)
    {
        (*i)->ThreadJumps();
    }
}

static void CombineNodes()
{
    typedef std::list<CodeNodePtr>::iterator vec_it;
    for(vec_it i = AllNodes.begin();
               i != AllNodes.end();
               ++i)
    {
        vec_it j = i; ++j;
        while(j != AllNodes.end())
        {
            vec_it next = j; ++next;
            
            unsigned chain_length = 0;
            const unsigned required_length = 3;
            
            CodeNode* chain = *i;
            std::set<CodeNode*> seen;
            
            /* This is slow. */
            while(chain)
            {
                if(seen.find(chain) != seen.end())
                {
                    // loop detected
                    chain_length += 10;
                    break;
                }
                seen.insert(chain);
                
                chain_length += chain->InsLen(); // average ins len
                if(chain_length >= required_length)
                {
                    // Ok it's long enough
                    break;
                }
                chain = chain->next_ins;
            }
            
            if(chain_length >= required_length
            && (*i)->IsFunctionallyEqual(**j))
            {
                /*
                std::printf("One combine - %p replaced with %p\n",
                    (const void *)*j,
                    (const void *)*i);
                */
                (*i)->Assimilate(**j);
                
                
                /* Everyone who linked to this instruction,
                 * is now urged to link to the new instruction
                 * instead.
                 */
                for(vec_it k = AllNodes.begin();
                           k != AllNodes.end();
                           ++k)
                {
                    (*k)->UpdateLinks(*j, *i);
                }
                
                (*j)->Terminate();
                
                AllNodes.erase(j);
            }
            j = next;
        }
    }
}

static void ReorderNodes()
{
    std::list<CodeNodePtr> code;
    unsigned top_offset = 0;
    
    while(!AllNodes.empty())
    {
        typedef std::list<CodeNodePtr>::iterator vec_it;
        typedef std::set<CodeNodePtr>::const_iterator ref_it;
        
        vec_it best_it = AllNodes.end();
        double bestscore = -1;

        for(vec_it i = AllNodes.begin();
                   i != AllNodes.end();
                   ++i)
        {
            const CodeNode& node = **i;
            
            /* Evaluate the reason for this to be here.
             */
             
            double score = 0;


            if(score > bestscore) { best_it = i; bestscore = score; }
        }
        code.push_back(*best_it);
        AllNodes.erase(best_it);
        ++top_offset;
    }
    AllNodes = code;
}
static void OptimizeWithNextNodes()
{
    typedef std::list<CodeNodePtr>::iterator vec_it;
    for(vec_it i = AllNodes.begin();
               i != AllNodes.end();
               ++i)
    {
        vec_it next = i; ++next;
        (*i)->OptimizeWithNextNode(*next);
    }
}

static void EliminateDeadInstructions()
{
    typedef std::list<CodeNodePtr>::iterator vec_it;
FindMore:
    for(vec_it i = AllNodes.begin();
               i != AllNodes.end();
               ++i)
    {
        CodeNode& node = **i;
        if(node.Dead())
        {
            // Dead, erase it.

            node.Terminate();
            AllNodes.erase(i);
            
            // Restart scanning, as this might have
            // created more dead statements.
            goto FindMore;
        }
    }
}

static void DumpNodes()
{
    typedef std::list<CodeNodePtr>::const_iterator vec_it;
    typedef std::set<CodeNodePtr>::const_iterator ref_it;
    typedef std::set<std::string>::const_iterator lab_it;

    std::fprintf(out, ";-------------\n");
    for(vec_it i = AllNodes.begin();
               i != AllNodes.end();
               ++i)
    {
        std::fprintf(out, ";%p ", (const void *)*i);
        (*i)->Dump();
        std::fprintf(out, "\n");
    }
    std::fprintf(out, ";-------------\n");
}

static void TidyDefines()
{
    typedef std::list<CodeNodePtr>::iterator vec_it;

    for(vec_it i = AllNodes.begin();
               i != AllNodes.end();
               ++i)
    {
        (*i)->defines.Tidy();
    }
}

static void AssignLabels()
{
    typedef std::list<CodeNodePtr>::const_iterator vec_it;
    typedef std::set<CodeNodePtr>::const_iterator ref_it;

    CodeNode *prev = NULL;
    for(vec_it i = AllNodes.begin();
               i != AllNodes.end();
               prev = *i++)
    {
        CodeNode& node = **i;

        if(node.referers.size() == 1
        && *node.referers.begin() == prev
        && prev->next_ins == &node)
        {
            // Only one referer.
            // And the only referer was the previous instruction.
            // No label needed
            continue;
        }

        if(node.important_labels.empty())
        {
            // Didn't have labels, create one.
            node.important_labels.insert(GenLabel());
        }
        
        const std::string& label = *node.important_labels.begin();
        
        for(ref_it j = node.referers.begin();
                   j != node.referers.end();
                   ++j)
        {
            if(*j != prev || prev->next_ins != &node)
            {
                (*j)->LabelLinks(&node, label);
            }
        }
    }
}

static void OutputNode(const CodeNode& node)
{
    typedef std::set<std::string>::const_iterator lab_it;
    
    if(node.defines != defines)
    {
        defines.End();
        defines = node.defines;
        defines.Begin();
        assumes.XY = UnknownXY;
        assumes.A  = UnknownA;
    }
    
    for(lab_it j = node.important_labels.begin();
               j != node.important_labels.end();
               ++j)
    {
        std::fprintf(out, "%s:\n", j->c_str());
    }
    
    std::fprintf(out, "\t");

    if(node.assumes.XY != assumes.XY)
        switch(node.assumes.XY)
        {
            case Assume8bitXY:
                assumes.XY = node.assumes.XY;
                std::fprintf(out, ".xs : ");
                break;
            case Assume16bitXY:
                assumes.XY = node.assumes.XY;
                std::fprintf(out, ".xl : ");
                break;
            case UnknownXY:
                // What here?
                break;
        }
    if(node.assumes.A != assumes.A)
        switch(node.assumes.A)
        {
            case Assume8bitA:
                assumes.A = node.assumes.A;
                std::fprintf(out, ".as : ");
                break;
            case Assume16bitA:
                assumes.A = node.assumes.A;
                std::fprintf(out, ".al : ");
                break;
            case UnknownA:
                // What here?
                break;
        }

    switch(node.type)
    {
        case CodeNode::t_code:
            std::fprintf(out, "%s", node.code.c_str());
            break;
        case CodeNode::t_jump:
        {
            const char *ins = "???";
            if(node.jump.Is_Call) ins = "jsr";
            else ins = node.jump.Is_C
                    ? node.jump.Is_SET ? "bcs" : "bcc"
                    : node.jump.Is_SET ? "beq" : "bne";
            std::fprintf(out, "%s %s", ins,
                node.jump.target_label.empty()
                ?    "???"
                :    node.jump.target_label.c_str());
            break;
        }
        default:
            break;
    }
    
    std::fprintf(out, "\n");
    
    if(!node.next_label.empty())
        std::fprintf(out, "\tbra %s\n", node.next_label.c_str());
}

static void OutputNodes()
{
    AssignLabels();
    
    assumes.XY = UnknownXY;
    assumes.A  = UnknownA;
    
    defines = DefList();
    defines.Begin();
    
    typedef std::list<CodeNodePtr>::const_iterator vec_it;
    for(vec_it i = AllNodes.begin();
               i != AllNodes.end();
               ++i)
    {
        vec_it next = i; ++next;
        OutputNode(**i);
    }
    
    defines.End();
}

void BeginCode(std::FILE *output)
{
    out = output ? output : stdout;
    
    CurNode = NULL;
    AllNodes.clear();
    NodeFinished = false;
    assumes.XY = UnknownXY;
    assumes.A  = UnknownA;
    
    defines = DefList();
    defhistory.clear();
}

void EndCode()
{
    while(!defhistory.empty()) EmitEndIfDef();
    
    FinishNode();
    CheckKeepList();
    CloseChain();
    LinkNodes();
#if COMBINE_NODES
    CombineNodes();
#endif
    ThreadJumps();
#if ELIMINATE_DEAD
    EliminateDeadInstructions();
#endif
#if REORDER_NODES
    ReorderNodes();
#endif
#if NEGATE_JUMPS
    OptimizeWithNextNodes();
#endif
    TidyDefines();
#if DEBUG_NODES
    DumpNodes();
#endif
    OutputNodes();
    
    //AllNodes.clear();
}


void Emit(const std::string& s, FlagWantedStateA A, FlagWantedStateXY XY)
{
    FinishNode(); // Finish the previous node.
    
    GetCurrentNode();
    
    std::string code = s;
    
    int sep = 0;
    int rep = 0;
    if(A == Want8bitA && assumes.A != Assume8bitA) { sep |= 0x20; assumes.A = Assume8bitA; }
    if(A ==Want16bitA && assumes.A !=Assume16bitA) { rep |= 0x20; assumes.A =Assume16bitA; }
    if(XY== Want8bitXY&& assumes.XY!= Assume8bitXY){ sep |= 0x10; assumes.XY= Assume8bitXY;}
    if(XY==Want16bitXY&& assumes.XY!=Assume16bitXY){ rep |= 0x10; assumes.XY=Assume16bitXY;}
    
    // sep and rep will be given separate nodes.
    // This could help the optimizer.
    if(sep != 0)
    {
        char Buf[32];
        sprintf(Buf, "sep #$%X", sep);
        CurNode->SetCode(Buf); FinishNode();
        GetCurrentNode();
    }
    if(rep != 0)
    {
        char Buf[32];
        sprintf(Buf, "rep #$%X", rep);
        CurNode->SetCode(Buf); FinishNode();
        GetCurrentNode();
    }
    
    CurNode->SetCode(code);
    
    if(code == "rts") { EmitBarrier(); }
}
void EmitLabel(const std::string& label)
{
    // Make sure this label will not be assigned to the PREVIOUS ins.
    FinishNode();
    
    GetCurrentNode()->KnownAs(label, false);
    CurNode->SetCode(";label");
}
void KeepLabel(const std::string& label)
{
    KeepList.insert(label);
}
void EmitBarrier()
{
    // Note that calling this function repetitively isn't an error,
    // and shouldn't increase memory usage.
    
    CloseChain();
}
void EmitBranch(const std::string& s, const std::string& target)
{
    if(s == "bra")
    {
        // No FinishNode here.
        // The followup label will be attached to the previous instruction.
        GetCurrentNode()->SetNextLabel(target);
        EmitBarrier();
    }
    else if(s == "jsr")
    {
        FinishNode(); // Finish the previous node.
        GetCurrentNode()->SetCall(target);
    }
    else
    {
        FinishNode(); // Finish the previous node.
        GetCurrentNode()->SetJump(s, target);
    }
}

const std::string GenLabel()
{
    static unsigned counter = 1;
    char Buf[64];
    sprintf(Buf, "L%u", counter++);
    return Buf;
}

void Assume(FlagAssumedStateA A, FlagAssumedStateXY XY)
{
    assumes.A  = A;
    assumes.XY = XY;
}

const FlagAssumption GetAssumption()
{
    return FlagAssumption(assumes.A, assumes.XY);
}

void EmitIfDef(const std::string& s)
{
    ++defines.Defines[s];
    defhistory.push_front(make_pair(s, true));
}
void EmitIfNDef(const std::string& s)
{
    ++defines.NonDefines[s];
    defhistory.push_front(make_pair(s, false));
}
void EmitEndIfDef()
{
    if(!defhistory.empty())
    {
        const std::string& s = defhistory.begin()->first;
        bool           isdef = defhistory.begin()->second;
        
        if(isdef)
            --defines.Defines[s];
        else
            --defines.NonDefines[s];
        
        defhistory.pop_front();
    }
}
