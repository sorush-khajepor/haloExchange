/* 
 * This example works with 4 cpus. The block below is replicated
 * on each processor. 
 *          
 *       element[ix][iy][iz] -> contguouity order: z, y then x
 *       ^ +-----+-----+-----+-----+-----+-----+-----+-----+-----+ 
 *       | |  7  |  15 |  23 |  31 |  39 | 47  | 55  | 63  |  71 |
 *       | +-----+-----+-----+-----+-----+-----+-----+-----+-----+ 
 *       | |  6  |  14 |  22 |  30 |  38 | 46  | 54  | 62  |  70 |  
         | +-----+-----+-----+-----+-----+-----+-----+-----+-----+ 
 *       | |  5  |  13 |  21 |  29 |  37 | 45  | 53  | 61  |  69 |
 *       | +-----+-----+-----+-----+-----+-----+-----+-----+-----+ 
 *       | |  4  |  12 |  20 |  28 |  36 | 44  | 52  | 60  |  68 |  
    	 | +-----+-----+-----+-----+-----+-----+-----+-----+-----+ 
 *       | |  3  |  11 |  19 |  27 |  35 | 43  | 51  | 59  | 67  |
 *       | +-----+-----+-----+-----+-----+-----+-----+-----+-----+ 
 *       | |  2  |  10 |  18 |  26 |  34 | 42  | 50  | 58  | 66  |      
 * dim[1]| +-----+-----+-----+-----+-----+-----+-----+-----+-----+
 *   ny  | |  1  |  9  | 17  |  25 |  33 | 41  | 49  | 57  | 65  | 
 *       | +-----+-----+-----+-----+-----+-----+-----+-----+-----+ 
 *       | |  0  |  8  | 16  |  24 |  32 | 40  | 48  | 56  | 64  | 
 *         +-----+-----+-----+-----+-----+-----+-----+-----+-----+
 *           0      1    2     3     4      5     6     7     8  
 *         ghost  ghost  first  ...                    nx-2   nx-1     
 *         +------ dim[0]=nx=9 --->          
 * 
 * The processor Id times 100 is added to the
 * values on each node. So 
 * node[0,1]=1 on processor 0
 * node[0,1]=101 on processor 1,... .  
 * 
 * In space the blocks are placed like
 * 
 * +-----+-----+
 * |  2  |  3  | 
 * +-----+-----+
 * |  0  |  1  |  
 * +-----+-----+
 * 
 * They are periodic in all directions and the ghost layer thickness (overlap) is 2 nodes.
 * After one communication of blocks, the block 0 turns to (starred ones are inner nodes, not ghosts): 
  343   351   219   227   235   243   251   319   327 
  342   350   218   226   234   242   250   318   326 
  145   153   *21   *29   *37   *45   *53   121   129 
  144   152   *20   *28   *36   *44   *52   120   128 
  143   151   *19   *27   *35   *43   *51   119   127 
  142   150   *18   *26   *34   *42   *50   118   126 
  345   353   221   229   237   245   253   321   329 
  344   352   220   228   236   244   252   320   328 
 * 

mpic++ example1.cpp -std=c++20
 run with
 mpirun -np 4 xterm -hold -e ./a.out
*/

#include "halo.h"

int main(){
	MPI_Init(NULL, NULL);
	int rank,size;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);

	Block block;
    block.overlap=2;
    block.dims ={9,8};
	block.nodes.assign(72,0);
    for (int i=0;i<block.nodes.size();i++)
            block.nodes[i] = rank*100 + i;
        
	if (rank==0){
        block.neighbours.emplace_back(1, Dir::right);
        block.neighbours.emplace_back(1, Dir::left);
        block.neighbours.emplace_back(2, Dir::bottom);
        block.neighbours.emplace_back(2, Dir::top);
	} else if (rank==1){
        block.neighbours.emplace_back(0, Dir::right);
        block.neighbours.emplace_back(0, Dir::left);
        block.neighbours.emplace_back(3, Dir::top);
        block.neighbours.emplace_back(3, Dir::bottom);
    }
    else if (rank==2){
        block.neighbours.emplace_back(0, Dir::bottom);
        block.neighbours.emplace_back(0, Dir::top);
        block.neighbours.emplace_back(3, Dir::left);
        block.neighbours.emplace_back(3, Dir::right);
    }
    else if (rank==3){
        block.neighbours.emplace_back(2, Dir::left);
        block.neighbours.emplace_back(2, Dir::right);
        block.neighbours.emplace_back(1, Dir::bottom);
        block.neighbours.emplace_back(1, Dir::top);
    }

    block.CreateAllBoundaryArrays();

    std::cout<<"Rank="<<rank<<"\n";
    std::cout<<"Initial state:\n";
	block.print(block.GetGhostBox());

    for (int i=0;i<10000;i++)
        block.communicate();
    
	std::cout<<"Final state:\n";
	block.print(block.GetGhostBox());

	MPI_Finalize();
}
