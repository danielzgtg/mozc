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

#include "base/clock_mock.h"

#include <cstdint>

#include "base/util.h"
#include "testing/googletest.h"
#include "testing/gunit.h"

namespace mozc {

namespace {
// 2020-12-23 13:24:35 (Wed) UTC
// 123456 [usec]
constexpr uint64_t kTestSeconds = 1608729875uLL;
constexpr uint32_t kTestMicroSeconds = 123456u;

constexpr uint64_t kDeltaSeconds = 12uLL;
constexpr uint32_t kDeltaMicroSeconds = 654321u;
}  // namespace

TEST(ClockMockTest, GetTimeTest) {
  ClockMock mock(kTestSeconds, kTestMicroSeconds);
  uint64_t current_time = mock.GetTime();
  EXPECT_EQ(current_time, kTestSeconds);
}

TEST(ClockMockTest, GetTimeOfDayTest) {
  ClockMock mock(kTestSeconds, kTestMicroSeconds);
  uint64_t current_sec;
  uint32_t current_usec;
  mock.GetTimeOfDay(&current_sec, &current_usec);
  EXPECT_EQ(current_sec, kTestSeconds);
  EXPECT_EQ(current_usec, kTestMicroSeconds);
}

TEST(ClockMockTest, GetCurrentTmWithOffsetTest) {
  // 2020-12-23 13:24:00 (Wed)
  ClockMock mock(kTestSeconds, kTestMicroSeconds);
  const int offset = -35;

  const absl::Time at = mock.GetAbslTime();
  const absl::TimeZone &tz = mock.GetTimeZone();
  const absl::CivilSecond cs = absl::ToCivilSecond(at, tz) + offset;

  EXPECT_EQ(cs.year(), 2020);
  EXPECT_EQ(cs.month(), 12);
  EXPECT_EQ(cs.day(), 23);
  EXPECT_EQ(cs.hour(), 13);
  EXPECT_EQ(cs.minute(), 24);
  EXPECT_EQ(cs.second(), 0);
  EXPECT_EQ(absl::GetWeekday(cs), absl::Weekday::wednesday);
}

TEST(ClockMockTest, GetCurrentTmWithOffsetWithTimeZoneOffsetTest) {
  // 2020-12-23 13:24:00 (Wed)
  ClockMock mock(kTestSeconds, kTestMicroSeconds);
  mock.SetTimeZoneOffset(3600);
  const int offset = -35;

  const absl::Time at = mock.GetAbslTime();
  const absl::TimeZone &tz = mock.GetTimeZone();
  const absl::CivilSecond cs = absl::ToCivilSecond(at, tz) + offset;

  EXPECT_EQ(cs.year(), 2020);
  EXPECT_EQ(cs.month(), 12);
  EXPECT_EQ(cs.day(), 23);
  EXPECT_EQ(cs.hour(), 14);
  EXPECT_EQ(cs.minute(), 24);
  EXPECT_EQ(cs.second(), 0);
  EXPECT_EQ(absl::GetWeekday(cs), absl::Weekday::wednesday);
}

TEST(ClockMockTest, GetFrequencyAndTicks) {
  ClockMock mock(0, 0);
  EXPECT_NE(mock.GetFrequency(), 0);
  EXPECT_EQ(mock.GetTicks(), 0);

  constexpr uint64_t kFrequency = 123456789uLL;
  mock.SetFrequency(kFrequency);
  EXPECT_EQ(mock.GetFrequency(), kFrequency);

  constexpr uint64_t kTicks = 987654321uLL;
  mock.SetTicks(kTicks);
  EXPECT_EQ(mock.GetTicks(), kTicks);
}

TEST(ClockMockTest, PutClockForwardTest) {
  // 2024/02/22 23:11:15
  uint64_t current_sec;
  uint32_t current_usec;

  // add seconds
  {
    ClockMock mock(kTestSeconds, kTestMicroSeconds);
    const uint64_t offset_seconds = 100uLL;
    mock.PutClockForward(offset_seconds, 0u);

    mock.GetTimeOfDay(&current_sec, &current_usec);
    EXPECT_EQ(current_sec, kTestSeconds + offset_seconds);
    EXPECT_EQ(current_usec, kTestMicroSeconds);
  }

  // add micro seconds
  // 123456 [usec] + 1 [usec] => 123457 [usec]
  {
    ClockMock mock(kTestSeconds, kTestMicroSeconds);
    const uint64_t offset_micro_seconds = 1uLL;
    mock.PutClockForward(uint64_t{0}, offset_micro_seconds);

    mock.GetTimeOfDay(&current_sec, &current_usec);
    EXPECT_EQ(current_sec, kTestSeconds);
    EXPECT_EQ(current_usec, kTestMicroSeconds + offset_micro_seconds);
  }

  // add micro seconds
  // 123456 [usec] + 900000 [usec] => 1 [sec] + 023456 [usec]
  {
    ClockMock mock(kTestSeconds, kTestMicroSeconds);
    const uint64_t offset_micro_seconds = 900000uLL;
    mock.PutClockForward(uint64_t{0}, offset_micro_seconds);

    mock.GetTimeOfDay(&current_sec, &current_usec);
    EXPECT_EQ(current_sec, kTestSeconds + 1);
    const uint32_t expected_usec =
        kTestMicroSeconds + offset_micro_seconds - 1000000;
    EXPECT_EQ(current_usec, expected_usec);
  }
}

TEST(ClockMockTest, PutClockForwardByTicksTest) {
  ClockMock mock(0, 0);
  ASSERT_EQ(0, mock.GetTicks());

  constexpr uint64_t kPutForwardTicks = 100;
  mock.PutClockForwardByTicks(kPutForwardTicks);
  EXPECT_EQ(mock.GetTicks(), kPutForwardTicks);
}

TEST(ClockMockTest, AutoPutForwardTest) {
  // GetTime()
  {
    ClockMock mock(kTestSeconds, kTestMicroSeconds);
    mock.SetAutoPutClockForward(kDeltaSeconds, kDeltaMicroSeconds);
    EXPECT_EQ(mock.GetTime(), kTestSeconds);
    EXPECT_EQ(mock.GetTime(), kTestSeconds + kDeltaSeconds);
    EXPECT_EQ(mock.GetTime(), kTestSeconds + 2 * kDeltaSeconds + 1);
  }

  // GetTimeOfDay()
  {
    ClockMock mock(kTestSeconds, kTestMicroSeconds);
    mock.SetAutoPutClockForward(kDeltaSeconds, kDeltaMicroSeconds);
    uint64_t current_sec;
    uint32_t current_usec;
    mock.GetTimeOfDay(&current_sec, &current_usec);
    EXPECT_EQ(current_sec, kTestSeconds);
    EXPECT_EQ(current_usec, kTestMicroSeconds);
    mock.GetTimeOfDay(&current_sec, &current_usec);
    EXPECT_EQ(current_sec, kTestSeconds + kDeltaSeconds);
    EXPECT_EQ(current_usec, kTestMicroSeconds + kDeltaMicroSeconds);
  }

  // GetTmWithOffsetSecond()
  {
    ClockMock mock(kTestSeconds, kTestMicroSeconds);
    mock.SetAutoPutClockForward(kDeltaSeconds, kDeltaMicroSeconds);
    const int offset = -35;

    const absl::Time at = mock.GetAbslTime();
    const absl::TimeZone &tz = mock.GetTimeZone();
    absl::CivilSecond cs = absl::ToCivilSecond(at, tz) + offset;

    EXPECT_EQ(cs.year(), 2020);
    EXPECT_EQ(cs.month(), 12);
    EXPECT_EQ(cs.day(), 23);
    EXPECT_EQ(cs.hour(), 13);
    EXPECT_EQ(cs.minute(), 24);
    EXPECT_EQ(cs.second(), 0);
    EXPECT_EQ(absl::GetWeekday(cs), absl::Weekday::wednesday);

    const absl::Time at2 = mock.GetAbslTime();
    cs = absl::ToCivilSecond(at2, tz) + offset;
    EXPECT_EQ(cs.year(), 2020);
    EXPECT_EQ(cs.month(), 12);
    EXPECT_EQ(cs.day(), 23);
    EXPECT_EQ(cs.hour(), 13);
    EXPECT_EQ(cs.minute(), 24);
    EXPECT_EQ(cs.second(), kDeltaSeconds);
    EXPECT_EQ(absl::GetWeekday(cs), absl::Weekday::wednesday);
  }
}

}  // namespace mozc
