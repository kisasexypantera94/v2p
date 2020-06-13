add_executable(tests run_tests.c)
target_include_directories(tests PRIVATE ..)
target_link_libraries(tests v2p)
