/*
 * acloudbank
 */

#include <graphene/protocol/room.hpp>
#include <fc/io/raw.hpp>

namespace graphene { namespace protocol {

// ============ room_create_operation ============

share_type room_create_operation::calculate_fee( const fee_parameters_type& k )const
{
   return k.fee;
}

void room_create_operation::validate()const
{
   FC_ASSERT( fee.amount >= 0 );
   FC_ASSERT( !name.empty(), "Room name cannot be empty" );
   FC_ASSERT( name.size() <= 256, "Room name too long (max 256 characters)" );
   FC_ASSERT( !room_key.empty(), "Room key cannot be empty" );
}

// ============ room_update_operation ============

share_type room_update_operation::calculate_fee( const fee_parameters_type& k )const
{
   return k.fee;
}

void room_update_operation::validate()const
{
   FC_ASSERT( fee.amount >= 0 );
   FC_ASSERT( !name.empty(), "Room name cannot be empty" );
   FC_ASSERT( name.size() <= 256, "Room name too long (max 256 characters)" );
}

// ============ room_add_participant_operation ============

share_type room_add_participant_operation::calculate_fee( const fee_parameters_type& k )const
{
   return k.fee;
}

void room_add_participant_operation::validate()const
{
   FC_ASSERT( fee.amount >= 0 );
   FC_ASSERT( !content_key.empty(), "Content key cannot be empty" );
}

// ============ room_remove_participant_operation ============

share_type room_remove_participant_operation::calculate_fee( const fee_parameters_type& k )const
{
   return k.fee;
}

void room_remove_participant_operation::validate()const
{
   FC_ASSERT( fee.amount >= 0 );
}

} } // graphene::protocol

GRAPHENE_IMPLEMENT_EXTERNAL_SERIALIZATION( graphene::protocol::room_create_operation::fee_parameters_type )
GRAPHENE_IMPLEMENT_EXTERNAL_SERIALIZATION( graphene::protocol::room_create_operation )
GRAPHENE_IMPLEMENT_EXTERNAL_SERIALIZATION( graphene::protocol::room_update_operation::fee_parameters_type )
GRAPHENE_IMPLEMENT_EXTERNAL_SERIALIZATION( graphene::protocol::room_update_operation )
GRAPHENE_IMPLEMENT_EXTERNAL_SERIALIZATION( graphene::protocol::room_add_participant_operation::fee_parameters_type )
GRAPHENE_IMPLEMENT_EXTERNAL_SERIALIZATION( graphene::protocol::room_add_participant_operation )
GRAPHENE_IMPLEMENT_EXTERNAL_SERIALIZATION( graphene::protocol::room_remove_participant_operation::fee_parameters_type )
GRAPHENE_IMPLEMENT_EXTERNAL_SERIALIZATION( graphene::protocol::room_remove_participant_operation )
