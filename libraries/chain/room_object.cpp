#include <graphene/chain/room_object.hpp>
#include <graphene/chain/database.hpp>

#include <fc/io/raw.hpp>
#include <fc/uint128.hpp>

namespace graphene { namespace chain {

} } // graphene::chain

FC_REFLECT_DERIVED_NO_TYPENAME( graphene::chain::room_object,
                    (graphene::db::object),
                    (owner)(name)(room_key)(timestamp)
                    )

FC_REFLECT_DERIVED_NO_TYPENAME( graphene::chain::room_participant_object,
                    (graphene::db::object),
                    (room)(participant)(content_key)(timestamp)
                    )

GRAPHENE_IMPLEMENT_EXTERNAL_SERIALIZATION( graphene::chain::room_object )
GRAPHENE_IMPLEMENT_EXTERNAL_SERIALIZATION( graphene::chain::room_participant_object )
