from conan import ConanFile
from conan.tools.cmake import CMakeToolchain, CMake, cmake_layout

class AmbaTlmRecipe(ConanFile):
    name = "amba-tlm"
    version = "1.0"

    # Optional metadata
    license = ""
    author = ""
    url = "http://gitlab.marqueesemi.com:8081/sw-tools/amba-tlm"
    description = "The Arm AMBA Transaction-Level Modeling (TLM) library allows you to model and simulate approximately-timed (AT) and cycle-accurate (CA) AXI4 and ACE ports"
    topics = ("", "", "")

    # Binary configuration
    settings = "os", "compiler", "build_type", "arch"

    options = {
        "shared": [True, False],
        "fPIC": [True, False]
    }

    default_options = {
        "shared": False,
        "fPIC": True
    }

    # Sources are located in the same place as this recipe, copy them to the recipe
    exports_sources = "CMakeLists.txt", "src/*", "include/*"

    def config_options(self):
        if self.settings.os == "Windows":
            del self.options.fPIC

    def layout(self):
        cmake_layout(self)

    def generate(self):
        tc = CMakeToolchain(self)
        tc.generate()

    def requirements(self):
        pass

    def build_requirements(self):
        self.tool_requires("cmake/3.27.6")

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()

    def package_info(self):
        self.cpp_info.libs = ["armtlmaxi4"]