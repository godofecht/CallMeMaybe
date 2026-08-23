add_executable(cmm_regression regression.cpp)
target_link_libraries(cmm_regression PRIVATE CallMeMaybe::cmm)
add_test(NAME cmm_regression COMMAND cmm_regression)

add_executable(cmm_advanced advanced.cpp)
target_link_libraries(cmm_advanced PRIVATE CallMeMaybe::cmm)
add_test(NAME cmm_advanced COMMAND cmm_advanced)

add_executable(cmm_constexpr_entities constexpr_entities.cpp)
target_link_libraries(cmm_constexpr_entities PRIVATE CallMeMaybe::cmm)
add_test(NAME cmm_constexpr_entities COMMAND cmm_constexpr_entities)
