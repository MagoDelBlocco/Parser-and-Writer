// Copyright 2019 Ciobanu Bogdan-Calin
// bgdnkln@gmail.com

#pragma once

#include <fstream>

#ifndef MAPPING_READ_ENABLED

class parser {
 public:
    parser() {}

    parser(const char* file_name) {
        input_file.open(file_name, std::ios::in | std::ios::binary);
        input_file.sync_with_stdio(false);
        index &= 0;
        input_file.read(buffer, SIZE);
    }

    template <typename T>
    inline parser& operator >>(T& n) {
        auto inc = [&, this]() mutable {
            if (++index == SIZE)
                index &= 0,
                input_file.read(buffer, SIZE);
        };
        for (; buffer[index] < '0' or buffer[index] > '9'; inc());
        n &= 0;
#ifdef SIGNED_READ
        sign &= 0;
        sign = (buffer[index - 1] == '-');
#endif  // SIGNED_READ
        for (; '0' <= buffer[index] and buffer[index] <= '9'; inc())
            n = (n << 3) + (n << 1) + buffer[index] - '0';
#ifdef SIGNED_READ
        n ^= ((n ^ -n) & -sign);
#endif  // SIGNED_READ
        return *this;
    }

    ~parser() {
        input_file.close();
    }

 private:
    std::fstream input_file;
    static const int SIZE = 0x400000; ///4MB
    char buffer[SIZE];
    int index;
#ifdef SIGNED_READ
    int sign;
#endif  // SIGNED_READ
};

#else

#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>

class parser {
 public:
    inline parser() {
        /// default c-tor
    }

    inline parser(const char* file_name) {
        int fd = open(file_name, O_RDONLY);
        index &= 0;
        fstat(fd, &sb);
        buffer = (char*)mmap(0, sb.st_size, PROT_READ, MAP_PRIVATE | MAP_NORESERVE, fd, 0);
        close(fd);
    }

    template <typename T>
    inline parser& operator >> (T& n) {
        n &= 0;
        for (; buffer[index] < '0' or buffer[index] > '9'; ++index);
#ifdef SIGNED
        sign &= 0;
        sign = (buffer[(index ? index - 1 : 0)] == '-');
#endif
        for (; '0' <= buffer[index] and buffer[index] <= '9'; ++index)
            n = (n << 3) + (n << 1) + buffer[index] - '0';
#ifdef SIGNED
        n ^= ((n ^ -n) & -sign);
#endif
        return *this;
    }

    ~parser() {
        munmap(buffer, sb.st_size);
    }

 private:
    struct stat sb;
    int index;
#ifdef SIGNED
    int sign;
#endif
    char* buffer;
};

#endif  // MAPPING_READ_ENABLED