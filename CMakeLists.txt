cmake_minimum_required(VERSION 3.30)

project(CallMeMaybe VERSION 1.0.0 LANGUAGES CXX)

include(CheckCXXSourceCompiles)
include(CMakePackageConfigHelpers)
include(GNUInstallDirs)
include(CTest)

set(CMM_COMPILE_OPTIONS)
set(CMM_LINK_OPTIONS)
set(CMM_SYSTEM_INCLUDE_DIRS)
set(CMM_LINK_DIRS)
set(CMM_LINK_LIBRARIES)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 16.1)
        message(FATAL_ERROR "CallMeMaybe requires GCC 16.1 or newer")
    endif()
    list(APPEND CMM_COMPILE_OPTIONS -freflection)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    get_filename_component(_cmm_compiler_bin_dir "${CMAKE_CXX_COMPILER}" DIRECTORY)
    get_filename_component(CMM_CLANG_TOOLCHAIN_ROOT "${_cmm_compiler_bin_dir}" DIRECTORY)

    set(CMM_CLANG_LIBCXX_INCLUDE "${CMM_CLANG_TOOLCHAIN_ROOT}/include/c++/v1")
    set(CMM_CLANG_RUNTIME_DIR "${CMM_CLANG_TOOLCHAIN_ROOT}/lib")

    if(NOT EXISTS "${CMM_CLANG_LIBCXX_INCLUDE}/__config")
        message(FATAL_ERROR
            "Clang/P2996 must be used with the libc++ built from the same toolchain. "
            "Expected ${CMM_CLANG_LIBCXX_INCLUDE}")
    endif()

    list(APPEND CMM_COMPILE_OPTIONS -freflection-latest -stdlib=libc++)
    list(APPEND CMM_SYSTEM_INCLUDE_DIRS "${CMM_CLANG_LIBCXX_INCLUDE}")
    list(APPEND CMM_LINK_DIRS "${CMM_CLANG_RUNTIME_DIR}")
    list(APPEND CMM_LINK_OPTIONS -stdlib=libc++ "-Wl,-rpath,${CMM_CLANG_RUNTIME_DIR}")

    if(EXISTS "${CMM_CLANG_RUNTIME_DIR}/libc++abi.dylib" OR
       EXISTS "${CMM_CLANG_RUNTIME_DIR}/libc++abi.so" OR
       EXISTS "${CMM_CLANG_RUNTIME_DIR}/libc++abi.a")
        list(APPEND CMM_LINK_LIBRARIES c++abi)
    endif()
else()
    message(FATAL_ERROR
        "CallMeMaybe requires GCC 16.1+ or the Bloomberg Clang/P2996 toolchain; detected "
        "${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
endif()

set(_cmm_saved_required_flags "${CMAKE_REQUIRED_FLAGS}")
set(_cmm_saved_required_includes "${CMAKE_REQUIRED_INCLUDES}")
set(_cmm_saved_required_link_options "${CMAKE_REQUIRED_LINK_OPTIONS}")
set(_cmm_saved_required_libraries "${CMAKE_REQUIRED_LIBRARIES}")

string(APPEND CMAKE_REQUIRED_FLAGS " -std=c++26")
foreach(_cmm_option IN LISTS CMM_COMPILE_OPTIONS)
    string(APPEND CMAKE_REQUIRED_FLAGS " ${_cmm_option}")
endforeach()
set(CMAKE_REQUIRED_INCLUDES ${CMM_SYSTEM_INCLUDE_DIRS})
set(CMAKE_REQUIRED_LINK_OPTIONS ${CMM_LINK_OPTIONS})
set(CMAKE_REQUIRED_LIBRARIES ${CMM_LINK_LIBRARIES})

check_cxx_source_compiles([=[
    #include <meta>

    struct probe_annotation {
        friend constexpr bool operator==(probe_annotation, probe_annotation) = default;
    };
    inline constexpr probe_annotation probe{};

    struct probe_type {
        [[=probe]] int value;
    };
    int probe_function(int value);

    static_assert(std::meta::is_class_type(^^probe_type));
    static_assert(std::meta::parameters_of(^^probe_function).size() == 1);
    static_assert(std::meta::is_function_parameter(
        std::meta::parameters_of(^^probe_function)[0]));

    #if defined(__clang__)
    static_assert(std::meta::annotations_of(
        ^^probe_type::value, ^^probe_annotation).size() == 1);
    #else
    static_assert(std::meta::annotations_of_with_type(
        ^^probe_type::value, ^^probe_annotation).size() == 1);
    #endif

    int main() { return 0; }
]=] CMM_COMPILER_HAS_REQUIRED_REFLECTION)

set(CMAKE_REQUIRED_FLAGS "${_cmm_saved_required_flags}")
set(CMAKE_REQUIRED_INCLUDES "${_cmm_saved_required_includes}")
set(CMAKE_REQUIRED_LINK_OPTIONS "${_cmm_saved_required_link_options}")
set(CMAKE_REQUIRED_LIBRARIES "${_cmm_saved_required_libraries}")
unset(_cmm_saved_required_flags)
unset(_cmm_saved_required_includes)
unset(_cmm_saved_required_link_options)
unset(_cmm_saved_required_libraries)

if(NOT CMM_COMPILER_HAS_REQUIRED_REFLECTION)
    message(FATAL_ERROR
        "${CMAKE_CXX_COMPILER} does not provide the C++26 reflection, parameter "
        "reflection, and annotation support required by CallMeMaybe")
endif()

add_library(cmm INTERFACE)
add_library(CallMeMaybe::cmm ALIAS cmm)

target_compile_features(cmm INTERFACE cxx_std_26)
target_compile_options(cmm INTERFACE ${CMM_COMPILE_OPTIONS})
target_include_directories(cmm INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

if(CMM_SYSTEM_INCLUDE_DIRS)
    target_include_directories(cmm SYSTEM INTERFACE ${CMM_SYSTEM_INCLUDE_DIRS})
endif()
if(CMM_LINK_DIRS)
    target_link_directories(cmm INTERFACE ${CMM_LINK_DIRS})
endif()
if(CMM_LINK_OPTIONS)
    target_link_options(cmm INTERFACE ${CMM_LINK_OPTIONS})
endif()
if(CMM_LINK_LIBRARIES)
    target_link_libraries(cmm INTERFACE ${CMM_LINK_LIBRARIES})
endif()

if(PROJECT_IS_TOP_LEVEL)
    set(_cmm_examples_default ON)
    set(_cmm_tests_default ON)
else()
    set(_cmm_examples_default OFF)
    set(_cmm_tests_default OFF)
endif()

option(CMM_BUILD_EXAMPLES "Build CallMeMaybe examples" ${_cmm_examples_default})
option(CMM_BUILD_TESTS "Build CallMeMaybe regression tests" ${_cmm_tests_default})
option(CMM_BUILD_BENCHMARKS "Build CallMeMaybe performance benchmarks" OFF)

if(CMM_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

if(CMM_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

if(CMM_BUILD_BENCHMARKS)
    add_subdirectory(benchmarks)
endif()

install(TARGETS cmm EXPORT CallMeMaybeTargets)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(EXPORT CallMeMaybeTargets
    FILE CallMeMaybeTargets.cmake
    NAMESPACE CallMeMaybe::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/CallMeMaybe
)

configure_package_config_file(
    cmake/CallMeMaybeConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/CallMeMaybeConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/CallMeMaybe
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/CallMeMaybeConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/CallMeMaybeConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/CallMeMaybeConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/CallMeMaybe
)
