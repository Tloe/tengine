# TEngine

Fresh start with focus on Data Oriented Design.
Old TEngine can be found in TEngineOld


### SDL3

Install SDL3, SDL_TTF, SDL_IMAGE

#### this should be added with conan once available in conan center

cd external/SDL3-3.2.2
cmake -S . -B build
cmake --build build
sudo cmake --install build --prefix /usr/local

### Build server (CI)


