// Copyright 2024 ICUBE Laboratory, University of Strasbourg
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

#include "ethercat_generic_plugins/el6224_ec_slave.hpp"

#include <ecrt.h>

namespace ethercat_generic_plugins
{

namespace
{

int sdo_download_segment(
  ec_master_t * master,
  uint16_t slave_position,
  uint16_t index,
  uint8_t subindex,
  const uint8_t * data,
  size_t data_size,
  uint32_t * abort_code)
{
  return ecrt_master_sdo_download(
    master,
    slave_position,
    index,
    subindex,
    const_cast<uint8_t *>(data),
    data_size,
    abort_code);
}

// Fallback for Beckhoff EL6224 IO-Link master: its channel-config objects
// (0x8000..0x8030, 20-byte payload) reject complete-access framing on Linux.
// This replays the same data as individual per-subindex SDO writes.
int el6224_ca_fallback(
  ec_master_t * master,
  uint16_t slave_position,
  uint16_t index,
  const uint8_t * payload,
  size_t payload_size,
  uint32_t * abort_code)
{
  if (payload_size != 20 || index < 0x8000 || index > 0x8030 || (index % 0x10) != 0) {
    return -1;
  }

  int ret = 0;

  if (index == 0x8000 || index == 0x8010) {
    ret = sdo_download_segment(master, slave_position, index, 0x04, payload + 0, 4, abort_code);
    if (ret) {return ret;}
    ret = sdo_download_segment(master, slave_position, index, 0x05, payload + 4, 4, abort_code);
    if (ret) {return ret;}
    ret = sdo_download_segment(master, slave_position, index, 0x20, payload + 8, 1, abort_code);
    if (ret) {return ret;}
    ret = sdo_download_segment(master, slave_position, index, 0x21, payload + 9, 1, abort_code);
    if (ret) {return ret;}
    ret = sdo_download_segment(master, slave_position, index, 0x22, payload + 10, 1, abort_code);
    if (ret) {return ret;}
    ret = sdo_download_segment(master, slave_position, index, 0x23, payload + 11, 1, abort_code);
    if (ret) {return ret;}
    ret = sdo_download_segment(master, slave_position, index, 0x24, payload + 12, 1, abort_code);
    if (ret) {return ret;}
    ret = sdo_download_segment(master, slave_position, index, 0x25, payload + 13, 1, abort_code);
    if (ret) {return ret;}
    ret = sdo_download_segment(master, slave_position, index, 0x26, payload + 14, 2, abort_code);
    if (ret) {return ret;}
    ret = sdo_download_segment(master, slave_position, index, 0x27, payload + 16, 2, abort_code);
    if (ret) {return ret;}
  }

  // All channels: write Master Control.
  return sdo_download_segment(master, slave_position, index, 0x28, payload + 18, 2, abort_code);
}

}  // namespace

ethercat_interface::CaSdoFallbackFn EL6224EcSlave::getCaSdoFallback() const
{
  return el6224_ca_fallback;
}

}  // namespace ethercat_generic_plugins

#include <pluginlib/class_list_macros.hpp>

PLUGINLIB_EXPORT_CLASS(ethercat_generic_plugins::EL6224EcSlave, ethercat_interface::EcSlave)
