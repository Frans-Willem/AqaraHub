from conans import ConanFile, CMake

class AqaraHubConan(ConanFile):
    name = "AqaraHub"
    version = "0.1"
    generators = "cmake"
    settings = { "os": None,
            "compiler": {"clang":{"version":["5.0"], "libcxx":["libc++"]}},
            "arch": None
            }
    requires = "Boost/1.62.0@lasote/stable"
    default_options = "Boost:shared=False", "Boost:without_iostreams=True"

    def build(self):
        cmake = CMake(self)
        self.run("cmake %s %s" % (self.conanfile_directory, cmake.command_line))
        self.run("cmake --build . %s" % (cmake.build_config))
