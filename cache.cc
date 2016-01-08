/*******************************************************
                          cache.cc
                  Ahmad Samih & Yan Solihin
                           2009
                {aasamih,solihin}@ece.ncsu.edu
********************************************************/

#include <stdlib.h>
#include <assert.h>
#include <iostream>
#include <stdio.h>
#include "cache.h"
#include <iomanip>
#include <iostream>
#include <fstream>

using namespace std;

vector <Cache *> L1_caches;
vector <Cache *> L2_caches;

void Cache::back_invalidate(ulong addr,int proc)
{
	cacheLine *line = L1_caches[proc]->findLine(addr);
	if (line!=NULL)
	{
		line->setFlag(INVALID);
		L1_caches[proc]->back_invalidations++;
	}
}

void Cache::BusRd(ulong addr, int proc, int num_proc, vector <Cache *> v)
{
    for(int i=0; i<num_proc; i++)
    {
        if (i!=proc)
        {
            cacheLine *line_exists = v[i]->findLine(addr);
            if(line_exists!=NULL)
            {
                ulong flag = line_exists->getFlag();
                
                if (flag == MODIFIED || flag == EXCLUSIVE)
                {
                    line_exists->setFlag(SHARED);
                    v[i]->interventions++;

                    if (flag == MODIFIED)
                    {
                        v[i]->writeBack(addr);
                        v[i]->flushes++;
                    }
                }
            }
        }
    }
}

void Cache::BusRdX(ulong addr, int proc, int num_proc,vector <Cache *> v)
{
    for(int i=0; i<num_proc; i++)
    {
        if (i!=proc)
        {
            cacheLine *line_exists = v[i]->findLine(addr);
            if(line_exists!=NULL)
            {
                ulong flag = line_exists->getFlag();

                if (flag == MODIFIED)
                {
                    v[i]->writeBack(addr);
                    line_exists->setFlag(INVALID);
					back_invalidate(calcAddr4Tag(line_exists->getTag()),i);
                    v[i]->invalidations++;
                    v[i]->flushes++;
                }
                else if (flag == SHARED || flag == EXCLUSIVE)
                {
                    line_exists->setFlag(INVALID);
					back_invalidate(calcAddr4Tag(line_exists->getTag()),i);
                    v[i]->invalidations++;
                }
            }
        }
    }
}


void Cache::BusUpgr (ulong addr, int proc, int num_proc, vector <Cache *> v)
{
    for(int i=0; i<num_proc; i++)
    {
        if (i!=proc)
        {
            cacheLine *line_exists = v[i]->findLine(addr);
            if(line_exists!=NULL)
            {
                ulong flag = line_exists->getFlag();
                if (flag == SHARED)
                {
                    line_exists->setFlag(INVALID);
					back_invalidate(calcAddr4Tag(line_exists->getTag()),i);
                    v[i]->invalidations++;
                }
            }
        }
    }
}

bool Cache::copyExists (ulong addr, int proc, int num_proc, vector <Cache *> v)
{
    int ctr = 0;
    for(int i=0; i<num_proc; i++)
    {
        if (i!=proc)
        {
            cacheLine *line_exists = v[i]->findLine(addr);
            if(line_exists!=NULL)
            {
                ctr++;
                break;
            }
        }
    }

    if (ctr == 0)
        return false;
    else
        return true;

}

Cache::Cache(int s,int a,int b)
{
   ulong i, j;
   reads = readMisses = writes = 0;
   writeMisses = writeBacks = currentCycle = 0;
   mem_trans = interventions = invalidations = flushes = busrdX = cache2cache = 0;
   back_invalidations = fills = evictions = 0;

   size       = (ulong)(s);
   lineSize   = (ulong)(b);
   assoc      = (ulong)(a);
   sets       = (ulong)((s/b)/a);
   numLines   = (ulong)(s/b);
   log2Sets   = (ulong)(log2(sets));
   log2Blk    = (ulong)(log2(b));

   //*******************//
   //initialize your counters here//
   //*******************//

   tagMask =0;
   for(i=0;i<log2Sets;i++)
   {
		tagMask <<= 1;
        tagMask |= 1;
   }

    cache = NULL;
   /**create a two dimentional cache, sized as cache[sets][assoc]**/
   cache = new cacheLine*[sets];
   for(i=0; i<sets; i++)
   {
      cache[i] = NULL;
      cache[i] = new cacheLine[assoc];
      for(j=0; j<assoc; j++)
      {
	   cache[i][j].invalidate();
      }
   }
}

/**you might add other parameters to Access()
since this function is an entry point
to the memory hierarchy (i.e. caches)**/
void Cache::L2_Access(ulong addr,uchar op,int proc,vector <Cache *> &v)
{
	currentCycle++;/*per cache global counter to maintain LRU order
			among cache ways, updated on every cache access*/
    int num_proc = v.size();

	if(op == 'w')
        writes++;
	else
        reads++;

	cacheLine * line = findLine(addr);

	if(line == NULL)/*miss*/
	{
		cacheLine *newline = fillLine(addr,2,proc);
		
        if(op == 'w') // WRITEMISS
        {
            writeMisses++;
            if (copyExists(addr, proc, num_proc, v))
				cache2cache++;
            busrdX++;
            BusRdX(addr, proc, num_proc, v);
            newline->setFlag(MODIFIED);         
                
        }
		else          // READMISS
        {
            readMisses++;
            if(copyExists(addr, proc, num_proc, v))
            {
                cache2cache++;
                newline->setFlag(SHARED);
            }
            else
				newline->setFlag(EXCLUSIVE);
                
            BusRd(addr, proc, num_proc, v);
        }
    }

	else //HIT
	{
		/**since it's a hit, update LRU and update dirty flag**/
		updateLRU(line);
		if(op == 'w') //WRITE HIT
        {
            ulong flag = line->getFlag();

            if (flag == SHARED)
            {
                line->setFlag(MODIFIED);
                BusUpgr(addr, proc, num_proc,v);
            }
            else if (flag == EXCLUSIVE)
                 line->setFlag(MODIFIED);                

        }
	}
}

int Cache::L1_Access(ulong addr, uchar op, int proc, vector <Cache *> &v)
{
	currentCycle++;/*per cache global counter to maintain LRU order
			among cache ways, updated on every cache access*/

	if(op == 'w')
        writes++;
	else
        reads++;

	cacheLine * line = findLine(addr);

	if(line == NULL)/*miss*/
	{
        if(op == 'w') // WRITEMISS
        {
            writeMisses++; 
			return 1;
        }
		else          // READMISS
        {
			cacheLine *newline = fillLine(addr,1,proc);
            readMisses++;
            newline->setFlag(SHARED);
			fills++;
			return 0;
        }
    }
	else //HIT
	{
		/**since it's a hit, update LRU and update dirty flag**/
		updateLRU(line);
		if(op == 'w') //WRITE HIT
			return 2;
		else 
			return 3;
	}
}

/*look up line*/
cacheLine * Cache::findLine(ulong addr)
{
   ulong i, j, tag, pos;
   pos = assoc;
   tag = calcTag(addr);
   i   = calcIndex(addr);

   for(j=0; j<assoc; j++)
   {
      if(cache[i][j].isValid())
        if(cache[i][j].getTag() == tag)
		{
		     pos = j; break;
		}
   }

   if(pos == assoc)
	return NULL;
   else
	return &(cache[i][pos]);
}

/*upgrade LRU line to be MRU line*/
void Cache::updateLRU(cacheLine *line)
{
  line->setSeq(currentCycle);
}

/*return an invalid line as LRU, if any, otherwise return LRU line*/
cacheLine * Cache::getLRU(ulong addr)
{
   ulong i, j, victim, min;

   victim = assoc;
   min    = currentCycle;
   i      = calcIndex(addr);

   for(j=0;j<assoc;j++)
   {
      if(cache[i][j].isValid() == 0)
        return &(cache[i][j]);
   }
   for(j=0;j<assoc;j++)
   {
	 if(cache[i][j].getSeq() <= min)
     {
        victim = j;
        min = cache[i][j].getSeq();
     }
   }
   assert(victim != assoc);
   evictions++;
   
   return &(cache[i][victim]);
}

/*find a victim, move it to MRU position*/
cacheLine *Cache::findLineToReplace(ulong addr)
{
   cacheLine * victim = getLRU(addr);
   updateLRU(victim);

   return (victim);
}

/*allocate a new line*/
cacheLine *Cache::fillLine(ulong addr, int level, int proc)
{
   ulong tag;
   cacheLine *victim = findLineToReplace(addr);
   
   if (level == 2)
	   if (victim->isValid() == 1)
		   back_invalidate(calcAddr4Tag(victim->getTag()),proc);
	   
		   
	

   assert(victim != 0);
   if(victim->getFlag() == MODIFIED)
      writeBack(addr);

   tag = calcTag(addr);
   victim->setTag(tag);
   victim->setFlag(INVALID);

   /**note that this cache line has been already
      upgraded to MRU in the previous function (findLineToReplace)**/

   return victim;
}

void Cache::printStats(int i,int level)
{
	switch(level)
	{
		case 1:
		{
			cout<<"============ Simulation results L1 Cache(Processor "<<i<<") ============\n";
			cout<<" 01. number of reads:      "<<reads<<"\n 02. number of read misses:      "<<readMisses;
			cout<<"\n 03. number of writes:      "<<writes<<"\n 04. number of write misses:      "<<writeMisses;
			cout<<"\n 05. total miss rate:      "<<fixed <<setprecision(2) <<(float)((readMisses+writeMisses)*100)/(reads+writes)<<"%";
			cout<<"\n 06. number of back invalidations:      "<<back_invalidations;
			cout<<"\n 07. number of fills:      "<<fills;
			cout<<"\n 08. number of evictions:      "<<evictions<<endl;
			break;
		}
		case 2:
		{
			cout<<"============ Simulation results L2 Cache(Processor "<<i<<") ============\n";
			cout<<" 01. number of reads:      "<<reads<<"\n 02. number of read misses:      "<<readMisses;
			cout<<"\n 03. number of writes:      "<<writes<<"\n 04. number of write misses:      "<<writeMisses;
			cout<<"\n 05. total miss rate:      "<<fixed <<setprecision(2) <<(float)((readMisses+writeMisses)*100)/(reads+writes)<<"%";
			cout<<"\n 06. number of writebacks:      "<<writeBacks;
			cout<<"\n 07. number of cache-to-cache transfers:     "<<cache2cache;
			cout<<"\n 08. number of memory transactions:       "<<readMisses+writeMisses+writeBacks-cache2cache;
			cout<<"\n 09. number of interventions:      "<<interventions;
			cout<<"\n 10. number of invalidations:      "<<invalidations;
			cout<<"\n 11. number of flushes:      "<<flushes;
			cout<<"\n 12. number of evictions:      "<<evictions<<endl;
		}
	}	
	/****print out the rest of statistics here.****/
	/****follow the ouput file format**************/
}

