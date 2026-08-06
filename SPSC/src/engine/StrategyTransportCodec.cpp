/**
 * Codec implementation for IPC message serialization/deserialization.
 *
 * Encoding flow:
 *   1. StrategyEvent/Action variant comes in
 *   2. std::visit determines which concrete type it is
 *   3. Maps variant alternative -> numeric type-id
 *   4. Glaze serializes the payload to JSON
 *   5. Wraps in envelope: {"type":<id>,"payload":{...}}
 *   6. Returns the full JSON string to be pushed into the SHM ring
 *
 * Decoding flow:
 *   1. Reads JSON string from the ring
 *   2. Parses the envelope to extract type-id
 *   3. Based on type-id, deserializes payload into the correct C++ struct
 *   4. Wraps in StrategyEvent/Action variant and returns
 */
#include "engine/StrategyTransportCodec.hpp"

#include <glaze/glaze.hpp>
#include <type_traits>

struct StrategyWireEnvelope {
    std::uint16_t type{0};
    glz::raw_json payload{};
};

GLAZE_OBJECT(StrategyWireEnvelope,
             &StrategyWireEnvelope::type,
             &StrategyWireEnvelope::payload);

namespace {

enum class StrategyWireMessageType : std::uint16_t {
    EventBookUpdate = 1,
    EventIndexPrice = 2,
    EventSnapshot = 3,
    EventTopOfBook = 4,
    EventMarkPrice = 5,
    EventInstrumentState = 6,
    EventFundingRate = 7,
    EventTrades = 8,
    EventReplaceResponse = 9,
    EventCancelResponse = 10,
    EventAmendResponse = 11,
    EventPlaceResponse = 12,
    EventOrderUpdate = 13,
    EventFill = 14,
    EventCancelAllResponse = 15,
    EventAccountStatus = 16,
    EventWalletUpdate = 17,
    EventPositionUpdate = 18,
    EventLoginResponse = 19,

    ActionPlace = 101,
    ActionCancel = 102,
    ActionAmend = 103,
    ActionReplace = 104,
    ActionCancelAll = 105,
    ActionSubscribe = 106,
    ActionUnsubscribe = 107,
    ActionSetupExchange = 108,
    ActionSubscribeMarketDataOnly = 109,
    ActionWorkerReady = 110,
    ActionWorkerEventAck = 111,
    ActionWorkerHeartbeat = 112,
};

template <typename T>
std::expected<std::string, std::string> encodeMessage(std::uint16_t type, const T& payload) {
    std::string payloadJson;
    if (auto ec = glz::write_json(payload, payloadJson); ec) {
        return std::unexpected("failed to serialize payload");
    }

    StrategyWireEnvelope envelope{type, glz::raw_json{std::move(payloadJson)}};
    std::string wire;
    if (auto ec = glz::write_json(envelope, wire); ec) {
        return std::unexpected("failed to serialize envelope");
    }
    return wire;
}

template <typename T>
std::expected<T, std::string> decodePayload(const StrategyWireEnvelope& envelope) {
    T payload{};
    if (auto ec = glz::read_json(payload, envelope.payload.str); ec) {
        return std::unexpected(glz::format_error(ec, envelope.payload.str));
    }
    return payload;
}

template <typename Variant>
std::expected<Variant, std::string> decodeEnvelope(std::string_view wirePayload);

template <>
std::expected<StrategyEvent, std::string> decodeEnvelope<StrategyEvent>(std::string_view wirePayload) {
    StrategyWireEnvelope envelope{};
    if (auto ec = glz::read_json(envelope, wirePayload); ec) {
        return std::unexpected(glz::format_error(ec, wirePayload));
    }

    switch (static_cast<StrategyWireMessageType>(envelope.type)) {
        case StrategyWireMessageType::EventBookUpdate: {
            auto payload = decodePayload<StrategyEventBookUpdate>(envelope);
            if (!payload) return std::unexpected(payload.error());
            return StrategyEvent{std::move(payload.value())};
        }
        case StrategyWireMessageType::EventIndexPrice: {
            auto payload = decodePayload<StrategyEventIndexPrice>(envelope);
            if (!payload) return std::unexpected(payload.error());
            return StrategyEvent{std::move(payload.value())};
        }
        case StrategyWireMessageType::EventSnapshot: {
            auto payload = decodePayload<StrategyEventSnapshot>(envelope);
            if (!payload) return std::unexpected(payload.error());
            return StrategyEvent{std::move(payload.value())};
        }
        case StrategyWireMessageType::EventTopOfBook: {
            auto payload = decodePayload<StrategyEventTopOfBook>(envelope);
            if (!payload) return std::unexpected(payload.error());
            return StrategyEvent{std::move(payload.value())};
        }
        case StrategyWireMessageType::EventMarkPrice: {
            auto payload = decodePayload<StrategyEventMarkPrice>(envelope);
            if (!payload) return std::unexpected(payload.error());
            return StrategyEvent{std::move(payload.value())};
        }
        case StrategyWireMessageType::EventInstrumentState: {
            auto payload = decodePayload<StrategyEventInstrumentState>(envelope);
            if (!payload) return std::unexpected(payload.error());
            return StrategyEvent{std::move(payload.value())};
        }
        case StrategyWireMessageType::EventFundingRate: {
            auto payload = decodePayload<StrategyEventFundingRate>(envelope);
            if (!payload) return std::unexpected(payload.error());
            return StrategyEvent{std::move(payload.value())};
        }
        case StrategyWireMessageType::EventTrades: {
            auto payload = decodePayload<StrategyEventTrades>(envelope);
            if (!payload) return std::unexpected(payload.error());
            return StrategyEvent{std::move(payload.value())};
        }
        case StrategyWireMessageType::EventReplaceResponse: {
            auto payload = decodePayload<StrategyEventReplaceResponse>(envelope);
            if (!payload) return std::unexpected(payload.error());
            return StrategyEvent{std::move(payload.value())};
        }
        case StrategyWireMessageType::EventCancelResponse: {
            auto payload = decodePayload<StrategyEventCancelResponse>(envelope);
            if (!payload) return std::unexpected(payload.error());
            return StrategyEvent{std::move(payload.value())};
        }
        case StrategyWireMessageType::EventAmendResponse: {
            auto payload = decodePayload<StrategyEventAmendResponse>(envelope);
            if (!payload) return std::unexpected(payload.error());
            return StrategyEvent{std::move(payload.value())};
        }
        case StrategyWireMessageType::EventPlaceResponse: {
            auto payload = decodePayload<StrategyEventPlaceResponse>(envelope);
            if (!payload) return std::unexpected(payload.error());
            return StrategyEvent{std::move(payload.value())};
        }
        case StrategyWireMessageType::EventOrderUpdate: {
            auto payload = decodePayload<StrategyEventOrderUpdate>(envelope);
            if (!payload) return std::unexpected(payload.error());
            return StrategyEvent{std::move(payload.value())};
        }
        case StrategyWireMessageType::EventFill: {
            auto payload = decodePayload<StrategyEventFill>(envelope);
            if (!payload) return std::unexpected(payload.error());
            return StrategyEvent{std::move(payload.value())};
        }
        case StrategyWireMessageType::EventCancelAllResponse: {
            auto payload = decodePayload<StrategyEventCancelAllResponse>(envelope);
            if (!payload) return std::unexpected(payload.error());
            return StrategyEvent{std::move(payload.value())};
        }
        case StrategyWireMessageType::EventAccountStatus: {
            auto payload = decodePayload<StrategyEventAccountStatus>(envelope);
            if (!payload) return std::unexpected(payload.error());
            return StrategyEvent{std::move(payload.value())};
        }
        case StrategyWireMessageType::EventWalletUpdate: {
            auto payload = decodePayload<StrategyEventWalletUpdate>(envelope);
            if (!payload) return std::unexpected(payload.error());
            return StrategyEvent{std::move(payload.value())};
        }
        case StrategyWireMessageType::EventPositionUpdate: {
            auto payload = decodePayload<StrategyEventPositionUpdate>(envelope);
            if (!payload) return std::unexpected(payload.error());
            return StrategyEvent{std::move(payload.value())};
        }
        case StrategyWireMessageType::EventLoginResponse: {
            auto payload = decodePayload<StrategyEventLoginResponse>(envelope);
            if (!payload) return std::unexpected(payload.error());
            return StrategyEvent{std::move(payload.value())};
        }
        default:
            return std::unexpected("unknown event message type");
    }
}

template <>
std::expected<StrategyAction, std::string> decodeEnvelope<StrategyAction>(std::string_view wirePayload) {
    StrategyWireEnvelope envelope{};
    if (auto ec = glz::read_json(envelope, wirePayload); ec) {
        return std::unexpected(glz::format_error(ec, wirePayload));
    }

    switch (static_cast<StrategyWireMessageType>(envelope.type)) {
        case StrategyWireMessageType::ActionPlace: {
            auto payload = decodePayload<StrategyActionPlace>(envelope);
            if (!payload) return std::unexpected(payload.error());
            return StrategyAction{std::move(payload.value())};
        }
        case StrategyWireMessageType::ActionCancel: {
            auto payload = decodePayload<StrategyActionCancel>(envelope);
            if (!payload) return std::unexpected(payload.error());
            return StrategyAction{std::move(payload.value())};
        }
        case StrategyWireMessageType::ActionAmend: {
            auto payload = decodePayload<StrategyActionAmend>(envelope);
            if (!payload) return std::unexpected(payload.error());
            return StrategyAction{std::move(payload.value())};
        }
        case StrategyWireMessageType::ActionReplace: {
            auto payload = decodePayload<StrategyActionReplace>(envelope);
            if (!payload) return std::unexpected(payload.error());
            return StrategyAction{std::move(payload.value())};
        }
        case StrategyWireMessageType::ActionCancelAll: {
            auto payload = decodePayload<StrategyActionCancelAll>(envelope);
            if (!payload) return std::unexpected(payload.error());
            return StrategyAction{std::move(payload.value())};
        }
        case StrategyWireMessageType::ActionSubscribe: {
            auto payload = decodePayload<StrategyActionSubscribe>(envelope);
            if (!payload) return std::unexpected(payload.error());
            return StrategyAction{std::move(payload.value())};
        }
        case StrategyWireMessageType::ActionUnsubscribe: {
            auto payload = decodePayload<StrategyActionUnsubscribe>(envelope);
            if (!payload) return std::unexpected(payload.error());
            return StrategyAction{std::move(payload.value())};
        }
        case StrategyWireMessageType::ActionSetupExchange: {
            auto payload = decodePayload<StrategyActionSetupExchange>(envelope);
            if (!payload) return std::unexpected(payload.error());
            return StrategyAction{std::move(payload.value())};
        }
        case StrategyWireMessageType::ActionSubscribeMarketDataOnly: {
            auto payload = decodePayload<StrategyActionSubscribeMarketDataOnly>(envelope);
            if (!payload) return std::unexpected(payload.error());
            return StrategyAction{std::move(payload.value())};
        }
        case StrategyWireMessageType::ActionWorkerReady: {
            auto payload = decodePayload<StrategyActionWorkerReady>(envelope);
            if (!payload) return std::unexpected(payload.error());
            return StrategyAction{std::move(payload.value())};
        }
        case StrategyWireMessageType::ActionWorkerEventAck: {
            auto payload = decodePayload<StrategyActionWorkerEventAck>(envelope);
            if (!payload) return std::unexpected(payload.error());
            return StrategyAction{std::move(payload.value())};
        }
        case StrategyWireMessageType::ActionWorkerHeartbeat: {
            auto payload = decodePayload<StrategyActionWorkerHeartbeat>(envelope);
            if (!payload) return std::unexpected(payload.error());
            return StrategyAction{std::move(payload.value())};
        }
        default:
            return std::unexpected("unknown action message type");
    }
}

} // namespace

std::expected<std::string, std::string> encodeStrategyEventMessage(const StrategyEvent& event) {
    return std::visit([](const auto& payload) -> std::expected<std::string, std::string> {
        using T = std::decay_t<decltype(payload)>;
        if constexpr (std::is_same_v<T, StrategyEventBookUpdate>) {
            return encodeMessage(static_cast<std::uint16_t>(StrategyWireMessageType::EventBookUpdate), payload);
        } else if constexpr (std::is_same_v<T, StrategyEventIndexPrice>) {
            return encodeMessage(static_cast<std::uint16_t>(StrategyWireMessageType::EventIndexPrice), payload);
        } else if constexpr (std::is_same_v<T, StrategyEventSnapshot>) {
            return encodeMessage(static_cast<std::uint16_t>(StrategyWireMessageType::EventSnapshot), payload);
        } else if constexpr (std::is_same_v<T, StrategyEventTopOfBook>) {
            return encodeMessage(static_cast<std::uint16_t>(StrategyWireMessageType::EventTopOfBook), payload);
        } else if constexpr (std::is_same_v<T, StrategyEventMarkPrice>) {
            return encodeMessage(static_cast<std::uint16_t>(StrategyWireMessageType::EventMarkPrice), payload);
        } else if constexpr (std::is_same_v<T, StrategyEventInstrumentState>) {
            return encodeMessage(static_cast<std::uint16_t>(StrategyWireMessageType::EventInstrumentState), payload);
        } else if constexpr (std::is_same_v<T, StrategyEventFundingRate>) {
            return encodeMessage(static_cast<std::uint16_t>(StrategyWireMessageType::EventFundingRate), payload);
        } else if constexpr (std::is_same_v<T, StrategyEventTrades>) {
            return encodeMessage(static_cast<std::uint16_t>(StrategyWireMessageType::EventTrades), payload);
        } else if constexpr (std::is_same_v<T, StrategyEventReplaceResponse>) {
            return encodeMessage(static_cast<std::uint16_t>(StrategyWireMessageType::EventReplaceResponse), payload);
        } else if constexpr (std::is_same_v<T, StrategyEventCancelResponse>) {
            return encodeMessage(static_cast<std::uint16_t>(StrategyWireMessageType::EventCancelResponse), payload);
        } else if constexpr (std::is_same_v<T, StrategyEventAmendResponse>) {
            return encodeMessage(static_cast<std::uint16_t>(StrategyWireMessageType::EventAmendResponse), payload);
        } else if constexpr (std::is_same_v<T, StrategyEventPlaceResponse>) {
            return encodeMessage(static_cast<std::uint16_t>(StrategyWireMessageType::EventPlaceResponse), payload);
        } else if constexpr (std::is_same_v<T, StrategyEventOrderUpdate>) {
            return encodeMessage(static_cast<std::uint16_t>(StrategyWireMessageType::EventOrderUpdate), payload);
        } else if constexpr (std::is_same_v<T, StrategyEventFill>) {
            return encodeMessage(static_cast<std::uint16_t>(StrategyWireMessageType::EventFill), payload);
        } else if constexpr (std::is_same_v<T, StrategyEventCancelAllResponse>) {
            return encodeMessage(static_cast<std::uint16_t>(StrategyWireMessageType::EventCancelAllResponse), payload);
        } else if constexpr (std::is_same_v<T, StrategyEventAccountStatus>) {
            return encodeMessage(static_cast<std::uint16_t>(StrategyWireMessageType::EventAccountStatus), payload);
        } else if constexpr (std::is_same_v<T, StrategyEventWalletUpdate>) {
            return encodeMessage(static_cast<std::uint16_t>(StrategyWireMessageType::EventWalletUpdate), payload);
        } else if constexpr (std::is_same_v<T, StrategyEventPositionUpdate>) {
            return encodeMessage(static_cast<std::uint16_t>(StrategyWireMessageType::EventPositionUpdate), payload);
        } else if constexpr (std::is_same_v<T, StrategyEventLoginResponse>) {
            return encodeMessage(static_cast<std::uint16_t>(StrategyWireMessageType::EventLoginResponse), payload);
        } else {
            return std::unexpected("unsupported event payload");
        }
    }, event);
}

std::expected<StrategyEvent, std::string> decodeStrategyEventMessage(std::string_view wirePayload) {
    return decodeEnvelope<StrategyEvent>(wirePayload);
}

std::expected<std::string, std::string> encodeStrategyActionMessage(const StrategyAction& action) {
    return std::visit([](const auto& payload) -> std::expected<std::string, std::string> {
        using T = std::decay_t<decltype(payload)>;
        if constexpr (std::is_same_v<T, StrategyActionPlace>) {
            return encodeMessage(static_cast<std::uint16_t>(StrategyWireMessageType::ActionPlace), payload);
        } else if constexpr (std::is_same_v<T, StrategyActionCancel>) {
            return encodeMessage(static_cast<std::uint16_t>(StrategyWireMessageType::ActionCancel), payload);
        } else if constexpr (std::is_same_v<T, StrategyActionAmend>) {
            return encodeMessage(static_cast<std::uint16_t>(StrategyWireMessageType::ActionAmend), payload);
        } else if constexpr (std::is_same_v<T, StrategyActionReplace>) {
            return encodeMessage(static_cast<std::uint16_t>(StrategyWireMessageType::ActionReplace), payload);
        } else if constexpr (std::is_same_v<T, StrategyActionCancelAll>) {
            return encodeMessage(static_cast<std::uint16_t>(StrategyWireMessageType::ActionCancelAll), payload);
        } else if constexpr (std::is_same_v<T, StrategyActionSubscribe>) {
            return encodeMessage(static_cast<std::uint16_t>(StrategyWireMessageType::ActionSubscribe), payload);
        } else if constexpr (std::is_same_v<T, StrategyActionUnsubscribe>) {
            return encodeMessage(static_cast<std::uint16_t>(StrategyWireMessageType::ActionUnsubscribe), payload);
        } else if constexpr (std::is_same_v<T, StrategyActionSetupExchange>) {
            return encodeMessage(static_cast<std::uint16_t>(StrategyWireMessageType::ActionSetupExchange), payload);
        } else if constexpr (std::is_same_v<T, StrategyActionSubscribeMarketDataOnly>) {
            return encodeMessage(static_cast<std::uint16_t>(StrategyWireMessageType::ActionSubscribeMarketDataOnly), payload);
        } else if constexpr (std::is_same_v<T, StrategyActionWorkerReady>) {
            return encodeMessage(static_cast<std::uint16_t>(StrategyWireMessageType::ActionWorkerReady), payload);
        } else if constexpr (std::is_same_v<T, StrategyActionWorkerEventAck>) {
            return encodeMessage(static_cast<std::uint16_t>(StrategyWireMessageType::ActionWorkerEventAck), payload);
        } else if constexpr (std::is_same_v<T, StrategyActionWorkerHeartbeat>) {
            return encodeMessage(static_cast<std::uint16_t>(StrategyWireMessageType::ActionWorkerHeartbeat), payload);
        } else {
            return std::unexpected("unsupported action payload");
        }
    }, action);
}

std::expected<StrategyAction, std::string> decodeStrategyActionMessage(std::string_view wirePayload) {
    return decodeEnvelope<StrategyAction>(wirePayload);
}
