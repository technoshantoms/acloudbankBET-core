#pragma once
/*
 *AcloudBank
 */

#include <graphene/chain/tnt/query_evaluator.hpp>
#include <graphene/chain/tnt/tap_flow_evaluator.hpp>

#include <graphene/protocol/tnt/operations.hpp>

namespace graphene { namespace chain { namespace tnt {

/**
 * @brief Set the tap_open_count and required_authorities fields on a @ref tap_open_operation
 * @param op The operation to configure
 * @ingroup TNT
 */
void set_tap_open_count_and_authorities(const database& db, tap_open_operation& op) {
   const auto& tank = db.get(*op.tap_to_open.tank_id);
   const auto& tap = tank.schematic.taps.at(op.tap_to_open.tap_id);
   op.required_authorities.clear();

   // Fast track: if destroying an empty tank, only the tap authority is required, and only one tap is opened
   if (op.deposit_claimed.valid() && tank.balance == 0) {
      FC_ASSERT(op.queries.empty(),
                "When destroying an empty tank, queries are not evaluated, so they must not be provided");
      op.tap_open_count = 1;
      if (tap.open_authority.valid())
         op.required_authorities = {*tap.open_authority};
      return;
   }

   query_evaluator eval;
   eval.set_query_tank(tank);
   cow_db_wrapper wdb(db);
   auto max_taps = db.get_global_properties().parameters.extensions.value.updatable_tnt_options->max_taps_to_open;

   auto add_auth = [&old_auths=op.required_authorities](authority auth) {
      if (std::find(old_auths.begin(), old_auths.end(), auth) == old_auths.end())
         old_auths.emplace_back(std::move(auth));
   };
   auto pay_account = [](account_id_type, asset, vector<ptnt::connection>) {};

   if (tap.open_authority.valid())
      add_auth(*tap.open_authority);

   for (const auto& query : op.queries) {
      auto new_auths = eval.evaluate_query(query, db);
      for (auto& auth : new_auths)
         add_auth(std::move(auth));
   }

   auto flows = evaluate_tap_flow(wdb, eval, op.payer, op.tap_to_open, op.release_amount, max_taps,
                                  std::move(pay_account));
   op.tap_open_count = flows.size();
}

} } } // namespace graphene::chain::tnt
