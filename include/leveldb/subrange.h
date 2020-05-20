
//
// Created by Haoyu Huang on 5/4/20.
// Copyright (c) 2020 University of Southern California. All rights reserved.
//

#ifndef LEVELDB_SUBRANGE_H
#define LEVELDB_SUBRANGE_H

#include <vector>
#include <atomic>
#include "nova/logging.hpp"

#include "leveldb/slice.h"
#include "leveldb/comparator.h"

namespace leveldb {

    struct Range {
        std::string lower = {};
        std::string upper = {};
        bool lower_inclusive = true;
        bool upper_inclusive = false;
        uint32_t num_duplicates = 0;
        double ninserts = 0;
        double insertion_ratio = 0;
        int prior_subrange_id = -1;

        uint32_t Encode(char *buf) const;

        bool Decode(Slice *input);

        std::string DebugString() const;

        bool Equals(const Range &other, const Comparator *comparator) const;

        bool
        IsSmallerThanLower(const Slice &key,
                           const Comparator *comparator) const;

        bool
        IsGreaterThanLower(const Slice &key,
                           const Comparator *comparator) const;

        bool
        IsGreaterThanUpper(const Slice &key,
                           const Comparator *comparator) const;

        bool IsAPoint(const Comparator *comparator) const;

        uint64_t lower_int() const;

        uint64_t upper_int() const;
    };


    struct SubRange {
        std::vector<Range> tiny_ranges;
        uint32_t num_duplicates = 0;
        double ninserts = 0;
        double insertion_ratio = 0;

        int start_tid = 0;
        int end_tid = 0;

        int GetCompactionThreadId(std::atomic_int_fast32_t *rr_id,
                                  bool *merge_memtables_without_flushing) const;

        uint32_t decoded_subrange_id = 0;

        void UpdateStats(double num_inserts_since_last_major);

        bool BinarySearch(const leveldb::Slice &key,
                          int *tinyrange_id,
                          const Comparator *user_comparator) const;

        Range &first() {
            return tiny_ranges[0];
        }

        Range &last() {
            return tiny_ranges[tiny_ranges.size() - 1];
        }

        int keys() const;

        uint32_t Encode(char *buf, uint32_t subrange_id) const;

        bool Decode(Slice *input);

        uint32_t EncodeForCompaction(char *buf, uint32_t subrange_id) const;

        bool DecodeForCompaction(Slice *input);

        std::string DebugString() const;

        bool Equals(const SubRange &other, const Comparator *comparator) const;

        bool
        IsSmallerThanLower(const Slice &key,
                           const Comparator *comparator) const;

        bool
        IsGreaterThanLower(const Slice &key,
                           const Comparator *comparator) const;

        bool
        IsGreaterThanUpper(const Slice &key,
                           const Comparator *comparator) const;

        bool IsAPoint(const Comparator *comparator);
    };

    class SubRanges {
    public:
        ~SubRanges();

        SubRanges() = default;

        SubRanges(const SubRanges &other);

        explicit SubRanges(const std::vector<SubRange> &other);

        SubRange &first() {
            return subranges[0];
        }

        SubRange &last() {
            return subranges[subranges.size() - 1];
        }

        std::vector<SubRange> subranges;

        bool BinarySearch(const leveldb::Slice &key,
                          int *subrange_id,
                          const Comparator *user_comparator) const;

        bool
        BinarySearchWithDuplicate(const leveldb::Slice &key,
                                  unsigned int *rand_seed, int *subrange_id,
                                  const Comparator *user_comparator) const;

        std::string DebugString() const;

        void AssertSubrangeBoundary(const Comparator *comparator);
    };
}


#endif //LEVELDB_SUBRANGE_H
