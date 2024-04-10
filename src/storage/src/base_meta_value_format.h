//  Copyright (c) 2024-present, Qihoo, Inc.  All rights reserved.
//  This source code is licensed under the BSD-style license found in the
//  LICENSE file in the root directory of this source tree. An additional grant
//  of patent rights can be found in the PATENTS file in the same directory.

#ifndef SRC_BASE_META_VALUE_FORMAT_H_
#define SRC_BASE_META_VALUE_FORMAT_H_

#include <string>

#include "src/base_value_format.h"
#include "storage/storage_define.h"

#define TYPE_SIZE 1

namespace storage {

/*
 * | value | version | reserve | cdate | timestamp |
 * |       |    8B   |   16B   |   8B  |     8B    |
 */
// TODO(wangshaoyi): reformat encode, AppendTimestampAndVersion
class BaseMetaValue : public InternalValue {
 public:
  explicit BaseMetaValue(const Slice& user_value) : InternalValue(user_value) {}

  rocksdb::Slice Encode() override {
    size_t usize = user_value_.size();
    size_t needed = usize + kVersionLength + kSuffixReserveLength + 2 * kTimestampLength;
    char* dst = ReAllocIfNeeded(needed);
    memcpy(dst, user_value_.data(), user_value_.size());
    dst += user_value_.size();
    EncodeFixed64(dst, version_);
    dst += sizeof(version_);
    memcpy(dst, reserve_, sizeof(reserve_));
    dst += sizeof(reserve_);
    EncodeFixed64(dst, ctime_);
    dst += sizeof(ctime_);
    EncodeFixed64(dst, etime_);
    return {start_, needed};
  }

  uint64_t UpdateVersion() {
    int64_t unix_time;
    rocksdb::Env::Default()->GetCurrentTime(&unix_time);
    if (version_ >= uint64_t(unix_time)) {
      version_++;
    } else {
      version_ = uint64_t(unix_time);
    }
    return version_;
  }
};

class ParsedBaseMetaValue : public ParsedInternalValue {
 public:
  // Use this constructor after rocksdb::DB::Get();
  explicit ParsedBaseMetaValue(std::string* internal_value_str) : ParsedInternalValue(internal_value_str) {
    if (internal_value_str->size() >= kBaseMetaValueSuffixLength) {
      int offset = 0;
      type_ = Slice(internal_value_str->data(), TYPE_SIZE);
      offset += TYPE_SIZE;
      user_value_ = Slice(internal_value_str->data() + TYPE_SIZE,
                          internal_value_str->size() - kBaseMetaValueSuffixLength - TYPE_SIZE);
      offset += user_value_.size();
      version_ = DecodeFixed64(internal_value_str->data() + offset);
      std::cout << "version: " << version_ << std::endl;
      offset += sizeof(version_);
      memcpy(reserve_, internal_value_str->data() + offset, sizeof(reserve_));
      offset += sizeof(reserve_);
      ctime_ = DecodeFixed64(internal_value_str->data() + offset);
      std::cout << "ctime: " << ctime_ << std::endl;
      offset += sizeof(ctime_);
      etime_ = DecodeFixed64(internal_value_str->data() + offset);
      std::cout << "etime: " << etime_ << std::endl;
    }
    count_ = DecodeFixed32(internal_value_str->data() + TYPE_SIZE);
  }

  // Use this constructor in rocksdb::CompactionFilter::Filter();
  explicit ParsedBaseMetaValue(const Slice& internal_value_slice) : ParsedInternalValue(internal_value_slice) {
    if (internal_value_slice.size() >= kBaseMetaValueSuffixLength) {
      int offset = 0;
      type_ = Slice(internal_value_slice.data(), TYPE_SIZE);
      offset += TYPE_SIZE;
      user_value_ = Slice(internal_value_slice.data() + TYPE_SIZE,
                          internal_value_slice.size() - kBaseMetaValueSuffixLength - TYPE_SIZE);
      offset += user_value_.size();
      version_ = DecodeFixed64(internal_value_slice.data() + offset);
      offset += sizeof(uint64_t);
      memcpy(reserve_, internal_value_slice.data() + offset, sizeof(reserve_));
      offset += sizeof(reserve_);
      ctime_ = DecodeFixed64(internal_value_slice.data() + offset);
      offset += sizeof(ctime_);
      etime_ = DecodeFixed64(internal_value_slice.data() + offset);
    }
    count_ = DecodeFixed32(internal_value_slice.data() + TYPE_SIZE);
  }

  void StripSuffix() override {
    if (value_) {
      value_->erase(value_->size() - kBaseMetaValueSuffixLength, kBaseMetaValueSuffixLength);
    }
  }

  void SetVersionToValue() override {
    if (value_) {
      char* dst = const_cast<char*>(value_->data()) + value_->size() - kBaseMetaValueSuffixLength;
      EncodeFixed64(dst, version_);
    }
  }

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

  uint64_t InitialMetaValue() {
    this->SetCount(0);
    this->SetEtime(0);
    this->SetCtime(0);
    return this->UpdateVersion();
  }

  bool IsValid() override { return !IsStale() && Count() != 0; }

  bool check_set_count(size_t count) { return count <= INT32_MAX; }

  int32_t Count() { return count_; }

  bool IsType(const Slice& str) { return type_ == str; }

  void SetCount(int32_t count) {
    count_ = count;
    if (value_) {
      char* dst = const_cast<char*>(value_->data());
      EncodeFixed32(dst + TYPE_SIZE, count_);
    }
  }

  bool CheckModifyCount(int32_t delta) {
    int64_t count = count_;
    count += delta;
    if (count < 0 || count > INT32_MAX) {
      return false;
    }
    return true;
  }

  void ModifyCount(int32_t delta) {
    count_ += delta;
    if (value_) {
      char* dst = const_cast<char*>(value_->data());
      EncodeFixed32(dst + TYPE_SIZE, count_);
    }
  }

  uint64_t UpdateVersion() {
    int64_t unix_time;
    rocksdb::Env::Default()->GetCurrentTime(&unix_time);
    if (version_ >= static_cast<uint64_t>(unix_time)) {
      version_++;
    } else {
      version_ = static_cast<uint64_t>(unix_time);
    }
    SetVersionToValue();
    return version_;
  }

 private:
  static const size_t kBaseMetaValueSuffixLength = kVersionLength + kSuffixReserveLength + 2 * kTimestampLength;
  int32_t count_ = 0;
};

using HashesMetaValue = BaseMetaValue;
using ParsedHashesMetaValue = ParsedBaseMetaValue;
using SetsMetaValue = BaseMetaValue;
using ParsedSetsMetaValue = ParsedBaseMetaValue;
using ZSetsMetaValue = BaseMetaValue;
using ParsedZSetsMetaValue = ParsedBaseMetaValue;

}  //  namespace storage
#endif  // SRC_BASE_META_VALUE_FORMAT_H_
