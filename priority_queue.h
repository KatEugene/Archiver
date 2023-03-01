#pragma once

#include <cstddef>

template <typename T, typename Container, typename Comparator>
class PriorityQueue {
public:
    explicit PriorityQueue(Comparator comparator) : comparator_(comparator) {
    }
    void Insert(T element) {
        priority_queue_.push_back(element);
        SiftUp(priority_queue_.size() - 1);
    }
    T ExtractRoot() {
        T root = priority_queue_.front();
        priority_queue_.front() = priority_queue_.back();
        priority_queue_.pop_back();
        SiftDown(0);
        return root;
    }
    T GetRoot() {
        return priority_queue_.front();
    }
    size_t Size() {
        return priority_queue_.size();
    }

private:
    Container priority_queue_;
    Comparator comparator_;

    void SiftUp(size_t position) {
        while (position > 0) {
            size_t ancestor = (position - 1) / 2;
            if (comparator_(priority_queue_[position], priority_queue_[ancestor])) {
                swap(priority_queue_[position], priority_queue_[ancestor]);
                position = ancestor;
            } else {
                return;
            }
        }
    }
    void SiftDown(size_t position) {
        while (position < priority_queue_.size()) {
            size_t first_child = position * 2 + 1;
            size_t second_child = position * 2 + 2;
            if (first_child >= priority_queue_.size()) {
                return;
            }

            size_t min_child = first_child;
            if (second_child < priority_queue_.size() and
                comparator_(priority_queue_[second_child], priority_queue_[first_child])) {
                min_child = second_child;
            }

            if (comparator_(priority_queue_[position], priority_queue_[min_child])) {
                return;
            }
            swap(priority_queue_[position], priority_queue_[min_child]);
            position = min_child;
        }
    }
};
