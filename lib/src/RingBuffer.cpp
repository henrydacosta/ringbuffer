/*
MIT License

Copyright (c) 2025 Henry da Costa

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

/**
 * @file
 * Implements class RingBufferAdapter.
 */

#include "RingBuffer.h"

#include <algorithm>
#include <cstring>

using namespace std;
using namespace hdc::ringbuffer;

// clang-format off
/**
 * @class hdc::ringbuffer::RingBuffer
 *
 * @internal
 *
 * # Internal Details
 *
 * These conditions are always true:
 * - <tt>_read >= 0</tt> (implicit since @c _read can only be positive)
 * - <tt>_write >= 0</tt> (implicit since @c _write can only be positive)
 * - <tt>_read < _size || _read == _size && _write == 0</tt>
 * - <tt>_write <= _size</tt>
 * - <tt>_read != _write</tt>
 *
 * The ring buffer can be in one of several states depending on the values of
 * @c _read and @c _write relative to @c _size and to each other. The following
 * table shows the combinations, implications and identifying characteristics of
 * each state.
 *
 * |Condition                                             |Empty  |Full   |Read       |Write      |Wrap Read|Wrap Write|State         |
 * |:----------------------------------------------------:|:-----:|:-----:|:---------:|:---------:|:-------:|:--------:|:------------:|
 * |<tt>_read == _size</tt>                               |_True_ |False  |N/A        |Beginning  |N/A      |False     |@ref e        |
 * |<tt>_read == 0 && _write == _size</tt>                |False  |_True_ |_Beginning_|N/A        |False    |N/A       |@ref frb      |
 * |<tt>_read > 0 && _write == _size</tt>                 |False  |_True_ |_Middle_   |N/A        |True     |N/A       |@ref frm      |
 * |<tt>_read == 0 && _write < _size</tt>                 |False  |_False_|_Beginning_|Middle     |False    |False     |@ref nfrb     |
 * |<tt>_read > 0 && _read < _write && _write < _size</tt>|False  |False  |Middle     |Middle     |False    |_True_    |@ref ww       |
 * |<tt>_read < _size && _write == 0</tt>                 |_False_|False  |Middle     |_Beginning_|False    |False     |@ref newb     |
 * |<tt>_read > _write && _write > 0</tt>                 |False  |_False_|Middle     |Middle     |_True_   |False     |@ref nfwr     |
 *
 * Another way of looking at the states:
 *
 * <table>
 *   <tr>
 *     <td>
 *     <td><center><tt>_write == 0</tt></center>
 *     <td><center><tt>_write > 0 && _write < _size</tt></center>
 *     <td><center><tt>_write == _size</tt></center>
 *   <tr>
 *     <td><center><tt>_read == 0</tt></center>
 *     <td><center>Should not happen</center>
 *     <td><center>@ref nfrb </center>
 *     <td><center>@ref frb </center>
 *   <tr>
 *     <td><center><tt>_read > 0 && _read < _write</tt></center>
 *     <td><center>Impossible</center>
 *     <td><center>@ref ww </center>
 *     <td><center>@ref frm </center>
 *   <tr>
 *     <td><center><tt>_read > _write && _read < _size</tt></center>
 *     <td><center>@ref newb </center>
 *     <td><center>@ref nfwr </center>
 *     <td><center>Should not happen</center>
 *   <tr>
 *     <td><center><tt>_read == _size</tt></center>
 *     <td><center>@ref e </center>
 *     <td><center>Should not happen</center>
 *     <td><center>Should not happen</center>
 * </table>
 *
 * ## Empty{#e}
 *
 * @verbatim
    0            _size-1  _size
+-------+-------+-------+
| Empty |  ...  | Empty |
+-------+-------+-------+
    ^                       ^
    |                       |
 _write                   _read
 * @endverbatim
 *
 * Properties:
 * - isEmpty() returns @c true
 * - isFull() returns @c false
 * - getReadableCount() returns 0
 * - getWritableCount() returns @c _size
 *
 * ## Full, Read Beginning{#frb}
 *
 * @verbatim
     0            _size-1  _size
 +-------+-------+-------+
 | First |  ...  | Last  |
 +-------+-------+-------+
     ^                       ^
     |                       |
   _read                  _write
 * @endverbatim
 *
 * Properties:
 * - isEmpty() returns @c false
 * - isFull() returns @c true
 * - getReadableCount() returns @c _size
 * - getWritableCount() returns 0
 *
 * ## Full, Read Middle{#frm}
 *
 * @verbatim
    0                                    _size-1  _size
+-------+-------+-------+-------+-------+-------+
| Valid |  ...  | Last  | First |  ...  | Valid |
+-------+-------+-------+-------+-------+-------+
                            ^                       ^
                            |                       |
                          _read                  _write
 * @endverbatim
 * 
 * Properties:
 * - isEmpty() returns @c false
 * - isFull() returns @c true
 * - getReadableCount() returns @c _size
 * - getWritableCount() returns 0
 * 
 * ## Non-Full, Read Beginning{#nfrb}
 * 
 * @verbatim
    0                                    _size-1  _size
+-------+-------+-------+-------+-------+-------+
| First |  ...  | Last  | Empty |  ...  | Empty |
+-------+-------+-------+-------+-------+-------+
    ^                       ^
    |                       |
  _read                  _write
 * @endverbatim
 * 
 * Properties:
 * - isEmpty() returns @c false
 * - isFull() returns @c false
 * - getReadableCount() returns <tt>_write - _read</tt>
 * - getWritableCount() returns <tt>_read + _size - _write</tt>
 * 
 * ## Wrap Write{#ww}
 * 
 * @verbatim
    0                                                            _size-1  _size
+-------+-------+-------+-------+-------+-------+-------+-------+-------+
| Empty |  ...  | Empty | First |  ...  | Last  | Empty |  ...  | Empty |
+-------+-------+-------+-------+-------+-------+-------+-------+-------+
                            ^                       ^
                            |                       |
                          _read                  _write
 * @endverbatim
 * 
 * Properties:
 * - isEmpty() returns @c false
 * - isFull() returns @c false
 * - getReadableCount() returns <tt>_write - _read</tt>
 * - getWritableCount() returns <tt>_read + _size - _write</tt>
 *
 * ## Non-Empty, Write Beginning{#newb}
 * 
 * @verbatim
    0                                    _size-1  _size
+-------+-------+-------+-------+-------+-------+
| Empty |  ...  | Empty | First |  ...  | Last  |
+-------+-------+-------+-------+-------+-------+
    ^                       ^
    |                       |
 _write                   _read
 * @endverbatim
 * 
 * Properties:
 * - isEmpty() returns @c false
 * - isFull() returns @c false
 * - getReadableCount() returns <tt>_write + _size - _read</tt>
 * - getWritableCount() returns <tt>_read - _write</tt>
 * 
 * ## Non-Full, Wrap Read{#nfwr}
 *
 * @verbatim
    0                                                            _size-1  _size
+-------+-------+-------+-------+-------+-------+-------+-------+-------+
| Valid |  ...  | Last  | Empty |  ...  | Empty | First |  ...  | Valid |
+-------+-------+-------+-------+-------+-------+-------+-------+-------+
                            ^                       ^
                            |                       |
                         _write                   _read
 * @endverbatim
 * 
 * Properties:
 * - isEmpty() returns @c false
 * - isFull() returns @c false
 * - getReadableCount() returns <tt>_write + _size - _read</tt>
 * - getWritableCount() returns <tt>_read - _write</tt>
 */
// clang-format on

std::size_t RingBuffer::writeBytes(const void *source, std::size_t count) {
    _assertValid();

    assert(source != nullptr);

    if (count == 0 || isFull()) {
        return 0;
    }

    auto original_count = count;

    if (_read < _write) {
        // clang-format off
        // The following states are possible here:
        //   Non-Full, Read Beginning
        //         0                                    _size-1  _size
        //     +-------+-------+-------+-------+-------+-------+
        //     | First |  ...  | Last  | Empty |  ...  | Empty |
        //     +-------+-------+-------+-------+-------+-------+
        //         ^                       ^
        //         |                       |
        //       _read                  _write
        //   Wrap Write
        //         0                                                            _size-1  _size
        //     +-------+-------+-------+-------+-------+-------+-------+-------+-------+
        //     | Empty |  ...  | Empty | First |  ...  | Last  | Empty |  ...  | Empty |
        //     +-------+-------+-------+-------+-------+-------+-------+-------+-------+
        //                                 ^                       ^
        //                                 |                       |
        //                               _read                  _write
        // clang-format on

        // Write up to count bytes up to end of buffer.
        auto n = std::min(count, _size - _write);
        std::memcpy(_buffer + _write, source, n);
        count -= n;
        _write += n;
        source = static_cast<const char *>(source) + n;

        // Wrap around if buffer not full.
        if (_write == _size && _read > 0) {
            _write = 0;
        }
    }

    if (_read > _write) {
        // clang-format off
        // The following states are possible here:
        //   Empty:
        //         0            _size-1  _size
        //     +-------+-------+-------+
        //     | Empty |  ...  | Empty |
        //     +-------+-------+-------+
        //         ^                       ^
        //         |                       |
        //      _write                   _read
        //   Non-Empty, Write Beginning:
        //         0                                    _size-1  _size
        //     +-------+-------+-------+-------+-------+-------+
        //     | Empty |  ...  | Empty | First |  ...  | Last  |
        //     +-------+-------+-------+-------+-------+-------+
        //         ^                       ^
        //         |                       |
        //      _write                   _read
        //   Non-Full, Wrap Read:
        //       0                                                              _size-1  _size
        //     +-------+-------+-------+-------+-------+-------+-------+-------+-------+
        //     | Valid |  ...  | Last  | Empty |  ...  | Empty | First |  ...  | Valid |
        //     +-------+-------+-------+-------+-------+-------+-------+-------+-------+
        //                                 ^                       ^
        //                                 |                       |
        //                              _write                   _read
        // clang-format on

        // Write up to count bytes up to read location.
        auto n = std::min(count, _read - _write);
        std::memcpy(_buffer + _write, source, n);
        _write += n;
        count -= n;

        if (_write == _read) {
            // Buffer full.
            _write = _size;
        }

        if (_read == _size) {
            // Buffer was empty.
            // We've just written to beginning of buffer.
            // Make next read() start at beginning of buffer.
            _read = 0;
        }
    }

    return original_count - count;
}

std::size_t RingBuffer::_readBytes(void *destination, std::size_t count,
                                   std::size_t &read, std::size_t &write) {
    _assertValid();

    if (count == 0 || isEmpty()) {
        return 0;
    }
    auto original_count = count;

    if (read > write || write == _size) {
        // clang-format off
        // The following states are possible here:
        //   Full, Read Beginning:
        //         0            _size-1  _size
        //     +-------+-------+-------+
        //     | First |  ...  | Last  |
        //     +-------+-------+-------+
        //         ^                       ^
        //         |                       |
        //       read                    write
        //   Full, Read Middle:
        //         0                                    _size-1  _size
        //     +-------+-------+-------+-------+-------+-------+
        //     | Valid |  ...  | Last  | First |  ...  | Valid |
        //     +-------+-------+-------+-------+-------+-------+
        //                                 ^                       ^
        //                                 |                       |
        //                               read                    write
        //   Non-Empty, Write Beginning:
        //         0                                    _size-1  _size
        //     +-------+-------+-------+-------+-------+-------+
        //     | Empty |  ...  | Empty | First |  ...  | Last  |
        //     +-------+-------+-------+-------+-------+-------+
        //         ^                       ^
        //         |                       |
        //       write                   read
        //   Non-Full, Wrap Read:
        //       0                                                              _size-1  _size
        //     +-------+-------+-------+-------+-------+-------+-------+-------+-------+
        //     | Valid |  ...  | Last  | Empty |  ...  | Empty | First |  ...  | Valid |
        //     +-------+-------+-------+-------+-------+-------+-------+-------+-------+
        //                                 ^                       ^
        //                                 |                       |
        //                               write                   read
        // clang-format on

        if (write == _size) {
            // Buffer full.
            // Reading will free up space for writing.
            // Make next write() start at current read location.
            write = read;
        }

        // Read up to count bytes up to end of buffer.
        auto n = std::min(count, _size - read);
        if (destination != nullptr) {
            std::memcpy(destination, _buffer + read, n);
        }
        count -= n;
        read += n;
        destination = static_cast<char *>(destination) + n;

        // Wrap around reading if buffer not empty.
        if (read == _size && write > 0) {
            read = 0;
        }
    }

    if (read < write) {
        // clang-format off
        // The following states are possible here:
        //   Non-Full, Read Beginning
        //        0                                    _size-1  _size
        //     +-------+-------+-------+-------+-------+-------+
        //     | First |  ...  | Last  | Empty |  ...  | Empty |
        //     +-------+-------+-------+-------+-------+-------+
        //         ^                       ^
        //         |                       |
        //       read                    write
        //   Wrap Write
        //         0                                                            _size-1  _size
        //     +-------+-------+-------+-------+-------+-------+-------+-------+-------+
        //     | Empty |  ...  | Empty | First |  ...  | Last  | Empty |  ...  | Empty |
        //     +-------+-------+-------+-------+-------+-------+-------+-------+-------+
        //                                 ^                       ^
        //                                 |                       |
        //                               read                    write
        // clang-format on

        // Read up to count bytes up to write location.
        auto n = std::min(count, write - read);
        if (destination != nullptr) {
            std::memcpy(destination, _buffer + read, n);
        }
        count -= n;
        read += n;

        if (read == write) {
            // Buffer empty.
            read = _size;
            write = 0;
        }
    }

    return original_count - count;
}

void RingBuffer::_assertValid() const {
    assert(_buffer != nullptr);
    assert(_size > 0);
    assert(_read < _size || (_read == _size && _write == 0));
    assert(_write <= _size);
    assert(_read != _write);
}
