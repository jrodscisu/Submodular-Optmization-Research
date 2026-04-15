@0x84928e7acd17ba2f;

struct GraphHeader {
       tag @0 : Text;
       numNodes @1 : UInt32;
       numEdges @2 : UInt64;
}

struct Edge {
       from @0 : UInt32;
       to :union {
          node @1 :UInt32;
          list @3 :List(UInt32);
       }
       weight :union {
          value @2 :Float32;
          list @4 :List(Float32);
       }
}