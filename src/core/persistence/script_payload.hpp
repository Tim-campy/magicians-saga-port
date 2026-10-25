#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>

namespace saga::core {
using ScriptRows = std::vector<std::vector<std::int32_t>>;
struct DecodedScriptPayload {
    ScriptRows rows;
    std::size_t consumed;
};
// AppData.SerializeScripts/DeserializeScripts block, not a save envelope or
// DelayEvent serialization. Input rows are a stable, non-null snapshot.
std::vector<std::uint8_t> encode_script_payload(const ScriptRows& rows);
// Finite-buffer prefix decoder: trailing bytes remain unconsumed. A signed
// outer count <= 0 produces an empty collection, as in the original reader.
// Negative row lengths throw length_error; incomplete data throws out_of_range.
// No emulation of incremental stream I/O or partial mutation on failure.
DecodedScriptPayload decode_script_payload(const std::vector<std::uint8_t>& bytes);
}
