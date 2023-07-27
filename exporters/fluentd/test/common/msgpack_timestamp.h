// Copyright The OpenTelemetry Authors
// SPDX-License-Identifier: Apache-2.0

// Custom implementation of serializer that supports Timestamp Ext type:
// https://github.com/msgpack/msgpack/blob/master/spec.md#timestamp-extension-type
#include "nlohmann/json.hpp"

using namespace nlohmann;

namespace nlohmann {
namespace detail {

template <typename X> static inline char to_char_type(X x) {
  return static_cast<char>(x);
}

///////////////////
// binary writer //
///////////////////
template <typename BasicJsonType, typename CharType>
class binary_writer2 : public binary_writer<BasicJsonType, CharType> {
  output_adapter_t<CharType> oa = nullptr;

  /// whether we can assume little endianess
  const bool is_little_endian = this->little_endianess();

public:
  /*
    @brief write a number to output input
    @param[in] n number of type @a NumberType
    @tparam NumberType the type of the number
    @tparam OutputIsLittleEndian Set to true if output data is
                                 required to be little endian

    @note This function needs to respect the system's endianess, because bytes
          in CBOR, MessagePack, and UBJSON are stored in network order (big
          endian) and therefore need reordering on little endian systems.
    */
  template <typename NumberType, bool OutputIsLittleEndian = false>
  void write_number(const NumberType n) {
    // step 1: write number to array of length NumberType
    std::array<CharType, sizeof(NumberType)> vec;
    std::memcpy(vec.data(), &n, sizeof(NumberType));

    // step 2: write array to output (with possible reordering)
    if (is_little_endian != OutputIsLittleEndian) {
      // reverse byte order prior to conversion if necessary
      std::reverse(vec.begin(), vec.end());
    }

    oa->write_characters(vec.data(), sizeof(NumberType));
  };

  binary_writer2(output_adapter_t<CharType> adapter)
      : binary_writer<BasicJsonType, CharType>(adapter), oa(adapter){};

  /**
   * @brief Add support for Timestamp extension type:
   * https://github.com/msgpack/msgpack/blob/master/spec.md#timestamp-extension-type
   * Timestamp must be populated via byte_container_with_subtype<...> template.
   *
   * @param j
   */
  void write_msgpack(const BasicJsonType &j) {
    switch (j.type()) {
    case value_t::null:
      // nobrk
    case value_t::boolean:
      // nobrk
    case value_t::number_integer:
      // nobrk
    case value_t::number_unsigned:
      // nobrk
    case value_t::number_float:
      // nobrk
    case value_t::string:
      // nobrk
      binary_writer<BasicJsonType, CharType>::write_msgpack(j);
      break;

    case value_t::array: {
      // step 1: write control byte and the array size
      const auto N = j.size();
      if (N <= 15) {
        // fixarray
        write_number(static_cast<std::uint8_t>(0x90 | N));
      } else if (N <= (std::numeric_limits<std::uint16_t>::max)()) {
        // array 16
        oa->write_character(to_char_type(0xDC));
        write_number(static_cast<std::uint16_t>(N));
      } else if (N <= (std::numeric_limits<std::uint32_t>::max)()) {
        // array 32
        oa->write_character(to_char_type(0xDD));
        write_number(static_cast<std::uint32_t>(N));
      }
      // step 2: write each element
      for (auto &v : j) {
        write_msgpack(v);
      }
      break;
    }

#if 0
      case value_t::binary: {
        auto& bytes = j.get_binary();
        const auto N = bytes.size();
        if (bytes.has_subtype()) {
          if (bytes.subtype()==0xff) {
            switch (N) {
              case 4:
                oa->write_character(0xd6);
                oa->write_character(0xff);
                break;
              case 8:
                oa->write_character(0xd7);
                oa->write_character(0xff);
              case 12:
                oa->write_character(0xc7);
                oa->write_character(0xff);
                break;
              default:
                // Ignore invalid format
            }
            return;
          }
          if (bytes.subtype()==0x00) {
            switch (N) {
              case 4:
                oa->write_character(0xd6);
                oa->write_character(0xff);
                break;
              case 8:
                oa->write_character(0xd7);
                oa->write_character(0xff);
              case 12:
                oa->write_character(0xc7);
                oa->write_character(0xff);
                break;
              default:
                // Ignore invalid format
            }
            return;
          }
          oa->write_characters(reinterpret_cast<const CharType*>(bytes.data()), N);
          return;
        }
        binary_writer<BasicJsonType, CharType>::write_msgpack(j);
      }
#endif

    case value_t::object: {
      // step 1: write control byte and the object size
      const auto N = j.size();
      if (N <= 15) {
        // fixmap
        write_number(static_cast<std::uint8_t>(0x80 | (N & 0xF)));
      } else if (N <= (std::numeric_limits<std::uint16_t>::max)()) {
        // map 16
        oa->write_character(to_char_type(0xDE));
        write_number(static_cast<std::uint16_t>(N));
      } else if (N <= (std::numeric_limits<std::uint32_t>::max)()) {
        // map 32
        oa->write_character(to_char_type(0xDF));
        write_number(static_cast<std::uint32_t>(N));
      }
      // step 2: write each element
      for (auto &el : j.items()) {
        write_msgpack(el.key());
        write_msgpack(el.value());
      }
      break;
    }
    }
  }
};

template <typename basic_json>
static void to_msgpack2(const basic_json &j, detail::output_adapter<char> o) {
  binary_writer2<basic_json, char>(o).write_msgpack(j);
}

template <typename basic_json>
static void to_msgpack2(const basic_json &j,
                        detail::output_adapter<uint8_t> o) {
  binary_writer2<basic_json, uint8_t>(o).write_msgpack(j);
}

template <typename basic_json>
static std::vector<uint8_t> to_msgpack2(const basic_json &j) {
  std::vector<uint8_t> result;
  to_msgpack2(j, result);
  return result;
}

} // namespace detail
} // namespace nlohmann