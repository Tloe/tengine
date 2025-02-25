#include "arena.h"
#include "render.h"

int main() {
  auto a = INIT_ARENA(100000);
  auto renderer = render::init(&a);
  render::update(renderer, &a);
  render::cleanup(renderer);

  return 0;
}
