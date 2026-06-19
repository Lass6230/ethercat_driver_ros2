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
//
// Author: Maciej Bednarczyk (macbednarczyk@gmail.com)

#ifndef ETHERCAT_INTERFACE__EC_SDO_MANAGER_HPP_
#define ETHERCAT_INTERFACE__EC_SDO_MANAGER_HPP_

#include <ecrt.h>
#include <string>
#include <vector>
#include <limits>
#include <algorithm>
#include <cctype>
#include <sstream>
#include <iostream>

#include "yaml-cpp/yaml.h"

namespace ethercat_interface
{

class SdoConfigEntry
{
public:
  SdoConfigEntry() {}
  ~SdoConfigEntry() {}

  void buffer_write(uint8_t * buffer)
  {
    if (complete_access && !raw_data.empty()) {
      std::copy(raw_data.begin(), raw_data.end(), buffer);
      return;
    }

    if (data_type == "uint8") {
      EC_WRITE_U8(buffer, static_cast<uint8_t>(data));
    } else if (data_type == "int8") {
      EC_WRITE_S8(buffer, static_cast<int8_t>(data));
    } else if (data_type == "uint16") {
      EC_WRITE_U16(buffer, static_cast<uint16_t>(data));
    } else if (data_type == "int16") {
      EC_WRITE_S16(buffer, static_cast<int16_t>(data));
    } else if (data_type == "uint32" || data_type == "real32" || data_type == "float") {
      EC_WRITE_U32(buffer, static_cast<uint32_t>(data));
    } else if (data_type == "int32") {
      EC_WRITE_S32(buffer, static_cast<int32_t>(data));
    } else if (data_type == "uint64" || data_type == "real64" || data_type == "double") {
      EC_WRITE_U64(buffer, static_cast<uint64_t>(data));
    } else if (data_type == "int64") {
      EC_WRITE_S64(buffer, static_cast<int64_t>(data));
    }
  }

  bool load_from_config(YAML::Node sdo_config)
  {
    // index
    if (sdo_config["index"]) {
      index = sdo_config["index"].as<uint16_t>();
    } else {
      std::cerr << "missing sdo index info" << std::endl;
      return false;
    }

    // complete access flag (optional)
    complete_access = false;
    if (sdo_config["complete_access"]) {
      complete_access = sdo_config["complete_access"].as<bool>();
    }

    // sub_index
    if (sdo_config["sub_index"]) {
      sub_index = sdo_config["sub_index"].as<uint8_t>();
    } else if (complete_access) {
      // Sub-index is ignored by complete access, keep default 0.
      sub_index = 0;
    } else {
      std::cerr << "sdo " << index << ": missing sdo info" << std::endl;
      return false;
    }

    // complete-access raw payload path
    if (complete_access) {
      std::string hex_value;
      if (sdo_config["value_hex"]) {
        hex_value = sdo_config["value_hex"].as<std::string>();
      } else if (sdo_config["value"]) {
        // Allow reusing value as hex string for CA entries.
        hex_value = sdo_config["value"].as<std::string>();
      } else {
        std::cerr << "sdo " << index << ": missing value_hex/value for complete access" << std::endl;
        return false;
      }

      std::string parse_error;
      if (!parse_hex_string(hex_value, raw_data, parse_error)) {
        std::cerr << "sdo " << index << ": invalid complete access payload: " << parse_error << std::endl;
        return false;
      }
      if (raw_data.empty()) {
        std::cerr << "sdo " << index << ": empty complete access payload" << std::endl;
        return false;
      }

      // Keep these for backwards diagnostics.
      data_type = "raw";
      data = 0;
      return true;
    }

    // scalar SDO path
    if (sdo_config["type"]) {
      data_type = sdo_config["type"].as<std::string>();
    } else {
      std::cerr << "sdo " << index << ": missing sdo data type info" << std::endl;
      return false;
    }

    if (sdo_config["value"]) {
      if (data_type == "float" || data_type == "real32") {
        float floatvalue = sdo_config["value"].as<float>();
        data = *reinterpret_cast<int *>(&floatvalue);
      } else if (data_type == "double" || data_type == "real64") {
        double doublevalue = sdo_config["value"].as<double>();
        data = *reinterpret_cast<int *>(&doublevalue);
      } else {
        data = sdo_config["value"].as<int>();
      }
    } else {
      std::cerr << "sdo " << index << ": missing sdo value" << std::endl;
      return false;
    }

    return true;
  }

  size_t data_size()
  {
    if (complete_access) {
      return raw_data.size();
    }
    return type2bytes(data_type);
  }

  uint16_t index = 0;
  uint8_t sub_index = 0;
  std::string data_type;
  int data = 0;
  bool complete_access = false;
  std::vector<uint8_t> raw_data;

private:
  static bool parse_hex_string(
    const std::string & input, std::vector<uint8_t> & output,
    std::string & error)
  {
    std::string compact;
    compact.reserve(input.size());

    for (size_t i = 0; i < input.size(); ++i) {
      const char c = input[i];
      const unsigned char uc = static_cast<unsigned char>(c);

      if (c == '0' && (i + 1) < input.size() && (input[i + 1] == 'x' || input[i + 1] == 'X')) {
        ++i;
        continue;
      }

      if (std::isxdigit(uc)) {
        compact.push_back(static_cast<char>(std::tolower(uc)));
      } else if (std::isspace(uc) || c == ',' || c == ':' || c == '_') {
        continue;
      } else {
        std::stringstream err;
        err << "unexpected character '" << c << "'";
        error = err.str();
        return false;
      }
    }

    if (compact.size() % 2 != 0) {
      error = "hex payload must contain an even number of hex digits";
      return false;
    }

    output.clear();
    output.reserve(compact.size() / 2);

    for (size_t i = 0; i < compact.size(); i += 2) {
      const std::string byte_str = compact.substr(i, 2);
      const unsigned long v = std::stoul(byte_str, nullptr, 16);
      output.push_back(static_cast<uint8_t>(v));
    }

    return true;
  }

  size_t type2bytes(std::string type)
  {
    if (type == "int8" || type == "uint8") {
      return 1;
    } else if (type == "int16" || type == "uint16") {
      return 2;
    } else if (type == "int32" || type == "uint32" || type == "float" || type == "real32") {
      return 4;
    } else if (type == "int64" || type == "uint64" || type == "double" || type == "real64") {
      return 8;
    }
    return 0;
  }
};

}  // namespace ethercat_interface
#endif  // ETHERCAT_INTERFACE__EC_SDO_MANAGER_HPP_
