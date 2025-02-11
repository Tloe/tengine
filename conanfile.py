from conan import ConanFile
from conan.tools.cmake import CMake, CMakeToolchain, CMakeDeps

class TEngine(ConanFile):
    name = "tengine"
    version = "0.1.0"
    license = "Copyright (c) Terje Loe 2012-2021. Commercial. All rights reserved, may not be used without a valid license."
    url = "<!-- <Package recipe repository url here, for issues about the package> -->"
    description = "Game engine"
    settings = "os", "compiler", "build_type", "arch"
    # "assimp/5.0.1" removed for now for shorter ci runtime
    options = {"shared": [True, False]}
    default_options = {"shared": False}

    def layout(self):
        self.folders.build = "build"

    def requirements(self):
        self.requires("glm/1.0.1")

    def build_requirements(self):
        self.test_requires("catch2/3.8.0")

    def generate(self):
        tc = CMakeToolchain(self)
        tc.generate()
        deps = CMakeDeps(self)
        deps.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()
