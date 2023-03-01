#pragma once

#include <tuple>
#include <memory>

enum class Edge { Left, Right };

Edge GetEdge(int32_t path, int16_t bit) {
    return ((path >> bit) & 1) ? Edge::Right : Edge::Left;
}

class TrieNode {
public:
    TrieNode() : terminal_(false) {
    }
    TrieNode(bool terminal, int16_t symbol, size_t frequency)
        : terminal_(terminal), symbol_(symbol), frequency_(frequency) {
    }

    int16_t GetSymbol() const {
        return symbol_;
    }
    size_t GetTerminal() const {
        return terminal_;
    }
    std::shared_ptr<TrieNode> Next(Edge edge) {
        if (edge == Edge::Left and left_child_ == nullptr) {
            left_child_ = std::shared_ptr<TrieNode>(new TrieNode());
        }
        if (edge == Edge::Right and right_child_ == nullptr) {
            right_child_ = std::shared_ptr<TrieNode>(new TrieNode());
        }
        return (edge == Edge::Left) ? left_child_ : right_child_;
    }

    friend void AddPath(std::shared_ptr<TrieNode> current_node, int16_t path_len, int32_t path, int16_t symbol);
    friend bool TrieNodeLess(const std::shared_ptr<TrieNode> first_node, const std::shared_ptr<TrieNode> second_node);
    friend bool TrieNodeGreater(const std::shared_ptr<TrieNode> first_node,
                                const std::shared_ptr<TrieNode> second_node);

    friend std::shared_ptr<TrieNode> MergeTries(std::shared_ptr<TrieNode> first_root,
                                                std::shared_ptr<TrieNode> second_root);

    friend void DFS(int16_t distance_to_root, std::shared_ptr<TrieNode> current_node,
                    std::unordered_map<int16_t, int16_t> &code_length);

private:
    bool terminal_;
    int16_t symbol_;
    size_t frequency_;
    int16_t code_length_;

    std::shared_ptr<TrieNode> left_child_ = nullptr;
    std::shared_ptr<TrieNode> right_child_ = nullptr;
};

using TrieNodePtr = std::shared_ptr<TrieNode>;

TrieNodePtr MergeTries(TrieNodePtr first_root, TrieNodePtr second_root) {
    TrieNodePtr new_root(new TrieNode(false, std::min(first_root->symbol_, second_root->symbol_),
                                      first_root->frequency_ + second_root->frequency_));
    new_root->left_child_ = first_root;
    new_root->right_child_ = second_root;
    return new_root;
}

bool TrieNodeLess(const TrieNodePtr first_node, const TrieNodePtr second_node) {
    if (first_node->frequency_ == second_node->frequency_) {
        return first_node->symbol_ < second_node->symbol_;
    }
    return first_node->frequency_ < second_node->frequency_;
}

bool TrieNodeGreater(const TrieNodePtr first_node, const TrieNodePtr second_node) {
    if (first_node->code_length_ == second_node->code_length_) {
        return first_node->symbol_ < second_node->symbol_;
    }
    return first_node->code_length_ < second_node->code_length_;
}

void DFS(int16_t distance_to_root, std::shared_ptr<TrieNode> current_node,
         std::unordered_map<int16_t, int16_t> &code_length) {
    if (current_node == nullptr) {
        return;
    }
    if (current_node->terminal_) {
        code_length[current_node->symbol_] = distance_to_root;
        current_node->code_length_ = distance_to_root;
    }
    ++distance_to_root;
    DFS(distance_to_root, current_node->left_child_, code_length);
    DFS(distance_to_root, current_node->right_child_, code_length);
}

void AddPath(TrieNodePtr current_node, int16_t path_len, int32_t path, int16_t symbol) {
    for (int16_t i = static_cast<int16_t>(path_len - 1); i >= 0; --i) {
        current_node = current_node->Next(GetEdge(path, i));
    }
    current_node->terminal_ = true;
    current_node->symbol_ = symbol;
}
