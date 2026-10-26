#include "call_context.hpp"
namespace saga::core {
namespace {
template<class T> T& required(const std::shared_ptr<T>& value) {
    if (!value) throw CultureManagedException("null call-context object");
    return *value;
}
std::shared_ptr<CallDataStore> destination(std::shared_ptr<CallDataStore>& data, CallContextHost& host) {
    if (!data) data = host.make_store();
    return data;
}
void copy_entries(CallDataEnumerator& entries, std::shared_ptr<CallDataStore>& data,
                  bool correlation, CallContextHost& host) {
    while (entries.next()) {
        if (correlation) {
            auto key = entries.key();
            if (!key) throw CultureManagedException("null correlation key");
            const auto name = host.string_key(key);
            if (!name) throw CultureManagedException("null correlation key");
            const bool clone = *name == u"System.Diagnostics.Trace.CorrelationManagerSlot";
            auto target = destination(data, host);
            auto value = entries.value();
            if (clone) {
                if (!value) throw CultureManagedException("null correlation value");
                value = host.clone_correlation(value);
            }
            required(target).set(key, value);
        } else {
            auto target = destination(data, host);
            auto key = entries.key();
            auto value = entries.value();
            auto& target_ref = required(target);
            host.string_key(key); // Cast after fetching both key and value.
            target_ref.set(key, value);
        }
    }
}

}
// IllogicalCallContext.CreateCopy 0x1d554e0.
std::shared_ptr<IllogicalCallContext> copy_illogical_call_context(const IllogicalCallContext& source, CallContextHost& host) {
    auto result = std::make_shared<IllogicalCallContext>();
    result->host_context = source.host_context;
    auto data = source.data;
    if (data && data->count() > 0) {
        auto live_data = source.data;
        auto entries = required(live_data).enumerate();
        copy_entries(required(entries), result->data, false, host);
    }
    return result;
}
// LogicalCallContext.Clone 0x1d56280.
std::shared_ptr<LogicalCallContext> copy_logical_call_context(const LogicalCallContext& source, CallContextHost& host) {
    auto result = std::make_shared<LogicalCallContext>();
    // CallContextRemotingData.Clone 0x1d56888 and SecurityData.Clone 0x1d568f0:
    // fresh wrappers, shared string/principal references.
    if (source.remoting) result->remoting = std::make_shared<CallRemotingData>(*source.remoting);
    if (source.security) result->security = std::make_shared<CallSecurityData>(*source.security);
    result->host_context = source.host_context;
    result->correlation_manager = source.correlation_manager;
    auto data = source.data;
    if (data && data->count() > 0) {
        auto live_data = source.data;
        auto entries = required(live_data).enumerate();
        // The branch reads the source again after GetEnumerator, not the copied flag.
        const bool correlation = source.correlation_manager;
        copy_entries(required(entries), result->data, correlation, host);
    }
    return result;
}
AsyncValue TypedCallContextCopyHost::clone_logical_call(AsyncValue value) {
    return copy_logical_call_context(required(std::static_pointer_cast<LogicalCallContext>(value)), host_);
}
AsyncValue TypedCallContextCopyHost::clone_illogical_call(AsyncValue value) {
    return copy_illogical_call_context(required(std::static_pointer_cast<IllogicalCallContext>(value)), host_);
}
}
