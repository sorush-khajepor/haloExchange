/* 
 * This example works with 2 cpus. The block below is replicated
 * on each processor. 
 *          
 *       element[ix][iy][iz] -> contguouity order: z, y then x
    	  ^ +-----+-----+-----+-----+-----+ 
 *        | |  3  |  7  |  11 |  15 |  19 |
 *        | +-----+-----+-----+-----+-----+ 
 *        | |  2  |  6  |  10 |  14 |  18 |      
 * dim[1] | +-----+-----+-----+-----+-----+
 *   ny   | |  1  |  5  | 9   |  13 |  17 | 
 *        | +-----+-----+-----+-----+-----+ 
 *        | |  0  |  4  | 8   |  12 |  16 |   
 *         +-----+-----+-----+-----+-----+
 *           0      1     2     3     4    
 *         ghost  first  ...   nx-2   nx-1(ghost)     
 *         +------ dim[0]=nx=9 --->          
 * 
 * The processor Id times 100 is added to the
 * values on each node. So 
 * node[0,1]=1 on processor 0
 * node[0,1]=101 on processor 1,... .  
 * 
 * In space the blocks are placed like
 * 
 * 
 * +-----+-----+
 * |  0  |  1  |  
 * +-----+-----+
 * 
 * There is only one communication between 0 and 1. There is no periodic condition. Ghost thickness
 * (overlap) is 1 node.
 * After one communication of blocks, the block 0 turns to
    3     7    11    15    19 
    2     6    10    14   106 
    1     5     9    13   105 
    0     4     8    12    16
 * And block 1 will be
  103   107   111   115   119 
   14   106   110   114   118 
   13   105   109   113   117 
  100   104   108   112   116 
*/

#include "halo.h"

int main(){
	MPI_Init(NULL, NULL);
	int rank,size;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);

	Block block;
    block.overlap=1;
    block.dims ={5,4};
	block.nodes.assign(20,0);
    for (int i=0;i<block.nodes.size();i++)
            block.nodes[i] = rank*100 + i;
        
	if (rank==0){
        block.neighbours.emplace_back(1, Dir::right);
	} else if (rank==1){
        block.neighbours.emplace_back(0, Dir::left);
    } else throw;

    block.CreateAllBoundaryArrays();

    for (int i=0;i<10000;i++)
        block.communicate();
    
	if (rank==1){
		block.print();
	}

	MPI_Finalize();
}
