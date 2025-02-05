#include <render.h>


void cleanup() {
}

int main() {
  te::render::init();
  te::render::update();
  te::render::cleanup();

  return 0;
}
