extern crate docopt;
extern crate rustc_serialize;
extern crate capngraph;
extern crate petgraph;

use std::io::{Write, BufWriter};
use std::fs::File;

use capngraph::load_graph;
use docopt::Docopt;
use petgraph::visit::EdgeRef;

const USAGE: &'static str = "
Convert the input packed binary graph to the edge-list format.

Usage:
  deconvert <bin> <dest>
  deconvert (-h | --help)

Options:
  -h --help        Show this screen.
";

#[derive(RustcDecodable)]
// #[derive(serde:Deserialize)]
struct Args {
    arg_bin: String,
    arg_dest: String,
}

fn main() {
    let args: Args = Docopt::new(USAGE)
        .and_then(|d| d.decode())
        .unwrap_or_else(|e| e.exit());

    let graph = load_graph(args.arg_bin.as_str()).unwrap();

    let output = File::create(args.arg_dest).unwrap();
    let mut writer = BufWriter::new(output);

    writeln!(writer, "{} {}", graph.node_count(), graph.edge_count()).unwrap();

    for edge in graph.edge_references() {
        writeln!(writer,
                 "{} {} {}",
                 edge.source().index(),
                 edge.target().index(),
                 f32::from(*edge.weight()))
            .unwrap();
    }
}
