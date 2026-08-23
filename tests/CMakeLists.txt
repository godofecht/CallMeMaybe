add_executable(cmm_regression regression.cpp)
target_link_libraries(cmm_regression PRIVATE CallMeMaybe::cmm)
add_test(NAME cmm_regression COMMAND cmm_regression)

add_executable(cmm_advanced advanced.cpp)
target_link_libraries(cmm_advanced PRIVATE CallMeMaybe::cmm)
add_test(NAME cmm_advanced COMMAND cmm_advanced)
