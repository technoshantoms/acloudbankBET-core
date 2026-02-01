
#pragma once

#include <graphene/protocol/operations.hpp>
#include <graphene/chain/evaluator.hpp>
#include <graphene/chain/database.hpp>

namespace graphene { namespace chain {

   class betting_market_rules_create_evaluator : public fee_handling_evaluator<betting_market_rules_create_evaluator>
   {
      public:
         typedef betting_market_rules_create_operation operation_type;

         void_result do_evaluate( const betting_market_rules_create_operation& o );
         object_id_type do_apply( const betting_market_rules_create_operation& o );
   };

   class betting_market_rules_update_evaluator : public fee_handling_evaluator<betting_market_rules_update_evaluator>
   {
      public:
         typedef betting_market_rules_update_operation operation_type;

         void_result do_evaluate( const betting_market_rules_update_operation& o );
         void_result do_apply( const betting_market_rules_update_operation& o );
      private:
         const betting_market_rules_object* _rules;
   };

   class betting_market_group_create_evaluator : public fee_handling_evaluator<betting_market_group_create_evaluator>
   {
      public:
         typedef betting_market_group_create_operation operation_type;

         void_result do_evaluate(const betting_market_group_create_operation& o);
         object_id_type do_apply(const betting_market_group_create_operation& o);
      private:
         event_id_type _event_id;
         betting_market_rules_id_type _rules_id;
   };

   class betting_market_group_update_evaluator : public fee_handling_evaluator<betting_market_group_update_evaluator>
   {
      public:
         typedef betting_market_group_update_operation operation_type;

         void_result do_evaluate(const betting_market_group_update_operation& o);
         void_result do_apply(const betting_market_group_update_operation& o);
      private:
         betting_market_rules_id_type _rules_id;
         const betting_market_group_object* _betting_market_group;
   };

   class betting_market_create_evaluator : public fee_handling_evaluator<betting_market_create_evaluator>
   {
      public:
         typedef betting_market_create_operation operation_type;

         void_result do_evaluate( const betting_market_create_operation& o );
         object_id_type do_apply( const betting_market_create_operation& o );
      private:
         betting_market_group_id_type _group_id;
   };

   class betting_market_update_evaluator : public fee_handling_evaluator<betting_market_update_evaluator>
   {
      public:
         typedef betting_market_update_operation operation_type;

         void_result do_evaluate( const betting_market_update_operation& o );
         void_result do_apply( const betting_market_update_operation& o );
      private:
         const betting_market_object* _betting_market;
         betting_market_group_id_type _group_id;
   };

   class bet_place_evaluator : public fee_handling_evaluator<bet_place_evaluator>
   {
      public:
         typedef bet_place_operation operation_type;

         void_result do_evaluate( const bet_place_operation& o );
         object_id_type do_apply( const bet_place_operation& o );
      private:
         const betting_market_group_object* _betting_market_group;
         const betting_market_object* _betting_market;
         const chain_parameters* _current_params;
         const asset_object* _asset;
         share_type _stake_plus_fees;
   };

   class bet_cancel_evaluator : public fee_handling_evaluator<bet_cancel_evaluator>
   {
      public:
         typedef bet_cancel_operation operation_type;

         void_result do_evaluate( const bet_cancel_operation& o );
         void_result do_apply( const bet_cancel_operation& o );
      private:
         const bet_object* _bet_to_cancel;
   };

   class betting_market_group_resolve_evaluator : public fee_handling_evaluator<betting_market_group_resolve_evaluator>
   {
      public:
         typedef betting_market_group_resolve_operation operation_type;

         void_result do_evaluate( const betting_market_group_resolve_operation& o );
         void_result do_apply( const betting_market_group_resolve_operation& o );
      private:
         const betting_market_group_object* _betting_market_group;
   };

   class betting_market_group_cancel_unmatched_bets_evaluator : public fee_handling_evaluator<betting_market_group_cancel_unmatched_bets_evaluator>
   {
      public:
         typedef betting_market_group_cancel_unmatched_bets_operation operation_type;

         void_result do_evaluate( const betting_market_group_cancel_unmatched_bets_operation& o );
         void_result do_apply( const betting_market_group_cancel_unmatched_bets_operation& o );
      private:
         const betting_market_group_object* _betting_market_group;
   };


} } // graphene::chain
