#pragma once

#include <graphene/protocol/types.hpp>
#include <graphene/protocol/account.hpp>
#include <graphene/protocol/base.hpp>
#include <graphene/protocol/memo.hpp>

namespace graphene { namespace protocol {

   /**
    * Virtual op generated when an affiliate receives payout.
    */

   struct affiliate_payout_operation : public base_operation
   {
      affiliate_payout_operation(){}
      affiliate_payout_operation( account_id_type a, app_tag t, const asset& amount )
      : affiliate(a), tag(t), payout(amount) {}

      struct fee_parameters_type { };
      asset           fee;

      // Account of the receiving affiliate
      account_id_type affiliate;
      // App-tag for which the payout was generated
      app_tag         tag;
      // Payout amount
      asset           payout;

      account_id_type fee_payer()const { return affiliate; }
      void            validate()const {
         FC_ASSERT( false, "Virtual operation" );
      }

      share_type calculate_fee(const fee_parameters_type& params)const
      { return 0; }
   };

   /**
    * Virtual op generated when a player generates an affiliate payout
    */
   struct affiliate_referral_payout_operation : public base_operation
   {
      affiliate_referral_payout_operation(){}
      affiliate_referral_payout_operation( account_id_type p, const asset& amount )
      : player(p), payout(amount) {}

      struct fee_parameters_type { };
      asset           fee;

      // Account of the winning player
      account_id_type player;
      // Payout amount
      asset           payout;

      account_id_type fee_payer()const { return player; }
      void            validate()const {
         FC_ASSERT( false, "virtual operation" );
      }

      share_type calculate_fee(const fee_parameters_type& params)const
      { return 0; }
   };

} } // graphene::protocol

FC_REFLECT( graphene::protocol::affiliate_payout_operation::fee_parameters_type, )
FC_REFLECT( graphene::protocol::affiliate_referral_payout_operation::fee_parameters_type, )

FC_REFLECT( graphene::protocol::affiliate_payout_operation, (fee)(affiliate)(tag)(payout) )
FC_REFLECT( graphene::protocol::affiliate_referral_payout_operation, (fee)(player)(payout) )
