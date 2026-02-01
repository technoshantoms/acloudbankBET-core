#include <graphene/protocol/sport.hpp>

namespace graphene { namespace protocol {

void sport_create_operation::validate() const
{
   FC_ASSERT( fee.amount >= 0 );
}

void sport_update_operation::validate() const
{
   FC_ASSERT( fee.amount >= 0 );
}

void sport_delete_operation::validate() const
{
    FC_ASSERT( fee.amount >= 0 );
}

} } // graphene::protocol

