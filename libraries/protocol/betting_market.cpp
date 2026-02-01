
#include <graphene/protocol/betting_market.hpp>

namespace graphene { namespace protocol {

void betting_market_rules_create_operation::validate() const
{
   FC_ASSERT( fee.amount >= 0 );
}

void betting_market_rules_update_operation::validate() const
{
   FC_ASSERT( fee.amount >= 0 );
}

void betting_market_group_create_operation::validate() const
{
   FC_ASSERT( fee.amount >= 0 );
}

void betting_market_group_update_operation::validate() const
{
   FC_ASSERT( fee.amount >= 0 );
}

void betting_market_create_operation::validate() const
{
   FC_ASSERT( fee.amount >= 0 );
}

void betting_market_update_operation::validate() const
{
   FC_ASSERT( fee.amount >= 0 );
}

void betting_market_group_resolve_operation::validate() const
{
   FC_ASSERT( fee.amount >= 0 );
}

void betting_market_group_cancel_unmatched_bets_operation::validate() const
{
   FC_ASSERT( fee.amount >= 0 );
}

void bet_place_operation::validate() const
{
   FC_ASSERT( fee.amount >= 0 );
}

void bet_cancel_operation::validate() const
{
   FC_ASSERT( fee.amount >= 0 );
}



} } // graphene::protocol

