extern crate docopt;
extern crate rustc_serialize;
extern crate capngraph;
extern crate petgraph;
extern crate serde_json;
#[macro_use]
extern crate serde_derive;
extern crate colored;

use capngraph::load_graph;
use docopt::Docopt;
use colored::*;

const USAGE: &'static str = "
Describe a given graph. Provides node and edge counts.

Usage:
    describe-graph <graph> [--json]
    describe-graph (-h | --help)

Options:
    -h --help       Show this screen.
    --json          Output results as JSON.
";

#[derive(RustcDecodable)]
// #[derive(serde:Deserialize)]
struct Args {
    arg_graph: String,
    flag_json: bool,
}

#[derive(Serialize, Debug)]
struct Description {
    nodes: usize,
    edges: usize,
    islands: usize,
}

fn main() {
    let args: Args = Docopt::new(USAGE)
        .and_then(|d| d.decode())
        .unwrap_or_else(|e| e.exit());

    let graph = load_graph(args.arg_graph.as_str()).unwrap();

    let desc = Description {
        nodes: graph.node_count(),
        edges: graph.edge_count(),
        islands: graph.node_indices().filter(|&node| graph.neighbors(node).count() == 0).count(),
    };

    if args.flag_json {
        println!("{}", serde_json::to_string(&desc).unwrap());
    } else {
        println!("{} {}", "Nodes:".bold(), desc.nodes);
        println!("{} {}", "Edges:".bold(), desc.edges);
        println!("{} {}", "Islands:".bold(), desc.islands);
    }
}
