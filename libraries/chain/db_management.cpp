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

#include <graphene/chain/database.hpp>

#include <graphene/chain/chain_property_object.hpp>
#include <graphene/chain/witness_schedule_object.hpp>
#include <graphene/chain/special_authority_object.hpp>
#include <graphene/chain/operation_history_object.hpp>
#include <graphene/chain/nft_object.hpp>

#include <graphene/protocol/fee_schedule.hpp>

#include <fc/io/fstream.hpp>

#include <fstream>
#include <functional>
#include <iostream>
#include <queue>
#include <tuple>

namespace graphene { namespace chain {

database::database() :
   _random_number_generator(fc::ripemd160().data())
{
   initialize_indexes();
   initialize_evaluators();
}

database::~database()
{
   clear_pending();
}

void database::reindex( fc::path data_dir )
{ try {
   auto last_block = _block_id_to_block.last();
   if( !last_block ) {
      elog( "!no last block" );
      edump((last_block));
      return;
   }
   if( last_block->block_num() <= head_block_num()) return;

   ilog( "reindexing blockchain" );
   auto start = fc::time_point::now();
   const auto last_block_num = last_block->block_num();
   uint32_t undo_point = last_block_num < GRAPHENE_MAX_UNDO_HISTORY ? 0 : last_block_num - GRAPHENE_MAX_UNDO_HISTORY;

   ilog( "Replaying blocks, starting at ${next}...", ("next",head_block_num() + 1) );
   if( head_block_num() >= undo_point )
   {
      if( head_block_num() > 0 )
         _fork_db.start_block( *fetch_block_by_number( head_block_num() ) );
   }
   else
      _undo_db.disable();

   uint32_t skip = node_properties().skip_flags;

   size_t total_block_size = _block_id_to_block.total_block_size();
   const auto& gpo = get_global_properties();
   std::queue< std::tuple< size_t, signed_block, fc::future< void > > > blocks;
   uint32_t next_block_num = head_block_num() + 1;
   uint32_t i = next_block_num;
   while( next_block_num <= last_block_num || !blocks.empty() )
   {
      if( next_block_num <= last_block_num && blocks.size() < 20 )
      {
         const size_t processed_block_size = _block_id_to_block.blocks_current_position();
         fc::optional< signed_block > block = _block_id_to_block.fetch_by_number( next_block_num++ );
         if( block.valid() )
         {
            if( block->timestamp >= last_block->timestamp - gpo.parameters.maximum_time_until_expiration )
               skip &= ~skip_transaction_dupe_check;
            blocks.emplace( processed_block_size, std::move(*block), fc::future<void>() );
            std::get<2>(blocks.back()) = precompute_parallel( std::get<1>(blocks.back()), skip );
         }
         else
         {
            wlog( "Reindexing terminated due to gap:  Block ${i} does not exist!", ("i", i) );
            uint32_t dropped_count = 0;
            while( true )
            {
               fc::optional< block_id_type > last_id = _block_id_to_block.last_id();
               // this can trigger if we attempt to e.g. read a file that has block #2 but no block #1
               if( !last_id.valid() )
                  break;
               // we've caught up to the gap
               if( block_header::num_from_id( *last_id ) <= i )
                  break;
               _block_id_to_block.remove( *last_id );
               dropped_count++;
            }
            wlog( "Dropped ${n} blocks from after the gap", ("n", dropped_count) );
            next_block_num = last_block_num + 1; // don't load more blocks
         }
      }
      else
      {
         std::get<2>(blocks.front()).wait();
         const signed_block& block = std::get<1>(blocks.front());

         if( i % 10000 == 0 )
         {
            std::stringstream bysize;
            std::stringstream bynum;
            size_t current_pos = std::get<0>(blocks.front());
            if( current_pos > total_block_size )
               total_block_size = current_pos;
            bysize << std::fixed << std::setprecision(5) << double(current_pos) / total_block_size * 100;
            bynum << std::fixed << std::setprecision(5) << double(i)*100/last_block_num;
            ilog(
               "   [by size: ${size}%   ${processed} of ${total}]   [by num: ${num}%   ${i} of ${last}]",
               ("size", bysize.str())
               ("processed", current_pos)
               ("total", total_block_size)
               ("num", bynum.str())
               ("i", i)
               ("last", last_block_num)
            );
         }
         if( i == undo_point )
         {
            ilog( "Writing database to disk at block ${i}", ("i",i) );
            flush();
            ilog( "Done" );
         }
         if( i < undo_point )
            apply_block( block, skip );
         else
         {
            _undo_db.enable();
            push_block( block, skip );
         }
         blocks.pop();
         i++;
      }
   }
   _undo_db.enable();
   auto end = fc::time_point::now();
   ilog( "Done reindexing, elapsed time: ${t} sec", ("t",double((end-start).count())/1000000.0 ) );
} FC_CAPTURE_AND_RETHROW( (data_dir) ) }

void database::wipe(const fc::path& data_dir, bool include_blocks)
{
   ilog("Wiping database", ("include_blocks", include_blocks));
   if (_opened) {
     close();
   }
   object_database::wipe(data_dir);
   if( include_blocks )
      fc::remove_all( data_dir / "database" );
}

void database::open(
   const fc::path& data_dir,
   std::function<genesis_state_type()> genesis_loader,
   const std::string& db_version)
{
   try
   {
      bool wipe_object_db = false;
      if( !fc::exists( data_dir / "db_version" ) )
         wipe_object_db = true;
      else
      {
         std::string version_string;
         fc::read_file_contents( data_dir / "db_version", version_string );
         wipe_object_db = ( version_string != db_version );
      }
      if( wipe_object_db ) {
          ilog("Wiping object_database due to missing or wrong version");
          object_database::wipe( data_dir );
          std::ofstream version_file( (data_dir / "db_version").generic_string().c_str(),
                                      std::ios::out | std::ios::binary | std::ios::trunc );
          version_file.write( db_version.c_str(), db_version.size() );
          version_file.close();
      }

      object_database::open(data_dir);

      _block_id_to_block.open(data_dir / "database" / "block_num_to_block");

      if( !find(global_property_id_type()) )
         init_genesis(genesis_loader());
      else
      {
         _p_core_asset_obj = &get( asset_id_type() );
         _p_core_dynamic_data_obj = &get( asset_dynamic_data_id_type() );
         _p_global_prop_obj = &get( global_property_id_type() );
         _p_chain_property_obj = &get( chain_property_id_type() );
         _p_dyn_global_prop_obj = &get( dynamic_global_property_id_type() );
         _p_witness_schedule_obj = &get( witness_schedule_id_type() );
      }

      fc::optional<block_id_type> last_block = _block_id_to_block.last_id();
      if( last_block.valid() )
      {
         FC_ASSERT( *last_block >= head_block_id(),
                    "last block ID does not match current chain state",
                    ("last_block->id", last_block)("head_block_id",head_block_num()) );
         _block_id_to_block.set_replay_mode(true);
         reindex( data_dir );
         _block_id_to_block.set_replay_mode(false);
      }
      _opened = true;
   }
   FC_CAPTURE_LOG_AND_RETHROW( (data_dir) )
}
void database::force_slow_replays()
{
   ilog("enabling slow replays");
   _slow_replays = true;
}

void database::check_ending_lotteries()
{
   try {
      const auto& lotteries_idx = get_index_type<asset_index>().indices().get<active_lotteries>();
      for( auto checking_asset: lotteries_idx )
      {
         FC_ASSERT( checking_asset.is_lottery() );
         FC_ASSERT( checking_asset.lottery_options->is_active );
         FC_ASSERT( checking_asset.lottery_options->end_date != time_point_sec() );
         if( checking_asset.lottery_options->end_date > head_block_time() ) continue;
         checking_asset.end_lottery(*this);
      }
   } catch( ... ) {}
}

void database::check_ending_nft_lotteries()
{
   try {
      const auto &nft_lotteries_idx = get_index_type<nft_metadata_index>().indices().get<active_nft_lotteries>();
      for (auto checking_token : nft_lotteries_idx)
      {
         FC_ASSERT(checking_token.is_lottery());
         const auto &lottery_options = checking_token.lottery_data->lottery_options;
         FC_ASSERT(lottery_options.is_active);
         // Check the current supply of lottery tokens
         auto current_supply = checking_token.get_token_current_supply(*this);
         if ((lottery_options.ending_on_soldout && (current_supply == checking_token.max_supply)) ||
             (lottery_options.end_date != time_point_sec() && (lottery_options.end_date <= head_block_time())))
            checking_token.end_lottery(*this);
      }
   } catch( ... ) {}
}

void database::check_lottery_end_by_participants( asset_id_type asset_id )
{
   try {
      asset_object asset_to_check = asset_id( *this );
      auto asset_dyn_props = asset_to_check.dynamic_data( *this );
      FC_ASSERT( asset_dyn_props.current_supply == asset_to_check.options.max_supply );
      FC_ASSERT( asset_to_check.is_lottery() );
      FC_ASSERT( asset_to_check.lottery_options->ending_on_soldout );
      asset_to_check.end_lottery( *this );
   } catch( ... ) {}
}

void database::close(bool rewind)
{
   if (!_opened)
      return;
      
   // TODO:  Save pending tx's on close()
   clear_pending();

   // pop all of the blocks that we can given our undo history, this should
   // throw when there is no more undo history to pop
   if( rewind )
   {
      try
      {
         uint32_t cutoff = get_dynamic_global_properties().last_irreversible_block_num;

         ilog( "Rewinding from ${head} to ${cutoff}", ("head",head_block_num())("cutoff",cutoff) );
         while( head_block_num() > cutoff )
         {
            block_id_type popped_block_id = head_block_id();
            pop_block();
            _fork_db.remove(popped_block_id); // doesn't throw on missing
         }
      }
      catch ( const fc::exception& e )
      {
         wlog( "Database close unexpected exception: ${e}", ("e", e) );
      }
   }

   // Since pop_block() will move tx's in the popped blocks into pending,
   // we have to clear_pending() after we're done popping to get a clean
   // DB state (issue #336).
   clear_pending();

   object_database::flush();
   object_database::close();

   if( _block_id_to_block.is_open() )
      _block_id_to_block.close();

   _fork_db.reset();

   _opened = false;
}

} }
