
#pragma once

#include <graphene/chain/types.hpp>

#include <graphene/db/object.hpp>
#include <graphene/db/generic_index.hpp>

#include <boost/multi_index/composite_key.hpp>

namespace graphene { namespace chain {

class database;

struct by_sport_id;

class event_group_object : public graphene::db::abstract_object< event_group_object,protocol_ids,event_group_object_type >
{
   public:
      internationalized_string_type name;
      sport_id_type sport_id;
    
      void cancel_events(database& db) const;
};

typedef multi_index_container<
   event_group_object,
   indexed_by<
      ordered_unique< tag<by_id>, member< object, object_id_type, &object::id > >, 
      ordered_unique< tag<by_sport_id>, composite_key<event_group_object,
                                                      member< event_group_object, sport_id_type, &event_group_object::sport_id >,
                                                      member<object, object_id_type, &object::id> > > >
   > event_group_object_multi_index_type;

typedef generic_index<event_group_object, event_group_object_multi_index_type> event_group_object_index;
} } // graphene::chain

MAP_OBJECT_ID_TO_TYPE(graphene::chain::event_group_object)

FC_REFLECT_DERIVED( graphene::chain::event_group_object, (graphene::db::object), (name)(sport_id) )
