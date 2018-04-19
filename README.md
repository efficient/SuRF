# SuRF
The Succinct Range Filter (SuRF) is a fast and compact filter that
provides exact-match filtering, range filtering, and approximate
range counts. This is the source code for our
[SIGMOD paper](http://www.cs.cmu.edu/~huanche1/publications/surf_paper.pdf).

## Build
    git submodule init
    git submodule update
    mkdir build
    cd build
    cmake ..
    make -j

## Test
    make test

## License
    Copyright 2018, Carnegie Mellon University

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
