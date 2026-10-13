set -e

cmake -B build -DCMAKE_BUILD_TYPE=Release -DOPT_LEVEL=3
cmake --build build

cmake -B build_op0 -DCMAKE_BUILD_TYPE=Release -DOPT_LEVEL=0
cmake --build build_op0

cmake -B build_op3 -DCMAKE_BUILD_TYPE=Release -DOPT_LEVEL=3
cmake --build build_op3