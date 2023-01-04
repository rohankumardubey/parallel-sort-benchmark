#include "merge_path.h"

#include <atomic>
#include <thread>

#include "util.h"

void MergePath::merge(Segment& left, Segment& right, Segment& dest, const size_t processor_num) {
    CHECK(processor_num > 0);

    if (dest.len == 0) {
        left.forward = 0;
        right.forward = 0;
        return;
    }

    const size_t length = (left.len + right.len) / processor_num + 1;
    std::vector<std::thread> threads;

    for (size_t processor_idx = 0; processor_idx < processor_num; ++processor_idx) {
        threads.emplace_back([&left, &right, &dest, length, processor_idx, processor_num]() {
            auto pair = _eval_diagnoal_intersection(left, right, dest.len, processor_idx, processor_num);
            size_t li = pair.first;
            size_t ri = pair.second;
            size_t di = dest.start + (processor_idx * (dest.len) / processor_num);
            _do_merge_along_merge_path(left, li, right, ri, dest, di, length);

            if (processor_idx == processor_num - 1) {
                left.forward = li - left.start;
                right.forward = ri - right.start;
            }
        });
    }

    for (size_t i = 0; i < threads.size(); ++i) {
        threads[i].join();
    }
}

std::pair<size_t, size_t> MergePath::_eval_diagnoal_intersection(const Segment& left, const Segment& right,
                                                                 const size_t d_size, const size_t processor_idx,
                                                                 const size_t processor_num) {
    size_t diag = processor_idx * d_size / processor_num;
    CHECK(d_size > 0);
    if (diag > d_size - 1) {
        diag = d_size - 1;
    }

    size_t high = diag;
    size_t low = 0;
    if (high > left.len) {
        high = left.len;
    }

    // binary search
    while (low < high) {
        size_t l_offset = low + (high - low) / 2;
        size_t r_offset = diag - l_offset;

        auto pair = _is_intersection(left, left.start + l_offset, right, right.start + r_offset);
        bool is_intersection = pair.first;
        bool all_true = pair.second;

        if (is_intersection) {
            return std::make_pair(left.start + l_offset, right.start + r_offset);
        } else if (all_true) {
            high = l_offset;
        } else {
            low = l_offset + 1;
        }
    }

    // edge cases
    for (size_t offset = 0; offset <= 1; offset++) {
        size_t l_offset = (low + offset);
        size_t r_offset = (diag - l_offset);

        auto pair = _is_intersection(left, left.start + l_offset, right, right.start + r_offset);
        bool is_intersection = pair.first;

        if (is_intersection) {
            return std::make_pair(left.start + l_offset, right.start + r_offset);
        }
    }

    CHECK(false);
    return {};
}

std::pair<bool, bool> MergePath::_is_intersection(const Segment& left, const size_t li, const Segment& right,
                                                  const size_t ri) {
    // M matrix is a matrix conprising of only boolean value
    // if A[i] > B[j], then M[i, j] = true
    // if A[i] <= B[j], then M[i, j] = false
    // and for the edge cases (i or j beyond the matrix), think about the merge path, with A as the vertical vector and B as the horizontal vector,
    // which goes from left top to right bottom, the positions below the merge path should be true, and otherwise should be false
    auto evaluator = [&left, &right](int64_t i, int64_t j) {
        if (i < 0) {
            return false;
        } else if (i >= static_cast<int64_t>(left.start + left.len)) {
            return true;
        } else if (j < 0) {
            return true;
        } else if (j >= static_cast<int64_t>(right.start + right.len)) {
            return false;
        } else {
            return left.data[i] > right.data[j];
        }
    };

    bool has_true = false;
    bool has_false = false;

    if (evaluator(static_cast<int64_t>(li) - 1, static_cast<int64_t>(ri) - 1)) {
        has_true = true;
    } else {
        has_false = true;
    }
    if (evaluator(static_cast<int64_t>(li) - 1, static_cast<int64_t>(ri))) {
        has_true = true;
    } else {
        has_false = true;
    }
    if (evaluator(static_cast<int64_t>(li), static_cast<int64_t>(ri) - 1)) {
        has_true = true;
    } else {
        has_false = true;
    }
    if (evaluator(static_cast<int64_t>(li), static_cast<int64_t>(ri))) {
        has_true = true;
    } else {
        has_false = true;
    }

    return std::make_pair(has_true && has_false, has_true);
}

void MergePath::_do_merge_along_merge_path(const Segment& left, size_t& li, const Segment& right, size_t& ri,
                                           Segment& dest, size_t& di, const size_t length) {
    const size_t d_start = di;
    while (di - d_start < length && di < dest.start + dest.len) {
        if (li >= left.start + left.len) {
            dest.data[di] = right.data[ri];
            di++;
            ri++;
        } else if (ri >= right.start + right.len) {
            dest.data[di] = left.data[li];
            di++;
            li++;
        } else if (left.data[li] <= right.data[ri]) {
            dest.data[di] = left.data[li];
            di++;
            li++;
        } else {
            dest.data[di] = right.data[ri];
            di++;
            ri++;
        }
    }
}
