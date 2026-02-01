/*
 *AcloudBank
 */

#include <graphene/protocol/commit_reveal.hpp>

#include <fc/io/raw.hpp>

namespace graphene { namespace protocol {

share_type commit_create_operation::calculate_fee( const fee_parameters_type& k )const
{
   return 0;
}


void commit_create_operation::validate()const
{
   FC_ASSERT( fee.amount >= 0 );
}

share_type reveal_create_operation::calculate_fee( const fee_parameters_type& k )const
{
   return 0;
}


void reveal_create_operation::validate()const
{
   FC_ASSERT( fee.amount >= 0 );
}

} } // graphene::protocol

GRAPHENE_IMPLEMENT_EXTERNAL_SERIALIZATION( graphene::protocol::commit_create_operation::fee_parameters_type )
GRAPHENE_IMPLEMENT_EXTERNAL_SERIALIZATION( graphene::protocol::commit_create_operation )
GRAPHENE_IMPLEMENT_EXTERNAL_SERIALIZATION( graphene::protocol::reveal_create_operation::fee_parameters_type )
GRAPHENE_IMPLEMENT_EXTERNAL_SERIALIZATION( graphene::protocol::reveal_create_operation )
