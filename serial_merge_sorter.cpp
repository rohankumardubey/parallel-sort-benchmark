#include "serial_merge_sorter.h"

#include <algorithm>
#include <thread>

#include "util.h"

void SerialMergeSorter::sort(std::vector<int32_t>& nums, const int32_t processor_num) {
    std::vector<std::vector<int32_t>> partial_nums;
    partial_nums.resize(processor_num);

    for (int32_t i = 0; i < processor_num; ++i) {
        partial_nums[i].reserve(nums.size() / processor_num);
    }

    for (int32_t i = 0; i < nums.size(); ++i) {
        partial_nums[i % processor_num].push_back(nums[i]);
    }

    std::vector<std::thread> sort_threads;
    for (int32_t i = 0; i < processor_num; ++i) {
        sort_threads.emplace_back([&partial_nums, i]() { std::sort(partial_nums[i].begin(), partial_nums[i].end()); });
    }
    for (int32_t i = 0; i < sort_threads.size(); i++) {
        sort_threads[i].join();
    }

    std::vector<std::vector<int32_t>> current_level = std::move(partial_nums);
    while (current_level.size() > 1) {
        std::vector<std::vector<int32_t>> next_level;

        const auto current_size = current_level.size();

        int32_t i = 0;
        for (; i + 1 < current_size; i += 2) {
            auto& left = current_level[i];
            auto& right = current_level[i + 1];
            std::vector<int32_t> merged;
            merged.resize(left.size() + right.size());

            std::merge(left.begin(), left.end(), right.begin(), right.end(), merged.begin());

            next_level.emplace_back(std::move(merged));
        }

        if (i < current_size) {
            next_level.emplace_back(std::move(current_level[i]));
        }

        current_level = std::move(next_level);
    }

    std::swap(nums, current_level[0]);
}