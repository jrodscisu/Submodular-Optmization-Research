# extension of graph.capnp to a case with additional node data.
@0xaed9fb729c576ca5;

struct WeightedDirectedGraph {
    tag @0 : Text;
    numNodes @1 : UInt32;

    struct Edge {
        from @0 : UInt32;
        to @1 : UInt32;
        weight @2 : Float32;
    }

    struct Node {
        cost @0 : Float32 = 1;
        benefit @1 : Float32 = 1;
    }

    edges @2 : List(Edge);
    nodes @3 : List(Node);
}
