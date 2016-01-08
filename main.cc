/*******************************************************
                          main.cc
                  Ahmad Samih & Yan Solihin
                           2009
                {aasamih,solihin}@ece.ncsu.edu
********************************************************/

#include <cstdlib>
#include <assert.h>
#include <fstream>
#include <algorithm>
#include <vector>
#include <string>
#include <sstream>

using namespace std;

#include "cache.h"

int main(int argc, char *argv[])
{

	if(argv[1] == NULL){
		 printf("input format: ");
		 printf("./smp_cache <L1_cache_size> <L2_cache_size> <L1_assoc> <L2_assoc> <block_size> <trace_file> \n");
		 exit(0);
        }

	/*****uncomment the next five lines*****/
	int L1_cache_size = atoi(argv[1]);
	int L2_cache_size = atoi(argv[2]);
	int L1_assoc= atoi(argv[3]);
	int L2_assoc= atoi(argv[4]);
	int blk_size   = atoi(argv[5]);
	//int num_processors = atoi(argv[4]);/*1, 2, 4, 8*/
	//int protocol   = atoi(argv[5]);	 /*0:MSI, 1:MESI, 2:Dragon*/
	char *fname =  (char *)malloc(20);
 	fname = argv[6];
 	char op;

   ifstream fin;
   FILE * pFile;

	    //***************************************************//
    cout<<"===== 506 Personal Information ====="<<endl;
    cout<<"Ujan Sengupta"<<endl;
    cout<<"usengup"<<endl;
    cout<<"NO"<<endl;

	//****************************************************//
	//**printf("===== Simulator configuration =====\n");**//
	//*******print out simulator configuration here*******//
	//****************************************************//
	cout<<"===== 506 SMP Simulator configuration ====="<<endl;
    cout<<"L1_SIZE: "<<L1_cache_size<<endl;
    cout<<"L2_SIZE: "<<L2_cache_size<<endl;
    cout<<"L1_ASSOC: "<<L1_assoc<<endl;
    cout<<"L2_ASSOC: "<<L2_assoc<<endl;
    cout<<"BLOCKSIZE: "<<blk_size<<endl;
    cout<<"TRACE FILE: "<<argv[6]<<endl;


	//*********************************************//
        //*****create an array of caches here**********//
	//*********************************************//
	
	for(int i=0; i<4; i++)
        L1_caches.push_back(new Cache(L1_cache_size,L1_assoc,blk_size));

	for(int i=0; i<4; i++)
        L2_caches.push_back(new Cache(L2_cache_size,L2_assoc,blk_size));
    

	pFile = fopen (fname,"r"); //fname

	if(pFile == 0)
	{
		printf("Trace file problem\n");
		exit(0);
	}
	///******************************************************************//
	//**read trace file,line by line,each(processor#,operation,address)**//
	//*****propagate each request down through memory hierarchy**********//
	//*****by calling cachesArray[processor#]->Access(...)***************//
	///******************************************************************//

	fin.open(fname, ios::in); //fname
	string line;

	while (getline(fin,line))
	{
		ulong addr_in_hex;
		int proc;
		int request;

        proc = line[0] - '0';
		if (proc<0 || proc>8)
            break;
        op = line[2];
        string address = line.substr(4);

            
        istringstream buffer (address);
        buffer >> hex >> addr_in_hex;

        request = L1_caches[proc]->L1_Access(addr_in_hex,op,proc,L1_caches);   /* Return: 0 (Read Miss), 1 (Write Miss), 2 (Write Hit) 3 (Read Hit)*/
		if (request != 3)
			L2_caches[proc]->L2_Access(addr_in_hex,op,proc,L2_caches);
	}

	fclose(pFile);

	//********************************//
	//print out all caches' statistics //
	//********************************//

	for(int i=0; i<4; i++)
    {
        L1_caches[i]->printStats(i,1);
        L2_caches[i]->printStats(i,2);
    }

}
