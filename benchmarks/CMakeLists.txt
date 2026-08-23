add_executable(cmm_runtime_bench runtime_dispatch.cpp)
target_link_libraries(cmm_runtime_bench PRIVATE CallMeMaybe::cmm)
target_compile_features(cmm_runtime_bench PRIVATE cxx_std_26)

if(MSVC)
    target_compile_options(cmm_runtime_bench PRIVATE /O2)
else()
    target_compile_options(cmm_runtime_bench PRIVATE -O3 -DNDEBUG)
endif()
