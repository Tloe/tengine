# TEngine

Fresh start with focus on Data Oriented Design.
Old TEngine can be found in TEngineOld

### clang

Build clang with these options:
cmake -DLLVM_ENABLE_PROJECTS="clang;lldb;clang-tools-extra;compiler-rt" -DCMAKE_BUILD_TYPE=Release -G "Unix Makefiles" ../llvm


### SDL3

Install SDL3, SDL_TTF, SDL_IMAGE

#### this should be added with conan once available in conan center

cd external/SDL3-3.2.2
cmake -S . -B build
cmake --build build
sudo cmake --install build --prefix /usr/local

### Build server (CI)


