
#pragma once

#include <graphene/chain/types.hpp>

#include <graphene/db/object.hpp>
#include <graphene/db/generic_index.hpp>

namespace graphene { namespace chain {

class database;

class sport_object : public graphene::db::abstract_object< sport_object,protocol_ids, sport_object_type >
{
   public:
      internationalized_string_type name;
};

typedef multi_index_container<
   sport_object,
   indexed_by<
      ordered_unique< tag<by_id>, member< object, object_id_type, &object::id > > > > sport_object_multi_index_type;

typedef generic_index<sport_object, sport_object_multi_index_type> sport_object_index;
} } // graphene::chain

MAP_OBJECT_ID_TO_TYPE(graphene::chain::sport_object)

FC_REFLECT_DERIVED( graphene::chain::sport_object, (graphene::db::object), (name) )
