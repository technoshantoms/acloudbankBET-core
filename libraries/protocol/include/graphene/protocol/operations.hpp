
#pragma once
#include <graphene/protocol/base.hpp>
#include <graphene/protocol/account.hpp>
#include <graphene/protocol/assert.hpp>
#include <graphene/protocol/asset_ops.hpp>
#include <graphene/protocol/balance.hpp>
#include <graphene/protocol/ico_balance.hpp>
#include <graphene/protocol/custom.hpp>
#include <graphene/protocol/committee_member.hpp>
#include <graphene/protocol/custom_authority.hpp>
#include <graphene/protocol/fba.hpp>
#include <graphene/protocol/market.hpp>
#include <graphene/protocol/proposal.hpp>
#include <graphene/protocol/ticket.hpp>
#include <graphene/protocol/transfer.hpp>
#include <graphene/protocol/vesting.hpp>
#include <graphene/protocol/withdraw_permission.hpp>
#include <graphene/protocol/witness.hpp>
#include <graphene/protocol/worker.hpp>
#include <graphene/protocol/htlc.hpp>
#include <graphene/protocol/tnt/operations.hpp>
#include <graphene/protocol/personal_data.hpp>
#include <graphene/protocol/content_card.hpp>
#include <graphene/protocol/permission.hpp>
#include <graphene/protocol/commit_reveal.hpp>
#include <graphene/protocol/account_role.hpp>

//bet
#include <graphene/protocol/sport.hpp>
#include <graphene/protocol/event_group.hpp>
#include <graphene/protocol/event.hpp>
#include <graphene/protocol/betting_market.hpp>
#include <graphene/protocol/tournament.hpp>

#include <graphene/protocol/nft_lottery.hpp>
#include <graphene/protocol/lottery_ops.hpp>
//refferal payout
//#include <graphene/protocol/affiliate.hpp>
//nft
#include <graphene/protocol/custom_permission.hpp>
#include <graphene/protocol/custom_account_authority.hpp>
#include <graphene/protocol/offer.hpp>
#include <graphene/protocol/nft_ops.hpp>

/*
#include <graphene/protocol/sidechain_address.hpp>
#include <graphene/protocol/sidechain_transaction.hpp>
*/

#include <graphene/protocol/random_number.hpp>


namespace graphene { namespace protocol {

   /**
    * @ingroup operations
    *
    * Defines the set of valid operations as a discriminated union type.
    */
   typedef fc::static_variant<
            /*  0 */ transfer_operation,
            /*  1 */ account_create_operation,
            /*  2 */ account_update_operation,
            /*  3 */ account_whitelist_operation,
            /*  4 */ account_upgrade_operation,
            /*  5 */ account_transfer_operation,
            /*  6 */ asset_create_operation,
            /*  7 */ asset_update_operation,
            /*  8 */ asset_update_bitasset_operation,
            /*  9 */ asset_update_feed_producers_operation,
            /* 10 */ asset_issue_operation,
            /* 11 */ asset_reserve_operation,
            /* 12 */ asset_fund_fee_pool_operation,
            /* 13 */ asset_settle_operation,
            /* 14 */ asset_global_settle_operation,
            /* 15 */ asset_publish_feed_operation,
            /* 16 */ witness_create_operation,
            /* 17 */ witness_update_operation,
            /* 18 */ proposal_create_operation,
            /* 19 */ proposal_update_operation,
            /* 20 */ proposal_delete_operation,
            /* 21 */ withdraw_permission_create_operation,
            /* 22 */ withdraw_permission_update_operation,
            /* 23 */ withdraw_permission_claim_operation,
            /* 24 */ withdraw_permission_delete_operation,
            /* 25 */ committee_member_create_operation,
            /* 26 */ committee_member_update_operation,
            /* 27 */ committee_member_update_global_parameters_operation,
            /* 28 */ vesting_balance_create_operation,
            /* 29 */ vesting_balance_withdraw_operation,
            /* 30 */ custom_operation,
            /* 31 */ assert_operation,
            /* 32 */ balance_claim_operation,
            /* 33 */ override_transfer_operation,
            /* 34 */ asset_settle_cancel_operation,  // VIRTUAL
            /* 35 */ asset_claim_fees_operation,
            /* 36 */ fba_distribute_operation,       // VIRTUAL
            /* 37 */ asset_claim_pool_operation,
            /* 38 */ asset_update_issuer_operation,
            /* 39 */ personal_data_create_operation,
            /* 40 */ personal_data_remove_operation,
            /* 41 */ content_card_create_operation,
            /* 42 */ content_card_update_operation,
            /* 43 */ content_card_remove_operation,
            /* 44 */ permission_create_operation,
            /* 45 */ permission_remove_operation,
            /* 46 */ commit_create_operation,
            /* 47 */ reveal_create_operation,
            /* 48 */ worker_create_operation,
            /* 49 */ htlc_create_operation,
            /* 50 */ htlc_redeem_operation,
            /* 51 */ htlc_redeemed_operation,         // VIRTUAL
            /* 52 */ htlc_extend_operation,
            /* 53 */ htlc_refund_operation,           // VIRTUAL
            /* 54 */ limit_order_create_operation,
            /* 55 */ limit_order_cancel_operation,
            /* 56 */ call_order_update_operation,
            /* 57 */ fill_order_operation,            // VIRTUAL
            /* 58 */ custom_authority_create_operation,
            /* 59 */ custom_authority_update_operation,
            /* 60 */ custom_authority_delete_operation,
            /* 61 */ ticket_create_operation,
            /* 62 */ ticket_update_operation,
            /* 63 */ ico_balance_claim_operation,
            /* 64 */ tank_create_operation,
            /* 65 */ tank_update_operation,
            /* 66 */ tank_delete_operation,
            /* 67 */ tank_query_operation,
            /* 68 */ tap_open_operation,
            /* 69 */ tap_connect_operation,
            /* 70 */ account_fund_connection_operation,
            /* 71 */ connection_fund_account_operation,     // VIRTUAL
            /* 72 */lottery_asset_create_operation,
            /* 73 */ticket_purchase_operation,
            /* 74 */lottery_reward_operation,
            /* 75 */lottery_end_operation,
            /* 76 */ sweeps_vesting_claim_operation,
            /* 77 */ custom_permission_create_operation,
            /* 78 */ custom_permission_update_operation,
            /* 79 */ custom_permission_delete_operation,
            /* 80 */account_role_create_operation,
            /* 81 */account_role_update_operation,
            /* 82 */account_role_delete_operation,
            /* 83 */ custom_account_authority_create_operation,
            /* 84 */ custom_account_authority_update_operation,
            /* 85 */ custom_account_authority_delete_operation,
            /* 86 */ offer_operation,
            /* 87 */ bid_operation,
            /* 88 */ cancel_offer_operation,
            /* 89 */ finalize_offer_operation,
            /* 90 */ nft_metadata_create_operation,
            /* 91 */ nft_metadata_update_operation,
            /* 92 */ nft_mint_operation,
            /* 93 */ nft_safe_transfer_from_operation,
            /* 94 */ nft_approve_operation,
            /* 95 */ nft_set_approval_for_all_operation,
            /* 96 */ nft_lottery_token_purchase_operation,
            /* 97 */ nft_lottery_reward_operation,
            /* 98 */ nft_lottery_end_operation,
                  /* 99 */random_number_store_operation,
            //sidechain_address_add_operation,
            //sidechain_address_update_operation,
            //sidechain_address_delete_operation,
            //sidechain_transaction_create_operation,
            //sidechain_transaction_sign_operation,
            //sidechain_transaction_send_operation,
            //sidechain_transaction_settle_operation,
            tournament_create_operation,
            tournament_join_operation,
            game_move_operation,
            asset_update_dividend_operation,
            asset_dividend_distribution_operation, // VIRTUAL
            tournament_payout_operation, // VIRTUAL
            tournament_leave_operation,
            sport_create_operation,
            sport_update_operation,
            event_group_create_operation,
            event_group_update_operation,
            event_create_operation,
            event_update_operation,
            betting_market_rules_create_operation,
            betting_market_rules_update_operation,
            betting_market_group_create_operation,
            betting_market_create_operation,
            bet_place_operation,
            betting_market_group_resolve_operation,
            betting_market_group_resolved_operation, // VIRTUAL
            bet_adjusted_operation, // VIRTUAL
            betting_market_group_cancel_unmatched_bets_operation,
            bet_matched_operation, // VIRTUAL
            bet_cancel_operation,
            bet_canceled_operation, // VIRTUAL
            betting_market_group_update_operation,
            betting_market_update_operation,
            event_update_status_operation,
            sport_delete_operation,
            event_group_delete_operation
            //affiliate_payout_operation, // VIRTUAL
            //affiliate_referral_payout_operation // VIRTUAL
          // /* 100 */ affiliate_referral_payout_operation // VIRTUAL
         > operation;

   /// @} // operations group

   /**
    *  Appends required authorites to the result vector.  The authorities appended are not the
    *  same as those returned by get_required_auth 
    *
    *  @return a set of required authorities for @p op
    */
   void operation_get_required_authorities( const operation& op,
                                            flat_set<account_id_type>& active,
                                            flat_set<account_id_type>& owner,
                                            vector<authority>& other,
                                            bool ignore_custom_operation_required_auths );

   void operation_validate( const operation& op );

   /**
    *  @brief necessary to support nested operations inside the proposal_create_operation
    */
   struct op_wrapper
   {
      public:
         op_wrapper(const operation& op = operation()):op(op){}
         operation op;
   };

} } // graphene::protocol

FC_REFLECT_TYPENAME( graphene::protocol::operation )
FC_REFLECT( graphene::protocol::op_wrapper, (op) )

GRAPHENE_DECLARE_EXTERNAL_SERIALIZATION( graphene::protocol::op_wrapper )
