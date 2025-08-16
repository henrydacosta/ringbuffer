add_library(RingBufferLib
    include/RingBuffer.h
    src/RingBuffer.cpp)

set_target_properties(RingBufferLib PROPERTIES OUTPUT_NAME "ringbuffer")

target_include_directories(RingBufferLib PUBLIC include)
