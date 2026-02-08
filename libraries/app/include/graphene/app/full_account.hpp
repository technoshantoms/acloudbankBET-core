#pragma once

#include <graphene/chain/account_object.hpp>
#include <graphene/chain/market_object.hpp>
#include <graphene/chain/market_evaluator.hpp>
#include <graphene/chain/vesting_balance_object.hpp>
#include <graphene/chain/withdraw_permission_object.hpp>
#include <graphene/chain/proposal_object.hpp>

namespace graphene { namespace app {
using namespace graphene::chain;

struct full_account {
   account_object account;
   account_statistics_object statistics;
   string registrar_name;
   string referrer_name;
   string lifetime_referrer_name;
   vector<variant> votes;
   optional<vesting_balance_object> cashback_balance;
   vector<account_balance_object> balances;
   vector<vesting_balance_object> vesting_balances;
   vector<limit_order_object> limit_orders;
   vector<call_order_object> call_orders;
   vector<force_settlement_object> settle_orders;
   vector<proposal_object> proposals;
   vector<asset_id_type> assets;
   vector<withdraw_permission_object> withdraws;
   //      vector<pending_dividend_payout_balance_object> pending_dividend_payments;
   vector<pending_dividend_payout_balance_for_holder_object> pending_dividend_payments;
};

}} // namespace graphene::app

// clang-format off

FC_REFLECT(graphene::app::full_account,
      (account)
      (statistics)
      (registrar_name)
      (referrer_name)
      (lifetime_referrer_name)
      (votes)
      (cashback_balance)
      (balances)
      (vesting_balances)
      (limit_orders)
      (call_orders)
      (settle_orders)
      (proposals)
      (assets)
      (withdraws)
      (pending_dividend_payments))

// clang-format on
