#pragma once

#include <cstdint>
#include <fstream>

template <typename Stream>
class BitReader {
public:
    explicit BitReader(std::string filename) : filename_(filename) {
        stream_.open(filename, std::ios::in | std::ios::binary);
        current_index_ = 0;
        FillBuffer();
    }
    ~BitReader() {
        stream_.close();
    }
    unsigned char ReadNext() {
        unsigned char next_symbol = 0;
        if (buffer_[current_index_] < 0) {
            next_symbol = buffer_[current_index_];
        } else {
            next_symbol = 256 + buffer_[current_index_];
        }
        ++current_index_;
        --byte_count_;
        if (current_index_ == BUFFER_SIZE) {
            FillBuffer();
            current_index_ = 0;
        }
        return next_symbol;
    }
    void FillBuffer() {
        stream_.read(buffer_, BUFFER_SIZE);
        byte_count_ = stream_.gcount();
    }
    void Reset() {
        stream_ = Stream(filename_, std::ios::in | std::ios::binary);
        current_index_ = 0;
        FillBuffer();
    }
    bool Empty() {
        return byte_count_ == 0;
    }

private:
    std::string filename_;
    static const int16_t BUFFER_SIZE = 1024;
    char buffer_[BUFFER_SIZE];
    int16_t byte_count_;
    int16_t current_index_;
    Stream stream_;
};

template <typename Stream>
class BitWriter {
public:
    explicit BitWriter(std::string filename) {
        Open(filename);
    }
    ~BitWriter() {
        Close();
    }
    void WriteNext(char symbol) {
        buffer_[current_index_] = symbol;
        ++current_index_;
        ++byte_count_;
        if (current_index_ == BUFFER_SIZE) {
            OutBuffer();
            current_index_ = 0;
        }
    }
    void OutBuffer() {
        stream_.write(buffer_, byte_count_);
        byte_count_ = 0;
    }
    void Open(std::string filename) {
        stream_.open(filename, std::ios::out | std::ios::binary);
        current_index_ = 0;
    }
    void Close() {
        OutBuffer();
        stream_.close();
    }

private:
    static const int16_t BUFFER_SIZE = 1024;
    char buffer_[BUFFER_SIZE];
    int16_t byte_count_ = 0;
    int16_t current_index_ = 0;
    Stream stream_;
};

class BitString {
public:
    BitString(int32_t bit_string_length, int32_t bit_string)
        : bit_string_length_(bit_string_length), bit_string_(bit_string) {
    }
    void AddBits(int32_t bits_count, int32_t bits) {
        bit_string_length_ += bits_count;
        bit_string_ <<= bits_count;
        bit_string_ |= bits;
    }
    int16_t GetBits(int32_t bits_length) {
        int32_t residual_bits_count = bit_string_length_ - bits_length;
        bit_string_length_ -= bits_length;
        int16_t byte = static_cast<int16_t>(bit_string_ >> residual_bits_count);
        bit_string_ &= (1ll << residual_bits_count) - 1;
        return byte;
    }
    bool IsResidueFull(int32_t bits_length) const {
        return bit_string_length_ >= bits_length;
    }
    int32_t GetLength() const {
        return bit_string_length_;
    }
    template <typename T>
    void Update(BitWriter<T> &bit_writer, int32_t bits_count, int32_t bits) {
        AddBits(bits_count, bits);
        while (IsResidueFull(8)) {
            bit_writer.WriteNext(GetBits(8));
        }
    }

private:
    int32_t bit_string_length_;
    int64_t bit_string_;
};