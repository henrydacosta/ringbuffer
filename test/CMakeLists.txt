enable_testing()

add_executable(RingBufferTest
    RingBufferTest.cpp
)

set_target_properties(RingBufferTest PROPERTIES OUTPUT_NAME "ringbuffertest")

target_link_libraries(RingBufferTest PRIVATE RingBufferLib GTest::gtest_main)

include(GoogleTest)

gtest_discover_tests(RingBufferTest)
