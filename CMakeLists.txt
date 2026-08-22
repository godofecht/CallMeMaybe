cmake_minimum_required(VERSION 3.30)

project(CallMeMaybe VERSION 1.0.0 LANGUAGES CXX)

include(CheckCXXSourceCompiles)
include(CMakePackageConfigHelpers)
include(GNUInstallDirs)
include(CTest)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 16.1)
        message(FATAL_ERROR "CallMeMaybe requires GCC 16.1 or newer")
    endif()
    set(CMM_REFLECTION_FLAG -freflection)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(CMM_REFLECTION_FLAG -freflection-latest)
else()
    message(FATAL_ERROR
        "CallMeMaybe requires GCC 16.1+ or a Clang/P2996 toolchain; detected "
        "${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
endif()

set(_cmm_saved_required_flags "${CMAKE_REQUIRED_FLAGS}")
string(APPEND CMAKE_REQUIRED_FLAGS " -std=c++26 ${CMM_REFLECTION_FLAG}")
check_cxx_source_compiles([=[
    #include <meta>

    struct probe_type { int value; };
    int probe_function(int value);

    static_assert(std::meta::is_class_type(^^probe_type));
    static_assert(std::meta::parameters_of(^^probe_function).size() == 1);
    static_assert(std::meta::is_function_parameter(
        std::meta::parameters_of(^^probe_function)[0]));

    int main() { return 0; }
]=] CMM_COMPILER_HAS_REQUIRED_REFLECTION)
set(CMAKE_REQUIRED_FLAGS "${_cmm_saved_required_flags}")
unset(_cmm_saved_required_flags)

if(NOT CMM_COMPILER_HAS_REQUIRED_REFLECTION)
    message(FATAL_ERROR
        "${CMAKE_CXX_COMPILER} does not provide the C++26 reflection features "
        "required by CallMeMaybe")
endif()

add_library(cmm INTERFACE)
add_library(CallMeMaybe::cmm ALIAS cmm)

target_compile_features(cmm INTERFACE cxx_std_26)
target_compile_options(cmm INTERFACE ${CMM_REFLECTION_FLAG})
target_include_directories(cmm INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

if(PROJECT_IS_TOP_LEVEL)
    set(_cmm_examples_default ON)
    set(_cmm_tests_default ON)
else()
    set(_cmm_examples_default OFF)
    set(_cmm_tests_default OFF)
endif()

option(CMM_BUILD_EXAMPLES "Build CallMeMaybe examples" ${_cmm_examples_default})
option(CMM_BUILD_TESTS "Build CallMeMaybe regression tests" ${_cmm_tests_default})

if(CMM_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

if(CMM_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
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
