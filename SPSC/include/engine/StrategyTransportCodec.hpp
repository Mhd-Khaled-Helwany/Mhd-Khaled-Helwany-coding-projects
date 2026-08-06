/**
 * Codec layer for IPC message serialization.
 *
 * Translates between C++ variant types (StrategyEvent, StrategyAction) and
 * JSON wire format for transport over shared memory. Each message is wrapped
 * in an envelope: {"type":<id>,"payload":{...}} where the type-id identifies
 * which variant alternative to decode into.
 *
 * Does NOT contain the transport itself or the message definitions.
 * See StrategyRuntimeMessages.hpp for types, ShmStrategyTransport for transport.
 */
#pragma once

#include "engine/StrategyRuntimeMessages.hpp"

#include <expected>
#include <string>
#include <string_view>

std::expected<std::string, std::string> encodeStrategyEventMessage(const StrategyEvent& event);
std::expected<StrategyEvent, std::string> decodeStrategyEventMessage(std::string_view wirePayload);

std::expected<std::string, std::string> encodeStrategyActionMessage(const StrategyAction& action);
std::expected<StrategyAction, std::string> decodeStrategyActionMessage(std::string_view wirePayload);
