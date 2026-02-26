/*
 * Copyright (c) 2017 Cryptonomex, Inc., and contributors.
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
//std::string operator()(const asset_dividend_distribution_operation& op)const;
#include <algorithm>
#include <cctype>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <sstream>
#include <string>
#include <list>

#include <boost/version.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/replace.hpp>

#include <boost/range/adaptor/map.hpp>
#include <boost/range/algorithm_ext/erase.hpp>
#include <boost/range/algorithm/unique.hpp>
#include <boost/range/algorithm/sort.hpp>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/random_access_index.hpp>
#include <boost/multi_index/tag.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index/hashed_index.hpp>

#include <fc/git_revision.hpp>
#include <fc/io/fstream.hpp>
#include <fc/io/json.hpp>
#include <fc/io/stdio.hpp>
#include <fc/network/http/websocket.hpp>
#include <fc/rpc/cli.hpp>
#include <fc/rpc/websocket_api.hpp>
#include <fc/crypto/hex.hpp>
#include <fc/thread/mutex.hpp>
#include <fc/thread/scoped_lock.hpp>
#include <fc/crypto/base58.hpp>
#include <fc/crypto/aes.hpp>
#include <fc/crypto/rand.hpp>
#include <graphene/protocol/rock_paper_scissors.hpp>
#include <graphene/chain/rock_paper_scissors.hpp>

#include <graphene/bookie/bookie_api.hpp>
//#include <graphene/protocol/sidechain_defs.hpp>

#include <graphene/app/api.hpp>
#include <graphene/app/util.hpp>
#include <graphene/chain/asset_object.hpp>
#include <graphene/protocol/fee_schedule.hpp>
#include <graphene/protocol/pts_address.hpp>
#include <graphene/chain/hardfork.hpp>
#include <graphene/utilities/git_revision.hpp>
#include <graphene/utilities/key_conversion.hpp>
#include <graphene/utilities/words.hpp>
#include <graphene/wallet/wallet.hpp>
#include <graphene/wallet/api_documentation.hpp>
#include "wallet_api_impl.hpp"
#include <graphene/debug_witness/debug_api.hpp>

#include "operation_printer.hpp"
#include <graphene/wallet/reflect_util.hpp>

#define BRAIN_KEY_WORD_COUNT 16
#define RANGE_PROOF_MANTISSA 49 // Minimum mantissa bits to "hide" in the range proof.
                                // If this number is set too low, then for large value
                                // commitments the length of the range proof will hint
                                // strongly at the value amount that is being hidden.
namespace graphene { namespace wallet {
   using fc::optional;
   fc::sha256 signed_message::digest()const
   {
      fc::stringstream to_sign;
      to_sign << message << '\n';
      to_sign << "account=" << meta.account << '\n';
      to_sign << "memokey=" << std::string( meta.memo_key ) << '\n';
      to_sign << "block=" << meta.block << '\n';
      to_sign << "timestamp=" << meta.time;

      return fc::sha256::hash( to_sign.str() );
   }
   vector<brain_key_info> utility::derive_owner_keys_from_brain_key(string brain_key, int number_of_desired_keys)
   {
      // Safety-check
      FC_ASSERT( number_of_desired_keys >= 1 );

      // Create as many derived owner keys as requested
      vector<brain_key_info> results;
      for (int i = 0; i < number_of_desired_keys; ++i) {
        fc::ecc::private_key priv_key = graphene::wallet::detail::derive_private_key( brain_key, i );

        brain_key_info result;
        result.brain_priv_key = brain_key;
        result.wif_priv_key = key_to_wif( priv_key );
        result.pub_key = priv_key.get_public_key();

        results.push_back(result);
      }

      return results;
   }

   brain_key_info utility::suggest_brain_key()
   {
      brain_key_info result;
      // create a private key for secure entropy
      fc::sha256 sha_entropy1 = fc::ecc::private_key::generate().get_secret();
      fc::sha256 sha_entropy2 = fc::ecc::private_key::generate().get_secret();
      fc::bigint entropy1(sha_entropy1.data(), sha_entropy1.data_size());
      fc::bigint entropy2(sha_entropy2.data(), sha_entropy2.data_size());
      fc::bigint entropy(entropy1);
      entropy <<= 8 * sha_entropy1.data_size();
      entropy += entropy2;
      string brain_key = "";

      for (int i = 0; i < BRAIN_KEY_WORD_COUNT; i++)
      {
         fc::bigint choice = entropy % graphene::words::word_list_size;
         entropy /= graphene::words::word_list_size;
         if (i > 0)
            brain_key += " ";
         brain_key += graphene::words::word_list[choice.to_int64()];
      }

      brain_key = detail::normalize_brain_key(brain_key);
      fc::ecc::private_key priv_key = detail::derive_private_key(brain_key, 0);
      result.brain_priv_key = brain_key;
      result.wif_priv_key = key_to_wif(priv_key);
      result.pub_key = priv_key.get_public_key();
      return result;
   }
}}

namespace graphene { namespace wallet {

wallet_api::wallet_api(const wallet_data& initial_data, fc::api<login_api> rapi)
   : my( std::make_unique<detail::wallet_api_impl>(*this, initial_data, rapi) )
{
}

wallet_api::~wallet_api()
{
}

bool wallet_api::copy_wallet_file(string destination_filename)
{
   return my->copy_wallet_file(destination_filename);
}

optional<signed_block_with_info> wallet_api::get_block(uint32_t num)
{
   return my->_remote_db->get_block(num);
}

uint64_t wallet_api::get_account_count() const
{
   return my->_remote_db->get_account_count();
}

vector<account_object> wallet_api::list_my_accounts()
{
   return vector<account_object>(my->_wallet.my_accounts.begin(), my->_wallet.my_accounts.end());
}

map<string,account_id_type> wallet_api::list_accounts(const string& lowerbound, uint32_t limit)
{
   return my->_remote_db->lookup_accounts(lowerbound, limit, {});
}

vector<asset> wallet_api::list_account_balances(const string& id)
{
   return my->_remote_db->get_account_balances(id, flat_set<asset_id_type>());
}

vector<extended_asset_object> wallet_api::list_assets(const string& lowerbound, uint32_t limit)const
{
   return my->_remote_db->list_assets( lowerbound, limit );
}

uint64_t wallet_api::get_asset_count()const
{
   return my->_remote_db->get_asset_count();
}

vector<asset_object> wallet_api::get_lotteries( asset_id_type stop,
                                                unsigned limit,
                                                asset_id_type start )const
{
   return my->_remote_db->get_lotteries( stop, limit, start );
}

vector<asset_object> wallet_api::get_account_lotteries(  account_id_type issuer,
                                                         asset_id_type stop,
                                                         unsigned limit,
                                                         asset_id_type start )const
{
   return my->_remote_db->get_account_lotteries( issuer, stop, limit, start );
}

asset wallet_api::get_lottery_balance( asset_id_type lottery_id )const
{
   return my->_remote_db->get_lottery_balance( lottery_id );
}
signed_transaction wallet_api::htlc_create( string source, string destination, string amount, string asset_symbol,
         string hash_algorithm, const std::string& preimage_hash, uint32_t preimage_size,
         const uint32_t claim_period_seconds, const std::string& memo, bool broadcast)
{
   return my->htlc_create(source, destination, amount, asset_symbol, hash_algorithm, preimage_hash, preimage_size,
         claim_period_seconds, memo, broadcast);
}

fc::optional<fc::variant> wallet_api::get_htlc(std::string htlc_id) const
{
   fc::optional<htlc_object> optional_obj = my->get_htlc(htlc_id);
   if ( optional_obj.valid() )
   {
      const htlc_object& obj = *optional_obj;
      // convert to formatted variant
      fc::mutable_variant_object transfer;
      const auto& from = my->get_account( obj.transfer.from );
      transfer["from"] = from.name;
      const auto& to = my->get_account( obj.transfer.to );
      transfer["to"] = to.name;
      const auto& asset = my->get_asset( obj.transfer.asset_id );
      transfer["asset"] = asset.symbol;
      transfer["amount"] = graphene::app::uint128_amount_to_string( obj.transfer.amount.value, asset.precision );
      if (obj.memo.valid())
         transfer["memo"] = my->read_memo( *obj.memo );
      
      class htlc_hash_to_variant_visitor
      {
         public:
         typedef fc::mutable_variant_object result_type;

         result_type operator()(const fc::ripemd160& obj)const
         { return convert("RIPEMD160", obj.str()); }
         result_type operator()(const fc::sha1& obj)const
         { return convert("SHA1", obj.str()); }
         result_type operator()(const fc::sha256& obj)const
         { return convert("SHA256", obj.str()); }
         result_type operator()(const fc::hash160& obj)const
         { return convert("HASH160", obj.str()); }
         private:
         result_type convert(const std::string& type, const std::string& hash)const
         {
            fc::mutable_variant_object ret_val;
            ret_val["hash_algo"] = type;
            ret_val["preimage_hash"] = hash;
            return ret_val;
         }
      };
      static htlc_hash_to_variant_visitor hash_visitor;
      fc::mutable_variant_object htlc_lock = obj.conditions.hash_lock.preimage_hash.visit(hash_visitor);
      htlc_lock["preimage_size"] = obj.conditions.hash_lock.preimage_size;
      fc::mutable_variant_object time_lock;
      time_lock["expiration"] = obj.conditions.time_lock.expiration;
      time_lock["time_left"] = fc::get_approximate_relative_time_string(obj.conditions.time_lock.expiration);
      fc::mutable_variant_object conditions;
      conditions["htlc_lock"] = htlc_lock;
      conditions["time_lock"] = time_lock;
      fc::mutable_variant_object result;
      result["transfer"] = transfer;
      result["conditions"] = conditions;
      return fc::optional<fc::variant>(result);
   }
   return fc::optional<fc::variant>();
}

signed_transaction wallet_api::htlc_redeem( std::string htlc_id, std::string issuer, const std::string& preimage,
      bool broadcast)
{

   return my->htlc_redeem(htlc_id, issuer, std::vector<char>(preimage.begin(), preimage.end()), broadcast);
}

signed_transaction wallet_api::htlc_extend ( std::string htlc_id, std::string issuer, const uint32_t seconds_to_add,
      bool broadcast)
{
   return my->htlc_extend(htlc_id, issuer, seconds_to_add, broadcast);
}

vector<operation_detail> wallet_api::get_account_history(const string& name, uint32_t limit)const
{
   vector<operation_detail> result;

   while( limit > 0 )
   {
      bool skip_first_row = false;
      operation_history_id_type start;
      if( result.size() )
      {
         start = result.back().op.id;
         if( start == operation_history_id_type() ) // no more data
            break;
         start = start + (-1);
         if( start == operation_history_id_type() ) // will return most recent history if
                                                    // directly call remote API with this
         {
            start = start + 1;
            skip_first_row = true;
         }
      }

      uint32_t default_page_size = 100;
      uint32_t page_limit = skip_first_row ? std::min<uint32_t>( default_page_size, limit + 1 )
                                           : std::min<uint32_t>( default_page_size, limit );

      vector<operation_history_object> current = my->_remote_hist->get_account_history(
            name,
            operation_history_id_type(),
            page_limit,
            start );
      bool first_row = true;
      for( auto& o : current )
      {
         if( first_row )
         {
            first_row = false;
            if( skip_first_row )
            {
               continue;
            }
         }
         std::stringstream ss;
         auto memo = o.op.visit(detail::operation_printer(ss, *my, o));
         result.push_back( operation_detail{ memo, ss.str(), o } );
      }

      if( current.size() < page_limit )
         break;

      limit -= current.size();
      if( skip_first_row )
         ++limit;
   }

   return result;
}

 signed_transaction withdraw_GPOS_vesting_balance(
      string account_name,
      string amount,
      string asset_symbol,
      bool broadcast = false)
   { try {

      //Can be deleted after GPOS hardfork time
      time_point_sec now = time_point::now();
      if(now < HARDFORK_GPOS_TIME)
         FC_THROW("GPOS related functionality is not avaiable until next Spring");

      asset_object asset_obj = get_asset( asset_symbol );
      vector< vesting_balance_object > vbos;
      vesting_balance_object vbo;
      fc::optional<vesting_balance_id_type> vbid = maybe_id<vesting_balance_id_type>(account_name);
      if( !vbid )
      {
         vbos = _remote_db->get_vesting_balances( account_name );
         if( vbos.size() == 0 )
            FC_THROW("Account ${account} has no core TOKEN vested and thus its not allowed to withdraw.", ("account", account_name));
      }

      //whether it is a witness or user, keep it in a container and iterate over to process all vesting balances and types
      if(!vbos.size())
         vbos.emplace_back( get_object(*vbid) );

      for (const vesting_balance_object& vesting_balance_obj: vbos) {
         if(vesting_balance_obj.balance_type == vesting_balance_type::gpos)
         {
            vbo = vesting_balance_obj;
            break;
         }
      }

      vesting_balance_withdraw_operation vesting_balance_withdraw_op;

      vesting_balance_withdraw_op.vesting_balance = vbo.id;
      vesting_balance_withdraw_op.owner = vbo.owner;
      vesting_balance_withdraw_op.amount = asset_obj.amount_from_string(amount);

      signed_transaction tx;
      tx.operations.push_back( vesting_balance_withdraw_op );
      set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees );
      tx.validate();

      return sign_transaction( tx, broadcast );
   } FC_CAPTURE_AND_RETHROW( (account_name)(amount) )}

   signed_transaction create_vesting_balance(string owner_account,
                                     string amount,
                                     string asset_symbol,
                                     vesting_balance_type vesting_type,
                                     bool broadcast /* = false */)
   { try {
      FC_ASSERT( !is_locked() );
      account_object user_account = get_account(owner_account);
      fc::optional<asset_object> asset_obj = get_asset(asset_symbol);
      FC_ASSERT(asset_obj, "Invalid asset symbol {asst}", ("asst", asset_symbol));

      vesting_balance_create_operation op;
      op.creator = user_account.get_id();
      op.owner = user_account.get_id();
      op.amount = asset_obj->amount_from_string(amount);
      op.balance_type = vesting_type;
      if (op.balance_type == vesting_balance_type::son)
          op.policy = dormant_vesting_policy_initializer {};

      signed_transaction tx;
      tx.operations.push_back( op );
      set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees);
      tx.validate();

      return sign_transaction( tx, broadcast );
   } FC_CAPTURE_AND_RETHROW( (owner_account)(broadcast) ) }

vector<operation_detail> wallet_api::get_relative_account_history(
      const string& name,
      uint32_t stop,
      uint32_t limit,
      uint32_t start)const
{
   vector<operation_detail> result;
   auto account_id = get_account(name).get_id();

   const account_object& account = my->get_account(account_id);
   const account_statistics_object& stats = my->get_object(account.statistics);

   if(start == 0)
       start = stats.total_ops;
   else
      start = std::min<uint32_t>(start, stats.total_ops);

   uint32_t default_page_size = 100;
   while( limit > 0 )
   {
      uint32_t page_size = std::min<uint32_t>(default_page_size, limit);
      vector <operation_history_object> current = my->_remote_hist->get_relative_account_history(
            name,
            stop,
            page_size,
            start);
      for (auto &o : current) {
         std::stringstream ss;
         auto memo = o.op.visit(detail::operation_printer(ss, *my, o));
         result.push_back(operation_detail{memo, ss.str(), o});
      }
      if (current.size() < page_size)
         break;
      limit -= page_size;
      start -= page_size;
      if( start == 0 ) break;
   }
   return result;
}

account_history_operation_detail wallet_api::get_account_history_by_operations(
      const string& name,
      const flat_set<uint16_t>& operation_types,
      uint32_t start,
      uint32_t limit)
{
    account_history_operation_detail result;

    const auto& account = my->get_account(name);
    const auto& stats = my->get_object(account.statistics);

    // sequence of account_transaction_history_object start with 1
    start = start == 0 ? 1 : start;

    if (start <= stats.removed_ops) {
        start = stats.removed_ops;
        result.total_count =stats.removed_ops;
    }

    uint32_t default_page_size = 100;
    while (limit > 0 && start <= stats.total_ops) {
        uint32_t min_limit = std::min(default_page_size, limit);
        auto current = my->_remote_hist->get_account_history_by_operations(name, operation_types, start, min_limit);
        auto his_rend = current.operation_history_objs.rend();
        for( auto it = current.operation_history_objs.rbegin(); it != his_rend; ++it )
        {
            auto& obj = *it;
            std::stringstream ss;
            auto memo = obj.op.visit(detail::operation_printer(ss, *my, obj));

            transaction_id_type transaction_id;
            auto block = get_block(obj.block_num);
            if (block.valid() && obj.trx_in_block < block->transaction_ids.size()) {
                transaction_id = block->transaction_ids[obj.trx_in_block];
            }
            result.details.push_back(operation_detail_ex{memo, ss.str(), obj, transaction_id});
        }
        result.result_count += current.operation_history_objs.size();
        result.total_count += current.total_count;

        start += current.total_count > 0 ? current.total_count : min_limit;
        limit -= current.operation_history_objs.size();
    }

    return result;
}

full_account wallet_api::get_full_account( const string& name_or_id)
{
    return my->_remote_db->get_full_accounts({name_or_id}, false)[name_or_id];
}

vector<account_balance_object> wallet_api::list_core_accounts()const
{
   return my->_remote_hist->list_core_accounts();
}

vector<bucket_object> wallet_api::get_market_history(
      string symbol1,
      string symbol2,
      uint32_t bucket,
      fc::time_point_sec start,
      fc::time_point_sec end )const
{
   return my->_remote_hist->get_market_history( symbol1, symbol2, bucket, start, end );
}

vector<limit_order_object> wallet_api::get_account_limit_orders(
      const string& name_or_id,
      const string &base,
      const string &quote,
      uint32_t limit,
      optional<limit_order_id_type> ostart_id,
      optional<price> ostart_price)
{
   return my->_remote_db->get_account_limit_orders(name_or_id, base, quote, limit, ostart_id, ostart_price);
}

vector<limit_order_object> wallet_api::get_limit_orders(std::string a, std::string b, uint32_t limit)const
{
   return my->_remote_db->get_limit_orders(a, b, limit);
}

vector<call_order_object> wallet_api::get_call_orders(std::string a, uint32_t limit)const
{
   return my->_remote_db->get_call_orders(a, limit);
}

vector<force_settlement_object> wallet_api::get_settle_orders(std::string a, uint32_t limit)const
{
   return my->_remote_db->get_settle_orders(a, limit);
}

brain_key_info wallet_api::suggest_brain_key()const
{
   return graphene::wallet::utility::suggest_brain_key();
}

vector<brain_key_info> wallet_api::derive_owner_keys_from_brain_key(
      string brain_key,
      int number_of_desired_keys) const
{
   return graphene::wallet::utility::derive_owner_keys_from_brain_key(brain_key, number_of_desired_keys);
}

bool wallet_api::is_public_key_registered(string public_key) const
{
   bool is_known = my->_remote_db->is_public_key_registered(public_key);
   return is_known;
}


string wallet_api::serialize_transaction( signed_transaction tx )const
{
   return fc::to_hex(fc::raw::pack(tx));
}

variant wallet_api::get_object( object_id_type id ) const
{
   return my->_remote_db->get_objects({id}, {});
}

string wallet_api::get_wallet_filename() const
{
   return my->get_wallet_filename();
}

transaction_handle_type wallet_api::begin_builder_transaction()
{
   return my->begin_builder_transaction();
}

void wallet_api::add_operation_to_builder_transaction(
      transaction_handle_type transaction_handle,
      const operation& op)
{
   my->add_operation_to_builder_transaction(transaction_handle, op);
}

void wallet_api::replace_operation_in_builder_transaction(
      transaction_handle_type handle,
      unsigned operation_index,
      const operation& new_op)
{
   my->replace_operation_in_builder_transaction(handle, operation_index, new_op);
}

asset wallet_api::set_fees_on_builder_transaction(transaction_handle_type handle, string fee_asset)
{
   return my->set_fees_on_builder_transaction(handle, fee_asset);
}

transaction wallet_api::preview_builder_transaction(transaction_handle_type handle)
{
   return my->preview_builder_transaction(handle);
}

signed_transaction wallet_api::sign_builder_transaction(transaction_handle_type transaction_handle,
                                                        const vector<public_key_type>& explicit_keys,
                                                        bool broadcast)
{
   return my->sign_builder_transaction(transaction_handle, explicit_keys, broadcast);
}

pair<transaction_id_type,signed_transaction> wallet_api::broadcast_transaction(signed_transaction tx)
{
    return my->broadcast_transaction(tx);
}

signed_transaction wallet_api::propose_builder_transaction(
      transaction_handle_type handle,
      string account_name_or_id,
      time_point_sec expiration,
      uint32_t review_period_seconds,
      bool broadcast)
{
   return my->propose_builder_transaction(handle, account_name_or_id, expiration, review_period_seconds, broadcast);
}

void wallet_api::remove_builder_transaction(transaction_handle_type handle)
{
   return my->remove_builder_transaction(handle);
}

account_object wallet_api::get_account(string account_name_or_id) const
{
   return my->get_account(account_name_or_id);
}

extended_asset_object wallet_api::get_asset(string asset_name_or_id) const
{
   auto found_asset = my->find_asset(asset_name_or_id);
   FC_ASSERT( found_asset, "Unable to find asset '${a}'", ("a",asset_name_or_id) );
   return *found_asset;
}

asset_bitasset_data_object wallet_api::get_bitasset_data(string asset_name_or_id) const
{
   auto asset = get_asset(asset_name_or_id);
   FC_ASSERT(asset.is_market_issued() && asset.bitasset_data_id);
   return my->get_object(*asset.bitasset_data_id);
}

account_id_type wallet_api::get_account_id(string account_name_or_id) const
{
   return my->get_account_id(account_name_or_id);
}

asset_id_type wallet_api::get_asset_id(const string& asset_symbol_or_id) const
{
   return my->get_asset_id(asset_symbol_or_id);
}

bool wallet_api::import_key(string account_name_or_id, string wif_key)
{
   FC_ASSERT(!is_locked());
   // backup wallet
   fc::optional<fc::ecc::private_key> optional_private_key = wif_to_key(wif_key);
   if (!optional_private_key)
      FC_THROW("Invalid private key");
   string shorthash = detail::address_to_shorthash(optional_private_key->get_public_key());
   copy_wallet_file( "before-import-key-" + shorthash );

   if( my->import_key(account_name_or_id, wif_key) )
   {
      save_wallet_file();
      copy_wallet_file( "after-import-key-" + shorthash );
      return true;
   }
   return false;
}

string wallet_api::normalize_brain_key(string s) const
{
   return detail::normalize_brain_key( s );
}

variant wallet_api::info()
{
   return my->info();
}

variant_object wallet_api::about() const
{
    return my->about();
}

fc::ecc::private_key wallet_api::derive_private_key(const std::string& prefix_string, int sequence_number) const
{
   return detail::derive_private_key( prefix_string, sequence_number );
}

signed_transaction wallet_api::register_account(string name,
                                                public_key_type owner_pubkey,
                                                public_key_type active_pubkey,
                                                string  registrar_account,
                                                string  referrer_account,
                                                uint32_t referrer_percent,
                                                bool broadcast)
{
   return my->register_account( name, owner_pubkey, active_pubkey,
                                registrar_account, referrer_account, referrer_percent, broadcast );
}
signed_transaction wallet_api::create_account_with_brain_key(string brain_key, string account_name,
                                                             string registrar_account, string referrer_account,
                                                             bool broadcast /* = false */)
{
   return my->create_account_with_brain_key(
            brain_key, account_name, registrar_account,
            referrer_account, broadcast
            );
}
signed_transaction wallet_api::issue_asset(string to_account, string amount, string symbol,
                                           string memo, bool broadcast)
{
   return my->issue_asset(to_account, amount, symbol, memo, broadcast);
}

signed_transaction update_account_keys(string name,
                                          public_key_type old_owner,
                                          public_key_type new_owner,
                                          public_key_type old_active,
                                          public_key_type new_active,
                                          bool broadcast)
   { try {
      FC_ASSERT( !self.is_locked() );
      account_object account_obj = get_account(name);

      authority owner = account_obj.owner;
      owner.key_auths[new_owner] = owner.key_auths[old_owner];
      owner.key_auths.erase(old_owner);

      authority active = account_obj.active;
      active.key_auths[new_active] = active.key_auths[old_active];
      active.key_auths.erase(old_active);

      signed_transaction tx;
      account_update_operation op;

      op.account = account_obj.get_id();
      op.owner = owner;
      op.active = active;

      ilog("account_update_operation: ${op}", ("op", op));

      tx.operations = {op};
      set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees );
      tx.validate();

      return sign_transaction( tx, broadcast );
   } FC_CAPTURE_AND_RETHROW( (name) ) }


   signed_transaction update_account_auth_key(string account_name,
                                              authority_type type,
                                              public_key_type key,
                                              weight_type weight,
                                              bool broadcast)
   {
      FC_ASSERT( !is_locked() );
      account_object account_obj = get_account(account_name);

      account_update_operation op;
      op.account = account_obj.get_id();

      authority new_auth;

      switch( type )
      {
         case( owner ):
            new_auth = account_obj.owner;
            break;
         case( active ):
            new_auth = account_obj.active;
            break;
      }

      if( weight == 0 ) // Remove the key
      {
         new_auth.key_auths.erase( key );
      }
      else
      {
         new_auth.add_authority( key, weight );
      }

      if( new_auth.is_impossible() )
      {
         if ( type == owner )
         {
            FC_ASSERT( false, "Owner authority change would render account irrecoverable." );
         }

         wlog( "Authority is now impossible." );
      }

      switch( type )
      {
         case( owner ):
            op.owner = new_auth;
            break;
         case( active ):
            op.active = new_auth;
            break;
      }

      signed_transaction tx;
      tx.operations.push_back(op);
      set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees );
      tx.validate();

      return sign_transaction( tx, broadcast );
   }


signed_transaction wallet_api::transfer(string from, string to, string amount,
                                        string asset_symbol, string memo, bool broadcast /* = false */)
{
   return my->transfer(from, to, amount, asset_symbol, memo, broadcast);
}

signed_transaction wallet_api::create_asset(string issuer,
                                            string symbol,
                                            uint8_t precision,
                                            asset_options common,
                                            fc::optional<bitasset_options> bitasset_opts,
                                            bool broadcast)

     {
         return my->create_asset(issuer, symbol, precision, common, bitasset_opts, broadcast);
     }
signed_transaction create_lottery(string issuer,
      string symbol,
      asset_options common,
      lottery_asset_options lottery_opts,
      bool broadcast = false)
   { try {
      account_object issuer_account = get_account( issuer );
      FC_ASSERT(!find_asset(symbol).valid(), "Asset with that symbol already exists!");

      lottery_asset_create_operation create_op;
      create_op.issuer = issuer_account.id;
      create_op.symbol = symbol;
      create_op.precision = 0;
      create_op.common_options = common;

      create_op.extensions = lottery_opts;

      signed_transaction tx;
      tx.operations.push_back( create_op );
      set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees);
      tx.validate();

      return sign_transaction( tx, broadcast );
   } FC_CAPTURE_AND_RETHROW( (issuer)(symbol)(common)(broadcast) ) }

   signed_transaction wallet_api::create_lottery(string issuer,
      string symbol,
      asset_options common,
      lottery_asset_options lottery_opts,
      bool broadcast)

   {
      return my->create_lottery(issuer, symbol, common, lottery_opts, broadcast);
   }

   signed_transaction buy_ticket( asset_id_type lottery, account_id_type buyer, uint64_t tickets_to_buy )
   { try {
      auto asset_obj = get_asset( lottery );
      FC_ASSERT( asset_obj.is_lottery() );

      ticket_purchase_operation top;
      top.lottery = lottery;
      top.buyer = buyer;
      top.tickets_to_buy = tickets_to_buy;
      top.amount = asset( asset_obj.lottery_options->ticket_price.amount * tickets_to_buy, asset_obj.lottery_options->ticket_price.asset_id );

      signed_transaction tx;
      tx.operations.push_back( top );
      set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees);
      tx.validate();

      return sign_transaction( tx, true );
   } FC_CAPTURE_AND_RETHROW( (lottery)(tickets_to_buy) ) }

   signed_transaction update_dividend_asset(string symbol,
                                            dividend_asset_options new_options,
                                            bool broadcast /* = false */)
   { try {
      optional<asset_object> asset_to_update = find_asset(symbol);
      if (!asset_to_update)
        FC_THROW("No asset with that symbol exists!");

      asset_update_dividend_operation update_op;
      update_op.issuer = asset_to_update->issuer;
      update_op.asset_to_update = asset_to_update->id;
      update_op.new_options = new_options;

      signed_transaction tx;
      tx.operations.push_back( update_op );
      set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees);
      tx.validate();

      return sign_transaction( tx, broadcast );
   } FC_CAPTURE_AND_RETHROW( (symbol)(new_options)(broadcast) ) }

   signed_transaction propose_dividend_asset_update(
      const string& proposing_account,
      fc::time_point_sec expiration_time,
      const variant_object& changed_values,
      bool broadcast = false)
   {
      FC_ASSERT( changed_values.contains("asset_to_update") );

      const chain_parameters& current_params = get_global_properties().parameters;
      asset_update_dividend_operation changed_op;
      fc::reflector<asset_update_dividend_operation>::visit(
         fc::from_variant_visitor<asset_update_dividend_operation>( changed_values, changed_op, GRAPHENE_MAX_NESTED_OBJECTS )
         );

      optional<asset_object> asset_to_update = find_asset(changed_op.asset_to_update);
      if (!asset_to_update)
        FC_THROW("No asset with that symbol exists!");

      asset_update_dividend_operation update_op;
      update_op.issuer = asset_to_update->issuer;
      update_op.asset_to_update = asset_to_update->id;
      update_op.new_options = changed_op.new_options;

      proposal_create_operation prop_op;

      prop_op.expiration_time = expiration_time;
      prop_op.review_period_seconds = current_params.committee_proposal_review_period;
      prop_op.fee_paying_account = get_account(proposing_account).id;

      prop_op.proposed_ops.emplace_back( update_op );
      current_params.current_fees->set_fee( prop_op.proposed_ops.back().op );

      signed_transaction tx;
      tx.operations.push_back(prop_op);
      set_operation_fees(tx, current_params.current_fees);
      tx.validate();

      return sign_transaction(tx, broadcast);
   }

 std::string operation_printer::operator()(const asset_dividend_distribution_operation& op)const
{
   asset_object dividend_paying_asset = wallet.get_asset(op.dividend_asset_id);
   account_object receiver = wallet.get_account(op.account_id);

   out << receiver.name << " received dividend payments for " << dividend_paying_asset.symbol << ": ";
   std::vector<std::string> pretty_payout_amounts;
   for (const asset& payment : op.amounts)
   {
      asset_object payout_asset = wallet.get_asset(payment.asset_id);
      pretty_payout_amounts.push_back(payout_asset.amount_to_pretty_string(payment));
   }
   out << boost::algorithm::join(pretty_payout_amounts, ", ");
   return "";
}

signed_transaction wallet_api::update_asset(string symbol,
                                            optional<string> new_issuer,
                                            asset_options new_options,
                                            bool broadcast /* = false */)
{
   return my->update_asset(symbol, new_issuer, new_options, broadcast);
}

signed_transaction wallet_api::update_asset_issuer(string symbol,
                                            string new_issuer,
                                            bool broadcast /* = false */)
{
   return my->update_asset_issuer(symbol, new_issuer, broadcast);
}

signed_transaction wallet_api::update_bitasset(string symbol,
                                               bitasset_options new_options,
                                               bool broadcast /* = false */)
{
   return my->update_bitasset(symbol, new_options, broadcast);
}

signed_transaction wallet_api::update_asset_feed_producers(string symbol,
                                                           flat_set<string> new_feed_producers,
                                                           bool broadcast /* = false */)
{
   return my->update_asset_feed_producers(symbol, new_feed_producers, broadcast);
}

signed_transaction wallet_api::publish_asset_feed(string publishing_account,
                                                  string symbol,
                                                  price_feed feed,
                                                  bool broadcast /* = false */)
{
   return my->publish_asset_feed(publishing_account, symbol, feed, broadcast);
}

signed_transaction wallet_api::update_dividend_asset(string symbol,
                                                     dividend_asset_options new_options,
                                                     bool broadcast /* = false */)
{
   return my->update_dividend_asset(symbol, new_options, broadcast);
}
signed_transaction wallet_api::fund_asset_fee_pool(string from,
                                                   string symbol,
                                                   string amount,
                                                   bool broadcast /* = false */)
{
   return my->fund_asset_fee_pool(from, symbol, amount, broadcast);
}

signed_transaction wallet_api::claim_asset_fee_pool(string symbol,
                                                    string amount,
                                                    bool broadcast /* = false */)
{
   return my->claim_asset_fee_pool(symbol, amount, broadcast);
}

signed_transaction wallet_api::reserve_asset(string from,
                                          string amount,
                                          string symbol,
                                          bool broadcast /* = false */)
{
   return my->reserve_asset(from, amount, symbol, broadcast);
}

signed_transaction wallet_api::global_settle_asset(string symbol,
                                                   price settle_price,
                                                   bool broadcast /* = false */)
{
   return my->global_settle_asset(symbol, settle_price, broadcast);
}

signed_transaction wallet_api::settle_asset(string account_to_settle,
                                            string amount_to_settle,
                                            string symbol,
                                            bool broadcast /* = false */)
{
   return my->settle_asset(account_to_settle, amount_to_settle, symbol, broadcast);
}

signed_transaction wallet_api::whitelist_account(string authorizing_account,
                                                 string account_to_list,
                                                 account_whitelist_operation::account_listing new_listing_status,
                                                 bool broadcast /* = false */)
{
   return my->whitelist_account(authorizing_account, account_to_list, new_listing_status, broadcast);
}

signed_transaction wallet_api::create_committee_member(string owner_account, string url,
                                               bool broadcast /* = false */)
{
   return my->create_committee_member(owner_account, url, broadcast);
}

map<string,witness_id_type> wallet_api::list_witnesses(const string& lowerbound, uint32_t limit)
{
   return my->_remote_db->lookup_witness_accounts(lowerbound, limit);
}

map<string,committee_member_id_type> wallet_api::list_committee_members(const string& lowerbound, uint32_t limit)
{
   return my->_remote_db->lookup_committee_member_accounts(lowerbound, limit);
}

witness_object wallet_api::get_witness(string owner_account)
{
   return my->get_witness(owner_account);
}

committee_member_object wallet_api::get_committee_member(string owner_account)
{
   return my->get_committee_member(owner_account);
}

signed_transaction wallet_api::create_vesting_balance(string owner_account,
                                              string amount,
                                              string asset_symbol,
                                              vesting_balance_type vesting_type,
                                              bool broadcast /* = false */)
{
   return my->create_vesting_balance(owner_account, amount, asset_symbol, vesting_type, broadcast);
}

signed_transaction wallet_api::create_witness(string owner_account,
                                              string url,
                                              bool broadcast /* = false */)
{
   return my->create_witness(owner_account, url, broadcast);
}

signed_transaction wallet_api::create_worker(
   string owner_account,
   time_point_sec work_begin_date,
   time_point_sec work_end_date,
   share_type daily_pay,
   string name,
   string url,
   variant worker_settings,
   bool broadcast /* = false */)
{
   return my->create_worker( owner_account, work_begin_date, work_end_date,
      daily_pay, name, url, worker_settings, broadcast );
}

signed_transaction wallet_api::update_worker_votes(
   string owner_account,
   worker_vote_delta delta,
   bool broadcast /* = false */)
{
   return my->update_worker_votes( owner_account, delta, broadcast );
}

signed_transaction wallet_api::update_witness(
   string witness_name,
   string url,
   string block_signing_key,
   bool broadcast /* = false */)
{
   return my->update_witness(witness_name, url, block_signing_key, broadcast);
}
signed_transaction update_witness(string witness_name,
                                     string url,
                                     string block_signing_key,
                                     bool broadcast /* = false */)
   { try {
      witness_object witness = get_witness(witness_name);
      account_object witness_account = get_account( witness.witness_account );

      witness_update_operation witness_update_op;
      witness_update_op.witness = witness.id;
      witness_update_op.witness_account = witness_account.id;
      if( url != "" )
         witness_update_op.new_url = url;
      if( block_signing_key != "" ) {
         witness_update_op.new_signing_key = public_key_type( block_signing_key );
         witness_update_op.new_initial_secret = secret_hash_type::hash(secret_hash_type());
      }

      signed_transaction tx;
      tx.operations.push_back( witness_update_op );
      set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees );
      tx.validate();

      return sign_transaction( tx, broadcast );
     } FC_CAPTURE_AND_RETHROW( (witness_name)(url)(block_signing_key)(broadcast) ) 
   }

vector< vesting_balance_object_with_info > wallet_api::get_vesting_balances( string account_name )
{
   return my->get_vesting_balances( account_name );
}

signed_transaction wallet_api::withdraw_vesting(
   string witness_name,
   string amount,
   string asset_symbol,
   bool broadcast /* = false */)
{
   return my->withdraw_vesting( witness_name, amount, asset_symbol, broadcast );
}

signed_transaction wallet_api::vote_for_committee_member(string voting_account,
                                                 string witness,
                                                 bool approve,
                                                 bool broadcast /* = false */)
{
   return my->vote_for_committee_member(voting_account, witness, approve, broadcast);
}

signed_transaction wallet_api::vote_for_witness(string voting_account,
                                                string witness,
                                                bool approve,
                                                bool broadcast /* = false */)
{
   return my->vote_for_witness(voting_account, witness, approve, broadcast);
}

signed_transaction wallet_api::set_voting_proxy(string account_to_modify,
                                                optional<string> voting_account,
                                                bool broadcast /* = false */)
{
   return my->set_voting_proxy(account_to_modify, voting_account, broadcast);
}

signed_transaction wallet_api::set_desired_witness_and_committee_member_count(string account_to_modify,
                                                                      uint16_t desired_number_of_witnesses,
                                                                      uint16_t desired_number_of_committee_members,
                                                                      bool broadcast /* = false */)
{
   return my->set_desired_witness_and_committee_member_count(account_to_modify, desired_number_of_witnesses,
                                                     desired_number_of_committee_members, broadcast);
}

void wallet_api::set_wallet_filename(string wallet_filename)
{
   my->_wallet_filename = wallet_filename;
}

signed_transaction wallet_api::sign_transaction(signed_transaction tx, bool broadcast /* = false */)
{ try {
   return my->sign_transaction( tx, broadcast);
} FC_CAPTURE_AND_RETHROW( (tx) ) }

signed_transaction create_custom_permission(string owner,
                                               string permission_name,
                                               authority auth,
                                               bool broadcast)
   {
      custom_permission_create_operation create_op;
      create_op.owner_account = get_account(owner).id;
      create_op.permission_name = permission_name;
      create_op.auth = auth;

      signed_transaction tx;
      tx.operations.push_back(create_op);
      set_operation_fees(tx, get_global_properties().parameters.current_fees);
      tx.validate();
      return sign_transaction(tx, broadcast);
   }

   signed_transaction update_custom_permission(string owner,
                                               custom_permission_id_type permission_id,
                                               fc::optional<authority> new_auth,
                                               bool broadcast)
   {
      custom_permission_update_operation update_op;
      update_op.owner_account = get_account(owner).id;
      update_op.permission_id = permission_id;
      update_op.new_auth = new_auth;

      signed_transaction tx;
      tx.operations.push_back(update_op);
      set_operation_fees(tx, get_global_properties().parameters.current_fees);
      tx.validate();
      return sign_transaction(tx, broadcast);
   }

   signed_transaction delete_custom_permission(string owner,
                                               custom_permission_id_type permission_id,
                                               bool broadcast)
   {
      custom_permission_delete_operation delete_op;
      delete_op.owner_account = get_account(owner).id;
      delete_op.permission_id = permission_id;

      signed_transaction tx;
      tx.operations.push_back(delete_op);
      set_operation_fees(tx, get_global_properties().parameters.current_fees);
      tx.validate();
      return sign_transaction(tx, broadcast);
   }

   signed_transaction create_custom_account_authority(string owner,
                                                      custom_permission_id_type permission_id,
                                                      int operation_type,
                                                      fc::time_point_sec valid_from,
                                                      fc::time_point_sec valid_to,
                                                      bool broadcast)
   {
      custom_account_authority_create_operation create_op;
      create_op.owner_account = get_account(owner).id;
      create_op.permission_id = permission_id;
      create_op.operation_type = operation_type;
      create_op.valid_from = valid_from;
      create_op.valid_to = valid_to;

      signed_transaction tx;
      tx.operations.push_back(create_op);
      set_operation_fees(tx, get_global_properties().parameters.current_fees);
      tx.validate();
      return sign_transaction(tx, broadcast);
   }
   signed_transaction withdraw_vesting(
      string witness_name,
      string amount,
      string asset_symbol,
      bool broadcast = false )
   { try {
      asset_object asset_obj = get_asset( asset_symbol );
      fc::optional<vesting_balance_id_type> vbid = maybe_id<vesting_balance_id_type>(witness_name);
      if( !vbid )
      {
         if (is_witness(witness_name))
         {
            witness_object wit = get_witness( witness_name );
            FC_ASSERT( wit.pay_vb, "Account ${account} has no core Token ${TOKEN} vested and thus its not allowed to withdraw.", ("account", witness_name)("TOKEN", GRAPHENE_SYMBOL));
            vbid = wit.pay_vb;
         }
         else
            FC_THROW("Account ${account} has no core Token ${TOKEN} vested and thus its not allowed to withdraw.", ("account", witness_name)("TOKEN", GRAPHENE_SYMBOL));
      }

      vesting_balance_object vbo = get_object( *vbid );

      if(vbo.balance_type == vesting_balance_type::gpos)
         FC_THROW("Allowed to withdraw only Normal and Son type vest balances with this method");

      vesting_balance_withdraw_operation vesting_balance_withdraw_op;

      vesting_balance_withdraw_op.vesting_balance = *vbid;
      vesting_balance_withdraw_op.owner = vbo.owner;
      vesting_balance_withdraw_op.amount = asset_obj.amount_from_string(amount);

      signed_transaction tx;
      tx.operations.push_back( vesting_balance_withdraw_op );
      set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees );
      tx.validate();

      return sign_transaction( tx, broadcast );
   } FC_CAPTURE_AND_RETHROW( (witness_name)(amount) )}

   signed_transaction update_custom_account_authority(string owner,
                                                      custom_account_authority_id_type auth_id,
                                                      fc::optional<fc::time_point_sec> new_valid_from,
                                                      fc::optional<fc::time_point_sec> new_valid_to,
                                                      bool broadcast)
   {
      custom_account_authority_update_operation update_op;
      update_op.owner_account = get_account(owner).id;
      update_op.auth_id = auth_id;
      update_op.new_valid_from = new_valid_from;
      update_op.new_valid_to = new_valid_to;

      signed_transaction tx;
      tx.operations.push_back(update_op);
      set_operation_fees(tx, get_global_properties().parameters.current_fees);
      tx.validate();
      return sign_transaction(tx, broadcast);
   }
   vector< vesting_balance_object_with_info > get_vesting_balances( string account_name )
   { try {
      fc::optional<vesting_balance_id_type> vbid = maybe_id<vesting_balance_id_type>( account_name );
      std::vector<vesting_balance_object_with_info> result;
      fc::time_point_sec now = _remote_db->get_dynamic_global_properties().time;

      if( vbid )
      {
         result.emplace_back( get_object(*vbid), now );
         return result;
      }

      vector< vesting_balance_object > vbos = _remote_db->get_vesting_balances( account_name );
      if( vbos.size() == 0 )
         return result;

      for( const vesting_balance_object& vbo : vbos )
         result.emplace_back( vbo, now );

      return result;
   } FC_CAPTURE_AND_RETHROW( (account_name) )}
   signed_transaction delete_custom_account_authority(string owner,
                                                      custom_account_authority_id_type auth_id,
                                                      bool broadcast)
   {
      custom_account_authority_delete_operation delete_op;
      delete_op.owner_account = get_account(owner).id;
      delete_op.auth_id = auth_id;

      signed_transaction tx;
      tx.operations.push_back(delete_op);
      set_operation_fees(tx, get_global_properties().parameters.current_fees);
      tx.validate();
      return sign_transaction(tx, broadcast);
   }

   vector<custom_permission_object> get_custom_permissions(string owner) const
   {
      return _remote_db->get_custom_permissions(get_account(owner).id);
   }

   fc::optional<custom_permission_object> get_custom_permission_by_name(string owner, string permission_name) const
   {
      return _remote_db->get_custom_permission_by_name(get_account(owner).id, permission_name);
   }

   vector<custom_account_authority_object> get_custom_account_authorities(string owner) const
   {
      return _remote_db->get_custom_account_authorities(get_account(owner).id);
   }

   vector<custom_account_authority_object> get_custom_account_authorities_by_permission_id(custom_permission_id_type permission_id) const
   {
      return _remote_db->get_custom_account_authorities_by_permission_id(permission_id);
   }

   vector<custom_account_authority_object> get_custom_account_authorities_by_permission_name(string owner, string permission_name) const
   {
      return _remote_db->get_custom_account_authorities_by_permission_name(get_account(owner).id, permission_name);
   }

   vector<authority> get_active_custom_account_authorities_by_operation(string owner, int operation_type) const
   {
      return _remote_db->get_active_custom_account_authorities_by_operation(get_account(owner).id, operation_type);
   }
 vector<uint64_t> wallet_api::get_random_number_ex(string account,
                                                  uint64_t minimum,
                                                  uint64_t maximum,
                                                  uint64_t selections,
                                                  bool duplicates,
                                                  bool broadcast)
{
   return my->get_random_number_ex( account, minimum, maximum, selections, duplicates, broadcast );
}

uint64_t wallet_api::get_random_number(string account,
                                       uint64_t bound,
                                       bool broadcast)
{
   return my->get_random_number( account, bound, broadcast );
}

  vector<uint64_t> get_random_number_ex(string account,
                                         uint64_t minimum,
                                         uint64_t maximum,
                                         uint64_t selections,
                                         bool duplicates,
                                         bool broadcast)
   {

      vector<uint64_t> v = _remote_db->get_random_number_ex(minimum, maximum, selections, duplicates);

      random_number_store_operation op;
      op.account = get_account(account).id;
      op.random_number = v;
      op.data = "";

      signed_transaction trx;
      trx.operations.push_back(op);
      set_operation_fees( trx, _remote_db->get_global_properties().parameters.current_fees );
      trx.validate();
      sign_transaction( trx, broadcast );

      return v;
   }

   uint64_t get_random_number(string account,
                              uint64_t bound,
                              bool broadcast)
   {
      vector<uint64_t> v = get_random_number_ex(account, 0, bound, 1, false, broadcast);
      return v.at(0);
   }

   ///////////////
// peerplays //
///////////////

vector<sport_object> wallet_api::list_sports() const
{
    return my->_remote_db->list_sports();
}

vector<event_group_object> wallet_api::list_event_groups(sport_id_type sport_id) const
{
    return my->_remote_db->list_event_groups(sport_id);
}

vector<betting_market_group_object> wallet_api::list_betting_market_groups(event_id_type event_id) const
{
    return my->_remote_db->list_betting_market_groups(event_id);
}

vector<betting_market_object> wallet_api::list_betting_markets(betting_market_group_id_type betting_market_group_id) const
{
    return my->_remote_db->list_betting_markets(betting_market_group_id);
}

global_betting_statistics_object wallet_api::get_global_betting_statistics() const
{
    return my->_remote_db->get_global_betting_statistics();
}

vector<event_object>                wallet_api::list_events_in_group(event_group_id_type event_group_id) const
{
    return my->_remote_db->list_events_in_group(event_group_id);
}

vector<bet_object> wallet_api::get_unmatched_bets_for_bettor(betting_market_id_type betting_market_id, account_id_type account_id) const
{
    return my->_remote_db->get_unmatched_bets_for_bettor(betting_market_id, account_id);
}

vector<bet_object> wallet_api::get_all_unmatched_bets_for_bettor(account_id_type account_id) const
{
    return my->_remote_db->get_all_unmatched_bets_for_bettor(account_id);
}

signed_transaction wallet_api::propose_create_sport(
        const string& proposing_account,
        fc::time_point_sec expiration_time,
        internationalized_string_type name,
        bool broadcast /*= false*/)
{
   FC_ASSERT( !is_locked() );
   const chain_parameters& current_params = get_global_properties().parameters;

   sport_create_operation sport_create_op;
   sport_create_op.name = name;

   proposal_create_operation prop_op;
   prop_op.expiration_time = expiration_time;
   prop_op.review_period_seconds = current_params.committee_proposal_review_period;
   prop_op.fee_paying_account = get_account(proposing_account).id;
   prop_op.proposed_ops.emplace_back( sport_create_op );
   current_params.current_fees->set_fee( prop_op.proposed_ops.back().op );

   signed_transaction tx;
   tx.operations.push_back(prop_op);
   my->set_operation_fees(tx, current_params.current_fees);
   tx.validate();

   return my->sign_transaction(tx, broadcast);
}

signed_transaction wallet_api::propose_update_sport(
        const string& proposing_account,
        fc::time_point_sec expiration_time,
        sport_id_type sport_id,
        fc::optional<internationalized_string_type> name,
        bool broadcast /*= false*/)
{
   FC_ASSERT( !is_locked() );
   const chain_parameters& current_params = get_global_properties().parameters;

   sport_update_operation sport_update_op;
   sport_update_op.sport_id = sport_id;
   sport_update_op.new_name = name;

   proposal_create_operation prop_op;
   prop_op.expiration_time = expiration_time;
   prop_op.review_period_seconds = current_params.committee_proposal_review_period;
   prop_op.fee_paying_account = get_account(proposing_account).id;
   prop_op.proposed_ops.emplace_back( sport_update_op );
   current_params.current_fees->set_fee( prop_op.proposed_ops.back().op );

   signed_transaction tx;
   tx.operations.push_back(prop_op);
   my->set_operation_fees(tx, current_params.current_fees);
   tx.validate();

   return my->sign_transaction(tx, broadcast);
}

signed_transaction wallet_api::propose_delete_sport(
        const string& proposing_account,
        fc::time_point_sec expiration_time,
        sport_id_type sport_id,
        bool broadcast /*= false*/)
{
    FC_ASSERT( !is_locked() );
    const chain_parameters& current_params = get_global_properties().parameters;

    sport_delete_operation sport_delete_op;
    sport_delete_op.sport_id = sport_id;

    proposal_create_operation prop_op;
    prop_op.expiration_time = expiration_time;
    prop_op.review_period_seconds = current_params.committee_proposal_review_period;
    prop_op.fee_paying_account = get_account(proposing_account).id;
    prop_op.proposed_ops.emplace_back( sport_delete_op );
    current_params.current_fees->set_fee( prop_op.proposed_ops.back().op );

    signed_transaction tx;
    tx.operations.push_back(prop_op);
    my->set_operation_fees(tx, current_params.current_fees);
    tx.validate();

    return my->sign_transaction(tx, broadcast);
}

signed_transaction wallet_api::propose_create_event_group(
        const string& proposing_account,
        fc::time_point_sec expiration_time,
        internationalized_string_type name,
        sport_id_type sport_id,
        bool broadcast /*= false*/)
{
    FC_ASSERT( !is_locked() );
    const chain_parameters& current_params = get_global_properties().parameters;

    event_group_create_operation event_group_create_op;
    event_group_create_op.name = name;
    event_group_create_op.sport_id = sport_id;

    proposal_create_operation prop_op;
    prop_op.expiration_time = expiration_time;
    prop_op.review_period_seconds = current_params.committee_proposal_review_period;
    prop_op.fee_paying_account = get_account(proposing_account).id;
    prop_op.proposed_ops.emplace_back( event_group_create_op );
    current_params.current_fees->set_fee( prop_op.proposed_ops.back().op );

    signed_transaction tx;
    tx.operations.push_back(prop_op);
    my->set_operation_fees(tx, current_params.current_fees);
    tx.validate();

    return my->sign_transaction(tx, broadcast);
}

signed_transaction wallet_api::propose_update_event_group(
        const string& proposing_account,
        fc::time_point_sec expiration_time,
        event_group_id_type event_group,
        fc::optional<object_id_type> sport_id,
        fc::optional<internationalized_string_type> name,
        bool broadcast /*= false*/)
{
    FC_ASSERT( !is_locked() );
    const chain_parameters& current_params = get_global_properties().parameters;

    event_group_update_operation event_group_update_op;
    event_group_update_op.new_sport_id = sport_id;
    event_group_update_op.new_name = name;
    event_group_update_op.event_group_id = event_group;

    proposal_create_operation prop_op;
    prop_op.expiration_time = expiration_time;
    prop_op.review_period_seconds = current_params.committee_proposal_review_period;
    prop_op.fee_paying_account = get_account(proposing_account).id;
    prop_op.proposed_ops.emplace_back( event_group_update_op );
    current_params.current_fees->set_fee( prop_op.proposed_ops.back().op );

    signed_transaction tx;
    tx.operations.push_back(prop_op);
    my->set_operation_fees(tx, current_params.current_fees);
    tx.validate();

    return my->sign_transaction(tx, broadcast);
}

signed_transaction wallet_api::propose_delete_event_group(
        const string& proposing_account,
        fc::time_point_sec expiration_time,
        event_group_id_type event_group,
        bool broadcast /*= false*/)
{
    FC_ASSERT( !is_locked() );
    const chain_parameters& current_params = get_global_properties().parameters;

    event_group_delete_operation event_group_delete_op;
    event_group_delete_op.event_group_id = event_group;

    proposal_create_operation prop_op;
    prop_op.expiration_time = expiration_time;
    prop_op.review_period_seconds = current_params.committee_proposal_review_period;
    prop_op.fee_paying_account = get_account(proposing_account).id;
    prop_op.proposed_ops.emplace_back( event_group_delete_op );
    current_params.current_fees->set_fee( prop_op.proposed_ops.back().op );

    signed_transaction tx;
    tx.operations.push_back(prop_op);
    my->set_operation_fees(tx, current_params.current_fees);
    tx.validate();

    return my->sign_transaction(tx, broadcast);
}

signed_transaction wallet_api::propose_create_event(
        const string& proposing_account,
        fc::time_point_sec expiration_time,
        internationalized_string_type name,
        internationalized_string_type season,
        fc::optional<time_point_sec> start_time,
        event_group_id_type event_group_id,
        bool broadcast /*= false*/)
{
    FC_ASSERT( !is_locked() );
    const chain_parameters& current_params = get_global_properties().parameters;

    event_create_operation event_create_op;
    event_create_op.start_time = start_time;
    event_create_op.name = name;
    event_create_op.season = season;
    event_create_op.event_group_id = event_group_id;

    proposal_create_operation prop_op;
    prop_op.expiration_time = expiration_time;
    prop_op.review_period_seconds = current_params.committee_proposal_review_period;
    prop_op.fee_paying_account = get_account(proposing_account).id;
    prop_op.proposed_ops.emplace_back( event_create_op );
    current_params.current_fees->set_fee( prop_op.proposed_ops.back().op );

    signed_transaction tx;
    tx.operations.push_back(prop_op);
    my->set_operation_fees(tx, current_params.current_fees);
    tx.validate();

    return my->sign_transaction(tx, broadcast);
}

signed_transaction wallet_api::propose_update_event(
        const string& proposing_account,
        fc::time_point_sec expiration_time,
        event_id_type event_id,
        fc::optional<object_id_type> event_group_id,
        fc::optional<internationalized_string_type> name,
        fc::optional<internationalized_string_type> season,
        fc::optional<event_status> status,
        fc::optional<time_point_sec> start_time,
        bool broadcast /*= false*/)
{
    FC_ASSERT( !is_locked() );
    const chain_parameters& current_params = get_global_properties().parameters;

    event_update_operation event_update_op;
    event_update_op.event_id = event_id;
    event_update_op.new_event_group_id = event_group_id;
    event_update_op.new_start_time = start_time;
    event_update_op.new_name = name;
    event_update_op.new_season = season;
    event_update_op.new_status = status;

    proposal_create_operation prop_op;
    prop_op.expiration_time = expiration_time;
    prop_op.review_period_seconds = current_params.committee_proposal_review_period;
    prop_op.fee_paying_account = get_account(proposing_account).id;
    prop_op.proposed_ops.emplace_back( event_update_op );
    current_params.current_fees->set_fee( prop_op.proposed_ops.back().op );

    signed_transaction tx;
    tx.operations.push_back(prop_op);
    my->set_operation_fees(tx, current_params.current_fees);
    tx.validate();

    return my->sign_transaction(tx, broadcast);
}

signed_transaction wallet_api::propose_create_betting_market_rules(
        const string& proposing_account,
        fc::time_point_sec expiration_time,
        internationalized_string_type name,
        internationalized_string_type description,
        bool broadcast /*= false*/)
{
    FC_ASSERT( !is_locked() );
    const chain_parameters& current_params = get_global_properties().parameters;

    betting_market_rules_create_operation betting_market_rules_create_op;
    betting_market_rules_create_op.name = name;
    betting_market_rules_create_op.description = description;

    proposal_create_operation prop_op;
    prop_op.expiration_time = expiration_time;
    prop_op.review_period_seconds = current_params.committee_proposal_review_period;
    prop_op.fee_paying_account = get_account(proposing_account).id;
    prop_op.proposed_ops.emplace_back( betting_market_rules_create_op );
    current_params.current_fees->set_fee( prop_op.proposed_ops.back().op );

    signed_transaction tx;
    tx.operations.push_back(prop_op);
    my->set_operation_fees(tx, current_params.current_fees);
    tx.validate();

    return my->sign_transaction(tx, broadcast);
}

signed_transaction wallet_api::propose_update_betting_market_rules(
        const string& proposing_account,
        fc::time_point_sec expiration_time,
        betting_market_rules_id_type rules_id,
        fc::optional<internationalized_string_type> name,
        fc::optional<internationalized_string_type> description,
        bool broadcast /*= false*/)
{
    FC_ASSERT( !is_locked() );
    const chain_parameters& current_params = get_global_properties().parameters;

    betting_market_rules_update_operation betting_market_rules_update_op;
    betting_market_rules_update_op.betting_market_rules_id = rules_id;
    betting_market_rules_update_op.new_name = name;
    betting_market_rules_update_op.new_description = description;

    proposal_create_operation prop_op;
    prop_op.expiration_time = expiration_time;
    prop_op.review_period_seconds = current_params.committee_proposal_review_period;
    prop_op.fee_paying_account = get_account(proposing_account).id;
    prop_op.proposed_ops.emplace_back( betting_market_rules_update_op );
    current_params.current_fees->set_fee( prop_op.proposed_ops.back().op );

    signed_transaction tx;
    tx.operations.push_back(prop_op);
    my->set_operation_fees(tx, current_params.current_fees);
    tx.validate();

    return my->sign_transaction(tx, broadcast);
}

signed_transaction wallet_api::propose_create_betting_market_group(
        const string& proposing_account,
        fc::time_point_sec expiration_time,
        internationalized_string_type description,
        event_id_type event_id,
        betting_market_rules_id_type rules_id,
        asset_id_type asset_id,
        bool broadcast /*= false*/)
{
    FC_ASSERT( !is_locked() );
    const chain_parameters& current_params = get_global_properties().parameters;

    betting_market_group_create_operation betting_market_group_create_op;
    betting_market_group_create_op.description = description;
    betting_market_group_create_op.event_id = event_id;
    betting_market_group_create_op.rules_id = rules_id;
    betting_market_group_create_op.asset_id = asset_id;

    proposal_create_operation prop_op;
    prop_op.expiration_time = expiration_time;
    prop_op.review_period_seconds = current_params.committee_proposal_review_period;
    prop_op.fee_paying_account = get_account(proposing_account).id;
    prop_op.proposed_ops.emplace_back( betting_market_group_create_op );
    current_params.current_fees->set_fee( prop_op.proposed_ops.back().op );

    signed_transaction tx;
    tx.operations.push_back(prop_op);
    my->set_operation_fees(tx, current_params.current_fees);
    tx.validate();

    return my->sign_transaction(tx, broadcast);
}

signed_transaction wallet_api::propose_update_betting_market_group(
        const string& proposing_account,
        fc::time_point_sec expiration_time,
        betting_market_group_id_type betting_market_group_id,
        fc::optional<internationalized_string_type> description,
        fc::optional<object_id_type> rules_id,
        fc::optional<betting_market_group_status> status,
        bool broadcast /*= false*/)
{
    FC_ASSERT( !is_locked() );
    const chain_parameters& current_params = get_global_properties().parameters;

    betting_market_group_update_operation betting_market_group_update_op;
    betting_market_group_update_op.betting_market_group_id = betting_market_group_id;
    betting_market_group_update_op.new_description = description;
    betting_market_group_update_op.new_rules_id = rules_id;
    betting_market_group_update_op.status = status;

    proposal_create_operation prop_op;
    prop_op.expiration_time = expiration_time;
    prop_op.review_period_seconds = current_params.committee_proposal_review_period;
    prop_op.fee_paying_account = get_account(proposing_account).id;
    prop_op.proposed_ops.emplace_back( betting_market_group_update_op );
    current_params.current_fees->set_fee( prop_op.proposed_ops.back().op );

    signed_transaction tx;
    tx.operations.push_back(prop_op);
    my->set_operation_fees(tx, current_params.current_fees);
    tx.validate();

    return my->sign_transaction(tx, broadcast);
}

signed_transaction wallet_api::propose_create_betting_market(
        const string& proposing_account,
        fc::time_point_sec expiration_time,
        betting_market_group_id_type group_id,
        internationalized_string_type description,
        internationalized_string_type payout_condition,
        bool broadcast /*= false*/)
{
    FC_ASSERT( !is_locked() );
    const chain_parameters& current_params = get_global_properties().parameters;

    betting_market_create_operation betting_market_create_op;
    betting_market_create_op.group_id = group_id;
    betting_market_create_op.description = description;
    betting_market_create_op.payout_condition = payout_condition;

    proposal_create_operation prop_op;
    prop_op.expiration_time = expiration_time;
    prop_op.review_period_seconds = current_params.committee_proposal_review_period;
    prop_op.fee_paying_account = get_account(proposing_account).id;
    prop_op.proposed_ops.emplace_back( betting_market_create_op );
    current_params.current_fees->set_fee( prop_op.proposed_ops.back().op );

    signed_transaction tx;
    tx.operations.push_back(prop_op);
    my->set_operation_fees(tx, current_params.current_fees);
    tx.validate();

    return my->sign_transaction(tx, broadcast);
}

signed_transaction wallet_api::propose_update_betting_market(
        const string& proposing_account,
        fc::time_point_sec expiration_time,
        betting_market_id_type market_id,
        fc::optional<object_id_type> group_id,
        fc::optional<internationalized_string_type> description,
        fc::optional<internationalized_string_type> payout_condition,
        bool broadcast /*= false*/)
{
    FC_ASSERT( !is_locked() );
    const chain_parameters& current_params = get_global_properties().parameters;

    betting_market_update_operation betting_market_update_op;
    betting_market_update_op.betting_market_id = market_id;
    betting_market_update_op.new_group_id = group_id;
    betting_market_update_op.new_description = description;
    betting_market_update_op.new_payout_condition = payout_condition;

    proposal_create_operation prop_op;
    prop_op.expiration_time = expiration_time;
    prop_op.review_period_seconds = current_params.committee_proposal_review_period;
    prop_op.fee_paying_account = get_account(proposing_account).id;
    prop_op.proposed_ops.emplace_back( betting_market_update_op );
    current_params.current_fees->set_fee( prop_op.proposed_ops.back().op );

    signed_transaction tx;
    tx.operations.push_back(prop_op);
    my->set_operation_fees(tx, current_params.current_fees);
    tx.validate();

    return my->sign_transaction(tx, broadcast);
}

signed_transaction wallet_api::place_bet(string betting_account,
                                         betting_market_id_type betting_market_id,
                                         bet_type back_or_lay,
                                         string amount,
                                         string asset_symbol,
                                         double backer_multiplier,
                                         bool broadcast /*= false*/)
{
    FC_ASSERT( !is_locked() );
    fc::optional<asset_object> asset_obj = get_asset(asset_symbol);
    FC_ASSERT(asset_obj, "Could not find asset matching ${asset}", ("asset", asset_symbol));

    const chain_parameters& current_params = get_global_properties().parameters;

    bet_place_operation bet_place_op;
    bet_place_op.bettor_id = get_account(betting_account).id;
    bet_place_op.betting_market_id = betting_market_id;
    bet_place_op.amount_to_bet = asset_obj->amount_from_string(amount);
    bet_place_op.backer_multiplier = (bet_multiplier_type)(backer_multiplier * GRAPHENE_BETTING_ODDS_PRECISION);
    bet_place_op.back_or_lay = back_or_lay;

    signed_transaction tx;
    tx.operations.push_back(bet_place_op);
    my->set_operation_fees(tx, current_params.current_fees);
    tx.validate();

    return my->sign_transaction(tx, broadcast);
}

signed_transaction wallet_api::cancel_bet(string betting_account,
                                          bet_id_type bet_id,
                                          bool broadcast /*= false*/)
{
    FC_ASSERT( !is_locked() );

    const chain_parameters& current_params = get_global_properties().parameters;

    bet_cancel_operation bet_cancel_op;
    bet_cancel_op.bettor_id = get_account(betting_account).id;
    bet_cancel_op.bet_to_cancel = bet_id;

    signed_transaction tx;
    tx.operations.push_back(bet_cancel_op);
    my->set_operation_fees(tx, current_params.current_fees);
    tx.validate();

    return my->sign_transaction(tx, broadcast);
}

signed_transaction wallet_api::propose_resolve_betting_market_group(
        const string& proposing_account,
        fc::time_point_sec expiration_time,
        betting_market_group_id_type betting_market_group_id,
        const std::map<betting_market_id_type, betting_market_resolution_type>& resolutions,
        bool broadcast /*= false*/)
{
    FC_ASSERT( !is_locked() );
    const chain_parameters& current_params = get_global_properties().parameters;

    betting_market_group_resolve_operation betting_market_group_resolve_op;
    betting_market_group_resolve_op.betting_market_group_id = betting_market_group_id;
    betting_market_group_resolve_op.resolutions = resolutions;

    proposal_create_operation prop_op;
    prop_op.expiration_time = expiration_time;
    prop_op.review_period_seconds = current_params.committee_proposal_review_period;
    prop_op.fee_paying_account = get_account(proposing_account).id;
    prop_op.proposed_ops.emplace_back( betting_market_group_resolve_op );
    current_params.current_fees->set_fee( prop_op.proposed_ops.back().op );

    signed_transaction tx;
    tx.operations.push_back(prop_op);
    my->set_operation_fees(tx, current_params.current_fees);
    tx.validate();

    return my->sign_transaction(tx, broadcast);
}

signed_transaction wallet_api::propose_cancel_betting_market_group(
        const string& proposing_account,
        fc::time_point_sec expiration_time,
        betting_market_group_id_type betting_market_group_id,
        bool broadcast /*= false*/)
{
    FC_ASSERT( !is_locked() );
    const chain_parameters& current_params = get_global_properties().parameters;

    betting_market_group_cancel_unmatched_bets_operation betting_market_group_cancel_unmatched_bets_op;
    betting_market_group_cancel_unmatched_bets_op.betting_market_group_id = betting_market_group_id;

    proposal_create_operation prop_op;
    prop_op.expiration_time = expiration_time;
    prop_op.review_period_seconds = current_params.committee_proposal_review_period;
    prop_op.fee_paying_account = get_account(proposing_account).id;
    prop_op.proposed_ops.emplace_back( betting_market_group_cancel_unmatched_bets_op );
    current_params.current_fees->set_fee( prop_op.proposed_ops.back().op );

    signed_transaction tx;
    tx.operations.push_back(prop_op);
    my->set_operation_fees(tx, current_params.current_fees);
    tx.validate();

    return my->sign_transaction(tx, broadcast);
}

signed_transaction wallet_api::tournament_create( string creator, tournament_options options, bool broadcast )
{
   FC_ASSERT( !is_locked() );
   account_object creator_account_obj = get_account(creator);

   signed_transaction tx;
   tournament_create_operation op;
   op.creator = creator_account_obj.get_id();
   op.options = options;
   tx.operations = {op};
   my->set_operation_fees( tx, my->_remote_db->get_global_properties().parameters.current_fees );
   tx.validate();

   return my->sign_transaction( tx, broadcast );
}

signed_transaction wallet_api::tournament_join( string payer_account,
                                                string player_account,
                                                tournament_id_type tournament_id,
                                                string buy_in_amount,
                                                string buy_in_asset_symbol,
                                                bool broadcast )
{
   FC_ASSERT( !is_locked() );
   account_object payer_account_obj = get_account(payer_account);
   account_object player_account_obj = get_account(player_account);
   //graphene::chain::tournament_object tournament_obj = my->get_object<graphene::chain::tournament_object>(tournament_id);

   fc::optional<asset_object> buy_in_asset_obj = get_asset(buy_in_asset_symbol);
   FC_ASSERT(buy_in_asset_obj, "Could not find asset matching ${asset}", ("asset", buy_in_asset_symbol));

   signed_transaction tx;
   tournament_join_operation op;
   op.payer_account_id = payer_account_obj.get_id();
   op.player_account_id = player_account_obj.get_id();
   op.tournament_id = tournament_id;
   op.buy_in = buy_in_asset_obj->amount_from_string(buy_in_amount);

   tx.operations = {op};
   my->set_operation_fees( tx, my->_remote_db->get_global_properties().parameters.current_fees );
   tx.validate();

   return my->sign_transaction( tx, broadcast );
}

signed_transaction wallet_api::tournament_leave( string canceling_account,
                                                 string player_account,
                                                 tournament_id_type tournament_id,
                                                 bool broadcast)
{
    FC_ASSERT( !is_locked() );
    account_object player_account_obj = get_account(player_account);
    account_object canceling_account_obj = get_account(canceling_account);
    //graphene::chain::tournament_object tournament_obj = my->get_object<graphene::chain::tournament_object>(tournament_id);

    signed_transaction tx;
    tournament_leave_operation op;
    op.canceling_account_id = canceling_account_obj.get_id();
    op.player_account_id = player_account_obj.get_id();
    op.tournament_id = tournament_id;

    tx.operations = {op};
    my->set_operation_fees( tx, my->_remote_db->get_global_properties().parameters.current_fees );
    tx.validate();

    return my->sign_transaction( tx, broadcast );
}

vector<tournament_object> wallet_api::get_upcoming_tournaments(uint32_t limit)
{
   return my->_remote_db->get_tournaments_in_state(tournament_state::accepting_registrations, limit);
}
vector<tournament_object> wallet_api::get_tournaments(tournament_id_type stop,
                                          unsigned limit,
                                          tournament_id_type start) {
   return my->_remote_db->get_tournaments(stop, limit, start);
}

vector<tournament_object> wallet_api::get_tournaments_by_state(tournament_id_type stop,
                                                   unsigned limit,
                                                   tournament_id_type start,
                                                   tournament_state state) {
   return my->_remote_db->get_tournaments_by_state(stop, limit, start, state);
}

tournament_object wallet_api::get_tournament(tournament_id_type id)
{
   return my->_remote_db->get_objects({id})[0].as<tournament_object>( GRAPHENE_MAX_NESTED_OBJECTS );
}

signed_transaction wallet_api::rps_throw(game_id_type game_id,
                                         string player_account,
                                         rock_paper_scissors_gesture gesture,
                                         bool broadcast)
{
   FC_ASSERT( !is_locked() );

   // check whether the gesture is appropriate for the game we're playing
   graphene::chain::game_object game_obj = my->get_object(game_id);
   graphene::chain::match_object match_obj = my->get_object(game_obj.match_id);
   graphene::chain::tournament_object tournament_obj = my->get_object(match_obj.tournament_id);
   graphene::chain::rock_paper_scissors_game_options game_options = 
      tournament_obj.options.game_options.get<graphene::chain::rock_paper_scissors_game_options>();
   if ((int)gesture >= game_options.number_of_gestures)
      FC_THROW("Gesture ${gesture} not supported in this game", ("gesture", gesture));

   account_object player_account_obj = get_account(player_account);

   // construct the complete throw, the commit, and reveal
   rock_paper_scissors_throw full_throw;
   fc::rand_bytes((char*)&full_throw.nonce1, sizeof(full_throw.nonce1));
   fc::rand_bytes((char*)&full_throw.nonce2, sizeof(full_throw.nonce2));
   full_throw.gesture = gesture;

   rock_paper_scissors_throw_commit commit_throw;
   commit_throw.nonce1 = full_throw.nonce1;
   std::vector<char> full_throw_packed(fc::raw::pack(full_throw));
   commit_throw.throw_hash = fc::sha256::hash(full_throw_packed.data(), full_throw_packed.size());

   rock_paper_scissors_throw_reveal reveal_throw;
   reveal_throw.nonce2 = full_throw.nonce2;
   reveal_throw.gesture = full_throw.gesture;

   // store off the reveal for transmitting after both players commit
   my->_wallet.committed_game_moves[commit_throw] = reveal_throw;

   // broadcast the commit
   signed_transaction tx;
   game_move_operation move_operation;
   move_operation.game_id = game_id;
   move_operation.player_account_id = player_account_obj.id;
   move_operation.move = commit_throw;
   tx.operations = {move_operation};
   my->set_operation_fees( tx, my->_remote_db->get_global_properties().parameters.current_fees );
   tx.validate();

   return my->sign_transaction( tx, broadcast );
}

// signed_transaction add_sidechain_address(string account,
//                                             sidechain_type sidechain,
//                                             string deposit_public_key,
//                                             string deposit_address,
//                                             string withdraw_public_key,
//                                             string withdraw_address,
//                                             bool broadcast)
//    { try {
//       account_id_type sidechain_address_account_id = get_account_id(account);

//       sidechain_address_add_operation op;
//       op.payer = sidechain_address_account_id;
//       op.sidechain_address_account = sidechain_address_account_id;
//       op.sidechain = sidechain;
//       op.deposit_public_key = deposit_public_key;
//       op.deposit_address = deposit_address;
//       op.withdraw_public_key = withdraw_public_key;
//       op.withdraw_address = withdraw_address;

//       signed_transaction tx;
//       tx.operations.push_back( op );
//       set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees);
//       tx.validate();

//       return sign_transaction( tx, broadcast );
//    } FC_CAPTURE_AND_RETHROW() }

   // signed_transaction delete_sidechain_address(string account,
   //                                             sidechain_type sidechain,
   //                                             bool broadcast )
   // { try {
   //    account_id_type sidechain_address_account_id = get_account_id(account);
   //    fc::optional<sidechain_address_object> sao = _remote_db->get_sidechain_address_by_account_and_sidechain(sidechain_address_account_id, sidechain);
   //    if (!sao)
   //       FC_THROW("No sidechain address for account ${account} and sidechain ${sidechain}", ("account", sidechain_address_account_id)("sidechain", sidechain));

   //    sidechain_address_delete_operation op;
   //    op.payer = sidechain_address_account_id;
   //    op.sidechain_address_id = sao->id;
   //    op.sidechain_address_account = sidechain_address_account_id;
   //    op.sidechain = sidechain;

   //    signed_transaction tx;
   //    tx.operations.push_back( op );
   //    set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees);
   //    tx.validate();

   //    return sign_transaction( tx, broadcast );

   // } FC_CAPTURE_AND_RETHROW() }

signed_transaction wallet_api::nft_metadata_create(string owner_account_id_or_name,
                                                   string name,
                                                   string symbol,
                                                   string base_uri,
                                                   optional<string> revenue_partner,
                                                   optional<uint16_t> revenue_split,
                                                   bool is_transferable,
                                                   bool is_sellable,
                                                   optional<account_role_id_type> role_id,
                                                   optional<share_type> max_supply,
                                                   optional<nft_lottery_options> lottery_options,
                                                   bool broadcast)
{
   account_object owner_account = my->get_account(owner_account_id_or_name);

   nft_metadata_create_operation op;
   op.owner = owner_account.id;
   op.name = name;
   op.symbol = symbol;
   op.base_uri = base_uri;
   if( revenue_partner )
   {
      account_object partner_account = my->get_account(*revenue_partner);
      op.revenue_partner = partner_account.id;
      uint16_t rev_split = 0;
      if( revenue_split )
      {
         rev_split = *revenue_split;
      }
      op.revenue_split = rev_split;
   }
   op.is_transferable = is_transferable;
   op.is_sellable = is_sellable;

   signed_transaction trx;
   trx.operations.push_back(op);
   my->set_operation_fees( trx, my->_remote_db->get_global_properties().parameters.current_fees );
   trx.validate();

   return my->sign_transaction( trx, broadcast );
}

signed_transaction wallet_api::nft_metadata_create(string owner_account_id_or_name,
                                                   string name,
                                                   string symbol,
                                                   string base_uri,
                                                   optional<string> revenue_partner,
                                                   optional<uint16_t> revenue_split,
                                                   bool is_transferable,
                                                   bool is_sellable,
                                                   optional<account_role_id_type> role_id,
                                                   optional<share_type> max_supply,
                                                   optional<nft_lottery_options> lottery_options,
                                                   bool broadcast)
{
   account_object owner_account = my->get_account(owner_account_id_or_name);

   nft_metadata_create_operation op;
   op.owner = owner_account.id;
   op.name = name;
   op.symbol = symbol;
   op.base_uri = base_uri;
   if( revenue_partner )
   {
      account_object partner_account = my->get_account(*revenue_partner);
      op.revenue_partner = partner_account.id;
      uint16_t rev_split = 0;
      if( revenue_split )
      {
         rev_split = *revenue_split;
      }
      op.revenue_split = rev_split;
   }
   op.is_transferable = is_transferable;
   op.is_sellable = is_sellable;
   op.account_role = role_id;
   op.max_supply = max_supply;
   op.lottery_options = lottery_options;

   signed_transaction trx;
   trx.operations.push_back(op);
   my->set_operation_fees( trx, my->_remote_db->get_global_properties().parameters.current_fees );
   trx.validate();

   return my->sign_transaction( trx, broadcast );
}

signed_transaction wallet_api::nft_metadata_update(string owner_account_id_or_name,
                                                   nft_metadata_id_type nft_metadata_id,
                                                   optional<string> name,
                                                   optional<string> symbol,
                                                   optional<string> base_uri,
                                                   optional<string> revenue_partner,
                                                   optional<uint16_t> revenue_split,
                                                   optional<bool> is_transferable,
                                                   optional<bool> is_sellable,
                                                   optional<account_role_id_type> role_id,
                                                   bool broadcast)
{
   account_object owner_account = my->get_account(owner_account_id_or_name);

   nft_metadata_update_operation op;
   op.nft_metadata_id = nft_metadata_id;
   op.owner = owner_account.id;
   op.name = name;
   op.symbol = symbol;
   op.base_uri = base_uri;
   if( revenue_partner )
   {
      account_object partner_account = my->get_account(*revenue_partner);
      op.revenue_partner = partner_account.id;
      uint16_t rev_split = 0;
      if( revenue_split )
      {
         rev_split = *revenue_split;
      }
      op.revenue_split = rev_split;
   }
   op.is_transferable = is_transferable;
   op.is_sellable = is_sellable;
   op.account_role = role_id;

   signed_transaction trx;
   trx.operations.push_back(op);
   my->set_operation_fees( trx, my->_remote_db->get_global_properties().parameters.current_fees );
   trx.validate();

   return my->sign_transaction( trx, broadcast );
}

signed_transaction wallet_api::nft_create(string metadata_owner_account_id_or_name,
                                          nft_metadata_id_type metadata_id,
                                          string owner_account_id_or_name,
                                          string approved_account_id_or_name,
                                          string token_uri,
                                          bool broadcast)
{
   account_object metadata_owner_account = my->get_account(metadata_owner_account_id_or_name);
   account_object owner_account = my->get_account(owner_account_id_or_name);
   account_object approved_account = my->get_account(approved_account_id_or_name);

   nft_mint_operation op;
   op.payer = metadata_owner_account.id;
   op.nft_metadata_id = metadata_id;
   op.owner = owner_account.id;
   op.approved = approved_account.id;
   op.token_uri = token_uri;

   signed_transaction trx;
   trx.operations.push_back(op);
   my->set_operation_fees( trx, my->_remote_db->get_global_properties().parameters.current_fees );
   trx.validate();

   return my->sign_transaction( trx, broadcast );
}

uint64_t wallet_api::nft_get_balance(string owner_account_id_or_name) const
{
   account_object owner_account = my->get_account(owner_account_id_or_name);
   return my->_remote_db->nft_get_balance(owner_account.id);
}

optional<account_id_type> wallet_api::nft_owner_of(const nft_id_type token_id) const
{
   return my->_remote_db->nft_owner_of(token_id);
}

signed_transaction wallet_api::nft_safe_transfer_from(string operator_account_id_or_name,
                                                      string from_account_id_or_name,
                                                      string to_account_id_or_name,
                                                      nft_id_type token_id,
                                                      string data,
                                                      bool broadcast)
{
   account_object operator_account = my->get_account(operator_account_id_or_name);
   account_object from_account = my->get_account(from_account_id_or_name);
   account_object to_account = my->get_account(to_account_id_or_name);

   nft_safe_transfer_from_operation op;
   op.operator_ = operator_account.id;
   op.from = from_account.id;
   op.to = to_account.id;
   op.token_id = token_id;
   op.data = data;

   signed_transaction trx;
   trx.operations.push_back(op);
   my->set_operation_fees( trx, my->_remote_db->get_global_properties().parameters.current_fees );
   trx.validate();

   return my->sign_transaction( trx, broadcast );
}

signed_transaction wallet_api::nft_transfer_from(string operator_account_id_or_name,
                                                 string from_account_id_or_name,
                                                 string to_account_id_or_name,
                                                 nft_id_type token_id,
                                                 bool broadcast)
{
   return nft_safe_transfer_from(operator_account_id_or_name, from_account_id_or_name, to_account_id_or_name, token_id, "", broadcast);
}

signed_transaction wallet_api::nft_approve(string operator_account_id_or_name,
                                           string approved_account_id_or_name,
                                           nft_id_type token_id,
                                           bool broadcast)
{
   account_object operator_account = my->get_account(operator_account_id_or_name);
   account_object approved_account = my->get_account(approved_account_id_or_name);

   nft_approve_operation op;
   op.operator_ = operator_account.id;
   op.approved = approved_account.id;
   op.token_id = token_id;

   signed_transaction trx;
   trx.operations.push_back(op);
   my->set_operation_fees( trx, my->_remote_db->get_global_properties().parameters.current_fees );
   trx.validate();

   return my->sign_transaction( trx, broadcast );
}

signed_transaction wallet_api::nft_set_approval_for_all(string owner_account_id_or_name,
                                                        string operator_account_id_or_name,
                                                        bool approved,
                                                        bool broadcast)
{
   account_object owner_account = my->get_account(owner_account_id_or_name);
   account_object operator_account = my->get_account(operator_account_id_or_name);

   nft_set_approval_for_all_operation op;
   op.owner = owner_account.id;
   op.operator_ = operator_account.id;
   op.approved = approved;

   signed_transaction trx;
   trx.operations.push_back(op);
   my->set_operation_fees( trx, my->_remote_db->get_global_properties().parameters.current_fees );
   trx.validate();

   return my->sign_transaction( trx, broadcast );
}

optional<account_id_type> wallet_api::nft_get_approved(const nft_id_type token_id) const
{
   return my->_remote_db->nft_get_approved(token_id);
}

bool wallet_api::nft_is_approved_for_all(string owner_account_id_or_name, string operator_account_id_or_name) const
{
   account_object owner_account = my->get_account(owner_account_id_or_name);
   account_object operator_account = my->get_account(operator_account_id_or_name);
   return my->_remote_db->nft_is_approved_for_all(owner_account.id, operator_account.id);
}

vector<nft_object> wallet_api::nft_get_all_tokens(uint32_t limit, optional<nft_id_type> lower_id) const
{
   nft_id_type lb_id;
   if(lower_id)
      lb_id = *lower_id;
   return my->_remote_db->nft_get_all_tokens(lb_id, limit);
}

vector<nft_object> wallet_api::nft_get_tokens_by_owner(account_id_type owner, uint32_t limit, optional<nft_id_type> lower_id) const
{
   nft_id_type lb_id;
   if(lower_id)
      lb_id = *lower_id;
   return my->_remote_db->nft_get_tokens_by_owner(owner, lb_id, limit);
}

vector<nft_metadata_object> wallet_api::nft_get_metadata_by_owner(account_id_type owner, uint32_t limit, optional<nft_metadata_id_type> lower_id) const
{
   nft_metadata_id_type lb_id;
   if(lower_id)
      lb_id = *lower_id;
   return my->_remote_db->nft_get_metadata_by_owner(owner, lb_id, limit);
}

signed_transaction wallet_api::buy_ticket( asset_id_type lottery, account_id_type buyer, uint64_t tickets_to_buy )
{
   return my->buy_ticket(lottery, buyer, tickets_to_buy);
}


signed_transaction wallet_api::create_offer(set<nft_id_type> item_ids,
                                             string issuer_accound_id_or_name,
                                             asset minimum_price,
                                             asset maximum_price,
                                             bool buying_item,
                                             time_point_sec offer_expiration_date,
                                             optional<memo_data> memo,
                                             bool broadcast)
{
   account_object issuer_account = my->get_account(issuer_accound_id_or_name);

   offer_operation op;
   op.item_ids = item_ids;
   op.issuer = issuer_account.id;
   op.minimum_price = minimum_price;
   op.maximum_price = maximum_price;
   op.buying_item = buying_item;
   op.offer_expiration_date = offer_expiration_date;
   op.memo = memo;

   signed_transaction trx;
   trx.operations.push_back(op);
   my->set_operation_fees( trx, my->_remote_db->get_global_properties().parameters.current_fees );
   trx.validate();

   return my->sign_transaction( trx, broadcast );
}

signed_transaction wallet_api::create_bid(string bidder_account_id_or_name,
                                          asset bid_price,
                                          offer_id_type offer_id,
                                          bool broadcast)
{
   account_object bidder_account = my->get_account(bidder_account_id_or_name);

   bid_operation op;
   op.bidder = bidder_account.id;
   op.offer_id = offer_id;
   op.bid_price = bid_price;

   signed_transaction trx;
   trx.operations.push_back(op);
   my->set_operation_fees( trx, my->_remote_db->get_global_properties().parameters.current_fees );
   trx.validate();

   return my->sign_transaction( trx, broadcast );
}

signed_transaction wallet_api::cancel_offer(string issuer_account_id_or_name,
                                            offer_id_type offer_id,
                                            bool broadcast)
{
   account_object issuer_account = my->get_account(issuer_account_id_or_name);

   cancel_offer_operation op;
   op.issuer = issuer_account.id;
   op.offer_id = offer_id;

   signed_transaction trx;
   trx.operations.push_back(op);
   my->set_operation_fees( trx, my->_remote_db->get_global_properties().parameters.current_fees );
   trx.validate();

   return my->sign_transaction( trx, broadcast );
}

vector<offer_object> wallet_api::list_offers(uint32_t limit, optional<offer_id_type> lower_id) const
{
   offer_id_type lb_id;
   if(lower_id)
      lb_id = *lower_id;
   return my->_remote_db->list_offers(lb_id, limit);
}

vector<offer_object> wallet_api::list_sell_offers(uint32_t limit, optional<offer_id_type> lower_id) const
{
   offer_id_type lb_id;
   if(lower_id)
      lb_id = *lower_id;
   return my->_remote_db->list_sell_offers(lb_id, limit);
}

vector<offer_object> wallet_api::list_buy_offers(uint32_t limit, optional<offer_id_type> lower_id) const
{
   offer_id_type lb_id;
   if(lower_id)
      lb_id = *lower_id;
   return my->_remote_db->list_buy_offers(lb_id, limit);
}

vector<offer_history_object> wallet_api::list_offer_history(uint32_t limit, optional<offer_history_id_type> lower_id) const
{
   offer_history_id_type lb_id;
   if(lower_id)
      lb_id = *lower_id;
   return my->_remote_db->list_offer_history(lb_id, limit);
}

vector<offer_object> wallet_api::get_offers_by_issuer(string issuer_account_id_or_name,
                                                      uint32_t limit, optional<offer_id_type> lower_id) const
{
   offer_id_type lb_id;
   if(lower_id)
      lb_id = *lower_id;
   account_object issuer_account = my->get_account(issuer_account_id_or_name);
   return my->_remote_db->get_offers_by_issuer(lb_id, issuer_account.id, limit);
}

vector<offer_object> wallet_api::get_offers_by_item(const nft_id_type item, uint32_t limit, optional<offer_id_type> lower_id) const
{
   offer_id_type lb_id;
   if(lower_id)
      lb_id = *lower_id;
   return my->_remote_db->get_offers_by_item(lb_id, item, limit);
}

vector<offer_history_object> wallet_api::get_offer_history_by_issuer(string issuer_account_id_or_name, uint32_t limit, optional<offer_history_id_type> lower_id) const
{
   offer_history_id_type lb_id;
   if(lower_id)
      lb_id = *lower_id;
   account_object issuer_account = my->get_account(issuer_account_id_or_name);
   return my->_remote_db->get_offer_history_by_issuer(lb_id, issuer_account.id, limit);
}

vector<offer_history_object> wallet_api::get_offer_history_by_item(const nft_id_type item, uint32_t limit, optional<offer_history_id_type> lower_id) const
{
   offer_history_id_type lb_id;
   if(lower_id)
      lb_id = *lower_id;
   return my->_remote_db->get_offer_history_by_item(lb_id, item, limit);
}

vector<offer_history_object> wallet_api::get_offer_history_by_bidder(string bidder_account_id_or_name, uint32_t limit, optional<offer_history_id_type> lower_id) const
{
   offer_history_id_type lb_id;
   if(lower_id)
      lb_id = *lower_id;
   account_object bidder_account = my->get_account(bidder_account_id_or_name);
   return my->_remote_db->get_offer_history_by_bidder(lb_id, bidder_account.id, limit);
}

signed_transaction wallet_api::create_account_role(string owner_account_id_or_name,
                                                   string name,
                                                   string metadata,
                                                   flat_set<int> allowed_operations,
                                                   flat_set<account_id_type> whitelisted_accounts,
                                                   time_point_sec valid_to,
                                                   bool broadcast)
{
   account_object owner_account = my->get_account(owner_account_id_or_name);

   account_role_create_operation op;
   op.owner = owner_account.id;
   op.name = name;
   op.metadata = metadata;
   op.allowed_operations = allowed_operations;
   op.whitelisted_accounts = whitelisted_accounts;
   op.valid_to = valid_to;

   signed_transaction trx;
   trx.operations.push_back(op);
   my->set_operation_fees( trx, my->_remote_db->get_global_properties().parameters.current_fees );
   trx.validate();

   return my->sign_transaction( trx, broadcast );
}

signed_transaction wallet_api::update_account_role(string owner_account_id_or_name,
                                                   account_role_id_type role_id,
                                                   optional<string> name,
                                                   optional<string> metadata,
                                                   flat_set<int> operations_to_add,
                                                   flat_set<int> operations_to_remove,
                                                   flat_set<account_id_type> accounts_to_add,
                                                   flat_set<account_id_type> accounts_to_remove,
                                                   optional<time_point_sec> valid_to,
                                                   bool broadcast)
{
   account_object owner_account = my->get_account(owner_account_id_or_name);

   account_role_update_operation op;
   op.owner = owner_account.id;
   op.account_role_id = role_id;
   op.name = name;
   op.metadata = metadata;
   op.allowed_operations_to_add = operations_to_add;
   op.allowed_operations_to_remove = operations_to_remove;
   op.accounts_to_add = accounts_to_add;
   op.accounts_to_remove = accounts_to_remove;
   op.valid_to = valid_to;

   signed_transaction trx;
   trx.operations.push_back(op);
   my->set_operation_fees( trx, my->_remote_db->get_global_properties().parameters.current_fees );
   trx.validate();

   return my->sign_transaction( trx, broadcast );
}

signed_transaction wallet_api::delete_account_role(string owner_account_id_or_name,
                                                   account_role_id_type role_id,
                                                   bool broadcast)
{
   account_object owner_account = my->get_account(owner_account_id_or_name);

   account_role_delete_operation op;
   op.owner = owner_account.id;
   op.account_role_id = role_id;

   signed_transaction trx;
   trx.operations.push_back(op);
   my->set_operation_fees( trx, my->_remote_db->get_global_properties().parameters.current_fees );
   trx.validate();

   return my->sign_transaction( trx, broadcast );
}

vector<account_role_object> wallet_api::get_account_roles_by_owner(string owner_account_id_or_name) const
{
   account_object owner_account = my->get_account(owner_account_id_or_name);
   return my->_remote_db->get_account_roles_by_owner(owner_account.id);
}
signed_transaction wallet_api::sign_transaction2(signed_transaction tx, const vector<public_key_type>& signing_keys,
                                                 bool broadcast /* = false */)
{ try {
   return my->sign_transaction2( tx, signing_keys, broadcast);
} FC_CAPTURE_AND_RETHROW( (tx) ) }

flat_set<public_key_type> wallet_api::get_transaction_signers(const signed_transaction &tx) const
{ try {
   return my->get_transaction_signers(tx);
} FC_CAPTURE_AND_RETHROW( (tx) ) }

vector<flat_set<account_id_type>> wallet_api::get_key_references(const vector<public_key_type> &keys) const
{ try {
   return my->get_key_references(keys);
} FC_CAPTURE_AND_RETHROW( (keys) ) }

operation wallet_api::get_prototype_operation(string operation_name)
{
   return my->get_prototype_operation( operation_name );
}

void wallet_api::dbg_make_uia(string creator, string symbol)
{
   FC_ASSERT(!is_locked());
   my->dbg_make_uia(creator, symbol);
}

void wallet_api::dbg_make_mia(string creator, string symbol)
{
   FC_ASSERT(!is_locked());
   my->dbg_make_mia(creator, symbol);
}

void wallet_api::dbg_push_blocks( std::string src_filename, uint32_t count )
{
   my->dbg_push_blocks( src_filename, count );
}

void wallet_api::dbg_generate_blocks( std::string debug_wif_key, uint32_t count )
{
   my->dbg_generate_blocks( debug_wif_key, count );
}

void wallet_api::dbg_stream_json_objects( const std::string& filename )
{
   my->dbg_stream_json_objects( filename );
}

void wallet_api::dbg_update_object( fc::variant_object update )
{
   my->dbg_update_object( update );
}

void wallet_api::network_add_nodes( const vector<string>& nodes )
{
   my->network_add_nodes( nodes );
}

vector< variant > wallet_api::network_get_connected_peers()
{
   return my->network_get_connected_peers();
}

void wallet_api::flood_network(string prefix, uint32_t number_of_transactions)
{
   FC_ASSERT(!is_locked());
   my->flood_network(prefix, number_of_transactions);
}

signed_transaction wallet_api::propose_parameter_extension_change(
   const string& proposing_account,
   fc::time_point_sec expiration_time,
   const variant_object& changed_extensions,
   bool broadcast /* = false */
   )
{
   return my->propose_parameter_extension_change( proposing_account, expiration_time, changed_extensions, broadcast );
}

signed_transaction wallet_api::propose_parameter_change(
   const string& proposing_account,
   fc::time_point_sec expiration_time,
   const variant_object& changed_values,
   bool broadcast /* = false */
   )
{
   return my->propose_parameter_change( proposing_account, expiration_time, changed_values, broadcast );
}

signed_transaction wallet_api::propose_fee_change(
   const string& proposing_account,
   fc::time_point_sec expiration_time,
   const variant_object& changed_fees,
   bool broadcast /* = false */
   )
{
   return my->propose_fee_change( proposing_account, expiration_time, changed_fees, broadcast );
}

signed_transaction wallet_api::propose_dividend_asset_update(
   const string& proposing_account,
   fc::time_point_sec expiration_time,
   const variant_object& changed_fees,
   bool broadcast /* = false */
   )
{
   return my->propose_dividend_asset_update( proposing_account, expiration_time, changed_fees, broadcast );
}

signed_transaction wallet_api::approve_proposal(
   const string& fee_paying_account,
   const string& proposal_id,
   const approval_delta& delta,
   bool broadcast /* = false */
   )
{
   return my->approve_proposal( fee_paying_account, proposal_id, delta, broadcast );
}

global_property_object wallet_api::get_global_properties() const
{
   return my->get_global_properties();
}

dynamic_global_property_object wallet_api::get_dynamic_global_properties() const
{
   return my->get_dynamic_global_properties();
}

signed_transaction wallet_api::add_transaction_signature( signed_transaction tx,
                                                          bool broadcast )
{
   return my->add_transaction_signature( tx, broadcast );
}

signed_transaction wallet_api::create_personal_data(
      const string& subject_account,
      const string& operator_account,
      const string& url,
      const string& hash,
      const string& storage_data,
      bool broadcast )
{ 
   return my->create_personal_data( subject_account, operator_account, url, hash, storage_data, broadcast );
}

signed_transaction wallet_api::remove_personal_data(
      const string& subject_account,
      const string& operator_account,
      const string& hash,
      bool broadcast )
{
   return my->remove_personal_data(subject_account, operator_account, hash, broadcast );
}

std::vector<personal_data_object> wallet_api::get_personal_data(
      const string& subject_account,
      const string& operator_account) const
{
   return my->get_personal_data(subject_account, operator_account);
}

personal_data_object wallet_api::get_last_personal_data(
      const string& subject_account,
      const string& operator_account) const
{
   return my->get_last_personal_data(subject_account, operator_account);
}

signed_transaction wallet_api::create_content_card(
      const string& subject_account,
      const string& hash,
      const string& url,
      const string& type,
      const string& description,
      const string& content_key,
      const string& storage_data,
      const string& room,
      bool broadcast ) const
{
   return my->create_content_card(subject_account, hash, url, type, description, content_key, storage_data, room, broadcast);
}

signed_transaction wallet_api::update_content_card(
      const string& subject_account,
      const string& hash,
      const string& url,
      const string& type,
      const string& description,
      const string& content_key,
      const string& storage_data,
      const string& room,
      bool broadcast ) const
{
   return my->update_content_card(subject_account, hash, url, type, description, content_key, storage_data, room, broadcast);
}

signed_transaction wallet_api::remove_content_card( const string& subject_account,
      uint64_t content_id,
      bool broadcast ) const
{
   return my->remove_content_card(subject_account, content_id, broadcast);
}

signed_transaction wallet_api::create_permission(
      const string& subject_account,
      const string& operator_account,
      const string& permission_type,
      const string& object_id,
      const string& content_key,
      bool broadcast ) const
{
   return my->create_permission(subject_account, operator_account, permission_type, object_id, content_key, broadcast);
}
signed_transaction wallet_api::create_permission_many(
      const string& subject_account,
      const vector<permission_create_many_operation::permission_data>& permissions,
      bool broadcast ) const
{
   return my->create_permission_many(subject_account, permissions, broadcast);
}

signed_transaction wallet_api::remove_permission( const string& subject_account,
      uint64_t permission_id,
      bool broadcast ) const
{
   return my->remove_permission(subject_account, permission_id, broadcast);
}

content_card_object wallet_api::get_content_card_by_id( uint64_t content_id ) const
{
   return my->get_content_card_by_id(content_id);
}

std::vector<content_card_object> wallet_api::get_content_cards(
      const string& subject_account,
      uint64_t content_id,
      unsigned limit ) const
{
   return my->get_content_cards(subject_account, content_id, limit);
}
std::vector<content_card_object> wallet_api::get_content_cards_by_room(
      const string& room,
      uint64_t start_content,
      unsigned limit ) const
{
   return my->get_content_cards_by_room(room, start_content, limit);
}
permission_object wallet_api::get_permission_by_id( uint64_t permission_id ) const
{
   return my->get_permission_by_id(permission_id);
}

std::vector<permission_object> wallet_api::get_permissions(
      const string& operator_account,
      uint64_t permission_id,
      unsigned limit ) const
{
   return my->get_permissions(operator_account, permission_id, limit);
}
signed_transaction wallet_api::create_room(
      const string& owner,
      const string& name,
      const string& room_key,
      bool broadcast ) const
{
   return my->create_room(owner, name, room_key, broadcast);
}

signed_transaction wallet_api::update_room(
      const string& owner,
      const string& room,
      const string& name,
      bool broadcast ) const
{
   return my->update_room(owner, room, name, broadcast);
}

signed_transaction wallet_api::add_room_participant(
      const string& owner,
      const string& room,
      const string& participant,
      const string& content_key,
      bool broadcast ) const
{
   return my->add_room_participant(owner, room, participant, content_key, broadcast);
}

signed_transaction wallet_api::remove_room_participant(
      const string& owner,
      const string& room,
      const string& participant,
      bool broadcast ) const
{
   return my->remove_room_participant(owner, room, participant, broadcast);
}

room_object wallet_api::get_room_by_id( const string& room ) const
{
   return my->get_room_by_id(room);
}

std::vector<room_object> wallet_api::get_rooms_by_owner(
      const string& owner,
      uint64_t start_room,
      unsigned limit ) const
{
   return my->get_rooms_by_owner(owner, start_room, limit);
}

std::vector<room_participant_object> wallet_api::get_room_participants(
      const string& room,
      uint64_t start_participant,
      unsigned limit ) const
{
   return my->get_room_participants(room, start_participant, limit);
}

std::vector<room_participant_object> wallet_api::get_rooms_by_participant(
      const string& participant,
      uint64_t start_record,
      unsigned limit ) const
{
   return my->get_rooms_by_participant(participant, start_record, limit);
}

signed_transaction wallet_api::rotate_room_key(
      const string& owner,
      const string& room,
      const string& new_room_key,
      const flat_map<string, string>& participant_keys,
      bool broadcast ) const
{
   return my->rotate_room_key(owner, room, new_room_key, participant_keys, broadcast);
}

std::vector<room_key_epoch_object> wallet_api::get_room_key_epochs(
      const string& room,
      const string& participant,
      uint32_t limit ) const
{
   return my->get_room_key_epochs(room, participant, limit);
}

string wallet_api::help()const
{
   std::vector<std::string> method_names = my->method_documentation.get_method_names();
   std::stringstream ss;
   for (const std::string& method_name : method_names)
   {
      try
      {
         ss << my->method_documentation.get_brief_description(method_name);
      }
      catch (const fc::key_not_found_exception&)
      {
         ss << method_name << " (no help available)\n";
      }
   }
   return ss.str();
}

string wallet_api::gethelp(const string& method)const
{
   fc::api<wallet_api> tmp;
   std::stringstream ss;
   ss << "\n";

   std::string doxygenHelpString = my->method_documentation.get_detailed_description(method);
   if (!doxygenHelpString.empty())
      ss << doxygenHelpString << "\n";

   if( method == "import_key" )
   {
      ss << "usage: import_key ACCOUNT_NAME_OR_ID  WIF_PRIVATE_KEY\n\n";
      ss << "example: import_key \"1.3.11\" 5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3\n";
      ss << "example: import_key \"usera\" 5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3\n";
   }
   else if( method == "transfer" )
   {
      ss << "usage: transfer FROM TO AMOUNT SYMBOL \"memo\" BROADCAST\n\n";
      ss << "example: transfer \"1.3.11\" \"1.3.4\" 1000.03 CORE \"memo\" true\n";
      ss << "example: transfer \"usera\" \"userb\" 1000.123 CORE \"memo\" true\n";
   }
   else if( method == "create_account_with_brain_key" )
   {
      ss << "usage: create_account_with_brain_key BRAIN_KEY ACCOUNT_NAME REGISTRAR REFERRER BROADCAST\n\n";
      ss << "example: create_account_with_brain_key "
         << "\"my really long brain key\" \"newaccount\" \"1.3.11\" \"1.3.11\" true\n";
      ss << "example: create_account_with_brain_key "
         << "\"my really long brain key\" \"newaccount\" \"someaccount\" \"otheraccount\" true\n";
      ss << "\n";
      ss << "This method should be used if you would like the wallet to generate new keys derived from the "
         << "brain key.\n";
      ss << "The BRAIN_KEY will be used as the owner key, and the active key will be derived from the BRAIN_KEY.  "
         << "Use\n";
      ss << "register_account if you already know the keys you know the public keys that you would like to "
         << "register.\n";

   }
   else if( method == "register_account" )
   {
      ss << "usage: register_account ACCOUNT_NAME OWNER_PUBLIC_KEY ACTIVE_PUBLIC_KEY REGISTRAR "
         << "REFERRER REFERRER_PERCENT BROADCAST\n\n";
      ss << "example: register_account \"newaccount\" \"CORE6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV\" "
         << "\"CORE6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV\" \"1.3.11\" \"1.3.11\" 50 true\n";
      ss << "\n";
      ss << "Use this method to register an account for which you do not know the private keys.";
   }
   else if( method == "create_asset" )
   {
      ss << "usage: ISSUER SYMBOL PRECISION_DIGITS OPTIONS BITASSET_OPTIONS BROADCAST\n\n";
      ss << "PRECISION_DIGITS: the number of digits after the decimal point\n\n";
      ss << "Example value of OPTIONS: \n";
      ss << fc::json::to_pretty_string( graphene::chain::asset_options() );
      ss << "\nExample value of BITASSET_OPTIONS: \n";
      ss << fc::json::to_pretty_string( graphene::chain::bitasset_options() );
      ss << "\nBITASSET_OPTIONS may be null\n";
   }
   else if (doxygenHelpString.empty())
      ss << "No help defined for method " << method << "\n";

   return ss.str();
}

bool wallet_api::load_wallet_file( string wallet_filename )
{
   return my->load_wallet_file( wallet_filename );
}

void wallet_api::quit()
{
   my->quit();
}

void wallet_api::save_wallet_file( string wallet_filename )
{
   my->save_wallet_file( wallet_filename );
}

std::map<string,std::function<string(fc::variant,const fc::variants&)> >
wallet_api::get_result_formatters() const
{
   return my->get_result_formatters();
}

bool wallet_api::is_locked()const
{
   return my->is_locked();
}
bool wallet_api::is_new()const
{
   return my->_wallet.cipher_keys.size() == 0;
}

void wallet_api::encrypt_keys()
{
   my->encrypt_keys();
}

void wallet_api::lock()
{ try {
   FC_ASSERT( !is_locked() );
   encrypt_keys();
   for( auto key : my->_keys )
      key.second = key_to_wif(fc::ecc::private_key());
   my->_keys.clear();
   my->_checksum = fc::sha512();
   my->self.lock_changed(true);
} FC_CAPTURE_AND_RETHROW() }

void wallet_api::unlock(string password)
{ try {
   FC_ASSERT(password.size() > 0);
   auto pw = fc::sha512::hash(password.c_str(), password.size());
   vector<char> decrypted = fc::aes_decrypt(pw, my->_wallet.cipher_keys);
   auto pk = fc::raw::unpack<plain_keys>(decrypted);
   FC_ASSERT(pk.checksum == pw);
   my->_keys = std::move(pk.keys);
   my->_checksum = pk.checksum;
   my->self.lock_changed(false);
} FC_CAPTURE_AND_RETHROW() }

void wallet_api::set_password( string password )
{
   if( !is_new() )
      FC_ASSERT( !is_locked(), "The wallet must be unlocked before the password can be set" );
   my->_checksum = fc::sha512::hash( password.c_str(), password.size() );
   lock();
}

vector< signed_transaction > wallet_api::import_balance(
      string name_or_id,
      const vector<string>& wif_keys,
      bool broadcast )
{
   return my->import_balance( name_or_id, wif_keys, broadcast );
}

vector< signed_transaction > wallet_api::ico_import_balance(
      string account_name_or_id,
      string eth_pub_key,
      string eth_sign,
      bool broadcast )
{
   return my->ico_import_balance( account_name_or_id, eth_pub_key, eth_sign, broadcast );
}

map<public_key_type, string> wallet_api::dump_private_keys()
{
   FC_ASSERT(!is_locked());
   return my->_keys;
}

/*std::map<string,std::function<string(fc::variant,const fc::variants&)> > m;
m["list_core_accounts"] = [this](variant result, const fc::variants& a)
      {
         std::stringstream ss;

         auto balances = result.as<vector<account_balance_object>>( GRAPHENE_MAX_NESTED_OBJECTS );
         for (const account_balance_object& balance: balances)
         {
             const account_object& account = get_account(balance.owner);
             //ss << account.name << " " <<  std::string(balance.id) << " " << balance.balance.value << "\n";
             ss << account.name << " " <<  std::string(balance.id) << " " << get_asset(balance.asset_type).amount_to_pretty_string(balance.balance) << "\n";
         }

   return ss.str();
   return m;
};*/
   


signed_transaction wallet_api::update_account_keys(string name,
                                          public_key_type old_owner,
                                          public_key_type new_owner,
                                          public_key_type old_active,
                                          public_key_type new_active,
                                          bool broadcast )
{
   return my->update_account_keys(name, old_owner, new_owner, old_active, new_active, broadcast);
}

signed_transaction wallet_api::update_account_auth_key(string account_name,
                                                       authority_type type,
                                                       public_key_type key,
                                                       weight_type weight,
                                                       bool broadcast)
{
   return my->update_account_auth_key(account_name, type, key, weight, broadcast);
}

signed_transaction wallet_api::upgrade_account( string name, bool broadcast )
{
   return my->upgrade_account(name,broadcast);
}

signed_transaction wallet_api::sell_asset(string seller_account,
                                          string amount_to_sell,
                                          string symbol_to_sell,
                                          string min_to_receive,
                                          string symbol_to_receive,
                                          uint32_t expiration,
                                          bool   fill_or_kill,
                                          bool   broadcast)
{
   return my->sell_asset(seller_account, amount_to_sell, symbol_to_sell, min_to_receive,
                         symbol_to_receive, expiration, fill_or_kill, broadcast);
}

signed_transaction wallet_api::cancel_order(object_id_type order_id, bool broadcast)
{
   FC_ASSERT(!is_locked());
   return my->cancel_order(order_id, broadcast);
}

memo_data wallet_api::sign_memo(string from, string to, string memo)
{
   FC_ASSERT(!is_locked());
   return my->sign_memo(from, to, memo);
}

string wallet_api::read_memo(const memo_data& memo)
{
   FC_ASSERT(!is_locked());
   return my->read_memo(memo);
}

signed_message wallet_api::sign_message(string signer, string message)
{
   FC_ASSERT(!is_locked());
   return my->sign_message(signer, message);
}

bool wallet_api::verify_message( string message, string account, int block, const string& time, compact_signature sig )
{
   return my->verify_message( message, account, block, time, sig );
}

/** Verify a message signed with sign_message
 *
 * @param message the signed_message structure containing message, meta data and signature
 * @return true if signature matches
 */
bool wallet_api::verify_signed_message( signed_message message )
{
   return my->verify_signed_message( message );
}

// signed_transaction wallet_api::add_sidechain_address(string account,
//                                           sidechain_type sidechain,
//                                           string deposit_public_key,
//                                           string deposit_address,
//                                           string withdraw_public_key,
//                                           string withdraw_address,
//                                           bool broadcast /* = false */)
// {
//    return my->add_sidechain_address(account, sidechain, deposit_public_key, deposit_address, withdraw_public_key, withdraw_address, broadcast);
// }

// signed_transaction wallet_api::delete_sidechain_address(string account,
//                                           sidechain_type sidechain,
//                                           bool broadcast /* = false */)
// {
//    return my->delete_sidechain_address(account, sidechain, broadcast);
// }

// vector<optional<sidechain_address_object>> wallet_api::get_sidechain_addresses_by_account(string account)
// {
//    account_id_type account_id = get_account_id(account);
//    return my->_remote_db->get_sidechain_addresses_by_account(account_id);
// }

// vector<optional<sidechain_address_object>> wallet_api::get_sidechain_addresses_by_sidechain(sidechain_type sidechain)
// {
//    return my->_remote_db->get_sidechain_addresses_by_sidechain(sidechain);
// }

// fc::optional<sidechain_address_object> wallet_api::get_sidechain_address_by_account_and_sidechain(string account, sidechain_type sidechain)
// {
//    account_id_type account_id = get_account_id(account);
//    return my->_remote_db->get_sidechain_address_by_account_and_sidechain(account_id, sidechain);
// }

// uint64_t wallet_api::get_sidechain_addresses_count()
// {
//    return my->_remote_db->get_sidechain_addresses_count();
// }

/** Verify a message signed with sign_message, in its encapsulated form.
 *
 * @param message the complete encapsulated message string including separators and line feeds
 * @return true if signature matches
 */
bool wallet_api::verify_encapsulated_message( string message )
{
   return my->verify_encapsulated_message( message );
}

string wallet_api::get_private_key( public_key_type pubkey )const
{
   return key_to_wif( my->get_private_key( pubkey ) );
}

public_key_type  wallet_api::get_public_key( string label )const
{
   try { return fc::variant(label, 1).as<public_key_type>( 1 ); } catch ( ... ){}

   auto key_itr   = my->_wallet.labeled_keys.get<by_label>().find(label);
   if( key_itr != my->_wallet.labeled_keys.get<by_label>().end() )
      return key_itr->key;
   return public_key_type();
}
order_book wallet_api::get_order_book( const string& base, const string& quote, unsigned limit )
{
   return( my->_remote_db->get_order_book( base, quote, limit ) );
}
// default ctor necessary for FC_REFLECT

order_book wallet_api::get_order_book( const string& base, const string& quote, unsigned limit )
{
   return( my->_remote_db->get_order_book( base, quote, limit ) );
}

asset wallet_api::get_total_matched_bet_amount_for_betting_market_group(betting_market_group_id_type group_id)
{
    return( my->_remote_bookie->get_total_matched_bet_amount_for_betting_market_group(group_id) );
}

std::vector<event_object> wallet_api::get_events_containing_sub_string(const std::string& sub_string, const std::string& language)
{
    return( my->_remote_bookie->get_events_containing_sub_string(sub_string, language) );
}

binned_order_book wallet_api::get_binned_order_book(graphene::chain::betting_market_id_type betting_market_id, int32_t precision)
{
    return( my->_remote_bookie->get_binned_order_book(betting_market_id, precision) );
}

std::vector<matched_bet_object> wallet_api::get_matched_bets_for_bettor(account_id_type bettor_id) const
{
    return( my->_remote_bookie->get_matched_bets_for_bettor(bettor_id) );
}

std::vector<matched_bet_object> wallet_api::get_all_matched_bets_for_bettor(account_id_type bettor_id, bet_id_type start, unsigned limit) const
{
    return( my->_remote_bookie->get_all_matched_bets_for_bettor(bettor_id, start, limit) );
}

// custom operations
signed_transaction wallet_api::account_store_map(string account, string catalog, bool remove,
      flat_map<string, optional<string>> key_values, bool broadcast)
{
   return my->account_store_map(account, catalog, remove, key_values, broadcast);
}

vector<account_storage_object> wallet_api::get_account_storage(string account, string catalog)
{ try {
   return my->_custom_operations->get_storage_info(account, catalog);
} FC_CAPTURE_AND_RETHROW( (account)(catalog) ) }

signed_block_with_info::signed_block_with_info( const signed_block& block )
   : signed_block( block )
{
   block_id = id();
   signing_key = signee();
   transaction_ids.reserve( transactions.size() );
   for( const processed_transaction& tx : transactions )
      transaction_ids.push_back( tx.id() );
}

vesting_balance_object_with_info::vesting_balance_object_with_info(
      const vesting_balance_object& vbo,
      fc::time_point_sec now )
   : vesting_balance_object( vbo )
{
   allowed_withdraw = get_allowed_withdraw( now );
   allowed_withdraw_time = now;
}

} } // graphene::wallet

namespace fc {
   void to_variant( const account_multi_index_type& accts, variant& vo, uint32_t max_depth )
   {
      to_variant( std::vector<account_object>(accts.begin(), accts.end()), vo, max_depth );
   }

   void from_variant( const variant& var, account_multi_index_type& vo, uint32_t max_depth )
   {
      const std::vector<account_object>& v = var.as<std::vector<account_object>>( max_depth );
      vo = account_multi_index_type(v.begin(), v.end());
   }
}
