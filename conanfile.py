from conan import ConanFile
from conan.tools.cmake import CMakeToolchain, CMake, CMakeDeps, cmake_layout

class AmbaTlmRecipe(ConanFile):
    name = "amba-tlm"
    version = "20230601"

    license = "The Clear BSD License"
    author = "Arm Limited (or its affiliates)"
    url = "http://gitlab.marqueesemi.com:8081/sw-tools/amba-tlm"
    description = "The Arm AMBA Transaction-Level Modeling (TLM) library allows you to model and simulate approximately-timed (AT) and cycle-accurate (CA) AXI4 and ACE ports"
    topics = ("AMBA ARM", "TLM", "AXI4", "SystemC")

    settings = "os", "compiler", "build_type", "arch"

    options = {
        "shared": [True, False],
        "fPIC": [True, False]
    }

    default_options = {
        "shared": False,
        "fPIC": True,

        "systemc/2.3.3:fPIC": True,
        "systemc/2.3.3:shared": False,
        "systemc/2.3.3:enable_pthreads": False,
        "systemc/2.3.3:enable_assertions": True,
        "systemc/2.3.3:disable_virtual_bind": False,
        "systemc/2.3.3:disable_async_updates": False,
        "systemc/2.3.3:disable_copyright_msg": True,
        "systemc/2.3.3:enable_phase_callbacks": True,
        "systemc/2.3.3:enable_phase_callbacks_tracing": False,
        "systemc/2.3.3:enable_immediate_self_notifications": False,
    }

    exports_sources = "CMakeLists.txt", "src/*", "include/*"

    def config_options(self):
        if self.settings.os == "Windows":
            del self.options.fPIC

    def layout(self):
        cmake_layout(self)

    def generate(self):
        tc = CMakeToolchain(self)
        tc.generate()

        deps = CMakeDeps(self)
        deps.generate()

    def requirements(self):
        self.requires("systemc/2.3.3")

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
        self.cpp_info.components["armtlmaxi4"].libs = ["armtlmaxi4"]
        self.cpp_info.components["armtlmchi"].libs = ["armtlmchi"]
