add_executable(test test.cc)
add_executable(test_scheduler test_scheduler.cc)
add_executable(test_fiber test_fiber.cc)
add_executable(test_thread test_thread.cc)

target_link_libraries(test sylar)
target_link_libraries(test_scheduler sylar)
target_link_libraries(test_fiber sylar)
target_link_libraries(test_thread sylar)