/*
 *AcloudBank
 */
#include <graphene/chain/commit_reveal_object.hpp>
#include <graphene/chain/database.hpp>

#include <fc/io/raw.hpp>
#include <fc/uint128.hpp>

namespace graphene { namespace chain {

} } // graphene::chain

FC_REFLECT_DERIVED_NO_TYPENAME( graphene::chain::commit_reveal_object,
                    (graphene::db::object),
                    (account)(hash)(value)(maintenance_time)
                    )

GRAPHENE_IMPLEMENT_EXTERNAL_SERIALIZATION( graphene::chain::commit_reveal_object )
