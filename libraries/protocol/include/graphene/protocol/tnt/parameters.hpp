/*
 *AcloudBank
 */
#pragma once
#include <graphene/protocol/base.hpp>
#include <graphene/protocol/tnt/accessories_fwd.hpp>

namespace graphene { namespace protocol { namespace tnt {

/// Chain-defined parameters and limits for TNT structures
struct parameters_type {
   /// The maximum length of a connection chain (such as a sequence of tank attachments)
   uint16_t max_connection_chain_length = GRAPHENE_DEFAULT_MAX_CONNECTION_CHAIN_LENGTH;
   /// The maximum number of taps a single transaction may open
   uint16_t max_taps_to_open = GRAPHENE_DEFAULT_MAX_TAPS_TO_OPEN;
   /// The base deposit required for all tanks
   uint64_t tank_deposit = GRAPHENE_DEFAULT_TANK_DEPOSIT;
   /// The default deposit for tap requirements
   uint64_t default_tap_requirement_deposit = GRAPHENE_DEFAULT_TAP_REQUIREMENT_DEPOSIT;
   /// The default deposit for tank attachments
   uint64_t default_tank_attachment_deposit = GRAPHENE_DEFAULT_TANK_ATTACHMENT_DEPOSIT;
   /// The premium added to the deposit on stateful tank accessories
   uint64_t stateful_accessory_deposit_premium = GRAPHENE_DEFAULT_STATEFUL_ACCESSORY_DEPOSIT_PREMIUM;
   /// Override deposit amounts for specific tank accessories
   flat_map<uint64_t, uint64_t> override_deposits = GRAPHENE_DEFAULT_OVERRIDE_TANK_ACCESSORY_DEPOSITS;

   extensions_type extensions;
};

} } } // namespace graphene::protocol::tnt

FC_REFLECT(graphene::protocol::tnt::parameters_type,
           (max_connection_chain_length)(max_taps_to_open)(tank_deposit)(default_tap_requirement_deposit)
           (default_tank_attachment_deposit)(stateful_accessory_deposit_premium)(override_deposits)(extensions))
