meta:
  id: ifx_metrics_bin
  encoding: UTF-8
  endian: le
doc: |
  Ifx Metrics binary protocol provides efficient way to emit metrics
  data to Geneva Metrics. This particular version of protocol is best suited
  for transfer over networked transport (UDP, TCP, UNIX domain sockets) and/or
  file storage, as it is completely self-contained. "Body" of this protocol
  closely matches "UserData" portion of IfxMetrics-over-ETW messages
  (typically emitted by IfxMetrics.dll or compatible apps/libraries on Windows).
seq:
  - id: event_id
    type: u2
    doc: Type of message, affects format of the body.
  - id: len_body
    type: u2
    doc: Size of body in bytes.
  - id: body
    size: len_body
    type: userdata(event_id)
    doc: Body of IfxMetrics binary protocol message.
types:
  userdata:
    doc: |
      This type represents "UserData" or "body" portion of Ifx Metrics message.
    params:
      - id: event_id
        type: u2
        doc: Type of message, affects format of the body.
    seq:
      - id: num_dimensions
        type: u2
        doc: Number of dimensions specified in this event.
      - id: padding
        size: 2
      - id: value_section
        type:
          switch-on: event_type
          cases:
            'metric_event_type::old_ifx': single_uint64_value
            'metric_event_type::uint64_metric': single_uint64_value
            'metric_event_type::double_scaled_to_long_metric': single_double_value
            'metric_event_type::externally_aggregated_ulong_metric': ext_aggregated_uint64_value
            'metric_event_type::externally_aggregated_double_metric': ext_aggregated_double_value
            'metric_event_type::double_metric': single_double_value
            'metric_event_type::externally_aggregated_ulong_distribution_metric': ext_aggregated_uint64_value
            'metric_event_type::externally_aggregated_double_distribution_metric': ext_aggregated_double_value
            'metric_event_type::externally_aggregated_double_scaled_to_long_distribution_metric': ext_aggregated_double_value
        doc: Value section of the body, stores fixed numeric metric value(s), as per event type.
      - id: metric_account
        type: len_string
        doc: Geneva Metrics account name to be used for this metric.
      - id: metric_namespace
        type: len_string
        doc: Geneva Metrics namespace name to be used for this metric.
      - id: metric_name
        type: len_string
        doc: Geneva Metrics metric name to be used.
      - id: dimensions_names
        type: len_string
        repeat: expr
        repeat-expr: num_dimensions
        doc: |
          Dimension names strings ("key" parts of key-value pairs). Must be sorted,
          unless MetricsExtenion's option `enableDimensionSortingOnIngestion` is
          enabled.
      - id: dimensions_values
        type: len_string
        repeat: expr
        repeat-expr: num_dimensions
        doc: Dimension values strings ("value" parts of key-value pairs).
      - id: ap_container
        type: len_string
        if: not _io.eof
        doc: |
          AutoPilot container string, required for correct AP PKI certificate loading
          in AutoPilot containers environment.
      - id: histogram
        type: histogram
        if: >
          (event_type == metric_event_type::externally_aggregated_ulong_distribution_metric or
          event_type == metric_event_type::externally_aggregated_double_distribution_metric or
          event_type == metric_event_type::externally_aggregated_double_scaled_to_long_distribution_metric)
          and not _io.eof
    instances:
      event_type:
        value: event_id
        enum: metric_event_type
  histogram:
    -orig-id: 'IfxProcessor::ReadDistribution'
    seq:
      - id: version
        type: u1
      - id: type
        type: u1
        enum: distribution_type
      - id: body
        type:
          switch-on: type
          cases:
            'distribution_type::ifx_bucketed': histogram_uint16_bucketed
            'distribution_type::mon_agent_bucketed': histogram_uint16_bucketed
            'distribution_type::value_count_pairs': histogram_value_count_pairs
  histogram_uint16_bucketed:
    doc: |
      Payload of a histogram with linear distribution of buckets. Such histogram
      is defined by the parameters specified in `min`, `bucket_size` and
      `bucket_count`. It is modelled as a series of buckets. First (index 0) and
      last (indexed `bucket_count - 1`) buckets are special and are supposed to
      catch all "underflow" and "overflow" values. Buckets with indexes 1 up to
      `bucket_count - 2` are regular buckets of size `bucket_size`.
    -orig-id: 'IfxProcessor::ReadUint16BucketedDistribution'
    seq:
      - id: min
        type: u8
      - id: bucket_size
        type: u4
      - id: bucket_count
        type: u4
      - id: distribution_size
        type: u2
      - id: columns
        type: pair_uint16
        repeat: expr
        repeat-expr: distribution_size
  histogram_value_count_pairs:
    -orig-id: 'IfxProcessor::ReadValueCountPairsDistribution'
    seq:
      - id: distribution_size
        type: u2
      - id: columns
        type: pair_value_count
        repeat: expr
        repeat-expr: distribution_size
  pair_uint16:
    doc: |
      Bucket #index, claiming to hold exactly `count` hits. See notes in
      `histogram_uint16_bucketed` for interpreting index.
    seq:
      - id: index
        type: u2
      - id: count
        type: u2
  pair_value_count:
    doc: |
      Bucket with an explicitly-defined value coordinate `value`, claiming to
      hold `count` hits. Normally used to represent non-linear (e.g. exponential)
      histograms payloads.
    seq:
      - id: value
        type: u8
      - id: count
        type: u4
  single_uint64_value:
    -orig-id: MDM_EVENT_PAYLOAD
    seq:
      - id: padding
        size: 4
      - id: timestamp
        type: u8
        doc: Timestamp in Windows FILETIME format, i.e. number of 100 ns ticks passed since 1601-01-01 00:00:00 UTC.
      - id: value
        type: u8
        doc: Metric value as 64-bit unsigned integer.
  single_double_value:
    -orig-id: MDM_EVENT_PAYLOAD
    seq:
      - id: padding
        size: 4
      - id: timestamp
        type: u8
        doc: Timestamp in Windows FILETIME format, i.e. number of 100 ns ticks passed since 1601-01-01 00:00:00 UTC.
      - id: value
        type: f8
        doc: Metric value as double.
  ext_aggregated_uint64_value:
    -orig-id: MDM_EXTERNAL_AGG_EVENT_PAYLOAD
    seq:
      - id: count
        type: u4
        doc: Count of events aggregated in this event.
      - id: timestamp
        type: u8
        doc: Timestamp in Windows FILETIME format, i.e. number of 100 ns ticks passed since 1601-01-01 00:00:00 UTC.
      - id: sum
        type: u8
        doc: Sum of all metric values aggregated in this event.
      - id: min
        type: u8
        doc: Minimum of all metric values aggregated in this event.
      - id: max
        type: u8
        doc: Maximum of all metric values aggregated in this event.
  ext_aggregated_double_value:
    -orig-id: MDM_EXTERNAL_AGG_EVENT_PAYLOAD
    seq:
      - id: count
        type: u4
        doc: Count of events aggregated in this event.
      - id: timestamp
        type: u8
        doc: Timestamp in Windows FILETIME format, i.e. number of 100 ns ticks passed since 1601-01-01 00:00:00 UTC.
      - id: sum
        type: f8
        doc: Sum of all metric values aggregated in this event.
      - id: min
        type: f8
        doc: Minimum of all metric values aggregated in this event.
      - id: max
        type: f8
        doc: Maximum of all metric values aggregated in this event.
  len_string:
    doc: A simple string, length-prefixed with a 2-byte integer.
    seq:
      - id: len_value
        type: u2
      - id: value
        size: len_value
        type: str
enums:
  metric_event_type:
    0: old_ifx
    50: uint64_metric
    51: double_scaled_to_long_metric
    52: batch_metric
    53: externally_aggregated_ulong_metric
    54: externally_aggregated_double_metric
    55: double_metric
    56: externally_aggregated_ulong_distribution_metric
    57: externally_aggregated_double_distribution_metric
    58: externally_aggregated_double_scaled_to_long_distribution_metric
  distribution_type:
    0: ifx_bucketed
    1: mon_agent_bucketed
    2: value_count_pairs
