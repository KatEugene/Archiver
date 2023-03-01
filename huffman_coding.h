#pragma once

#include <algorithm>
#include <bitset>
#include <tuple>
#include <unordered_map>
#include <vector>

#include "bit_handler.h"
#include "trie.h"
#include "priority_queue.h"

const int16_t FILENAME_END = 256;
const int16_t ONE_MORE_FILE = 257;
const int16_t ARCHIVE_END = 258;
const int32_t BIT = 1;
const int32_t BYTE = 8;
const int32_t ARCHIVED_BYTE = 9;

template <typename in_stream, typename out_stream>
class HuffmanCoding {
public:
    HuffmanCoding() {
    }
    void EncodeFile(BitReader<in_stream> &bit_reader, BitWriter<out_stream> &bit_writer, BitString &bit_string,
                    std::string filename, bool is_file_last) {
        std::unordered_map<int16_t, size_t> symbol_frequency;
        std::unordered_map<int16_t, int16_t> code_length;
        std::vector<TrieNodePtr> trie_nodes;
        std::unordered_map<int16_t, int32_t> code;

        CountFrequencies(bit_reader, symbol_frequency);
        symbol_frequency[FILENAME_END] = 1;
        symbol_frequency[ONE_MORE_FILE] = 1;
        symbol_frequency[ARCHIVE_END] = 1;
        for (size_t i = 0; i < filename.size(); ++i) {
            ++symbol_frequency[filename[i]];
        }
        SetCodeLengths(symbol_frequency, code_length, trie_nodes);
        SetCodes(code_length, trie_nodes, code);

        bit_reader.Reset();
        int32_t symbols_count = static_cast<int32_t>(trie_nodes.size());
        bit_string.Update(bit_writer, ARCHIVED_BYTE, symbols_count);

        for (int32_t i = 0; i < symbols_count; ++i) {
            bit_string.Update(bit_writer, ARCHIVED_BYTE, trie_nodes[i]->GetSymbol());
        }
        int32_t max_symbol_code_size = 0;
        for (const auto &trie_node : trie_nodes) {
            max_symbol_code_size =
                std::max(max_symbol_code_size, static_cast<int32_t>(code_length[trie_node->GetSymbol()]));
        }
        std::vector<int32_t> count_of_length(max_symbol_code_size + 1);
        for (const auto &[symbol, length] : code_length) {
            count_of_length[length]++;
        }
        for (int32_t i = 1; i <= max_symbol_code_size; ++i) {
            bit_string.Update(bit_writer, ARCHIVED_BYTE, count_of_length[i]);
        }
        for (size_t i = 0; i < filename.size(); ++i) {
            bit_string.Update(bit_writer, code_length[filename[i]], code[filename[i]]);
        }
        bit_string.Update(bit_writer, code_length[FILENAME_END], code[FILENAME_END]);
        while (not bit_reader.Empty()) {
            int16_t symbol = bit_reader.ReadNext();
            bit_string.Update(bit_writer, code_length[symbol], code[symbol]);
        }
        if (is_file_last) {
            bit_string.Update(bit_writer, code_length[ARCHIVE_END], code[ARCHIVE_END]);

            bit_string.AddBits(BYTE - bit_string.GetLength(), 0);
            bit_writer.WriteNext(bit_string.GetBits(BYTE));

            bit_writer.OutBuffer();
        } else {
            bit_string.Update(bit_writer, code_length[ONE_MORE_FILE], code[ONE_MORE_FILE]);
        }
    }

    bool DecodeFile(BitReader<in_stream> &bit_reader, BitString &bit_string) {
        unsigned char byte = bit_reader.ReadNext();
        bit_string.AddBits(BYTE, byte);
        if (not bit_string.IsResidueFull(ARCHIVED_BYTE)) {
            byte = bit_reader.ReadNext();
            bit_string.AddBits(BYTE, byte);
        }
        int32_t symbols_count = bit_string.GetBits(ARCHIVED_BYTE);
        std::vector<std::pair<int16_t, int16_t>> symbols_and_lengths(symbols_count);
        for (int32_t i = 0; i < symbols_count; ++i) {
            byte = bit_reader.ReadNext();
            bit_string.AddBits(BYTE, byte);
            if (not bit_string.IsResidueFull(ARCHIVED_BYTE)) {
                byte = bit_reader.ReadNext();
                bit_string.AddBits(BYTE, byte);
            }
            int16_t symbol = bit_string.GetBits(ARCHIVED_BYTE);
            symbols_and_lengths[i] = {-1, symbol};
        }

        int32_t current_index = 0;
        for (int16_t current_length = 1; current_index < symbols_count; ++current_length) {
            byte = bit_reader.ReadNext();
            bit_string.AddBits(BYTE, byte);
            if (not bit_string.IsResidueFull(ARCHIVED_BYTE)) {
                byte = bit_reader.ReadNext();
                bit_string.AddBits(BYTE, byte);
            }
            int16_t length_count = bit_string.GetBits(ARCHIVED_BYTE);
            for (int16_t j = 0; j < length_count; ++j) {
                symbols_and_lengths[current_index + j].first = current_length;
            }
            current_index += length_count;
        }

        sort(symbols_and_lengths.begin(), symbols_and_lengths.end());
        int32_t current_code = 0;
        int32_t current_length = 0;
        std::unordered_map<int16_t, int32_t> code;
        for (size_t i = 0; i < symbols_and_lengths.size(); ++i) {
            current_code <<= symbols_and_lengths[i].first - current_length;
            current_length = symbols_and_lengths[i].first;
            code[symbols_and_lengths[i].second] = current_code;
            ++current_code;
        }
        TrieNodePtr root(new TrieNode);
        for (const auto &[length, symbol] : symbols_and_lengths) {
            AddPath(root, length, code[symbol], symbol);
        }

        //-------------------------------------------------------------------------------------------------

        std::string filename = GetFilename(root, bit_reader, bit_string);
        TrieNodePtr current_node = root;
        BitWriter<out_stream> bit_writer(filename);
        bool end_of_file = false;
        bool end_of_archive = false;
        while (not end_of_file) {
            byte = bit_reader.ReadNext();
            bit_string.AddBits(BYTE, byte);
            while (not end_of_file and bit_string.IsResidueFull(BIT)) {
                int16_t bit = bit_string.GetBits(BIT);
                current_node = current_node->Next(bit ? Edge::Right : Edge::Left);
                if (current_node->GetTerminal()) {
                    int16_t symbol = current_node->GetSymbol();
                    if (symbol == ARCHIVE_END) {
                        end_of_file = true;
                        end_of_archive = true;
                    } else if (symbol == ONE_MORE_FILE) {
                        end_of_file = true;
                    } else {
                        bit_writer.WriteNext(static_cast<char>(symbol));
                    }
                    current_node = root;
                }
            }
        }
        bit_writer.OutBuffer();
        return end_of_archive;
    }

private:
    void CountFrequencies(BitReader<in_stream> &bit_reader, std::unordered_map<int16_t, size_t> &symbol_frequency) {
        while (not bit_reader.Empty()) {
            unsigned char symbol = bit_reader.ReadNext();
            ++symbol_frequency[symbol];
        }
    }
    void SetCodeLengths(const std::unordered_map<int16_t, size_t> &symbol_frequency,
                        std::unordered_map<int16_t, int16_t> &code_length, std::vector<TrieNodePtr> &trie_nodes) {
        PriorityQueue<TrieNodePtr, std::vector<TrieNodePtr>, decltype(&TrieNodeLess)> priority_queue(&TrieNodeLess);
        for (const auto &[symbol, frequency] : symbol_frequency) {
            TrieNodePtr new_node = TrieNodePtr(new TrieNode(true, symbol, frequency));
            priority_queue.Insert(new_node);
            trie_nodes.push_back(new_node);
        }
        while (priority_queue.Size() > 1) {
            TrieNodePtr first_min_node = priority_queue.ExtractRoot();
            TrieNodePtr second_min_node = priority_queue.ExtractRoot();
            TrieNodePtr merged_node = MergeTries(first_min_node, second_min_node);
            priority_queue.Insert(merged_node);
        }

        TrieNodePtr root = priority_queue.GetRoot();
        DFS(0, root, code_length);
    }
    void SetCodes(std::unordered_map<int16_t, int16_t> &code_length, std::vector<TrieNodePtr> &trie_nodes,
                  std::unordered_map<int16_t, int32_t> &code) {
        sort(trie_nodes.begin(), trie_nodes.end(), TrieNodeGreater);
        int32_t current_code = 0;
        int32_t current_length = 0;
        for (size_t i = 0; i < trie_nodes.size(); ++i) {
            current_code <<= code_length[trie_nodes[i]->GetSymbol()] - current_length;
            current_length = code_length[trie_nodes[i]->GetSymbol()];
            code[trie_nodes[i]->GetSymbol()] = current_code;
            ++current_code;
        }
    }
    std::string GetFilename(TrieNodePtr root, BitReader<in_stream> &bit_reader, BitString &bit_string) {
        TrieNodePtr current_node = root;
        std::string filename;
        while (true) {
            unsigned char byte = bit_reader.ReadNext();
            bit_string.AddBits(BYTE, byte);
            while (bit_string.IsResidueFull(BIT)) {
                int16_t bit = bit_string.GetBits(BIT);
                current_node = current_node->Next(bit ? Edge::Right : Edge::Left);
                if (current_node->GetTerminal()) {
                    int16_t symbol = current_node->GetSymbol();
                    if (symbol == FILENAME_END) {
                        return filename;
                    } else {
                        filename += static_cast<char>(symbol);
                    }
                    current_node = root;
                }
            }
        }
    }
};
