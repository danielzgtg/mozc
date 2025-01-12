// Copyright 2010-2021, Google Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "base/singleton.h"

#include <cstdlib>

#include "base/thread.h"
#include "testing/gunit.h"
#include "absl/time/clock.h"
#include "absl/time/time.h"

namespace mozc {
namespace {

static int g_counter = 0;

class TestInstance {
 public:
  TestInstance() { ++g_counter; }
};

class ThreadInstance {
 public:
  ThreadInstance() {
    // Wait two secs to test the singleton
    // can safely block the initialization procedure.
    absl::SleepFor(absl::Seconds(2));
    ++g_counter;
  }
};

class ThreadTest : public Thread {
 public:
  void Run() override { instance_ = Singleton<ThreadInstance>::get(); }

  ThreadInstance *get() { return instance_; }

  ThreadTest() : instance_(nullptr) {}

 private:
  ThreadInstance *instance_;
};

// Cannot have a testcase for Singleton::SingletonFinalizer,
// since it affects other tests using Singleton objects.
TEST(SingletonTest, BasicTest) {
  g_counter = 0;
  TestInstance *t1 = Singleton<TestInstance>::get();
  TestInstance *t2 = Singleton<TestInstance>::get();
  TestInstance *t3 = Singleton<TestInstance>::get();
  EXPECT_EQ(t1, t2);
  EXPECT_EQ(t2, t3);
  EXPECT_EQ(g_counter, 1);
}

TEST(SingletonTest, ThreadTest) {
  // call Singleton::get() at the same time from
  // different threads. Make sure that get() returns
  // the same instance

  g_counter = 0;

  ThreadTest test1;
  ThreadTest test2;
  ThreadTest test3;

  // Create the ThreadInstance simultaneously.
  test1.Start("ThreadTest");
  test2.Start("ThreadTest");
  test3.Start("ThreadTest");

  test1.Join();
  test2.Join();
  test3.Join();

  EXPECT_EQ(g_counter, 1);

  // three instances must be the same.
  EXPECT_EQ(test1.get(), test2.get());
  EXPECT_EQ(test2.get(), test3.get());
}

class ValueHolder {
 public:
  ValueHolder() = default;
  ~ValueHolder() { *dtor_called_ = true; }

  int value_ = 0;
  bool *dtor_called_ = nullptr;
};

TEST(SingletonTest, Reset) {
  bool dtor_called = false;
  {
    auto *ptr = Singleton<ValueHolder>::get();
    ptr->value_ = 12345;
    ptr->dtor_called_ = &dtor_called;
  }
  {
    auto *ptr = Singleton<ValueHolder>::get();
    EXPECT_EQ(ptr->value_, 12345);
    EXPECT_FALSE(dtor_called);
  }
  {
    Singleton<ValueHolder>::Delete();
    EXPECT_TRUE(dtor_called);
    auto *ptr = Singleton<ValueHolder>::get();
    // Reconstructed value, so the it's not equal to 12345.
    EXPECT_EQ(ptr->value_, 0);
  }
}

}  // namespace
}  // namespace mozc
