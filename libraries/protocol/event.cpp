
#include <graphene/protocol/event.hpp>

namespace graphene { namespace protocol {

void event_create_operation::validate() const
{
   FC_ASSERT( fee.amount >= 0 );
}

void event_update_operation::validate() const
{
   FC_ASSERT( fee.amount >= 0 );
}

void event_update_status_operation::validate() const
{
   FC_ASSERT( fee.amount >= 0 );
}

} } // graphene::protocol

