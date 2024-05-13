default: all

all:
	conan install . --build missing -pr:h conan_profiles/default -pr:b conan_profiles/default
	conan build . -pr:h conan_profiles/default -pr:b conan_profiles/default

clean:
	@rm -rf build CMakeUserPresets.json

create:
	conan create . --build missing -pr:h conan_profiles/default -pr:b conan_profiles/default
