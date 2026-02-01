
#pragma once

#include <graphene/protocol/asset.hpp>
#include <graphene/protocol/affiliate.hpp>
#include <graphene/chain/betting_market_object.hpp>
#include <graphene/chain/tournament_object.hpp>

namespace graphene { namespace chain {
   class database;

   namespace impl {
      class game_type_visitor {
      public:
         typedef app_tag result_type;

         inline app_tag operator()( const rock_paper_scissors_game_options& o )const { return rps; }
      };
   }

   template<typename GAME>
   app_tag get_tag_for_game( const GAME& game );

   template<>
   inline app_tag get_tag_for_game( const betting_market_group_object& game )
   {
      return bookie;
   }

   template<>
   inline app_tag get_tag_for_game( const tournament_object& game )
   {
      return game.options.game_options.visit( impl::game_type_visitor() );
   }

   template<typename GAME>
   asset_id_type get_asset_for_game( const GAME& game );

   template<>
   inline asset_id_type get_asset_for_game( const betting_market_group_object& game )
   {
      return game.asset_id;
   }

   template<>
   inline asset_id_type get_asset_for_game( const tournament_object& game )
   {
      return game.options.buy_in.asset_id;
   }

   class affiliate_payout_helper {
   public:
      template<typename GAME>
      affiliate_payout_helper( database& db, const GAME& game )
      : _db(db), tag( get_tag_for_game( game ) ), payout_asset( get_asset_for_game( game ) ) {}

      share_type payout( account_id_type player, share_type amount );
      share_type payout( const account_object& player, share_type amount );
      void commit();

   private:
      database&                             _db;
      app_tag                               tag;
      asset_id_type                         payout_asset;
      std::map<account_id_type, share_type> accumulator;
   };

} } // graphene::chain
