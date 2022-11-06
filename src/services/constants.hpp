#pragma once

namespace services::constants {
    typedef std::string_view metadata;
    // defines the size of a row or column in the database.
    constexpr metadata size_md = "m_size";
    // defines the round number.
    constexpr metadata round_md = "round";
    // defines the epoch number.
    constexpr metadata epoch_md = "epoch";
}