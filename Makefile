conan_install:
	conan install . --build missing -pr:h conan_profiles/default -pr:b conan_profiles/default

conan_build: conan_install
	conan build . -pr:h conan_profiles/default -pr:b conan_profiles/default

conan_create:
	conan create . --build missing -pr:h conan_profiles/default -pr:b conan_profiles/default

conan_export_pkg: conan_build
	conan export-pkg .

conan_upload:
	conan upload amba-tlm/* -r signature-local -c

clean:
	@rm -rf build CMakeUserPresets.json test_package/build test_package/CMakeUserPresets.json
