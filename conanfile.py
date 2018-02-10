#!/usr/bin/env python
# -*- coding: utf-8 -*-

from conans import ConanFile, CMake, tools


class LowIOConan(ConanFile):
    name = "lowio"
    version = "1.0.0"
    license = "MIT"
    url = "https://github.com/objectx/lowio"
    description = "Cross platform handle based I/O"
    options = {"shared": [True, False]}
    settings = "os", "compiler", "build_type", "arch"
    build_requires = "boost_filesystem/1.66.0@bincrafters/stable", "catch2/2.1.1@bincrafters/stable"
    default_options = "shared=False"
    generators = "cmake"
    exports = "LICENSE"
    exports_sources = "CMakeLists.txt", "include/*", "src/*", "test/*"

    def configure(self):
        if self.options.shared:
            self.options["boost_filesystem"].shared = True
        else:
            self.options["boost_filesystem"].shared = False

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()
        cmake.test()

    def package(self):
        self.copy("LICENSE", dst="licenses")
        self.copy("*.hpp", dst="include", src="include")
        self.copy("*lowio.lib", dst="lib", keep_path=False)
        self.copy("*.dll", dst="bin", keep_path=False)
        self.copy("*.so", dst="lib", keep_path=False)
        self.copy("*.dylib", dst="lib", keep_path=False)
        self.copy("*.a", dst="lib", keep_path=False)

    def package_info(self):
        self.cpp_info.libs = tools.collect_libs(self)
