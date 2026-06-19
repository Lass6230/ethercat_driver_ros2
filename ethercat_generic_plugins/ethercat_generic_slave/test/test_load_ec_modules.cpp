// Copyright 2023 ICUBE Laboratory, University of Strasbourg
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <gtest/gtest.h>
#include <memory>

#include <pluginlib/class_loader.hpp>
#include "ethercat_interface/ec_slave.hpp"
#include "ethercat_generic_plugins/generic_ec_slave.hpp"
#include "ethercat_generic_plugins/el6224_ec_slave.hpp"

TEST(TestLoadGenericEcSlave, load_generic_ec_module)
{
  pluginlib::ClassLoader<ethercat_interface::EcSlave> ec_loader_(
    "ethercat_interface", "ethercat_interface::EcSlave");
  ASSERT_NO_THROW(ec_loader_.createSharedInstance("ethercat_generic_plugins/GenericEcSlave"));
}

TEST(TestLoadGenericEcSlave, load_el6224_ec_module)
{
  pluginlib::ClassLoader<ethercat_interface::EcSlave> ec_loader_(
    "ethercat_interface", "ethercat_interface::EcSlave");
  ASSERT_NO_THROW(ec_loader_.createSharedInstance("ethercat_generic_plugins/EL6224EcSlave"));
}

// ── CA SDO fallback contract tests ─────────────────────────────────────────
// These exercise only the gate-condition path of the fallback (wrong payload
// size / wrong index → returns -1 immediately, no ecrt calls → no hardware needed).

TEST(TestCaSdoFallback, generic_slave_has_no_fallback)
{
  ethercat_generic_plugins::GenericEcSlave slave;
  EXPECT_FALSE(slave.getCaSdoFallback())
    << "GenericEcSlave must not carry any device-specific fallback";
}

TEST(TestCaSdoFallback, el6224_slave_has_fallback)
{
  ethercat_generic_plugins::EL6224EcSlave slave;
  EXPECT_TRUE(slave.getCaSdoFallback())
    << "EL6224EcSlave must provide a non-null CA SDO fallback";
}

TEST(TestCaSdoFallback, fallback_ignores_wrong_payload_size)
{
  ethercat_generic_plugins::EL6224EcSlave slave;
  auto fn = slave.getCaSdoFallback();
  ASSERT_TRUE(fn);
  uint8_t dummy[20] = {};
  uint32_t abort_code = 0;
  // EL6224 expects exactly 20 bytes; anything else must be rejected
  EXPECT_LT(fn(nullptr, 0, 0x8000, dummy, 10, &abort_code), 0);
  EXPECT_LT(fn(nullptr, 0, 0x8000, dummy, 21, &abort_code), 0);
}

TEST(TestCaSdoFallback, fallback_ignores_out_of_range_index)
{
  ethercat_generic_plugins::EL6224EcSlave slave;
  auto fn = slave.getCaSdoFallback();
  ASSERT_TRUE(fn);
  uint8_t dummy[20] = {};
  uint32_t abort_code = 0;
  // Only 0x8000-0x8030 aligned to 0x10 are EL6224 channel-config objects
  EXPECT_LT(fn(nullptr, 0, 0x7ff0, dummy, 20, &abort_code), 0);
  EXPECT_LT(fn(nullptr, 0, 0x8040, dummy, 20, &abort_code), 0);
}

TEST(TestCaSdoFallback, fallback_ignores_unaligned_index)
{
  ethercat_generic_plugins::EL6224EcSlave slave;
  auto fn = slave.getCaSdoFallback();
  ASSERT_TRUE(fn);
  uint8_t dummy[20] = {};
  uint32_t abort_code = 0;
  // Index must be a multiple of 0x10
  EXPECT_LT(fn(nullptr, 0, 0x8005, dummy, 20, &abort_code), 0);
  EXPECT_LT(fn(nullptr, 0, 0x8011, dummy, 20, &abort_code), 0);
}
