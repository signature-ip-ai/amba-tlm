#!/bin/env python3

import os

from conan import ConanFile
from conan.tools.cmake import CMake, cmake_layout
from conan.tools.build import can_run


class AmbaTlmDemoRecipe(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeDeps", "CMakeToolchain"

    default_options = {
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

        "amba-tlm/1.0:fPIC": True,
        "amba-tlm/1.0:shared": False
    }

    def requirements(self):
        self.requires("systemc/2.3.3")
        self.requires("amba-tlm/1.0")

    def build_requirements(self):
        self.tool_requires("cmake/3.27.6")

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def layout(self):
        cmake_layout(self)

    def test(self):
        if can_run(self):
            cmd = os.path.join(self.cpp.build.bindir, "TrafficExample")
            self.run(cmd, env="conanrun")

            cmd = os.path.join(self.cpp.build.bindir, "TransactorExample")
            self.run(cmd, env="conanrun")
