#include "casegen.hh"
#include <algorithm>

#define CASE_VALUES_THRESHOLD 6

/* Set this to nonzero if you want the last EQ-compare
 * to be NE-compare instead. It results in slightly
 * better code if the case target is immediately
 * after the switchcase code.
 */
#define USE_LAST_NE        1

/* Set this to nonzero if you
 * like GT comparisons more
 * than LT comparisons.
 */
#define GT_BETTER_THAN_LT        1

/* Set this to nonzero if you
 * like GT&GE comparisons more
 * than LT&LE comparisons.
 */
#define GT_GE_BETTER_THAN_LT_LE  0

#define OPTIMIZE_SIZE  0
#define SUPPORT_TABLES 0

bool CaseGenerator::IntCaseItem::HasLowerBound(const CaseGenerator& g) const
{
    if(low <= g.GetMinValue()) return true;
    if(left) return false;
    
    CaseValue low_minus_one = low - 1;
    if(low_minus_one >= low) return false;
    
    for(struct IntCaseItem *pnode = parent; pnode; pnode = pnode->parent)
        if(low_minus_one == pnode->high)
            return true;
        
    return false;
}

bool CaseGenerator::IntCaseItem::HasUpperBound(const CaseGenerator& g) const
{
    if(high >= g.GetMaxValue()) return true;
    if(right) return false;
    
    CaseValue high_plus_one = high + 1;
    if(high_plus_one <= high) return false;
    
    for(struct IntCaseItem *pnode = parent; pnode; pnode = pnode->parent)
        if(high_plus_one == pnode->low)
            return true;
    
    return false;
}

bool CaseGenerator::IntCaseItem::IsBounded(const CaseGenerator& g) const
{
    return HasLowerBound(g) && HasUpperBound(g);
}

void CaseGenerator::InitializePointers()
{
    for(unsigned a=0; a<items.size(); ++a)
    {
        IntCaseItem& item = items[a];
        
        //std::printf("%ld,%ld\n", item.low, item.high);
        
        item.parent = NULL;
        item.left   = NULL;
        item.right  = NULL;
        if(a > 0)
        {
            IntCaseItem& prev = items[a-1];
            
            item.parent = &prev;
            prev.right  = &item;
        }
    }
}

void CaseGenerator::BalanceNodes(IntCaseItem** Head, IntCaseItem* Parent)
{
    IntCaseItem* np = *Head;
    if(np)
    {
        int i = 0, ranges = 0;
        IntCaseItem** npp;
        IntCaseItem* Left;
        while(np)
        {
            if(np->IsRange()) { ++ranges; }
            ++i;
            np = np->right;
        }
        if(i > 2)
        {
            npp  = Head;
            Left = *npp;
            if(i == 3)
                npp = &(*npp)->right;
            else
            {
                i = (i + ranges + 1) / 2;
                for(;;)
                {
                    if((*npp)->IsRange()) --i;
                    if(--i <= 0) break;
                    npp = &(*npp)->right;
                }
            }
            *Head = np = *npp;
            *npp  = NULL;
            np->parent = Parent;
            np->left   = Left;
            BalanceNodes(&np->left,  np);
            BalanceNodes(&np->right, np);
        }
        else
        {
            np         = *Head;
            np->parent = Parent;
            for(; np->right; np = np->right)
                np->right->parent = np;
        }
    }
}

void CaseGenerator::EmitLastEQlow(const IntCaseItem& node)
{
#if USE_LAST_NE
    EmitCompareNE(node.low, defaultlabel);
    EmitJump(node.target);
#else
    EmitCompareEQ(node.low, node.target);
#endif
}

void CaseGenerator::EmitCaseTree(const IntCaseItem& node)
{
/*
    std::printf("Case(%ld,%ld)", node.low, node.high);
    if(node.left)
    {
        std::printf(" - left(%ld,%ld)", node.left->low, node.left->high);
        bool lb = node.left->HasLowerBound(*this);
        bool ub = node.left->HasUpperBound(*this);
        if(lb && ub)std::printf("(bounded)");
        else if(lb)std::printf("(lbound)");
        else if(ub)std::printf("(ubound)");
    }
    if(node.right)
    {
        std::printf(" - right(%ld,%ld)", node.right->low, node.right->high);
        bool lb = node.right->HasLowerBound(*this);
        bool ub = node.right->HasUpperBound(*this);
        if(lb && ub)std::printf("(bounded)");
        else if(lb)std::printf("(lbound)");
        else if(ub)std::printf("(ubound)");
    }
    std::printf("\n");
*/
    if(node.IsBounded(*this))
        EmitJump(node.target);
    else if(!node.IsRange())
    {
        // single value.
        if(node.right && node.left)
        {
            EmitCompareEQ(node.low, node.target);

            bool rb = node.right->IsBounded(*this);
            bool lb = node.left->IsBounded(*this);
            
            if(rb && lb)
            {
                if(node.right->target == node.left->target)
                    EmitJump(node.right->target);
                else
                {
                    if(GT_BETTER_THAN_LT)
                    {
                        EmitCompareGT(node.high, node.right->target);
                        EmitJump(node.left->target);
                    }
                    else
                    {
                        EmitCompareLT(node.low, node.left->target);
                        EmitJump(node.right->target);
                    }
                }
            }
            else if(rb)
            {
                EmitCompareGT(node.high, node.right->target);
                EmitCaseTree(*node.left);
            }
            else if(lb)
            {
                EmitCompareLT(node.low, node.left->target);
                EmitCaseTree(*node.right);
            }
            else
            {
                if(GT_BETTER_THAN_LT) /* If GT better than LT */
                {
                    std::string test_label = EmitBlock();
                    EmitCompareGT(node.high, test_label);
                    EmitCaseTree(*node.left);
                    EmitJump(defaultlabel); // if necessary
                    EmitEndBlock(test_label);
                    EmitCaseTree(*node.right);
                }
                else
                {
                    std::string test_label = EmitBlock();
                    EmitCompareLT(node.low, test_label);
                    EmitCaseTree(*node.right);
                    EmitJump(defaultlabel); // if necessary
                    EmitEndBlock(test_label);
                    EmitCaseTree(*node.left);
                }
            }
        }
        else if(node.right)
        {
            EmitCompareEQ(node.low, node.target);

            if(node.right->right
            || node.right->left
            || node.right->IsRange())
            {
                if(!node.HasLowerBound(*this))
                    EmitCompareLT(node.high, defaultlabel);
            }
            EmitCaseTree(*node.right);
        }
        else if(node.left)
        {
            EmitCompareEQ(node.low, node.target);

            if(node.left->left
            || node.left->right
            || node.left->IsRange())
            {
                if(!node.HasUpperBound(*this))
                    EmitCompareGT(node.high, defaultlabel);
            }
            EmitCaseTree(*node.left);
        }
        else
        {
            EmitLastEQlow(node);
        }
    }
    else
    {
        // range.
        if(node.right && node.left)
        {
            std::string test_label;
            
            bool rb = node.right->IsBounded(*this);
            bool lb = node.left->IsBounded(*this);
            
            if(rb && lb)
            {
                EmitCompareGT(node.high, node.right->target);
                EmitCompareLT(node.low,  node.left->target);
            }
            else if(rb)
            {
                EmitCompareGT(node.high, node.right->target);
                EmitCompareGE(node.low, node.target);
                EmitCaseTree(*node.left);
            }
            else if(lb)
            {
                EmitCompareLT(node.low, node.left->target);
                EmitCompareLE(node.high, node.target);
                EmitCaseTree(*node.right);
            }
            else
            {
                if(GT_GE_BETTER_THAN_LT_LE) /* If GT/GE better than LT/LE */
                {
                    std::string test_label = EmitBlock();
                    EmitCompareGT(node.high, test_label);
                    EmitCompareGE(node.low, node.target);
                    EmitCaseTree(*node.left);
                    EmitJump(defaultlabel);
                    EmitEndBlock(test_label);
                    EmitCaseTree(*node.right);
                }
                else
                {
                    std::string test_label = EmitBlock();
                    EmitCompareLT(node.low, test_label);
                    EmitCompareLE(node.high, node.target);
                    EmitCaseTree(*node.right);
                    EmitJump(defaultlabel);
                    EmitEndBlock(test_label);
                    EmitCaseTree(*node.left);
                }
            }
        }
        else if(node.right)
        {
            if(!node.HasLowerBound(*this))
                EmitCompareLT(node.low, defaultlabel);
            EmitCompareLE(node.high, node.target);
            EmitCaseTree(*node.right);
        }
        else if(node.left)
        {
            if(!node.HasUpperBound(*this))
                EmitCompareGT(node.high, defaultlabel);
            EmitCompareGE(node.low, node.target);
            EmitCaseTree(*node.left);
        }
        else
        {
            // range: no children
            bool upb = node.HasUpperBound(*this);
            bool lob = node.HasLowerBound(*this);
            if(!upb && lob)
                EmitCompareGT(node.high, defaultlabel);
            else if(!lob && upb)
                EmitCompareLT(node.low, defaultlabel);
            else if(!lob && !upb)
            {
                EmitSubtract(node.low);
                EmitCompareGT(node.high - node.low, defaultlabel);
            }
        }
        EmitJump(node.target);
    }
}

void CaseGenerator::CreateTree()
{
    InitializePointers();
    IntCaseItem* head = &items[0];
    BalanceNodes(&head, NULL);
    EmitCaseTree(*head);
    EmitJump(defaultlabel);
}

void CaseGenerator::CreateTable(CaseValue minval, CaseValue range)
{
    if(minval != 0) EmitSubtract(minval);
    
    EmitCompareGT(range, defaultlabel);
    
    //printf("min=%ld, range=%ld\n", minval, range);
    std::vector<std::string> table(range + 1);
    for(unsigned a=0; a<items.size(); ++a)
        for(CaseValue n = items[a].low; n <= items[a].high; ++n)
            table[n-minval] = items[a].target;
    for(unsigned a=0; a<table.size(); ++a)
        if(table[a].empty())
            table[a] = defaultlabel;

    EmitJumpTable(table);
}

void CaseGenerator::Generate()
{
    CaseValue minval=0, maxval=0;
    unsigned count = items.size();
    for(unsigned a=0; a < items.size(); ++a)
    {
        if(!a)
        {
            minval = items[a].low;
            maxval = items[a].high;
        }
        else
        {
            if(items[a].low < minval) minval = items[a].low;
            if(items[a].high > maxval) maxval = items[a].high;
        }
        if(items[a].low != items[a].high) ++count;
    }
    CaseValue range = maxval-minval;
    
    unsigned size_tree  = (3+2) * count;
    unsigned size_table = (range+1)*2 + 3+2;

#if SUPPORT_TABLES
#if OPTIMIZE_SIZE
    if(size_tree < size_table)
#else
    if(count < CASE_VALUES_THRESHOLD
    || range > 10*count
    || range < 0)
#endif
#endif
    {
        CreateTree();
    }
#if SUPPORT_TABLES
    else
    {
        CreateTable(minval, range);
    }
#endif
}

void CaseGenerator::Generate(const CaseItemList& source, const CaseLabel& over)
{
    defaultlabel = over;
    for(unsigned a=0; a<source.size(); ++a)
    {
        const CaseItem& item = source[a];
        std::set<CaseValue>::const_iterator i;
        for(i=item.values.begin(); i!=item.values.end(); ++i)
        {
            //printf("Pondering %ld (%s)\n", *i, item.target.c_str());
            if(*i == GetDefaultCase())
            {
                defaultlabel = item.target;
            NextValue:
                continue;
            }

            for(unsigned b=0; b<items.size(); ++b)
                if(items[b].Includes(*i))
                {
                    if(items[b].target != item.target)
                    {
                        fprintf(stderr, "Error: Key %ld conflicts\n", *i);
                    }
                    goto NextValue;
                }
            /* None of them included this one */
            
            CaseValue new_low  = *i;
            CaseValue new_high = *i;
            unsigned newnum = items.size();
            for(unsigned b=0; b<items.size(); )
            {
                if(items[b].target != item.target)
                {
                    /* Wrong target, don't assimilate */
                DontAssimilate:
                    ++b;
                    continue;
                }
                
                bool assimilate = false;
                if(*i == items[b].low - 1)
                {
                    assimilate = true;
                    if(items[b].high > new_high) new_high = items[b].high;
                }
                if(*i == items[b].high + 1)
                {
                    assimilate = true;
                    if(items[b].low < new_low) new_low = items[b].low;
                }
                if(!assimilate) goto DontAssimilate;
                if(b < newnum)
                    newnum = b++;
                else
                    items.erase(items.begin() + b);
            }
            if(newnum == items.size())
            {
                IntCaseItem new_item;
                items.push_back(new_item);
            }
            items[newnum].low    = new_low;
            items[newnum].high   = new_high;
            items[newnum].target = item.target;
        }
    }
    std::sort(items.begin(), items.end());
    Generate();
}

CaseGenerator::~CaseGenerator()
{
}