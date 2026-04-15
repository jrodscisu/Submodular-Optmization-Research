extern crate capnpc;

fn main() {
    capnpc::CompilerCommand::new()
        .src_prefix("schemas")
        .file("schemas/graph.capnp")
        .run()
        .expect("schema compiler command");
}
