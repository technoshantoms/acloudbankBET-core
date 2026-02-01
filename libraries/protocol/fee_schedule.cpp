/*
 * Copyright (c) 2015 Cryptonomex, Inc., and contributors.
 * Copyright (c) 2020-2023 Revolution Populi Limited, and contributors.
 *
 * The MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#include <graphene/protocol/fee_schedule.hpp>

namespace graphene { namespace protocol {

   fee_schedule fee_schedule::get_default_impl()
   {
      fee_schedule result;
      const auto count = fee_parameters::count();
      result.parameters.reserve(count);
      for( size_t i = 0; i < count; ++i )
      {
         fee_parameters x;
         x.set_which(i);
         result.parameters.insert(x);
      }
      return result;
   }

   const fee_schedule& fee_schedule::get_default()
   {
      static const auto result = get_default_impl();
      return result;
   }

   struct zero_fee_visitor
   {
      using result_type = void;

      template<typename ParamType>
      result_type operator()(  ParamType& op )const
      {
         memset( (char*)&op, 0, sizeof(op) );
      }
   };

   void fee_schedule::zero_all_fees()
   {
      *this = get_default();
      for( fee_parameters& i : parameters )
         i.visit( zero_fee_visitor() );
      this->scale = 0;
   }
   /* void chain_parameters::validate()const
   {
      current_fees->validate();
      FC_ASSERT( reserve_percent_of_fee <= GRAPHENE_100_PERCENT );
      FC_ASSERT( network_percent_of_fee <= GRAPHENE_100_PERCENT );
      FC_ASSERT( lifetime_referrer_percent_of_fee <= GRAPHENE_100_PERCENT );
      FC_ASSERT( network_percent_of_fee + lifetime_referrer_percent_of_fee <= GRAPHENE_100_PERCENT );

      FC_ASSERT( block_interval >= GRAPHENE_MIN_BLOCK_INTERVAL );
      FC_ASSERT( block_interval <= GRAPHENE_MAX_BLOCK_INTERVAL );
      FC_ASSERT( block_interval > 0 );
      FC_ASSERT( maintenance_interval > block_interval,
                 "Maintenance interval must be longer than block interval" );
      FC_ASSERT( maintenance_interval % block_interval == 0,
                 "Maintenance interval must be a multiple of block interval" );
      FC_ASSERT( maximum_transaction_size >= GRAPHENE_MIN_TRANSACTION_SIZE_LIMIT,
                 "Transaction size limit is too low" );
      FC_ASSERT( maximum_block_size >= GRAPHENE_MIN_BLOCK_SIZE_LIMIT,
                 "Block size limit is too low" );
      FC_ASSERT( maximum_time_until_expiration > block_interval,
                 "Maximum transaction expiration time must be greater than a block interval" );
      FC_ASSERT( maximum_proposal_lifetime - committee_proposal_review_period > block_interval,
                 "Committee proposal review period must be less than the maximum proposal lifetime" );
   }*/

} } // graphene::protocol
