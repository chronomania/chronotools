#include <vector>
#include <iostream>
#include <cstdarg>
#include <algorithm>
#include <ctime>

using namespace std;

#define NOWHERE ((unsigned)-1)

#include "binpacker.hh"

const vector<unsigned> ArrayToVector(unsigned n, ...)
{
    vector<unsigned> result(n);
    va_list ap; va_start(ap, n);
    for(unsigned a=0; a<n; ++a) result[a] = va_arg(ap, unsigned);
    return result;
}

static bool Tester()
{
    vector<unsigned> holes;
    vector<unsigned> items;
    unsigned holecount = 1 + rand()%20;
    for(unsigned a=0; a<holecount; ++a)
    {
        unsigned size = 10 + rand()%5000;
        holes.push_back(size);
        while(size > 0)
        {
            unsigned itemsize = 1 + rand()%size;
            if(itemsize >= size) itemsize = size;
            if(!(rand() % 1000))break;
            items.push_back(itemsize);
            size -= itemsize;
        }
    }
    random_shuffle(holes.begin(), holes.end());
    random_shuffle(items.begin(), items.end());
    
    const vector<unsigned> result = PackBins(holes, items);
    
    unsigned errors = 0;
    
    if(result.size() != items.size())
    {
        cerr << "ERROR: Eeek\n";
        ++errors;
    }
    
    for(unsigned a=0; a<items.size(); ++a)
    {
        unsigned hole = result[a];
        if(holes[hole] < items[a])
        {
            cerr << "ERROR: Hole " << hole << " has no space!\n";
            ++errors;
            continue;
        }
        holes[hole] -= items[a];
    }
    return errors;
}

static void StressTest()
{
    //srand(time(0));
    while(!Tester())
    {
        cout << "\n---------------\n";
    }
}

int main(void)
{
    //StressTest();

    vector<unsigned> Holes = ArrayToVector(2,   200, 190);
    vector<unsigned> Items = ArrayToVector(6,   180, 20, 50, 50, 50, 40);
    
    cerr << Holes.size() << " holes, "
         << Items.size() << " items\n";
    
    PackBins(Holes, Items);
    
    return 0;
}
