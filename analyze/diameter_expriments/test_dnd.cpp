#include "DNDTree.hpp"


int main(int argc, char* argv[]) {
    uint32_t n = 1000;

    IDTree idtree(n);
    for (uint32_t i = 0; i < n-1; i++) {
        idtree.InsertEdge(i, i+1);
        if (!idtree.IsConnected(i, i+1)) {
            std::cerr << "IDTree query incorrect after insertion." << std::endl;
            return 1;
        }
    }
    for (uint32_t i = 0; i < n-1; i++) {
        idtree.DeleteEdge(i, i+1);
        if (idtree.IsConnected(i, i+1)) {
            std::cerr << "IDTree query incorrect after deletion." << std::endl;
            return 1;
        }
    }

    DNDTree dndtree(n);
    for (uint32_t i = 0; i < n-1; i++) {
        dndtree.InsertEdge(i, i+1);
        if (!dndtree.IsConnected(i, i+1)) {
            std::cerr << "DNDTree query incorrect after insertion." << std::endl;
            return 1;
        }
    }
    for (uint32_t i = 0; i < n-1; i++) {
        dndtree.DeleteEdge(i, i+1);
        if (dndtree.IsConnected(i, i+1)) {
            std::cerr << "DNDTree query incorrect after deletion." << std::endl;
            return 1;
        }
    }

    return 0;
}
