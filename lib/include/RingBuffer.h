
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
 * Declares class RingBuffer.
 */

#ifndef _HDC_RINGBUFFER_H
#define _HDC_RINGBUFFER_H

#include <cassert>
#include <cstddef>

namespace hdc {
namespace ringbuffer {

/**
 * Ring buffer adapter.
 *
 * Adapts a client-supplied buffer into a ring buffer.
 *
 * @note
 * Uses <tt>std::memcpy()</tt> to copy data.
 *
 * @warning
 * The client code must not reuse or delete the supplied buffer memory for the
 * lifetime of the ring buffer.
 *
 * @warning
 * The client code is responsible for synchronization.
 */
class RingBuffer {
public:
    /**
     * Ring buffer constructor.
     *
     * @param[in] buffer
     * The buffer to adapt into a ring buffer.
     *
     * @param[in] size
     * The size of the buffer in bytes.
     */
    RingBuffer(void *buffer, std::size_t size)
        : _buffer(static_cast<char *>(buffer)), _size(size), _read(size),
          _write(0) {
        _assertValid();
    }

    /**
     * Returns whether the ring buffer is empty.
     */
    bool isEmpty() const {
        _assertValid();
        return _read == _size;
    }

    /**
     * Returns whether the ring buffer is full.
     */
    bool isFull() const {
        _assertValid();
        return _write == _size;
    }

    /**
     * Returns the number of bytes that can be read from the ring buffer.
     */
    std::size_t getReadableByteCount() const {
        _assertValid();
        return isFull()         ? _size
               : _read > _write ? _write + _size - _read
                                : _write - _read;
    }

    /**
     * Returns the number of bytes that can be written to the ring buffer.
     */
    ::size_t getWritableByteCount() const {
        _assertValid();
        return isFull()         ? 0
               : _read < _write ? _read + _size - _write
                                : _read - _write;
    }

    /**
     * Reads bytes from the ring buffer into a destination buffer.
     *
     * @param[out] destination
     * The destination buffer.
     * 
     * @param[in] count
     * The number of bytes to read.
     *
     * @return
     * The number of bytes read.
     *
     * @note
     * The number of bytes read may be less than the number requested if the
     * ring buffer becomes empty.
     */
    std::size_t readBytes(void *destination, std::size_t count) {
        assert(destination != nullptr);
        return _readBytes(destination, count, _read, _write);
    }

    /**
     * Writes bytes from a source buffer into the ring buffer.
     *
     * @param[in] source
     * The source buffer.
     * 
     * @param[in] count
     * The number of bytes to write.
     *
     * @return The number of bytes written.
     *
     * @note
     * The number of bytes written may be less than the number requested if the
     * ring buffer becomes full.
     */
    std::size_t writeBytes(const void *source, std::size_t count);

    /**
     * Discards bytes from the ring buffer.
     *
     * @param[in] count
     * The number of bytes to discard.
     *
     * @return
     * The number of bytes discarded.
     *
     * @note
     * The number of bytes discarded may be less than the number requested if
     * the ring buffer becomes empty.
     */
    std::size_t discardBytes(std::size_t count) {
        return _readBytes(nullptr, count, _read, _write);
    }

    /**
     * Reads bytes from the ring buffer into a destination buffer without
     * removing the bytes from the ring buffer.
     *
     * @param[out] destination
     * The destination buffer.
     * 
     * @param[in] count
     * The number of bytes to peek.
     *
     * @return
     * The number of bytes peeked.
     *
     * @note
     * The number of bytes peeked may be less than the number requested if the
     * ring buffer becomes empty.
     */
    std::size_t peekBytes(void *destination, std::size_t count) {
        assert(destination != nullptr);
        auto read = _read;
        auto write = _write;
        return _readBytes(destination, count, read, write);
    }

    /**
     * Reads bytes from the ring buffer into a destination buffer starting at a
     * given offset in the ring buffer without removing the bytes from the ring
     * buffer.
     *
     * @param[out] destination
     * The destination buffer.
     * 
     * @param[in] count
     * The number of bytes to peek.
     * 
     * @param[in] where
     * The offset relative to the first byte in the ring buffer.
     *
     * @return
     * The number of bytes peeked.
     *
     * @note
     * The number of bytes peeked may be less than the number requested if the
     * ring buffer becomes empty.
     */
    std::size_t peekBytesAt(void *destination, std::size_t count,
                            std::size_t where) {
        assert(destination != nullptr);
        auto read = _read;
        auto write = _write;
        if (_readBytes(nullptr, where, read, write) != where) {
            return 0;
        }
        return _readBytes(destination, count, read, write);
    }

    /**
     * Empties out the ring buffer.
     */
    void clear() {
        _read = _size;
        _write = 0;
    }

private:
    char *_buffer;      //!< The client-supplied buffer.
    std::size_t _size;  //!< The size of the buffer.
    std::size_t _read;  //!< Indexes the next byte to read. Set to _size when
                        //!< buffer empty.
    std::size_t _write; //!< Indexes the next byte to write. Set to _size when
                        //!< buffer full. Set to 0 when buffer empty.

    /**
     * Reads bytes from the ring buffer into a destination buffer or discards
     * bytes from the ring buffer if no destination buffer is provided.
     *
     * @param[out] destination
     * The destination buffer to read into or @c nullptr to discard.
     * 
     * @param[in] count
     * The number of bytes to read or discard.
     * 
     * @param[in,out] read
     * The index of the next byte to read.
     * 
     * @param[in,out] write
     * The index of the next byte to write.
     *
     * @return
     * The number of bytes read or discarded.
     *
     * @note
     * The number of bytes read or discarded may be less than the number
     * requested if the ring buffer becomes empty.
     */
    std::size_t _readBytes(void *destination, std::size_t count,
                           std::size_t &read, std::size_t &write);
    void _assertValid() const;
}; // class RingBuffer

} // namespace ringbuffer
} // namespace hdc

#endif // _HDC_RINGBUFFER_H