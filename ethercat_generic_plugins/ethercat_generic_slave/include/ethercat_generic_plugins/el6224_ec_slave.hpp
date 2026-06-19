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

#ifndef ETHERCAT_GENERIC_PLUGINS__EL6224_EC_SLAVE_HPP_
#define ETHERCAT_GENERIC_PLUGINS__EL6224_EC_SLAVE_HPP_

#include "ethercat_generic_plugins/generic_ec_slave.hpp"

namespace ethercat_generic_plugins
{

/**
 * @brief EtherCAT slave plugin for the Beckhoff EL6224 IO-Link master.
 *
 * Extends GenericEcSlave with a device-specific CA SDO fallback handler.
 * The EL6224 channel-config objects (0x8000-0x8030, 20-byte payloads)
 * reject complete-access framing on Linux; the fallback replays them as
 * individual per-subindex SDO writes.
 *
 * Use plugin name: ethercat_generic_plugins/EL6224EcSlave
 */
class EL6224EcSlave : public GenericEcSlave
{
public:
  EL6224EcSlave() = default;
  virtual ~EL6224EcSlave() = default;

  virtual ethercat_interface::CaSdoFallbackFn getCaSdoFallback() const override;
};

}  // namespace ethercat_generic_plugins

#endif  // ETHERCAT_GENERIC_PLUGINS__EL6224_EC_SLAVE_HPP_
