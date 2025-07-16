# Ring Buffer Library

## What is This?

This library implements a C++ ring buffer class (`hdc::ringbuffer::RingBuffer`).
The class has the following design characteristics:

- The class does not allocate memory. Rather, the client provides a buffer to
use as the ring buffer. The client decides how to allocate the memory, such as
statically or dynamically. This is how a client can create different types and
sizes of ring buffers:

  ```cpp
  #include <hdcringbuffer.h>
  
  // Example 1: ring buffer of 100 ints.
  int buffer[100];
  hdc::ringbuffer::RingBuffer ring_buffer(buffer, sizeof(buffer));
  
  // Example 2: ring buffer of 50 Foos.
  struct Foo {
      int a;
      int b;
  };
  Foo buffer2[50];
  RingBuffer ring_buffer2(buffer2, sizeof(buffer2));
  ```

- The class does not use C++ templates. The same object code handles different
item types and buffer sizes. In the above examples, the two ring buffers use
the same object code even though the two buffers hold different types and have
different sizes. The class sees the client-provided buffer as just an array of
bytes. Templates could result in code bloat from different object code for each
item type and buffer size combination, depending on the compiler.

## How to Build It

This build this library, you will need:

- [CMake](https://cmake.org/), version 3.19 or higher.

- A C++11 or higher compiler that CMake can use.

- Optionally, [Doxygen](https://www.doxygen.nl/index.html), if you want to build
the docs. The CMake configuration files automatically build the docs if CMake is
installed.

- An internet connection.

This library comes with unit tests. The unit tests use
[GoogleTest](https://github.com/google/googletest). The CMake configuration
files automatically get GoogleTest from GitHub and build the software.

If you have all the prerequisites, you can issue the following shell commands
from the directory where you cloned this project to build the library and unit
tests:

```shell
cmake -B build .
cmake --build build 
```

The author has successfully built this project in the following environments:

| Compiler     | CMake  | IDE                | OS              |
| ------------ | ------ | ------------------ | --------------- |
| MSVC 19.44   | 4.1.0  | Visual Studio 2022 | Windows 11      |
| Clang 19.1.5 | 4.1.0  | Visual Studio Code | Windows 11      |
| Clang 10.0.0 | 4.0.3  | (Bash)             | Ubuntu 20.04    |
| GNU 13.3.0   | 3.28.3 | (Bash)             | Linux Mint 22.1 |


## How to Use It

### Basic Usage

The following snippets show how to use the `hdc::ringbuffer::RingBuffer` class
from your C++ code.

Include the header:

```cpp
#include <hdcringbuffer.h>
```

Declare your buffer memory. This assumes you want a buffer of `MyType` objects: 

```cpp
MyType buffer[RING_BUFFER_SIZE];
```

Declare the ring buffer:

```cpp
hdc::ringbuffer::RingBuffer ring_buffer(buffer, sizeof(buffer));
```

> [!warning]
> Use `sizeof(buffer)` rather than `RING_BUFFER_SIZE` in the above example. The
> `hdc::ringbuffer::RingBuffer` class needs to know the size of the buffer in
> *bytes*, not items. In fact, all `hdc::ringbuffer::RingBuffer` operations are
> in bytes.

Add an item to the ring buffer:

```cpp
MyType temp;
// Set up item.
...
auto count = ring_buffer.writeBytes(&temp, sizeof(temp));
if (count == sizeof(temp)) {
    // Success.
}
```

Or, add multiple items to the ring buffer at the same time:

```cpp
MyType temp_buff[10];
// Set up the items.
...
auto count = ring_buffer.writeBytes(temp_buff, sizeof(temp_buff));
// count will hold the number of bytes written to the ring buffer. It's
// possible not all bytes were written, if the ring buffer became full.
```

Extract an item from the ring buffer:

```cpp
MyType temp;
auto count = ring_buffer.readBytes(&temp, sizeof(temp));
if (count == sizeof(temp)) {
    // Success.
}
```

Or, extract multiple items from the ring buffer:

```cpp
MyType temp[10];
auto count = ring_buffer.readBytes(temp, sizeof(temp));
// count will hold the number of bytes read from the ring buffer. It's
// possible not all bytes were read, if the ring buffer became empty.
```

### Advanced Usage

Check whether the ring buffer is full:

```cpp
if (ring_buffer.isFull()) {
    // It's full.
}
```

Check whether the ring buffer is empty:

```cpp
if (ring_buffer.isEmpty()) {
    // It's empty.
}
```

Get the number of bytes that you can write to the ring buffer before it becomes
full:

```cpp
auto count = ring_buffer.getWritableByteCount();
```

Get the number of bytes that you can read from the ring buffer before it becomes
empty:

```cpp
auto count = ring_buffer.getReadableByteCount();
```

Clear the ring buffer:

```cpp
ring_buffer.clear();
```

### Expert Usage

Peek an item in the ring buffer:

```cpp
MyType temp;
auto count = ring_buffer.peekBytes(&temp, sizeof(temp));
```

Or, peek multiple items in the ring buffer:

```cpp
MyType temp[10];
auto count = ring_buffer.peekBytes(temp, sizeof(temp));
// count will hold the number of bytes peeked in the ring buffer. It's
// possible not all bytes were peeked, if the ring buffer doesn't contain
// enough bytes.
```

Peek an item at an offset in the ring buffer:

```cpp
MyType temp;
// Peek the third item in the ring buffer.
auto count = ring_buffer.peekBytesAt(&temp, sizeof(temp),
                                     2 * sizeof(MyType));
```

Or, peek multiple items at an offset in the ring buffer:

```cpp
MyType temp[10];
// Peek from the fifth item in the ring buffer.
auto count = ring_buffer.peekBytesAt(temp, sizeof(temp),
                                     4 * sizeof(MyType));
// count will hold the number of bytes peeked in the ring buffer. It's
// possible not all bytes were peeked, if the ring buffer doesn't contain
// enough bytes.
```

Discard items from the ring buffer without reading them:

```cpp
// Discard 10 items.
auto count = ring_buffer.discard(10 * sizeof(MyType));
// count will hold the number of bytes discarded from the ring buffer.
// It's possible not all bytes were discarded, if the ring buffer became
// empty.
```
