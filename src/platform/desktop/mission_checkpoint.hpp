#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace saga::desktop {
class MissionProfile;
struct MissionStateDigest {
    std::string hash,logic_hash;
    std::uint64_t bytes=0,logic_bytes=0;
};
// Versioned, explicit field traversal; no object bytes, addresses, getters or report drawing.
MissionStateDigest mission_state_digest(const MissionProfile&);
std::vector<std::uint8_t> mission_state_bytes(const MissionProfile&);
}
