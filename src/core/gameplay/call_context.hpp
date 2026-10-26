#pragma once
#include "execution_context.hpp"
#include <optional>
#include <string>
namespace saga::core {
class CallDataEnumerator {
public:
    virtual ~CallDataEnumerator() = default;
    virtual bool next() = 0;
    virtual AsyncValue key() = 0;
    virtual AsyncValue value() = 0;
};
class CallDataStore {
public:
    virtual ~CallDataStore() = default;
    virtual std::int32_t count() = 0;
    virtual std::shared_ptr<CallDataEnumerator> enumerate() = 0;
    virtual void set(AsyncValue key, AsyncValue value) = 0;
};
class CallContextHost {
public:
    virtual ~CallContextHost() = default;
    // Default Hashtable allocation; enumeration/hash semantics remain a host contract.
    virtual std::shared_ptr<CallDataStore> make_store() = 0;
    // Reject non-string objects with a managed cast failure; null stays null.
    virtual std::optional<std::u16string> string_key(AsyncValue) = 0;
    // Require ICloneable and dispatch Clone; null results are accepted.
    virtual AsyncValue clone_correlation(AsyncValue) = 0;
};
struct CallRemotingData { AsyncValue logical_call_id; }; // +0x10
struct CallSecurityData { AsyncValue principal; }; // +0x10
struct IllogicalCallContext {
    std::shared_ptr<CallDataStore> data; // +0x10
    AsyncValue host_context; // +0x18
};
struct LogicalCallContext {
    std::shared_ptr<CallDataStore> data; // +0x10
    std::shared_ptr<CallRemotingData> remoting; // +0x18
    std::shared_ptr<CallSecurityData> security; // +0x20
    AsyncValue host_context; // +0x28
    bool correlation_manager = false; // +0x30
    AsyncValue send_headers, receive_headers; // +0x38, +0x40; not copied by Clone
};
std::shared_ptr<IllogicalCallContext> copy_illogical_call_context(const IllogicalCallContext&, CallContextHost&);
std::shared_ptr<LogicalCallContext> copy_logical_call_context(const LogicalCallContext&, CallContextHost&);
// Execution-state call fields must contain the matching portable context types.
// The host must outlive this adapter. Arbitrary runtime objects require a typed bridge.
class TypedCallContextCopyHost : public ExecutionContextCopyHost {
public:
    explicit TypedCallContextCopyHost(CallContextHost& host):host_(host){}
    AsyncValue clone_logical_call(AsyncValue) override;
    AsyncValue clone_illogical_call(AsyncValue) override;
private:
    CallContextHost& host_;
};
}
