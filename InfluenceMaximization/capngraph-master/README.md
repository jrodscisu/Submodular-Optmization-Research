# The Cap'n's Graph

This repository contains a [Cap'n Proto](https://capnproto.org/)
schema for graphs, along with tools for working with this format.

*Status*: Schema written, converter from edge-list format for the
particular implementation I need done.

**Installing the Binaries:**
```bash
cargo install --git https://github.com/emallson/capngraph.git --features bins
```

Statically-linked release binaries are also available on Github.

**Linking to the Library:**
```toml
[dependencies]
capngraph = { version = "0.3.1", git = "https://github.com/emallson/capngraph.git" }
```

## Installation

Unlike most rust libraries, this one has external dependencies. To compile any project using it, you need Cap'n Proto installed. On a Debian-like system, this can be done with

    sudo apt-get install libcapnp-dev capnproto

## Motivation

Do you know how many undocumented graph formats there are in the wild?
Go count them; I don't expect you back before I finish my doctorate.

Most of them follow a pretty simple convention: first line is some
metadata (typically number of nodes / number of edges or a comment)
followed by one line for each edge in the graph. This *seems*
straightforward, except for all the subtle ways that they vary (e.g.
some allow comments, some don't). For one recent project, I *manually*
converted several different formats to a single uniform one. Not
pleasant.

This is compounded with the fact that these files are horribly large
for large datasets: 2-4x larger than necessary since the numbers are
textual (1 byte per digit) instead of binary (4 bytes per number).
They also take an inordinate amount of time to read in text format: a
50GB dataset takes almost 15m to parse into a graph in one
implementation, while a (custom, undocumented, ugly) binary
implementation takes under a minute.<sup>*</sup>

What *I* want is a binary format that is documented, easy-to-parse,
and extensible. Cap'n Proto (successor to Protobuf) fits the bill.

<sup>*</sup> I haven't done formal benchmarks on this and probably
won't. It is readily apparent that reading more data takes longer,
which is magnified by the additional parsing and converting necessary
for the text format.

## License

Copyright (c) 2016-2018, J. David Smith
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the
   distribution.

3. Neither the name of the copyright holder nor the names of its
   contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
