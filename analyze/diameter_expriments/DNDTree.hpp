#include "DynamicCC.h"


class IDTree {
private:
    DynamicCC tree;
public:
    IDTree(uint32_t n) : tree(n, false) { tree.init(); }
    void InsertEdge(uint32_t u, uint32_t v) { tree.insert_edge(u, v); }
    void DeleteEdge(uint32_t u, uint32_t v) { tree.delete_edge(u, v); }
    bool IsConnected(uint32_t u, uint32_t v) { return tree.query(u, v); }
};

class DNDTree {
private:
    DynamicCC tree;
public:
    DNDTree(uint32_t n) : tree(n, true) { tree.init(); }
    void InsertEdge(uint32_t u, uint32_t v) { tree.insert_edge(u, v); }
    void DeleteEdge(uint32_t u, uint32_t v) { tree.delete_edge(u, v); }
    bool IsConnected(uint32_t u, uint32_t v) { return tree.query(u, v); }
};
