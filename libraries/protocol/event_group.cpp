
#include <graphene/protocol/event_group.hpp>

namespace graphene { namespace protocol {

void event_group_create_operation::validate() const
{
   FC_ASSERT( fee.amount >= 0 );
}

void event_group_update_operation::validate() const
{
   FC_ASSERT( fee.amount >= 0 );
}

void event_group_delete_operation::validate() const
{
    FC_ASSERT( fee.amount >= 0 );
}
    
} } // graphene::protocol

