
#include <graphene/protocol/tournament.hpp>
#include <fc/io/raw.hpp>

namespace graphene { namespace protocol {

void tournament_options::validate() const
{
   //FC_ASSERT( number_of_players >= 2 && (number_of_players & (number_of_players - 1)) == 0,
   //           "Number of players must be a power of two" );
}

share_type tournament_create_operation::calculate_fee(const fee_parameters_type& k)const
{
   return k.fee + calculate_data_fee( fc::raw::pack_size(*this), k.price_per_kbyte );
}

void  tournament_create_operation::validate()const
{
   FC_ASSERT( fee.amount >= 0 );
   options.validate();
}

share_type tournament_join_operation::calculate_fee(const fee_parameters_type& k)const
{
   return k.fee;
}

void  tournament_join_operation::validate()const
{
   FC_ASSERT( fee.amount >= 0 );
}

share_type tournament_leave_operation::calculate_fee(const fee_parameters_type& k)const
{
   return k.fee;
}

void  tournament_leave_operation::validate()const
{
   // todo FC_ASSERT( fee.amount >= 0 );
}


share_type game_move_operation::calculate_fee(const fee_parameters_type& k)const
{
   return k.fee;
}

void  game_move_operation::validate()const
{
}

} } // namespace graphene::protocol
