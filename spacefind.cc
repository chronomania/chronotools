#include <vector>
#include <iostream>
#include <cstdarg>
#include <algorithm>
#include <ctime>

using namespace std;

#define NOWHERE ((unsigned)-1)

#include "organizer.hh"

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
    
    map<unsigned, unsigned> result;
    Organizer(holes, items, result);
    
    unsigned errors = 0;
    
    for(unsigned a=0; a<items.size(); ++a)
    {
        map<unsigned, unsigned>::const_iterator i;
        i = result.find(a);
        if(i == result.end())
        {
            cerr << "ERROR: Item " << a << " nowhere!\n";
            ++errors;
            continue;
        }
        unsigned hole = i->second;
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
    
    map<unsigned, unsigned> result;
    
    Organizer(Holes, Items, result);
    
    return 0;
}
