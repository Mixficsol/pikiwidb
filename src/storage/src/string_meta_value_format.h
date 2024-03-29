//  Copyright (c) 2024-present, Qihoo, Inc.  All rights reserved.
//  This source code is licensed under the BSD-style license found in the
//  LICENSE file in the root directory of this source tree. An additional grant
//  of patent rights can be found in the PATENTS file in the same directory.

#ifndef SRC_STRING_META_VALUE_FORMAT_H_
#define SRC_STRING_META_VALUE_FORMAT_H_

#include <string>

#include "src/base_value_format.h"
#include "storage/storage_define.h"

#define TYPE_SIZE 1

namespace storage {

/*
 *| type | reserve | cdate | timestamp |
 *|  1B  |   16B   |   8B  |    8B     |
 */
class StringMetaValue : public InternalValue {
 public:
  explicit StringMetaValue(const rocksdb::Slice& user_value) : InternalValue(user_value) {}
  rocksdb::Slice Encode() override {
    size_t usize = user_value_.size();
    size_t needed = usize + kSuffixReserveLength + 2 * kTimestampLength;
    char* dst = ReAllocIfNeeded(needed);
    memcpy(dst, user_value_.data(), usize);
    dst += usize;
    memcpy(dst, reserve_, sizeof(reserve_));
    dst += sizeof(reserve_);
    EncodeFixed64(dst, ctime_);
    dst += sizeof(ctime_);
    EncodeFixed64(dst, etime_);
    return {start_, needed};
  }
};

class ParsedStringMetaValue : public ParsedInternalValue {
 public:
  // Use this constructor after rocksdb::DB::Get();
  explicit ParsedStringMetaValue(std::string* internal_value_str) : ParsedInternalValue(internal_value_str) {
    assert(internal_value_str->size() >= kStringMetaValueSuffixLength);
    if (internal_value_str->size() >= kStringMetaValueSuffixLength) {
      int offset = 0;
      type_ = Slice(internal_value_str->data(), TYPE_SIZE);
      offset += TYPE_SIZE;
      memcpy(reserve_, internal_value_str->data() + offset, sizeof(reserve_));
      offset += kSuffixReserveLength;
      ctime_ = DecodeFixed64(internal_value_str->data() + offset);
      offset += kTimestampLength;
      etime_ = DecodeFixed64(internal_value_str->data() + offset);
    }
  }

  // Use this constructor in rocksdb::CompactionFilter::Filter();
  explicit ParsedStringMetaValue(const rocksdb::Slice& internal_value_slice)
      : ParsedInternalValue(internal_value_slice) {
    assert(internal_value_slice.size() >= kStringMetaValueSuffixLength);
    if (internal_value_slice.size() >= kStringMetaValueSuffixLength) {
      int offset = 0;
      type_ = Slice(internal_value_slice.data(), TYPE_SIZE);
      offset += TYPE_SIZE;
      memcpy(reserve_, internal_value_slice.data() + offset, sizeof(reserve_));
      offset += kSuffixReserveLength;
      ctime_ = DecodeFixed64(internal_value_slice.data() + offset);
      offset += kTimestampLength;
      etime_ = DecodeFixed64(internal_value_slice.data() + offset);
    }
  }

  void StripSuffix() override {
    if (value_) {
      value_->erase(value_->size() - kStringMetaValueSuffixLength, kStringMetaValueSuffixLength);
    }
  }

  void SetVersionToValue() override {
    if (value_) {
      char* dst = const_cast<char*>(value_->data()) + value_->size() - kStringMetaValueSuffixLength;
      EncodeFixed64(dst, version_);
    }
  }

  bool IsType(const Slice& str) { return type_ == str; }

  void SetCtimeToValue() override {
    if (value_) {
      char* dst = const_cast<char*>(value_->data()) + value_->size() - 2 * kTimestampLength;
      EncodeFixed64(dst, ctime_);
    }
  }

  void SetEtimeToValue() override {
    if (value_) {
      char* dst = const_cast<char*>(value_->data()) + value_->size() - kTimestampLength;
      EncodeFixed64(dst, etime_);
    }
  }
  bool IsValid() override { return !IsStale() != 0; }

 private:
  const size_t kStringMetaValueSuffixLength = kSuffixReserveLength + 2 * kTimestampLength;
};

}  //  namespace storage
#endif  //  SRC_LISTS_META_VALUE_FORMAT_H_
