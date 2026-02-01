/*
 * AcloudBank
 *
 */
#include <graphene/protocol/ticket.hpp>

#include <fc/io/raw.hpp>

namespace graphene { namespace protocol {

void ticket_create_operation::validate()const
{
   FC_ASSERT( fee.amount >= 0, "Fee should not be negative" );
   FC_ASSERT( target_type != static_cast<uint64_t>(liquid), "Target type can not be liquid" );
   FC_ASSERT( target_type < static_cast<uint64_t>(TICKET_TYPE_COUNT), "Invalid target type" );
   FC_ASSERT( amount.amount > 0, "A positive amount is needed for creating a ticket" );
   FC_ASSERT( amount.asset_id == asset_id_type(), "Amount must be in RQRX so far" );
}

void ticket_update_operation::validate()const
{
   FC_ASSERT( fee.amount >= 0, "Fee should not be negative" );
   FC_ASSERT( target_type < static_cast<uint64_t>(TICKET_TYPE_COUNT), "Invalid target type" );
   if( amount_for_new_target.valid() )
   {
      FC_ASSERT( amount_for_new_target->amount > 0, "A positive amount is needed" );
      FC_ASSERT( amount_for_new_target->asset_id == asset_id_type(), "Amount must be in RQRX so far" );
   }
}

} } // graphene::protocol

GRAPHENE_IMPLEMENT_EXTERNAL_SERIALIZATION( graphene::protocol::ticket_create_operation::fee_parameters_type )
GRAPHENE_IMPLEMENT_EXTERNAL_SERIALIZATION( graphene::protocol::ticket_update_operation::fee_parameters_type )
GRAPHENE_IMPLEMENT_EXTERNAL_SERIALIZATION( graphene::protocol::ticket_create_operation )
GRAPHENE_IMPLEMENT_EXTERNAL_SERIALIZATION( graphene::protocol::ticket_update_operation )
