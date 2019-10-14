// Copyright 2019 Ciobanu Bogdan-Calin
// bgdnkln@gmail.com

#pragma once

#include <fstream>

#ifndef MAPPING_WRITE_ENABLED

class writer {
 public:
    writer() {};

    writer(const char* file_name) {
        output_file.open(file_name, std::ios::out | std::ios::binary);
        output_file.sync_with_stdio(false);
        index &= 0;
    }
    
    template <typename T>
    inline writer& operator <<(T target) {
        auto inc = [&, this]() mutable {
            if (++index == SIZE)
                index &= 0,
                output_file.write(buffer, SIZE);
        };

        aux &= 0;
#ifdef SIGNED_WRITE
        sign=1;
        if (target<0)
            sign=-1;
#endif  // SIGNED_WRITE
        if (target == 0) {
            buffer[index] = '0';
            inc();
            return *this;
        }
        for (; target; target = target / 10)
#ifndef SIGNED_WRITE
            nr[aux++] = target % 10 + '0';
#else
            nr[aux++] = (sign * target) % 10 + '0';
        if (sign==-1)
            buffer[index]='-',inc();
#endif  // SIGNED_WRITE
        for (; aux; inc())
            buffer[index] = nr[--aux];
        return *this;
    }

    inline writer& operator <<(const char* target) {
        auto inc = [&, this]() mutable {
            if (++index == SIZE)
                index &= 0,
                output_file.write(buffer, SIZE);
        };

        aux &= 0;
        while (target[aux])
            buffer[index] = target[aux++], inc();
        return *this;
    }

    inline void close() {
        delete this;
    }

    ~writer() {
        output_file.write(buffer, index);
        output_file.close();
    }

 private:
    static const int SIZE = 0x200000; ///2MB
    int index, aux;
#ifdef SIGNED_WRITE
    int sign;
#endif  // SIGNED_WRITE
    char buffer[SIZE], nr[24];
    std::fstream output_file;
};

#else

#include <vector>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>

class writer {
public:
    writer() {
        // DEFAULT C-TOR
    };

    writer(const char* file_name) {
        fd = open(file_name, O_RDWR | O_CREAT, (mode_t)0600);
        index &= 0;
        buffers.push_back((char*)mmap(0, SIZE, PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0));
        buffer_sizes.push_back(0);
        active_buffer &= 0;
    }

    template <typename T>
    inline writer& operator <<(T target) {
        auto inc = [&, this]() mutable {
            if (++index == SIZE) {
                buffer_sizes[active_buffer] = index;
                index &= 0;
                buffers.push_back((char*)mmap(0, SIZE, PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0));
                buffer_sizes.push_back(index);
                ++active_buffer;
            }
        };

        aux &= 0;
#ifdef SIGNED_WRITE
        sign = 1;
        if (target < 0)
            sign = -1;
#endif  // SIGNED_WRITE
        if (target == 0) {
            buffers[active_buffer][index] = '0';
            inc();
            return *this;
        }
        for (; target; target = target / 10)
#ifndef SIGNED_WRITE
            nr[aux++] = target % 10 + '0';
#else
            nr[aux++] = (sign * target) % 10 + '0';
        if (sign == -1)
            buffers[active_buffer][index] = '-', inc();
#endif  // SIGNED_WRITE
        for (; aux; inc())
            buffers[active_buffer][index] = nr[--aux];
        buffer_sizes[active_buffer] = index;
        return *this;
    }

    inline writer& operator <<(const char* target) {
        auto inc = [&, this]() mutable {
            if (++index == SIZE) {
                buffer_sizes[active_buffer] = index;
                index &= 0;
                buffers.push_back((char*)mmap(0, SIZE, PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0));
                buffer_sizes.push_back(index);
                ++active_buffer;
            }
        };

        aux &= 0;
        for (; target[aux]; inc())
            buffers[active_buffer][index] = target[aux++];
        return *this;
    }

    ~writer() {
        size_t total_size = 0;
        for (auto i : buffer_sizes)
            total_size += i;

        lseek(fd, total_size - 1, SEEK_SET);
        write(fd, "", 1);

        char* map = (char*)mmap(0, total_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        for (unsigned i = 0, accumulated = 0; i < buffers.size(); ++i) {
            memcpy(map + accumulated, buffers[i], buffer_sizes[i]);
            munmap(buffers[i], buffer_sizes[i]);
            accumulated += buffer_sizes[i];
        }

        munmap(map, total_size);
        close(fd);
    }

private:
    static const int SIZE = 0x200000;
    std::vector<char*> buffers;
    std::vector<size_t> buffer_sizes;
    int fd, index, aux, active_buffer;
#ifdef SIGNED_WRITE
    int sign;
#endif  // SIGNED_WRITE
    char nr[24];

/*
    inline static void fastMemcpy(void* pvDest, void* pvSrc, size_t nBytes) {
        //assert(nBytes % 32 == 0);
        //assert((intptr_t(pvDest) & 31) == 0);
        //assert((intptr_t(pvSrc) & 31) == 0);
        const __m256i* pSrc = reinterpret_cast<const __m256i*>(pvSrc);
        __m256i* pDest = reinterpret_cast<__m256i*>(pvDest);
        int64_t nVects = nBytes / sizeof(*pSrc);
        for (; nVects; --nVects, ++pSrc, ++pDest) {
            const __m256i loaded = _mm256_stream_load_si256(pSrc);
            _mm256_stream_si256(pDest, loaded);
        }
        _mm_sfence();
    }
*/
};

#endif  // MAPPING_WRITE_ENABLED