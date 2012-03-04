#include <cstdio>
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

#define SUPPORT_BITMASKTEST 0
#define PREFER_USING_SUB 1

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
        item.parent = a > 0              ? &items[a-1] : NULL;
        item.left   = NULL;
        item.right  = a+1 < items.size() ? &items[a+1] : NULL;
    }
}

void CaseGenerator::BalanceNodes(IntCaseItem** Head, IntCaseItem* Parent)
{
    IntCaseItem* np = *Head;
    if(np && np->low == 0)
    {
        BalanceNodes(&np->right, np);
        return;
    }
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
            {
                npp = &(*npp)->right;
            }
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

void CaseGenerator::SubtractTreeRecursively(
    IntCaseItem& head,
    CaseValue sub)
{
    head.low   -= sub;
    head.high  -= sub;
    if(head.left)  SubtractTreeRecursively(*head.left, sub);
    if(head.right) SubtractTreeRecursively(*head.right, sub);
}

void CaseGenerator::EmitCaseTree(IntCaseItem& node)
{
/*
    std::printf("; Case(%ld,%ld)", node.low, node.high);
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
    {
        EmitJump(node.target);
        return;
    }
    
#if PREFER_USING_SUB
    if(!node.left && node.low != 0 && node.IsRange())
    {
        EmitSubtract(node.low);
        SubtractTreeRecursively(node, node.low);
    }
#endif
    
    if(!node.IsRange())
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
    if(!items.empty())
    {
        InitializePointers();

        IntCaseItem* head = &items[0];
        BalanceNodes(&head, NULL);
        EmitCaseTree(*head);
    }
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

void CaseGenerator::AnalyzeItems(
    const std::vector<IntCaseItem>& items,
    CaseValue& minval,
    CaseValue& maxval,
    size_t&    n_comparisons)
{
    minval = items.front().low;
    maxval = items.back().high;
    n_comparisons = items.size();

    for(size_t a=0; a < items.size(); ++a)
        if(items[a].low != items[a].high)
            ++n_comparisons;
}

static bool IsDisconnectedBitset(unsigned bitmask)
{
    bool off=false, on=false;
    for(unsigned a=0; a<=15; ++a)
    {
        if(bitmask & (1 << a))
            { if(off) return true; on=true; }
        else
            { if(on) off=true; }
    }
    return false;
}

void CaseGenerator::OffsetItems(
    std::vector<IntCaseItem>& items,
    CaseValue offset)
{
    for(size_t a=0; a<items.size(); ++a)
    {
        items[a].low  = (items[a].low  - offset) & 255;
        items[a].high = (items[a].high - offset) & 255;
    }

    std::sort(items.begin(), items.end());
    for(size_t a=0; a<items.size(); ++a)
    {
        if(items[a].low > items[a].high)
        {
            IntCaseItem tmp(items[a]);
            tmp.low       = 0;
            items[a].high = 255;
            items.insert(items.begin(), tmp);
            --a;
        }
    }
    /*for(size_t a=0; a+1<items.size(); )
    {
        if(items[a].high+1 >= items[a+1].low
        && items[a].target == items[a+1].target)
        {
            items[a].high = items[a+1].high;
            items.erase(items.begin() + a+1);
        }
        else
            ++a;
    }*/
}

void CaseGenerator::Generate()
{
    size_t count;
    CaseValue minval, maxval;
    AnalyzeItems(items, minval,maxval, count);

#if SUPPORT_TABLES
    CaseValue range = maxval-minval;
  #if OPTIMIZE_SIZE
    size_t size_tree  = (3+2) * count;
    size_t size_table = (range+1)*2 + 14;//3+2;
    if(size_tree >= size_table)
  #else
    if(count >= CASE_VALUES_THRESHOLD
    && range <= 10*count
    && range >= 0)
  #endif
    {
        CreateTable(minval, range);
        return;
    }
#endif
    // Not created as a table.

#if SUPPORT_BITMASKTEST
    // Check if the test can be severely shortened by the
    // application of 16-bit tests.
    // Supported only if the dataset is 8-bit with wrap-around.
    if(GetMinValue() == 0 && GetMaxValue() == 255)
    {
        for(;;)
        {
            std::set<CaseLabel> distinct_targets;
            for(size_t a=0; a<items.size(); ++a)
                distinct_targets.insert(items[a].target);

            size_t cost_estimate_before = count*2;

            size_t   suggested_offset = 0;
            unsigned suggested_bitmask = 0;
            std::string suggested_target;

            for(std::set<CaseLabel>::const_iterator
                i = distinct_targets.begin();
                i != distinct_targets.end();
                ++i)
            {
                for(size_t offset = 0; offset < 256; ++offset)
                {
                    std::vector<IntCaseItem> modified_items(items);
                    OffsetItems(modified_items, offset);

                    unsigned bitmask = 0;
                    unsigned n_bits_set = 0;
                    for(size_t a=0; a<modified_items.size(); )
                    {
                        if(modified_items[a].target != *i) { ++a; continue; }
                        CaseValue mi = modified_items[a].low;
                        CaseValue ma = modified_items[a].high;

                        for(CaseValue b=0; b<=15; ++b)
                            if(b >= mi && b <= ma)
                            {
                                ++n_bits_set;
                                bitmask |= 1<<b;
                            }
                        bool remain = ma > 15;
                        if(!remain)
                            modified_items.erase(modified_items.begin() + a);
                        else
                            ++a;
                    }

                    if(n_bits_set > 0 && n_bits_set < 15
                    && ((offset==0&&!(bitmask&1))
                          || IsDisconnectedBitset(bitmask)))
                    {
                        size_t mod_count;
                        CaseValue mod_minval, mod_maxval;
                        AnalyzeItems(modified_items, mod_minval, mod_maxval, mod_count);
                        
                        size_t mod_cost_estimate = mod_count*2;
                        if(offset != 0) mod_cost_estimate += 2;
                        if(bitmask >= 0x100 || (bitmask & 1))
                        {
                            mod_cost_estimate += 2; // for 16-bit bitmask
                            if(mod_maxval >= 16)
                                mod_cost_estimate += 2;
                        }
                        else
                        {
                            mod_cost_estimate += 1; // for bitmask
                            if(mod_maxval >= 8)
                                mod_cost_estimate += 2;
                        }
                        
                        if(mod_cost_estimate <= cost_estimate_before)
                        {
                            suggested_offset  = offset;
                            suggested_bitmask = bitmask;
                            suggested_target  = *i;
                            cost_estimate_before = mod_cost_estimate;
                        }
                    }
                }
            }
            
            if(suggested_bitmask == 0) break;
            
            /*for(size_t a=0; a<items.size(); ++a)
            {
                std::printf("> <%s> %ld-%ld\n",
                    items[a].target.c_str(),
                    items[a].low, items[a].high);
            }*/

            if(suggested_offset != 0)
            {
                EmitSubtract(suggested_offset);
                OffsetItems(items, suggested_offset);
            }

            AnalyzeItems(items, minval,maxval, count);

            if(suggested_bitmask < 0x100)
                EmitTestBits8( suggested_bitmask, suggested_target, maxval < 8 );
            else
                EmitTestBits16( suggested_bitmask, suggested_target, maxval < 16 );
            std::fflush(stdout);

            for(size_t a=0; a<items.size(); )
            {
                /*std::printf("; <%s> %ld-%ld\n",
                    items[a].target.c_str(),
                    items[a].low, items[a].high);*/
                if(items[a].target != suggested_target) ++a;
                if(items[a].high <= 15)
                    items.erase(items.begin() + a);
                else
                {
                    if(items[a].low <= 15) items[a].low = 16;
                    ++a;
                }
            }
            std::sort(items.begin(), items.end());
            
            AnalyzeItems(items, minval,maxval, count);
        }
    }
#endif
#if PREFER_USING_SUB
    if(!items.empty() && minval != 0)
    {
        EmitSubtract(minval);
        OffsetItems(items, minval);
        Generate();
        return;
    }
#endif
    //std::printf("; Create tree %d\n", (int) items.size());
    CreateTree();
}

void CaseGenerator::Generate(const CaseItemList& source, const CaseLabel& over)
{
    defaultlabel = over;
    for(size_t a=0; a<source.size(); ++a)
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

            for(size_t b=0; b<items.size(); ++b)
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
            size_t newnum = items.size();
            for(size_t b=0; b<items.size(); )
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
