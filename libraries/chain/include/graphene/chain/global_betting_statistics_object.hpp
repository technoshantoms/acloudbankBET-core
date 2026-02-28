
#pragma once

#include <graphene/chain/types.hpp>

#include <graphene/db/object.hpp>
#include <graphene/db/generic_index.hpp>

namespace graphene { namespace chain {

class database;

class global_betting_statistics_object : public graphene::db::abstract_object< global_betting_statistics_object,implementation_ids,impl_global_betting_statistics_object_type >
{
   public:
      uint32_t number_of_active_events = 0;
      map<asset_id_type, share_type> total_amount_staked;
};

typedef multi_index_container<
   global_betting_statistics_object,
   indexed_by<
     ordered_unique< tag<by_id>, member< object, object_id_type, &object::id > > > > global_betting_statistics_object_multi_index_type;
typedef generic_index<global_betting_statistics_object, global_betting_statistics_object_multi_index_type> global_betting_statistics_object_index;

} } // graphene::chain

MAP_OBJECT_ID_TO_TYPE(graphene::chain::global_betting_statistics_object)

FC_REFLECT_DERIVED( graphene::chain::global_betting_statistics_object, (graphene::db::object), (number_of_active_events)(total_amount_staked) )
