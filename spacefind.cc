#include <map>
#include <set>
#include <vector>
#include <iostream>
#include <cstdarg>

using namespace std;

#define NOWHERE ((unsigned)-1)

// This function organizes items to holes
// so that the following goals are met:
//  1) a hole is selected for all items
//  2) hole may contain multiple items
//  3) total size of items in one hole
//     may not exceed the size of hole
//
// If possible, this goal is desirable:
//  4) free space is gathered together instead
//     of being scattered in different holes
class Organizer
{
public:
	Organizer
	(
	   // Input: List of holes. Value = hole size
	   const vector<unsigned> &Holes,
	   // Input: List of items. Value = item size
	   const vector<unsigned> &Items,
	   // Result: Key = item number, value = hole number.
	   map<unsigned, unsigned> &Result
	) : holes(Holes.size()),
	    items(Items.size())
    {
    	for(unsigned a=0; a<Items.size(); ++a)
    	{
    		items[a].index    = a;
    		items[a].size     = Items[a];
    		items[a].location = NOWHERE;
    	}
    	for(unsigned a=0; a<Holes.size(); ++a)
    	{
    		holes[a].size     = Holes[a];
    		holes[a].free     = Holes[a];
    	}
    	Shuffle();
    	
    	Dump();
    	
    	Result.clear();
    	for(unsigned a=0; a<items.size(); ++a)
    		Result[items[a].index] = items[a].location;
    }
private:
	struct hole
	{
		int size;
		int free;
		set<unsigned> contents;
	};
	struct item
	{
		unsigned index;
		int size;
		unsigned location;
		
		bool operator< (const item &b) const { return size > b.size; }
	};
	vector<hole> holes;
	vector<item> items;
	
	void MoveItem(unsigned itemno, unsigned holeno)
	{
		if(holeno == items[itemno].location) return;
		unsigned oldhole = items[itemno].location;
		if(oldhole != NOWHERE)
		{
			holes[oldhole].free += items[itemno].size;
			holes[oldhole].contents.erase(itemno);
		}
		items[itemno].location = holeno;
		if(holeno != NOWHERE)
		{
			holes[holeno].free -= items[itemno].size;
			holes[holeno].contents.insert(itemno);
		}
	}
	int GetFreeSpace(unsigned holeno) const { return holes[holeno].free; }
	int GetHoleSize(unsigned holeno) const {  return holes[holeno].size; }
	int GetItemSize(unsigned itemno) const {  return items[itemno].size; }
	unsigned GetItemLocation(unsigned itemno) const { return items[itemno].location; }
	
	void Shuffle()
	{
		sort(items.begin(), items.end());
		
		for(unsigned a=0; a<items.size(); ++a)
		{
			int leastfree=0; unsigned besthole=0; bool first=true;
			for(unsigned b=0; b<holes.size(); ++b)
				if(holes[b].free >= items[a].size
				&& (first || holes[b].free < leastfree)
				  )
				{
					first = false;
					besthole = b;
					leastfree = holes[b].free;
				}
			
			MoveItem(a, besthole);
		}
	}
	void Dump() const
	{
		for(unsigned a=0; a<holes.size(); ++a)
		{
			cerr << "Hole " << a << ": ";
			cerr << "size=" << GetHoleSize(a)
			     << ", free=" << GetFreeSpace(a)
			     << "\n   Items:";
			for(set<unsigned>::const_iterator
			    i = holes[a].contents.begin();
			    i != holes[a].contents.end();
			    ++i)
			{
			    cerr << ' ' << items[*i].index
			         << '(' << GetItemSize(*i) << ')';
			}
			cerr << endl;
		}
	}
};


const vector<unsigned> ArrayToVector(unsigned n, ...)
{
    vector<unsigned> result(n);
    va_list ap; va_start(ap, n);
    for(unsigned a=0; a<n; ++a) result[a] = va_arg(ap, unsigned);
    return result;
}

int main(void)
{
    vector<unsigned> Holes = ArrayToVector(2,   200, 190);
    vector<unsigned> Items = ArrayToVector(6,   180, 20, 50, 50, 50, 40);
    
    cerr << Holes.size() << " holes, "
         << Items.size() << " items\n";
    
    map<unsigned, unsigned> result;
    
    Organizer(Holes, Items, result);
    
    return 0;
}
