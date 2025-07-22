
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

#include "RingBuffer.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <numeric>

using namespace std;
using namespace hdc::ringbuffer;

class RingBufferTest : public testing::Test {
protected:
    RingBufferTest() : m_ring_buffer(m_buffer.data(), BUFFER_SIZE) {}

    static const size_t ZERO_SIZE = 0;
    static const size_t BUFFER_SIZE = 100;
    static const size_t EXTRA_BUFFER_SIZE = 3;

    array<int8_t, BUFFER_SIZE + EXTRA_BUFFER_SIZE> m_check_buffer;
    array<int8_t, BUFFER_SIZE + EXTRA_BUFFER_SIZE> m_read_buffer;
    array<int8_t, BUFFER_SIZE + EXTRA_BUFFER_SIZE> m_write_buffer;
    array<int8_t, BUFFER_SIZE> m_buffer;
    RingBuffer m_ring_buffer;

    void checkState(bool empty, bool full, size_t readable_count,
                    size_t writable_count) const {
        ASSERT_EQ(m_ring_buffer.isEmpty(), empty);
        ASSERT_EQ(m_ring_buffer.isFull(), full);
        ASSERT_EQ(m_ring_buffer.getReadableByteCount(), readable_count);
        ASSERT_EQ(m_ring_buffer.getWritableByteCount(), writable_count);
    }

    void testRead(size_t requested, size_t expected) {
        ASSERT_EQ(m_ring_buffer.readBytes(m_read_buffer.data(), requested),
                  expected);
        ASSERT_TRUE(equal(m_read_buffer.begin(), m_read_buffer.end(),
                          m_check_buffer.begin(), m_check_buffer.end()));
    }

    void testWrite(size_t requested, size_t expected) {
        ASSERT_EQ(m_ring_buffer.writeBytes(m_write_buffer.data(), requested),
                  expected);
    }

    void testDiscard(size_t requested, size_t expected) {
        ASSERT_EQ(m_ring_buffer.discardBytes(requested), expected);
    }

    void testPeek(size_t requested, size_t expected) {
        ASSERT_EQ(m_ring_buffer.peekBytes(m_read_buffer.data(), requested),
                  expected);
        ASSERT_TRUE(equal(m_read_buffer.begin(), m_read_buffer.end(),
                          m_check_buffer.begin(), m_check_buffer.end()));
    }

    void testPeekAt(size_t requested, size_t where, size_t expected) {
        ASSERT_EQ(
            m_ring_buffer.peekBytesAt(m_read_buffer.data(), requested, where),
            expected);
        ASSERT_TRUE(equal(m_read_buffer.begin(), m_read_buffer.end(),
                          m_check_buffer.begin(), m_check_buffer.end()));
    }
};

const size_t RingBufferTest::ZERO_SIZE;
const size_t RingBufferTest::BUFFER_SIZE;
const size_t RingBufferTest::EXTRA_BUFFER_SIZE;

TEST_F(RingBufferTest, IsInitiallyEmpty) {
    checkState(true, false, 0, BUFFER_SIZE);
}

TEST_F(RingBufferTest, ReadEmptyReturnsZero) {
    for (auto i = ZERO_SIZE; i <= BUFFER_SIZE + EXTRA_BUFFER_SIZE; ++i) {
        testRead(i, 0);
        checkState(true, false, 0, BUFFER_SIZE);
    }
}

TEST_F(RingBufferTest, TestWriteReadClear) {
    for (auto i = ZERO_SIZE; i < BUFFER_SIZE + EXTRA_BUFFER_SIZE; ++i) {
        for (auto j = ZERO_SIZE; j < BUFFER_SIZE + EXTRA_BUFFER_SIZE; ++j) {
            // Write up to i items to the ring buffer.
            auto expected_write = min(i, BUFFER_SIZE);
            iota(m_write_buffer.begin(), m_write_buffer.begin() + i, 0);
            testWrite(i, expected_write);
            checkState(expected_write == ZERO_SIZE,
                       expected_write == BUFFER_SIZE, expected_write,
                       BUFFER_SIZE - expected_write);

            // Read up to j items from the ring buffer.
            auto expected_read = min(j, expected_write);
            fill(m_read_buffer.begin(), m_read_buffer.end(), -1);
            iota(m_check_buffer.begin(), m_check_buffer.begin() + expected_read,
                 0);
            fill(m_check_buffer.begin() + expected_read, m_check_buffer.end(),
                 -1);
            testRead(j, expected_read);
            checkState(expected_write - expected_read == ZERO_SIZE,
                       expected_write - expected_read == BUFFER_SIZE,
                       expected_write - expected_read,
                       BUFFER_SIZE - expected_write + expected_read);

            // Clear the ring buffer for next iteration.
            m_ring_buffer.clear();
            checkState(true, false, ZERO_SIZE, BUFFER_SIZE);
        }
    }
}

TEST_F(RingBufferTest, TestDiscard) {
    for (auto i = ZERO_SIZE; i < BUFFER_SIZE + EXTRA_BUFFER_SIZE; ++i) {
        for (auto j = ZERO_SIZE; j < BUFFER_SIZE + EXTRA_BUFFER_SIZE; ++j) {
            // Write up to i items to the ring buffer.
            auto expected_write = min(i, BUFFER_SIZE);
            iota(m_write_buffer.begin(), m_write_buffer.begin() + i, 0);
            testWrite(i, expected_write);
            checkState(expected_write == ZERO_SIZE,
                       expected_write == BUFFER_SIZE, expected_write,
                       BUFFER_SIZE - expected_write);

            // Discard up to j items from the ring buffer.
            auto expected_discard = min(j, expected_write);
            testDiscard(j, expected_discard);
            checkState(expected_write - expected_discard == ZERO_SIZE,
                       expected_write - expected_discard == BUFFER_SIZE,
                       expected_write - expected_discard,
                       BUFFER_SIZE - expected_write + expected_discard);

            // Clear the ring buffer for next iteration.
            m_ring_buffer.clear();
            checkState(true, false, ZERO_SIZE, BUFFER_SIZE);
        }
    }
}

TEST_F(RingBufferTest, TestPeekPeekAt) {
    for (auto i = ZERO_SIZE; i < BUFFER_SIZE + EXTRA_BUFFER_SIZE; ++i) {
        for (auto j = ZERO_SIZE; j < BUFFER_SIZE + EXTRA_BUFFER_SIZE; ++j) {
            // Write up to i items to the ring buffer.
            auto expected_write = min(i, BUFFER_SIZE);
            iota(m_write_buffer.begin(), m_write_buffer.begin() + i, 0);
            testWrite(i, expected_write);
            checkState(expected_write == ZERO_SIZE,
                       expected_write == BUFFER_SIZE, expected_write,
                       BUFFER_SIZE - expected_write);

            // Peek up to j items from the ring buffer.
            auto expected_peek = min(j, expected_write);
            fill(m_read_buffer.begin(), m_read_buffer.end(), -1);
            iota(m_check_buffer.begin(), m_check_buffer.begin() + expected_peek,
                 0);
            fill(m_check_buffer.begin() + expected_peek, m_check_buffer.end(),
                 -1);
            testPeek(j, expected_peek);
            checkState(expected_write == ZERO_SIZE,
                       expected_write == BUFFER_SIZE, expected_write,
                       BUFFER_SIZE - expected_write);

            // Peek up to j items from the ring buffer at beginning of the
            // buffer (should give same result as above).
            fill(m_read_buffer.begin(), m_read_buffer.end(), -1);
            iota(m_check_buffer.begin(), m_check_buffer.begin() + expected_peek,
                 0);
            fill(m_check_buffer.begin() + expected_peek, m_check_buffer.end(),
                 -1);
            testPeekAt(j, 0, expected_peek);
            checkState(expected_write == ZERO_SIZE,
                       expected_write == BUFFER_SIZE, expected_write,
                       BUFFER_SIZE - expected_write);

            // Peek up to j items from the ring buffer at end of the buffer.
            auto where = j < expected_write ? expected_write - j : ZERO_SIZE;
            fill(m_read_buffer.begin(), m_read_buffer.end(), -1);
            iota(m_check_buffer.begin(), m_check_buffer.begin() + expected_peek,
                 static_cast<int8_t>(where));
            fill(m_check_buffer.begin() + expected_peek, m_check_buffer.end(),
                 -1);
            testPeekAt(j, where, expected_peek);
            checkState(expected_write == ZERO_SIZE,
                       expected_write == BUFFER_SIZE, expected_write,
                       BUFFER_SIZE - expected_write);

            // Clear the ring buffer for next iteration.
            m_ring_buffer.clear();
            checkState(true, false, ZERO_SIZE, BUFFER_SIZE);
        }
    }
}

TEST_F(RingBufferTest, TestCumulativeReadWrite) {
    // We will do the following in sequence without any discarding or clearing
    // in between:
    //  - write 4 times,
    //  - read 1 time,
    //  - write 3 times,
    //  - read 2 times,
    //  - write 2 times,
    //  - read 3 times,
    //  - write 1 time,
    //  - read 4 times.
    // In each sequence, we will have written and read 10 times. At any given
    // time, there will be at most 6 sets of data in the buffer. Therefore, we
    // can do the sequence for sets of data having at most BUFFER_SIZE / 6
    // bytes.
    for (auto i = ZERO_SIZE; i <= BUFFER_SIZE / 6; ++i) {
        // Write 4 times.
        // 1.
        iota(m_write_buffer.begin(), m_write_buffer.begin() + i, 0);
        testWrite(i, i);
        checkState(i == ZERO_SIZE, false, i, BUFFER_SIZE - i);
        // 2.
        iota(m_write_buffer.begin(), m_write_buffer.begin() + i,
             static_cast<int8_t>(i));
        testWrite(i, i);
        checkState(i == ZERO_SIZE, false, i * 2, BUFFER_SIZE - i * 2);
        // 3.
        iota(m_write_buffer.begin(), m_write_buffer.begin() + i,
             static_cast<int8_t>(i * 2));
        testWrite(i, i);
        checkState(i == ZERO_SIZE, false, i * 3, BUFFER_SIZE - i * 3);
        // 4.
        iota(m_write_buffer.begin(), m_write_buffer.begin() + i,
             static_cast<int8_t>(i * 3));
        testWrite(i, i);
        checkState(i == ZERO_SIZE, false, i * 4, BUFFER_SIZE - i * 4);

        // Read 1 time.
        fill(m_read_buffer.begin(), m_read_buffer.end(), -1);
        iota(m_check_buffer.begin(), m_check_buffer.begin() + i, 0);
        fill(m_check_buffer.begin() + i, m_check_buffer.end(), -1);
        testRead(i, i);
        checkState(i == ZERO_SIZE, false, i * 3, BUFFER_SIZE - i * 3);

        // Write 3 times.
        // 1.
        iota(m_write_buffer.begin(), m_write_buffer.begin() + i,
             static_cast<int8_t>(i * 4));
        testWrite(i, i);
        checkState(i == ZERO_SIZE, false, i * 4, BUFFER_SIZE - i * 4);
        // 2.
        iota(m_write_buffer.begin(), m_write_buffer.begin() + i,
             static_cast<int8_t>(i * 5));
        testWrite(i, i);
        checkState(i == ZERO_SIZE, false, i * 5, BUFFER_SIZE - i * 5);
        // 3.
        iota(m_write_buffer.begin(), m_write_buffer.begin() + i,
             static_cast<int8_t>(i * 6));
        testWrite(i, i);
        checkState(i == ZERO_SIZE,
                   i == BUFFER_SIZE / 6 && BUFFER_SIZE % 6 == 0, i * 6,
                   BUFFER_SIZE - i * 6);

        // Read 2 times.
        // 1.
        fill(m_read_buffer.begin(), m_read_buffer.end(), -1);
        iota(m_check_buffer.begin(), m_check_buffer.begin() + i,
             static_cast<int8_t>(i));
        fill(m_check_buffer.begin() + i, m_check_buffer.end(), -1);
        testRead(i, i);
        checkState(i == ZERO_SIZE, false, i * 5, BUFFER_SIZE - i * 5);
        // 2.
        fill(m_read_buffer.begin(), m_read_buffer.end(), -1);
        iota(m_check_buffer.begin(), m_check_buffer.begin() + i,
             static_cast<int8_t>(i * 2));
        fill(m_check_buffer.begin() + i, m_check_buffer.end(), -1);
        testRead(i, i);
        checkState(i == ZERO_SIZE, false, i * 4, BUFFER_SIZE - i * 4);

        // Write 2 times.
        // 1.
        iota(m_write_buffer.begin(), m_write_buffer.begin() + i,
             static_cast<int8_t>(i * 7));
        testWrite(i, i);
        checkState(i == ZERO_SIZE, false, i * 5, BUFFER_SIZE - i * 5);
        // 2.
        iota(m_write_buffer.begin(), m_write_buffer.begin() + i,
             static_cast<int8_t>(i * 8));
        testWrite(i, i);
        checkState(i == ZERO_SIZE,
                   i == BUFFER_SIZE / 6 && BUFFER_SIZE % 6 == 0, i * 6,
                   BUFFER_SIZE - i * 6);

        // Read 3 times.
        // 1.
        fill(m_read_buffer.begin(), m_read_buffer.end(), -1);
        iota(m_check_buffer.begin(), m_check_buffer.begin() + i,
             static_cast<int8_t>(i * 3));
        fill(m_check_buffer.begin() + i, m_check_buffer.end(), -1);
        testRead(i, i);
        checkState(i == ZERO_SIZE, false, i * 5, BUFFER_SIZE - i * 5);
        // 2.
        fill(m_read_buffer.begin(), m_read_buffer.end(), -1);
        iota(m_check_buffer.begin(), m_check_buffer.begin() + i,
             static_cast<int8_t>(i * 4));
        fill(m_check_buffer.begin() + i, m_check_buffer.end(), -1);
        testRead(i, i);
        checkState(i == ZERO_SIZE, false, i * 4, BUFFER_SIZE - i * 4);
        // 3.
        fill(m_read_buffer.begin(), m_read_buffer.end(), -1);
        iota(m_check_buffer.begin(), m_check_buffer.begin() + i,
             static_cast<int8_t>(i * 5));
        fill(m_check_buffer.begin() + i, m_check_buffer.end(), -1);
        testRead(i, i);
        checkState(i == ZERO_SIZE, false, i * 3, BUFFER_SIZE - i * 3);

        // Write 1 time.
        iota(m_write_buffer.begin(), m_write_buffer.begin() + i,
             static_cast<int8_t>(i * 9));
        testWrite(i, i);
        checkState(i == ZERO_SIZE, false, i * 4, BUFFER_SIZE - i * 4);

        // Read 4 times.
        // 1.
        fill(m_read_buffer.begin(), m_read_buffer.end(), -1);
        iota(m_check_buffer.begin(), m_check_buffer.begin() + i,
             static_cast<int8_t>(i * 6));
        fill(m_check_buffer.begin() + i, m_check_buffer.end(), -1);
        testRead(i, i);
        checkState(i == ZERO_SIZE, false, i * 3, BUFFER_SIZE - i * 3);
        // 2.
        fill(m_read_buffer.begin(), m_read_buffer.end(), -1);
        iota(m_check_buffer.begin(), m_check_buffer.begin() + i,
             static_cast<int8_t>(i * 7));
        fill(m_check_buffer.begin() + i, m_check_buffer.end(), -1);
        testRead(i, i);
        checkState(i == ZERO_SIZE, false, i * 2, BUFFER_SIZE - i * 2);
        // 3.
        fill(m_read_buffer.begin(), m_read_buffer.end(), -1);
        iota(m_check_buffer.begin(), m_check_buffer.begin() + i,
             static_cast<int8_t>(i * 8));
        fill(m_check_buffer.begin() + i, m_check_buffer.end(), -1);
        testRead(i, i);
        checkState(i == ZERO_SIZE, false, i, BUFFER_SIZE - i);
        // 4.
        fill(m_read_buffer.begin(), m_read_buffer.end(), -1);
        iota(m_check_buffer.begin(), m_check_buffer.begin() + i,
             static_cast<int8_t>(i * 9));
        fill(m_check_buffer.begin() + i, m_check_buffer.end(), -1);
        testRead(i, i);
        checkState(true, false, 0, BUFFER_SIZE);
    }
}
