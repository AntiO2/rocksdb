//  Copyright (c) 2016-present, Rockset, Inc.  All rights reserved.

#pragma once
#include "rocksdb/cloud/cloud_file_system.h"
#include "util/mutexlock.h"

namespace ROCKSDB_NAMESPACE {
class CloudScheduler;

// schedule/unschedule file deletion jobs
class DefaultCloudFileDeletionScheduler
    : public CloudFileDeletionScheduler,
      public std::enable_shared_from_this<DefaultCloudFileDeletionScheduler> {
  struct PrivateTag {};

 public:
  static std::shared_ptr<CloudFileDeletionScheduler> Create(
      const std::shared_ptr<CloudScheduler>& scheduler);

  explicit DefaultCloudFileDeletionScheduler(
      PrivateTag, const std::shared_ptr<CloudScheduler>& scheduler)
      : scheduler_(scheduler) {}

  ~DefaultCloudFileDeletionScheduler();

  void UnscheduleFileDeletion(const std::string& filename) override;
  using FileDeletionRunnable = std::function<void()>;
  // Schedule the file deletion runnable(which actually delets the file from
  // cloud) to be executed in the future (specified by `file_deletion_delay_`).
  rocksdb::IOStatus ScheduleFileDeletion(const std::string& filename,
                                         FileDeletionRunnable runnable,
                                         std::chrono::seconds delay) override;

  // Return all the files that are scheduled to be deleted(but not deleted yet)
  std::vector<std::string> TEST_FilesToDelete() const {
    std::lock_guard<std::mutex> lk(files_to_delete_mutex_);
    std::vector<std::string> files;
    for (auto& [file, handle] : files_to_delete_) {
      files.push_back(file);
      (void)handle;
    }
    return files;
  }

 private:
  // execute the `FileDeletionRunnable`
  void DoDeleteFile(const std::string& fname, FileDeletionRunnable cb);
  std::shared_ptr<CloudScheduler> scheduler_;

  mutable std::mutex files_to_delete_mutex_;
  std::unordered_map<std::string, int> files_to_delete_;
};

}  // namespace ROCKSDB_NAMESPACE
