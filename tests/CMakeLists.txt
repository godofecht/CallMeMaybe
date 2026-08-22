add_executable(cmm_regression regression.cpp)
target_link_libraries(cmm_regression PRIVATE CallMeMaybe::cmm)

add_test(NAME cmm_regression COMMAND cmm_regression)
