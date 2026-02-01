#pragma once
#include <graphene/chain/database.hpp>
#include <graphene/affiliate_stats/affiliate_stats_plugin.hpp>

namespace graphene { namespace affiliate_stats {
using namespace chain;

enum stats_object_type
{
   app_reward_object_type,
   referral_reward_object_type,
   STATS_OBJECT_TYPE_COUNT ///< Sentry value which contains the number of different object types
};

class app_reward_object : public graphene::db::abstract_object<app_reward_object>
{
public:
   static const uint8_t space_id = AFFILIATE_STATS_SPACE_ID;
   static const uint8_t type_id = app_reward_object_type;

   app_tag app;
   asset   total_payout;

   inline share_type    get_amount()const   { return total_payout.amount; }
   inline asset_id_type get_asset_id()const { return total_payout.asset_id; }
};

typedef object_id<AFFILIATE_STATS_SPACE_ID, app_reward_object_type> app_reward_id_type;

struct by_asset;
struct by_app_asset;
typedef multi_index_container<
   app_reward_object,
   indexed_by<
      ordered_unique<tag<by_id>, member<object, object_id_type, &object::id> >,
      ordered_non_unique<tag<by_asset>,
         composite_key<
            app_reward_object,
            const_mem_fun<app_reward_object, asset_id_type, &app_reward_object::get_asset_id>,
            const_mem_fun<app_reward_object, share_type,    &app_reward_object::get_amount> >,
         composite_key_compare<
            std::less<asset_id_type>,
            std::greater<share_type> >
      >,
      ordered_unique<tag<by_app_asset>,
         composite_key<
            app_reward_object,
            member<app_reward_object, app_tag, &app_reward_object::app>,
            const_mem_fun<app_reward_object, asset_id_type, &app_reward_object::get_asset_id> >
      > > > app_reward_multi_index_type;
typedef generic_index<app_reward_object, app_reward_multi_index_type> app_reward_index;

class referral_reward_object : public graphene::db::abstract_object<referral_reward_object>
{
public:
   static const uint8_t space_id = AFFILIATE_STATS_SPACE_ID;
   static const uint8_t type_id = referral_reward_object_type;

   account_id_type referral;
   asset           total_payout;

   inline share_type    get_amount()const   { return total_payout.amount; }
   inline asset_id_type get_asset_id()const { return total_payout.asset_id; }
};

typedef object_id<AFFILIATE_STATS_SPACE_ID, referral_reward_object_type> referral_reward_id_type;

struct by_referral_asset;
typedef multi_index_container<
   referral_reward_object,
   indexed_by<
      ordered_unique<tag<by_id>, member<object, object_id_type, &object::id> >,
      ordered_non_unique<tag<by_asset>,
         composite_key<
            referral_reward_object,
            const_mem_fun<referral_reward_object, asset_id_type, &referral_reward_object::get_asset_id>,
            const_mem_fun<referral_reward_object, share_type,    &referral_reward_object::get_amount> >,
         composite_key_compare<
            std::less<asset_id_type>,
            std::greater<share_type> >
      >,
      ordered_unique<tag<by_referral_asset>,
         composite_key<
            referral_reward_object,
            member<referral_reward_object, account_id_type, &referral_reward_object::referral>,
            const_mem_fun<referral_reward_object, asset_id_type, &referral_reward_object::get_asset_id> >
      > > > referral_reward_multi_index_type;
typedef generic_index<referral_reward_object, referral_reward_multi_index_type> referral_reward_index;

} } //graphene::affiliate_stats

FC_REFLECT_DERIVED( graphene::affiliate_stats::app_reward_object, (graphene::db::object), (app)(total_payout) )
FC_REFLECT_DERIVED( graphene::affiliate_stats::referral_reward_object, (graphene::db::object), (referral)(total_payout) )

