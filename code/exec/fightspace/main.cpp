#include <render.h>

int main() {
  render::init();
  render::update();
  render::cleanup();

  return 0;
}
