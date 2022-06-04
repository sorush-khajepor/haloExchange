/*
 * This example works with 4 cpus. The block below is replicated
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
 + +-----+-----+
 * |  2  |  3  |
 * +-----+-----+
 * |  0  |  1  |
 * +-----+-----+
 *
 * There is periodic condition. Ghost thickness
 * (overlap) is 1 node.
*/

#include "halo.h"

int main()
{
    MPI_Init(NULL, NULL);
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    Block block;
    block.overlap = 1;
    block.dims = {5, 4};
    block.nodes.assign(20, 0);

    if (rank == 0)
    {
        block.Get(1, 1) = 1.0;
        block.neighbours.emplace_back(1, Dir::right);
        block.neighbours.emplace_back(1, Dir::left);
        block.neighbours.emplace_back(2, Dir::bottom);
        block.neighbours.emplace_back(2, Dir::top);
    }
    else if (rank == 1)
    {
        block.neighbours.emplace_back(0, Dir::right);
        block.neighbours.emplace_back(0, Dir::left);
        block.neighbours.emplace_back(3, Dir::top);
        block.neighbours.emplace_back(3, Dir::bottom);
    }
    else if (rank == 2)
    {
        block.neighbours.emplace_back(0, Dir::bottom);
        block.neighbours.emplace_back(0, Dir::top);
        block.neighbours.emplace_back(3, Dir::left);
        block.neighbours.emplace_back(3, Dir::right);
    }
    else if (rank == 3)
    {
        block.neighbours.emplace_back(2, Dir::left);
        block.neighbours.emplace_back(2, Dir::right);
        block.neighbours.emplace_back(1, Dir::bottom);
        block.neighbours.emplace_back(1, Dir::top);
    }
    else
        throw;

    block.CreateAllBoundaryArrays();
     std::cout << "rank=" << rank << "\n";
    for (int loop = 0; loop < 5; loop++)
    {
        block.communicate();

        for (size_t i = 1; i < block.dims[0]; i++)
            for (size_t j = 1; j < block.dims[1]; j++)
                if (block.Get(i, j) > 0.1)
                {
                    block.Get(i, j) = 0;
                    block.Get(i - 1, j - 1) = 1;
                }

        std::cout<<"Step="<<loop<<"\n";
        block.print();
        std::cout<<"------------------\n";
    }

    MPI_Finalize();
}
