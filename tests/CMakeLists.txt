add_executable(test test.cc)
add_executable(test_scheduler test_scheduler.cc)

target_link_libraries(test sylar)
target_link_libraries(test_scheduler sylar)