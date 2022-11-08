// Copyright (c) 2017 Rockset

#include "cloud/cloud_manifest.h"

#include "env/composite_env_wrapper.h"
#include "file/writable_file_writer.h"
#include "rocksdb/env.h"
#include "test_util/testharness.h"

namespace ROCKSDB_NAMESPACE {

using std::make_shared;
class CloudManifestTest : public testing::Test {
 public:
  CloudManifestTest() {
    tmp_dir_ = test::TmpDir() + "/cloud_manifest_test";
    env_ = Env::Default();
    env_->CreateDir(tmp_dir_);
  }

 protected:
  std::string tmp_dir_;
  Env* env_;
};

TEST_F(CloudManifestTest, BasicTest) {
  {
    std::unique_ptr<CloudManifest> manifest;
    ASSERT_OK(CloudManifest::CreateForEmptyDatabase("firstEpoch", &manifest));

    ASSERT_EQ(manifest->GetEpoch(130), "firstEpoch");
  }
  {
    std::unique_ptr<CloudManifest> manifest;
    ASSERT_OK(CloudManifest::CreateForEmptyDatabase("firstEpoch", &manifest));
    EXPECT_TRUE(manifest->AddEpoch(10, make_shared<RandomUniqueEpoch>("secondEpoch")));
    EXPECT_TRUE(manifest->AddEpoch(10, make_shared<RandomUniqueEpoch>("thirdEpoch")));
    EXPECT_TRUE(manifest->AddEpoch(40, make_shared<RandomUniqueEpoch>("fourthEpoch")));

    for (int iter = 0; iter < 2; ++iter) {
      ASSERT_EQ(manifest->GetEpoch(0), "firstEpoch");
      ASSERT_EQ(manifest->GetEpoch(5), "firstEpoch");
      ASSERT_EQ(manifest->GetEpoch(10), "thirdEpoch");
      ASSERT_EQ(manifest->GetEpoch(11), "thirdEpoch");
      ASSERT_EQ(manifest->GetEpoch(39), "thirdEpoch");
      ASSERT_EQ(manifest->GetEpoch(40), "fourthEpoch");
      ASSERT_EQ(manifest->GetEpoch(41), "fourthEpoch");

      // serialize and deserialize
      auto tmpfile = tmp_dir_ + "/cloudmanifest";
      {
        std::unique_ptr<WritableFileWriter> writer;
        ASSERT_OK(WritableFileWriter::Create(env_->GetFileSystem(), tmpfile,
                                             FileOptions(), &writer, nullptr));
        ASSERT_OK(manifest->WriteToLog(std::move(writer)));
      }

      manifest.reset();
      {
        std::unique_ptr<SequentialFileReader> reader;
        ASSERT_OK(SequentialFileReader::Create(
            env_->GetFileSystem(), tmpfile, FileOptions(), &reader, nullptr));
        ASSERT_OK(CloudManifest::LoadFromLog(std::move(reader), &manifest));
      }
    }
  }
}

TEST_F(CloudManifestTest, IdempotencyTest) {
  std::unique_ptr<CloudManifest> manifest;
  ASSERT_OK(CloudManifest::CreateForEmptyDatabase("epoch1", &manifest));

  EXPECT_FALSE(manifest->AddEpoch(1, make_shared<RandomUniqueEpoch>("epoch1")));
  EXPECT_TRUE(manifest->AddEpoch(2, make_shared<RandomUniqueEpoch>("epoch2")));
  EXPECT_FALSE(manifest->AddEpoch(2, make_shared<RandomUniqueEpoch>("epoch2")));
  EXPECT_FALSE(manifest->AddEpoch(3, make_shared<RandomUniqueEpoch>("epoch1")));
  EXPECT_TRUE(manifest->AddEpoch(3, make_shared<RandomUniqueEpoch>("epoch3")));
}
}  //  namespace ROCKSDB_NAMESPACE

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
