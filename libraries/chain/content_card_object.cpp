

#include <graphene/chain/content_card_object.hpp>
#include <graphene/chain/database.hpp>

#include <fc/io/raw.hpp>
#include <fc/uint128.hpp>

namespace graphene { namespace chain {

} } // graphene::chain

FC_REFLECT_DERIVED_NO_TYPENAME( graphene::chain::content_card_object,
                    (graphene::db::object),
                    (subject_account)(hash)(url)(timestamp)(type)(description)(content_key)(storage_data)(room)
                    )

GRAPHENE_IMPLEMENT_EXTERNAL_SERIALIZATION( graphene::chain::content_card_object )
