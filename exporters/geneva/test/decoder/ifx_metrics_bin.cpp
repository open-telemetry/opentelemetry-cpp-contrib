// This is a generated file! Please edit source .ksy file and use kaitai-struct-compiler to rebuild

#include "ifx_metrics_bin.h"

#include <iostream>
ifx_metrics_bin_t::ifx_metrics_bin_t(kaitai::kstream* p__io, kaitai::kstruct* p__parent, ifx_metrics_bin_t* p__root) : kaitai::kstruct(p__io) {
    m__parent = p__parent;
    m__root = this;
    m_body = 0;
    m__io__raw_body = 0;

    try {
        _read();
    } catch(...) {
        _clean_up();
        throw;
    }
}

void ifx_metrics_bin_t::_read() {
    m_event_id = m__io->read_u2le();
    m_len_body = m__io->read_u2le();
    m__raw_body = m__io->read_bytes(len_body());
    m__io__raw_body = new kaitai::kstream(m__raw_body);
    m_body = new userdata_t(event_id(), m__io__raw_body, this, m__root);
}

ifx_metrics_bin_t::~ifx_metrics_bin_t() {
    _clean_up();
}

void ifx_metrics_bin_t::_clean_up() {
    if (m__io__raw_body) {
        delete m__io__raw_body; m__io__raw_body = 0;
    }
    if (m_body) {
        delete m_body; m_body = 0;
    }
}

ifx_metrics_bin_t::userdata_t::userdata_t(uint16_t p_event_id, kaitai::kstream* p__io, ifx_metrics_bin_t* p__parent, ifx_metrics_bin_t* p__root) : kaitai::kstruct(p__io) {
    m__parent = p__parent;
    m__root = p__root;
    m_event_id = p_event_id;
    m_metric_account = 0;
    m_metric_namespace = 0;
    m_metric_name = 0;
    m_dimensions_names = 0;
    m_dimensions_values = 0;
    m_ap_container = 0;
    m_histogram = 0;
    f_event_type = false;

    try {
        _read();
    } catch(...) {
        _clean_up();
        throw;
    }
}

void ifx_metrics_bin_t::userdata_t::_read() {
    m_num_dimensions = m__io->read_u2le();
    m_padding = m__io->read_bytes(2);
    n_value_section = true;
    switch (event_type()) {
    case ifx_metrics_bin_t::METRIC_EVENT_TYPE_EXTERNALLY_AGGREGATED_DOUBLE_SCALED_TO_LONG_DISTRIBUTION_METRIC: {
        n_value_section = false;
        m_value_section = new ext_aggregated_double_value_t(m__io, this, m__root);
        break;
    }
    case ifx_metrics_bin_t::METRIC_EVENT_TYPE_DOUBLE_METRIC: {
        n_value_section = false;
        m_value_section = new single_double_value_t(m__io, this, m__root);
        break;
    }
    case ifx_metrics_bin_t::METRIC_EVENT_TYPE_EXTERNALLY_AGGREGATED_ULONG_METRIC: {
        n_value_section = false;
        m_value_section = new ext_aggregated_uint64_value_t(m__io, this, m__root);
        break;
    }
    case ifx_metrics_bin_t::METRIC_EVENT_TYPE_EXTERNALLY_AGGREGATED_ULONG_DISTRIBUTION_METRIC: {
        n_value_section = false;
        m_value_section = new ext_aggregated_uint64_value_t(m__io, this, m__root);
        break;
    }
    case ifx_metrics_bin_t::METRIC_EVENT_TYPE_EXTERNALLY_AGGREGATED_DOUBLE_DISTRIBUTION_METRIC: {
        n_value_section = false;
        m_value_section = new ext_aggregated_double_value_t(m__io, this, m__root);
        break;
    }
    case ifx_metrics_bin_t::METRIC_EVENT_TYPE_DOUBLE_SCALED_TO_LONG_METRIC: {
        n_value_section = false;
        m_value_section = new single_double_value_t(m__io, this, m__root);
        break;
    }
    case ifx_metrics_bin_t::METRIC_EVENT_TYPE_UINT64_METRIC: {
        n_value_section = false;
        m_value_section = new single_uint64_value_t(m__io, this, m__root);
        break;
    }
    case ifx_metrics_bin_t::METRIC_EVENT_TYPE_EXTERNALLY_AGGREGATED_DOUBLE_METRIC: {
        n_value_section = false;
        m_value_section = new ext_aggregated_double_value_t(m__io, this, m__root);
        break;
    }
    case ifx_metrics_bin_t::METRIC_EVENT_TYPE_OLD_IFX: {
        n_value_section = false;
        m_value_section = new single_uint64_value_t(m__io, this, m__root);
        break;
    }
    }
    m_metric_account = new len_string_t(m__io, this, m__root);
    m_metric_namespace = new len_string_t(m__io, this, m__root);
    m_metric_name = new len_string_t(m__io, this, m__root);
    int l_dimensions_names = num_dimensions();
    m_dimensions_names = new std::vector<len_string_t*>();
    m_dimensions_names->reserve(l_dimensions_names);
    for (int i = 0; i < l_dimensions_names; i++) {
        m_dimensions_names->push_back(new len_string_t(m__io, this, m__root));
    }
    int l_dimensions_values = num_dimensions();
    m_dimensions_values = new std::vector<len_string_t*>();
    m_dimensions_values->reserve(l_dimensions_values);
    for (int i = 0; i < l_dimensions_values; i++) {
        m_dimensions_values->push_back(new len_string_t(m__io, this, m__root));
    }
    n_ap_container = true;
    if (!(_io()->is_eof())) {
        n_ap_container = false;
        m_ap_container = new len_string_t(m__io, this, m__root);
    }
    n_histogram = true;
    if ( (( ((event_type() == ifx_metrics_bin_t::METRIC_EVENT_TYPE_EXTERNALLY_AGGREGATED_ULONG_DISTRIBUTION_METRIC) || (event_type() == ifx_metrics_bin_t::METRIC_EVENT_TYPE_EXTERNALLY_AGGREGATED_DOUBLE_DISTRIBUTION_METRIC) || (event_type() == ifx_metrics_bin_t::METRIC_EVENT_TYPE_EXTERNALLY_AGGREGATED_DOUBLE_SCALED_TO_LONG_DISTRIBUTION_METRIC)) ) && (!(_io()->is_eof()))) ) {
        n_histogram = false;
        m_histogram = new histogram_t(m__io, this, m__root);
    }
}

ifx_metrics_bin_t::userdata_t::~userdata_t() {
    _clean_up();
}

void ifx_metrics_bin_t::userdata_t::_clean_up() {
    if (!n_value_section) {
        if (m_value_section) {
            delete m_value_section; m_value_section = 0;
        }
    }
    if (m_metric_account) {
        delete m_metric_account; m_metric_account = 0;
    }
    if (m_metric_namespace) {
        delete m_metric_namespace; m_metric_namespace = 0;
    }
    if (m_metric_name) {
        delete m_metric_name; m_metric_name = 0;
    }
    if (m_dimensions_names) {
        for (std::vector<len_string_t*>::iterator it = m_dimensions_names->begin(); it != m_dimensions_names->end(); ++it) {
            delete *it;
        }
        delete m_dimensions_names; m_dimensions_names = 0;
    }
    if (m_dimensions_values) {
        for (std::vector<len_string_t*>::iterator it = m_dimensions_values->begin(); it != m_dimensions_values->end(); ++it) {
            delete *it;
        }
        delete m_dimensions_values; m_dimensions_values = 0;
    }
    if (!n_ap_container) {
        if (m_ap_container) {
            delete m_ap_container; m_ap_container = 0;
        }
    }
    if (!n_histogram) {
        if (m_histogram) {
            delete m_histogram; m_histogram = 0;
        }
    }
}

ifx_metrics_bin_t::metric_event_type_t ifx_metrics_bin_t::userdata_t::event_type() {
    if (f_event_type)
        return m_event_type;
    m_event_type = static_cast<ifx_metrics_bin_t::metric_event_type_t>(event_id());
    f_event_type = true;
    return m_event_type;
}

ifx_metrics_bin_t::pair_value_count_t::pair_value_count_t(kaitai::kstream* p__io, ifx_metrics_bin_t::histogram_value_count_pairs_t* p__parent, ifx_metrics_bin_t* p__root) : kaitai::kstruct(p__io) {
    m__parent = p__parent;
    m__root = p__root;

    try {
        _read();
    } catch(...) {
        _clean_up();
        throw;
    }
}

void ifx_metrics_bin_t::pair_value_count_t::_read() {
    m_value = m__io->read_u8le();
    m_count = m__io->read_u4le();
}

ifx_metrics_bin_t::pair_value_count_t::~pair_value_count_t() {
    _clean_up();
}

void ifx_metrics_bin_t::pair_value_count_t::_clean_up() {
}

ifx_metrics_bin_t::histogram_uint16_bucketed_t::histogram_uint16_bucketed_t(kaitai::kstream* p__io, ifx_metrics_bin_t::histogram_t* p__parent, ifx_metrics_bin_t* p__root) : kaitai::kstruct(p__io) {
    m__parent = p__parent;
    m__root = p__root;
    m_columns = 0;

    try {
        _read();
    } catch(...) {
        _clean_up();
        throw;
    }
}

void ifx_metrics_bin_t::histogram_uint16_bucketed_t::_read() {
    m_min = m__io->read_u8le();
    m_bucket_size = m__io->read_u4le();
    m_bucket_count = m__io->read_u4le();
    m_distribution_size = m__io->read_u2le();
    int l_columns = distribution_size();
    m_columns = new std::vector<pair_uint16_t*>();
    m_columns->reserve(l_columns);
    for (int i = 0; i < l_columns; i++) {
        m_columns->push_back(new pair_uint16_t(m__io, this, m__root));
    }
}

ifx_metrics_bin_t::histogram_uint16_bucketed_t::~histogram_uint16_bucketed_t() {
    _clean_up();
}

void ifx_metrics_bin_t::histogram_uint16_bucketed_t::_clean_up() {
    if (m_columns) {
        for (std::vector<pair_uint16_t*>::iterator it = m_columns->begin(); it != m_columns->end(); ++it) {
            delete *it;
        }
        delete m_columns; m_columns = 0;
    }
}

ifx_metrics_bin_t::histogram_value_count_pairs_t::histogram_value_count_pairs_t(kaitai::kstream* p__io, ifx_metrics_bin_t::histogram_t* p__parent, ifx_metrics_bin_t* p__root) : kaitai::kstruct(p__io) {
    m__parent = p__parent;
    m__root = p__root;
    m_columns = 0;

    try {
        _read();
    } catch(...) {
        _clean_up();
        throw;
    }
}

void ifx_metrics_bin_t::histogram_value_count_pairs_t::_read() {
    m_distribution_size = m__io->read_u2le();
    int l_columns = distribution_size();
    m_columns = new std::vector<pair_value_count_t*>();
    m_columns->reserve(l_columns);
    for (int i = 0; i < l_columns; i++) {
        m_columns->push_back(new pair_value_count_t(m__io, this, m__root));
    }
}

ifx_metrics_bin_t::histogram_value_count_pairs_t::~histogram_value_count_pairs_t() {
    _clean_up();
}

void ifx_metrics_bin_t::histogram_value_count_pairs_t::_clean_up() {
    if (m_columns) {
        for (std::vector<pair_value_count_t*>::iterator it = m_columns->begin(); it != m_columns->end(); ++it) {
            delete *it;
        }
        delete m_columns; m_columns = 0;
    }
}

ifx_metrics_bin_t::histogram_t::histogram_t(kaitai::kstream* p__io, ifx_metrics_bin_t::userdata_t* p__parent, ifx_metrics_bin_t* p__root) : kaitai::kstruct(p__io) {
    m__parent = p__parent;
    m__root = p__root;

    try {
        _read();
    } catch(...) {
        _clean_up();
        throw;
    }
}

void ifx_metrics_bin_t::histogram_t::_read() {
    m_version = m__io->read_u1();
    m_type = static_cast<ifx_metrics_bin_t::distribution_type_t>(m__io->read_u1());
    n_body = true;
    switch (type()) {
    case ifx_metrics_bin_t::DISTRIBUTION_TYPE_IFX_BUCKETED: {
        n_body = false;
        m_body = new histogram_uint16_bucketed_t(m__io, this, m__root);
        break;
    }
    case ifx_metrics_bin_t::DISTRIBUTION_TYPE_MON_AGENT_BUCKETED: {
        n_body = false;
        m_body = new histogram_uint16_bucketed_t(m__io, this, m__root);
        break;
    }
    case ifx_metrics_bin_t::DISTRIBUTION_TYPE_VALUE_COUNT_PAIRS: {
        n_body = false;
        m_body = new histogram_value_count_pairs_t(m__io, this, m__root);
        break;
    }
    }
}

ifx_metrics_bin_t::histogram_t::~histogram_t() {
    _clean_up();
}

void ifx_metrics_bin_t::histogram_t::_clean_up() {
    if (!n_body) {
        if (m_body) {
            delete m_body; m_body = 0;
        }
    }
}

ifx_metrics_bin_t::pair_uint16_t::pair_uint16_t(kaitai::kstream* p__io, ifx_metrics_bin_t::histogram_uint16_bucketed_t* p__parent, ifx_metrics_bin_t* p__root) : kaitai::kstruct(p__io) {
    m__parent = p__parent;
    m__root = p__root;

    try {
        _read();
    } catch(...) {
        _clean_up();
        throw;
    }
}

void ifx_metrics_bin_t::pair_uint16_t::_read() {
    m_index = m__io->read_u2le();
    m_count = m__io->read_u2le();
}

ifx_metrics_bin_t::pair_uint16_t::~pair_uint16_t() {
    _clean_up();
}

void ifx_metrics_bin_t::pair_uint16_t::_clean_up() {
}

ifx_metrics_bin_t::single_uint64_value_t::single_uint64_value_t(kaitai::kstream* p__io, ifx_metrics_bin_t::userdata_t* p__parent, ifx_metrics_bin_t* p__root) : kaitai::kstruct(p__io) {
    m__parent = p__parent;
    m__root = p__root;

    try {
        _read();
    } catch(...) {
        _clean_up();
        throw;
    }
}

void ifx_metrics_bin_t::single_uint64_value_t::_read() {
    m_padding = m__io->read_bytes(4);
    m_timestamp = m__io->read_u8le();
    m_value = m__io->read_u8le();
}

ifx_metrics_bin_t::single_uint64_value_t::~single_uint64_value_t() {
    _clean_up();
}

void ifx_metrics_bin_t::single_uint64_value_t::_clean_up() {
}

ifx_metrics_bin_t::ext_aggregated_double_value_t::ext_aggregated_double_value_t(kaitai::kstream* p__io, ifx_metrics_bin_t::userdata_t* p__parent, ifx_metrics_bin_t* p__root) : kaitai::kstruct(p__io) {
    m__parent = p__parent;
    m__root = p__root;

    try {
        _read();
    } catch(...) {
        _clean_up();
        throw;
    }
}

void ifx_metrics_bin_t::ext_aggregated_double_value_t::_read() {
    m_count = m__io->read_u4le();
    m_timestamp = m__io->read_u8le();
    m_sum = m__io->read_f8le();
    m_min = m__io->read_f8le();
    m_max = m__io->read_f8le();
}

ifx_metrics_bin_t::ext_aggregated_double_value_t::~ext_aggregated_double_value_t() {
    _clean_up();
}

void ifx_metrics_bin_t::ext_aggregated_double_value_t::_clean_up() {
}

ifx_metrics_bin_t::ext_aggregated_uint64_value_t::ext_aggregated_uint64_value_t(kaitai::kstream* p__io, ifx_metrics_bin_t::userdata_t* p__parent, ifx_metrics_bin_t* p__root) : kaitai::kstruct(p__io) {
    m__parent = p__parent;
    m__root = p__root;

    try {
        _read();
    } catch(...) {
        _clean_up();
        throw;
    }
}

void ifx_metrics_bin_t::ext_aggregated_uint64_value_t::_read() {
    m_count = m__io->read_u4le();
    m_timestamp = m__io->read_u8le();
    m_sum = m__io->read_u8le();
    m_min = m__io->read_u8le();
    m_max = m__io->read_u8le();
}

ifx_metrics_bin_t::ext_aggregated_uint64_value_t::~ext_aggregated_uint64_value_t() {
    _clean_up();
}

void ifx_metrics_bin_t::ext_aggregated_uint64_value_t::_clean_up() {
}

ifx_metrics_bin_t::single_double_value_t::single_double_value_t(kaitai::kstream* p__io, ifx_metrics_bin_t::userdata_t* p__parent, ifx_metrics_bin_t* p__root) : kaitai::kstruct(p__io) {
    m__parent = p__parent;
    m__root = p__root;

    try {
        _read();
    } catch(...) {
        _clean_up();
        throw;
    }
}

void ifx_metrics_bin_t::single_double_value_t::_read() {
    m_padding = m__io->read_bytes(4);
    m_timestamp = m__io->read_u8le();
    m_value = m__io->read_f8le();
}

ifx_metrics_bin_t::single_double_value_t::~single_double_value_t() {
    _clean_up();
}

void ifx_metrics_bin_t::single_double_value_t::_clean_up() {
}

ifx_metrics_bin_t::len_string_t::len_string_t(kaitai::kstream* p__io, ifx_metrics_bin_t::userdata_t* p__parent, ifx_metrics_bin_t* p__root) : kaitai::kstruct(p__io) {
    m__parent = p__parent;
    m__root = p__root;

    try {
        _read();
    } catch(...) {
        _clean_up();
        throw;
    }
}

void ifx_metrics_bin_t::len_string_t::_read() {
    m_len_value = m__io->read_u2le();
    m_value = kaitai::kstream::bytes_to_str(m__io->read_bytes(len_value()), std::string("UTF-8"));
}

ifx_metrics_bin_t::len_string_t::~len_string_t() {
    _clean_up();
}

void ifx_metrics_bin_t::len_string_t::_clean_up() {
}
