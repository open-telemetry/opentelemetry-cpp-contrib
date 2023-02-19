#ifndef IFX_METRICS_BIN_H_
#define IFX_METRICS_BIN_H_

// This is a generated file! Please edit source .ksy file and use
// kaitai-struct-compiler to rebuild

#include "kaitai/kaitaistruct.h"
#include <stdint.h>
#include <vector>

#if KAITAI_STRUCT_VERSION < 9000L
#error                                                                         \
    "Incompatible Kaitai Struct C++/STL API: version 0.9 or later is required"
#endif

/**
 * Ifx Metrics binary protocol provides efficient way to emit metrics
 * data to Geneva Metrics. This particular version of protocol is best suited
 * for transfer over networked transport (UDP, TCP, UNIX domain sockets) and/or
 * file storage, as it is completely self-contained. "Body" of this protocol
 * closely matches "UserData" portion of IfxMetrics-over-ETW messages
 * (typically emitted by IfxMetrics.dll or compatible apps/libraries on
 * Windows).
 */

class ifx_metrics_bin_t : public kaitai::kstruct {

public:
  class userdata_t;
  class pair_value_count_t;
  class histogram_uint16_bucketed_t;
  class histogram_value_count_pairs_t;
  class histogram_t;
  class pair_uint16_t;
  class single_uint64_value_t;
  class ext_aggregated_double_value_t;
  class ext_aggregated_uint64_value_t;
  class single_double_value_t;
  class len_string_t;

  enum metric_event_type_t {
    METRIC_EVENT_TYPE_OLD_IFX = 0,
    METRIC_EVENT_TYPE_UINT64_METRIC = 50,
    METRIC_EVENT_TYPE_DOUBLE_SCALED_TO_LONG_METRIC = 51,
    METRIC_EVENT_TYPE_BATCH_METRIC = 52,
    METRIC_EVENT_TYPE_EXTERNALLY_AGGREGATED_ULONG_METRIC = 53,
    METRIC_EVENT_TYPE_EXTERNALLY_AGGREGATED_DOUBLE_METRIC = 54,
    METRIC_EVENT_TYPE_DOUBLE_METRIC = 55,
    METRIC_EVENT_TYPE_EXTERNALLY_AGGREGATED_ULONG_DISTRIBUTION_METRIC = 56,
    METRIC_EVENT_TYPE_EXTERNALLY_AGGREGATED_DOUBLE_DISTRIBUTION_METRIC = 57,
    METRIC_EVENT_TYPE_EXTERNALLY_AGGREGATED_DOUBLE_SCALED_TO_LONG_DISTRIBUTION_METRIC =
        58
  };

  enum distribution_type_t {
    DISTRIBUTION_TYPE_IFX_BUCKETED = 0,
    DISTRIBUTION_TYPE_MON_AGENT_BUCKETED = 1,
    DISTRIBUTION_TYPE_VALUE_COUNT_PAIRS = 2
  };

  ifx_metrics_bin_t(kaitai::kstream *p__io, kaitai::kstruct *p__parent = 0,
                    ifx_metrics_bin_t *p__root = 0);

private:
  void _read();
  void _clean_up();

public:
  ~ifx_metrics_bin_t();

  /**
   * This type represents "UserData" or "body" portion of Ifx Metrics message.
   */

  class userdata_t : public kaitai::kstruct {

  public:
    userdata_t(uint16_t p_event_id, kaitai::kstream *p__io,
               ifx_metrics_bin_t *p__parent = 0,
               ifx_metrics_bin_t *p__root = 0);

  private:
    void _read();
    void _clean_up();

  public:
    ~userdata_t();

  private:
    bool f_event_type;
    metric_event_type_t m_event_type;

  public:
    metric_event_type_t event_type();

  private:
    uint16_t m_num_dimensions;
    std::string m_padding;
    kaitai::kstruct *m_value_section;
    bool n_value_section;

  public:
    bool _is_null_value_section() {
      value_section();
      return n_value_section;
    };

  private:
    len_string_t *m_metric_account;
    len_string_t *m_metric_namespace;
    len_string_t *m_metric_name;
    std::vector<len_string_t *> *m_dimensions_names;
    std::vector<len_string_t *> *m_dimensions_values;
    len_string_t *m_ap_container;
    bool n_ap_container;

  public:
    bool _is_null_ap_container() {
      ap_container();
      return n_ap_container;
    };

  private:
    histogram_t *m_histogram;
    bool n_histogram;

  public:
    bool _is_null_histogram() {
      histogram();
      return n_histogram;
    };

  private:
    uint16_t m_event_id;
    ifx_metrics_bin_t *m__root;
    ifx_metrics_bin_t *m__parent;

  public:
    /**
     * Number of dimensions specified in this event.
     */
    uint16_t num_dimensions() const { return m_num_dimensions; }
    std::string padding() const { return m_padding; }

    /**
     * Value section of the body, stores fixed numeric metric value(s), as per
     * event type.
     */
    kaitai::kstruct *value_section() const { return m_value_section; }

    /**
     * Geneva Metrics account name to be used for this metric.
     */
    len_string_t *metric_account() const { return m_metric_account; }

    /**
     * Geneva Metrics namespace name to be used for this metric.
     */
    len_string_t *metric_namespace() const { return m_metric_namespace; }

    /**
     * Geneva Metrics metric name to be used.
     */
    len_string_t *metric_name() const { return m_metric_name; }

    /**
     * Dimension names strings ("key" parts of key-value pairs). Must be sorted,
     * unless MetricsExtenion's option `enableDimensionSortingOnIngestion` is
     * enabled.
     */
    std::vector<len_string_t *> *dimensions_names() const {
      return m_dimensions_names;
    }

    /**
     * Dimension values strings ("value" parts of key-value pairs).
     */
    std::vector<len_string_t *> *dimensions_values() const {
      return m_dimensions_values;
    }

    /**
     * AutoPilot container string, required for correct AP PKI certificate
     * loading in AutoPilot containers environment.
     */
    len_string_t *ap_container() const { return m_ap_container; }
    histogram_t *histogram() const { return m_histogram; }

    /**
     * Type of message, affects format of the body.
     */
    uint16_t event_id() const { return m_event_id; }
    ifx_metrics_bin_t *_root() const { return m__root; }
    ifx_metrics_bin_t *_parent() const { return m__parent; }
  };

  /**
   * Bucket with an explicitly-defined value coordinate `value`, claiming to
   * hold `count` hits. Normally used to represent non-linear (e.g. exponential)
   * histograms payloads.
   */

  class pair_value_count_t : public kaitai::kstruct {

  public:
    pair_value_count_t(
        kaitai::kstream *p__io,
        ifx_metrics_bin_t::histogram_value_count_pairs_t *p__parent = 0,
        ifx_metrics_bin_t *p__root = 0);

  private:
    void _read();
    void _clean_up();

  public:
    ~pair_value_count_t();

  private:
    uint64_t m_value;
    uint32_t m_count;
    ifx_metrics_bin_t *m__root;
    ifx_metrics_bin_t::histogram_value_count_pairs_t *m__parent;

  public:
    uint64_t value() const { return m_value; }
    uint32_t count() const { return m_count; }
    ifx_metrics_bin_t *_root() const { return m__root; }
    ifx_metrics_bin_t::histogram_value_count_pairs_t *_parent() const {
      return m__parent;
    }
  };

  /**
   * Payload of a histogram with linear distribution of buckets. Such histogram
   * is defined by the parameters specified in `min`, `bucket_size` and
   * `bucket_count`. It is modelled as a series of buckets. First (index 0) and
   * last (indexed `bucket_count - 1`) buckets are special and are supposed to
   * catch all "underflow" and "overflow" values. Buckets with indexes 1 up to
   * `bucket_count - 2` are regular buckets of size `bucket_size`.
   */

  class histogram_uint16_bucketed_t : public kaitai::kstruct {

  public:
    histogram_uint16_bucketed_t(kaitai::kstream *p__io,
                                ifx_metrics_bin_t::histogram_t *p__parent = 0,
                                ifx_metrics_bin_t *p__root = 0);

  private:
    void _read();
    void _clean_up();

  public:
    ~histogram_uint16_bucketed_t();

  private:
    uint64_t m_min;
    uint32_t m_bucket_size;
    uint32_t m_bucket_count;
    uint16_t m_distribution_size;
    std::vector<pair_uint16_t *> *m_columns;
    ifx_metrics_bin_t *m__root;
    ifx_metrics_bin_t::histogram_t *m__parent;

  public:
    uint64_t min() const { return m_min; }
    uint32_t bucket_size() const { return m_bucket_size; }
    uint32_t bucket_count() const { return m_bucket_count; }
    uint16_t distribution_size() const { return m_distribution_size; }
    std::vector<pair_uint16_t *> *columns() const { return m_columns; }
    ifx_metrics_bin_t *_root() const { return m__root; }
    ifx_metrics_bin_t::histogram_t *_parent() const { return m__parent; }
  };

  class histogram_value_count_pairs_t : public kaitai::kstruct {

  public:
    histogram_value_count_pairs_t(kaitai::kstream *p__io,
                                  ifx_metrics_bin_t::histogram_t *p__parent = 0,
                                  ifx_metrics_bin_t *p__root = 0);

  private:
    void _read();
    void _clean_up();

  public:
    ~histogram_value_count_pairs_t();

  private:
    uint16_t m_distribution_size;
    std::vector<pair_value_count_t *> *m_columns;
    ifx_metrics_bin_t *m__root;
    ifx_metrics_bin_t::histogram_t *m__parent;

  public:
    uint16_t distribution_size() const { return m_distribution_size; }
    std::vector<pair_value_count_t *> *columns() const { return m_columns; }
    ifx_metrics_bin_t *_root() const { return m__root; }
    ifx_metrics_bin_t::histogram_t *_parent() const { return m__parent; }
  };

  class histogram_t : public kaitai::kstruct {

  public:
    histogram_t(kaitai::kstream *p__io,
                ifx_metrics_bin_t::userdata_t *p__parent = 0,
                ifx_metrics_bin_t *p__root = 0);

  private:
    void _read();
    void _clean_up();

  public:
    ~histogram_t();

  private:
    uint8_t m_version;
    distribution_type_t m_type;
    kaitai::kstruct *m_body;
    bool n_body;

  public:
    bool _is_null_body() {
      body();
      return n_body;
    };

  private:
    ifx_metrics_bin_t *m__root;
    ifx_metrics_bin_t::userdata_t *m__parent;

  public:
    uint8_t version() const { return m_version; }
    distribution_type_t type() const { return m_type; }
    kaitai::kstruct *body() const { return m_body; }
    ifx_metrics_bin_t *_root() const { return m__root; }
    ifx_metrics_bin_t::userdata_t *_parent() const { return m__parent; }
  };

  /**
   * Bucket #index, claiming to hold exactly `count` hits. See notes in
   * `histogram_uint16_bucketed` for interpreting index.
   */

  class pair_uint16_t : public kaitai::kstruct {

  public:
    pair_uint16_t(kaitai::kstream *p__io,
                  ifx_metrics_bin_t::histogram_uint16_bucketed_t *p__parent = 0,
                  ifx_metrics_bin_t *p__root = 0);

  private:
    void _read();
    void _clean_up();

  public:
    ~pair_uint16_t();

  private:
    uint16_t m_index;
    uint16_t m_count;
    ifx_metrics_bin_t *m__root;
    ifx_metrics_bin_t::histogram_uint16_bucketed_t *m__parent;

  public:
    uint16_t index() const { return m_index; }
    uint16_t count() const { return m_count; }
    ifx_metrics_bin_t *_root() const { return m__root; }
    ifx_metrics_bin_t::histogram_uint16_bucketed_t *_parent() const {
      return m__parent;
    }
  };

  class single_uint64_value_t : public kaitai::kstruct {

  public:
    single_uint64_value_t(kaitai::kstream *p__io,
                          ifx_metrics_bin_t::userdata_t *p__parent = 0,
                          ifx_metrics_bin_t *p__root = 0);

  private:
    void _read();
    void _clean_up();

  public:
    ~single_uint64_value_t();

  private:
    std::string m_padding;
    uint64_t m_timestamp;
    uint64_t m_value;
    ifx_metrics_bin_t *m__root;
    ifx_metrics_bin_t::userdata_t *m__parent;

  public:
    std::string padding() const { return m_padding; }

    /**
     * Timestamp in Windows FILETIME format, i.e. number of 100 ns ticks passed
     * since 1601-01-01 00:00:00 UTC.
     */
    uint64_t timestamp() const { return m_timestamp; }

    /**
     * Metric value as 64-bit unsigned integer.
     */
    uint64_t value() const { return m_value; }
    ifx_metrics_bin_t *_root() const { return m__root; }
    ifx_metrics_bin_t::userdata_t *_parent() const { return m__parent; }
  };

  class ext_aggregated_double_value_t : public kaitai::kstruct {

  public:
    ext_aggregated_double_value_t(kaitai::kstream *p__io,
                                  ifx_metrics_bin_t::userdata_t *p__parent = 0,
                                  ifx_metrics_bin_t *p__root = 0);

  private:
    void _read();
    void _clean_up();

  public:
    ~ext_aggregated_double_value_t();

  private:
    uint32_t m_count;
    uint64_t m_timestamp;
    double m_sum;
    double m_min;
    double m_max;
    ifx_metrics_bin_t *m__root;
    ifx_metrics_bin_t::userdata_t *m__parent;

  public:
    /**
     * Count of events aggregated in this event.
     */
    uint32_t count() const { return m_count; }

    /**
     * Timestamp in Windows FILETIME format, i.e. number of 100 ns ticks passed
     * since 1601-01-01 00:00:00 UTC.
     */
    uint64_t timestamp() const { return m_timestamp; }

    /**
     * Sum of all metric values aggregated in this event.
     */
    double sum() const { return m_sum; }

    /**
     * Minimum of all metric values aggregated in this event.
     */
    double min() const { return m_min; }

    /**
     * Maximum of all metric values aggregated in this event.
     */
    double max() const { return m_max; }
    ifx_metrics_bin_t *_root() const { return m__root; }
    ifx_metrics_bin_t::userdata_t *_parent() const { return m__parent; }
  };

  class ext_aggregated_uint64_value_t : public kaitai::kstruct {

  public:
    ext_aggregated_uint64_value_t(kaitai::kstream *p__io,
                                  ifx_metrics_bin_t::userdata_t *p__parent = 0,
                                  ifx_metrics_bin_t *p__root = 0);

  private:
    void _read();
    void _clean_up();

  public:
    ~ext_aggregated_uint64_value_t();

  private:
    uint32_t m_count;
    uint64_t m_timestamp;
    uint64_t m_sum;
    uint64_t m_min;
    uint64_t m_max;
    ifx_metrics_bin_t *m__root;
    ifx_metrics_bin_t::userdata_t *m__parent;

  public:
    /**
     * Count of events aggregated in this event.
     */
    uint32_t count() const { return m_count; }

    /**
     * Timestamp in Windows FILETIME format, i.e. number of 100 ns ticks passed
     * since 1601-01-01 00:00:00 UTC.
     */
    uint64_t timestamp() const { return m_timestamp; }

    /**
     * Sum of all metric values aggregated in this event.
     */
    uint64_t sum() const { return m_sum; }

    /**
     * Minimum of all metric values aggregated in this event.
     */
    uint64_t min() const { return m_min; }

    /**
     * Maximum of all metric values aggregated in this event.
     */
    uint64_t max() const { return m_max; }
    ifx_metrics_bin_t *_root() const { return m__root; }
    ifx_metrics_bin_t::userdata_t *_parent() const { return m__parent; }
  };

  class single_double_value_t : public kaitai::kstruct {

  public:
    single_double_value_t(kaitai::kstream *p__io,
                          ifx_metrics_bin_t::userdata_t *p__parent = 0,
                          ifx_metrics_bin_t *p__root = 0);

  private:
    void _read();
    void _clean_up();

  public:
    ~single_double_value_t();

  private:
    std::string m_padding;
    uint64_t m_timestamp;
    double m_value;
    ifx_metrics_bin_t *m__root;
    ifx_metrics_bin_t::userdata_t *m__parent;

  public:
    std::string padding() const { return m_padding; }

    /**
     * Timestamp in Windows FILETIME format, i.e. number of 100 ns ticks passed
     * since 1601-01-01 00:00:00 UTC.
     */
    uint64_t timestamp() const { return m_timestamp; }

    /**
     * Metric value as double.
     */
    double value() const { return m_value; }
    ifx_metrics_bin_t *_root() const { return m__root; }
    ifx_metrics_bin_t::userdata_t *_parent() const { return m__parent; }
  };

  /**
   * A simple string, length-prefixed with a 2-byte integer.
   */

  class len_string_t : public kaitai::kstruct {

  public:
    len_string_t(kaitai::kstream *p__io,
                 ifx_metrics_bin_t::userdata_t *p__parent = 0,
                 ifx_metrics_bin_t *p__root = 0);

  private:
    void _read();
    void _clean_up();

  public:
    ~len_string_t();

  private:
    uint16_t m_len_value;
    std::string m_value;
    ifx_metrics_bin_t *m__root;
    ifx_metrics_bin_t::userdata_t *m__parent;

  public:
    uint16_t len_value() const { return m_len_value; }
    std::string value() const { return m_value; }
    ifx_metrics_bin_t *_root() const { return m__root; }
    ifx_metrics_bin_t::userdata_t *_parent() const { return m__parent; }
  };

private:
  uint16_t m_event_id;
  uint16_t m_len_body;
  userdata_t *m_body;
  ifx_metrics_bin_t *m__root;
  kaitai::kstruct *m__parent;
  std::string m__raw_body;
  kaitai::kstream *m__io__raw_body;

public:
  /**
   * Type of message, affects format of the body.
   */
  uint16_t event_id() const { return m_event_id; }

  /**
   * Size of body in bytes.
   */
  uint16_t len_body() const { return m_len_body; }

  /**
   * Body of IfxMetrics binary protocol message.
   */
  userdata_t *body() const { return m_body; }
  ifx_metrics_bin_t *_root() const { return m__root; }
  kaitai::kstruct *_parent() const { return m__parent; }
  std::string _raw_body() const { return m__raw_body; }
  kaitai::kstream *_io__raw_body() const { return m__io__raw_body; }
};

#endif // IFX_METRICS_BIN_H_
