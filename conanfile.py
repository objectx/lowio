from conans import ConanFile, CMake


class LowIOConan(ConanFile):

    name = "LowIO"
    version = "1.0"
    license = "MIT"
    url = "https://github.com/objectx/lowio"
    description = "Cross platform handle based I/O"
    options = {"shared": [True, False]}
    settings = "os", "compiler", "build_type", "arch"
    build_requires = "boost_filesystem/1.66.0@bincrafters/stable", "catch2/2.1.1@bincrafters/stable"
    default_options = "shared=False", "boost_filesystem:shared=False"
    generators = "cmake", "gcc", "txt"
    exports_sources = "CMakeLists.txt", "include/*", "src/*", "test/*"

    def imports(self):
        self.copy("*.dll", dst="bin", src="bin")        # From bin to bin
        self.copy("*.dylib*", dst="bin", src="lib")     # From lib to bin

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()
        cmake.test()

    def package(self):
        self.copy("*.hpp", dst="include", src="include")
        self.copy("*lowio.lib", dst="lib", keep_path=False)
        self.copy("*.dll", dst="bin", keep_path=False)
        self.copy("*.so", dst="lib", keep_path=False)
        self.copy("*.dylib", dst="lib", keep_path=False)
        self.copy("*.a", dst="lib", keep_path=False)

    def package_info(self):
        self.cpp_info.libs = ["lowio"]
