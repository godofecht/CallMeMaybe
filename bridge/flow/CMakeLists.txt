add_library(cmm_flow_bridge STATIC cmm_flow.cpp cmm_flow_bits.cpp generator.cpp)
add_library(CallMeMaybe::flow_bridge ALIAS cmm_flow_bridge)

target_link_libraries(cmm_flow_bridge PUBLIC CallMeMaybe::cmm)
target_compile_features(cmm_flow_bridge PUBLIC cxx_std_26)
target_include_directories(cmm_flow_bridge PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/cmm/flow>
)

if(CMM_BUILD_TESTS)
    add_executable(cmm_flow_bridge_test test.cpp)
    target_link_libraries(cmm_flow_bridge_test PRIVATE CallMeMaybe::flow_bridge)
    add_test(NAME cmm_flow_bridge COMMAND cmm_flow_bridge_test)
endif()
