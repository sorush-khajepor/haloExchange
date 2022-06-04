#include <vector>
#include <array>
#include <mpi.h>
#include <iostream>
#include <iomanip>
#include <chrono>
#include <thread>

enum Dir
{
    right,
    top,
    left,
    bottom
};
int oppositeDir[4]{left, bottom, right, top};
enum Axis
{
    x,
    y
};
Axis DirToAxis[4]{x, y, x, y};
Axis GetNormalAxis[2]{y, x};
std::array<Dir, 2> GetDirsOfAxis[2]{{left, right}, {bottom, top}};

struct Box
{
    std::array<int, 2> low;
    std::array<int, 2> high;
    Box(const std::array<int, 2> &low_, const std::array<int, 2> &high_) : low(low_), high(high_) {}
    Box(int ix0, int ix1, int iy0, int iy1) : low({ix0, iy0}), high({ix1, iy1}) {}
    std::array<int, 2> GetExtent() const
    {
        return std::array<int, 2>{high[0] - low[0] + 1, high[1] - low[1] + 1};
    }
    Box Shrink(int thickness, Axis axis)
    {
        return Box{low[0] + thickness * (1 - axis), high[0] - thickness * (1 - axis),
                   low[1] + thickness * axis, high[1] - thickness * axis};
    }
    Box Shrink(int thickness)
    {
        return Shrink(thickness, Axis::x).Shrink(thickness, Axis::y);
    }
    Box GetBoundary(Dir dir, int thickness)
    {
        int t1 = thickness - 1;
        if (dir == Dir::right)
            return Box{high[0] - t1, high[0], low[1], high[1]};
        else if (dir == Dir::left)
            return Box{low[0], low[0] + t1, low[1], high[1]};
        else if (dir == Dir::top)
            return Box{low[0], high[0], high[1] - t1, high[1]};
        else if (dir == Dir::bottom)
            return Box{low[0], high[0], low[1], low[1] + t1};
        else
            throw;
    }
    Box GetNoCornerBoundary(Dir dir, int thickness)
    {
        return Shrink(thickness, GetNormalAxis[DirToAxis[dir]]).GetBoundary(dir, thickness);
    }
    Box GetExtendedBoundary(Dir dir, int thickness)
    {
        return Shrink(-thickness, GetNormalAxis[DirToAxis[dir]]).GetBoundary(dir, thickness);
    }
    void print()
    {
        std::cout << low[0] << "," << low[1] << "\n";
        std::cout << high[0] << "," << high[1] << "\n";
    }
};

struct Block
{
    std::vector<double> nodes;
    std::array<int, 2> dims;
    int overlap = 1;
    std::vector<std::tuple<int, Dir>> neighbours;
    MPI_Datatype boundaryMpiType[4];
    MPI_Datatype ghostMpiType[4];
    std::vector<MPI_Request> sendRequests;
    std::vector<MPI_Request> recvRequests;

    auto cartToIndex(int ix, int iy)
    {
        return iy + ix * dims[1];
    }
    auto &Get(std::integral auto... ix)
    {
        return nodes[cartToIndex(ix...)];
    }
    auto toString(const Box &box)
    {
        auto extent = box.GetExtent();
        std::ostringstream s;
        for (int j = box.high[1]; j > box.low[1] - 1; j--)
        {
            s << "\n";
            for (int i = box.low[0]; i < box.high[0] + 1; i++)
            {
                s << std::setw(5) << Get(i, j) << " ";
            }
        }
        s << "\niy↑  ix-> \n";
        return s.str();
    }
    auto print(const Box &box)
    {
        std::cout << toString(box);
    }
    auto print()
    {
        std::cout << toString(GetDomainBox());
    }
    auto communicate(Dir dir, int dest)
    {
        MPI_Request recvRequest;
        MPI_Request sendRequest;

        MPI_Irecv(&nodes[0], 1, ghostMpiType[dir], dest, oppositeDir[dir], MPI_COMM_WORLD, &recvRequest);
        MPI_Isend(&nodes[0], 1, boundaryMpiType[dir], dest, dir, MPI_COMM_WORLD, &sendRequest);
        return std::array<MPI_Request, 2>{sendRequest, recvRequest};
    }

    void communicate(Axis axis)
    {
        for (auto &neighbour : neighbours)
        {
            const auto &[cpuId, dir] = neighbour;
            if (DirToAxis[dir] == axis)
            {
                auto [sendReq, recvReq] = communicate(dir, cpuId);
                sendRequests.emplace_back(sendReq);
                recvRequests.emplace_back(recvReq);
            }
        }

        for (auto &r : recvRequests)
        {
            MPI_Status s;
            MPI_Wait(&r, &s);
        }
        recvRequests.clear();

        if (axis == Axis::y)
        {
            for (auto &r : sendRequests)
            {
                MPI_Status s;
                MPI_Wait(&r, &s);
            }
            sendRequests.clear();
        }
    }
    void communicate()
    {
        communicate(Axis::x);
        communicate(Axis::y);
    }

    auto SetBoundaryMpiType(const Box &box, MPI_Datatype &type)
    {
        auto size = box.GetExtent();
        MPI_Type_create_subarray(2, dims.data(), size.data(), box.low.data(),
                                 MPI_ORDER_C, MPI_DOUBLE, &type);
        MPI_Type_commit(&type);
    }
    Box GetGhostBox()
    {
        return Box{0, dims[0] - 1, 0, dims[1] - 1};
    }
    Box GetDomainBox()
    {
        return GetGhostBox().Shrink(overlap);
    }
    auto CreateAllBoundaryArrays()
    {
        auto domain = GetDomainBox();
        SetBoundaryMpiType(domain.GetBoundary(Dir::right, overlap), boundaryMpiType[Dir::right]);
        SetBoundaryMpiType(domain.GetBoundary(Dir::left, overlap), boundaryMpiType[Dir::left]);
        SetBoundaryMpiType(domain.GetExtendedBoundary(Dir::top, overlap), boundaryMpiType[Dir::top]);
        SetBoundaryMpiType(domain.GetExtendedBoundary(Dir::bottom, overlap), boundaryMpiType[Dir::bottom]);

        auto ghost = GetGhostBox();
        SetBoundaryMpiType(ghost.GetNoCornerBoundary(Dir::right, overlap), ghostMpiType[Dir::right]);
        SetBoundaryMpiType(ghost.GetNoCornerBoundary(Dir::left, overlap), ghostMpiType[Dir::left]);
        SetBoundaryMpiType(ghost.GetBoundary(Dir::top, overlap), ghostMpiType[Dir::top]);
        SetBoundaryMpiType(ghost.GetBoundary(Dir::bottom, overlap), ghostMpiType[Dir::bottom]);
    }
};
