#pragma once
/*
 * Copyright (c) 2015 Cryptonomex, Inc., and contributors.
 * Copyright (c) 2020-2023 Revolution Populi Limited, and contributors.
 * ... (License)
 */
#include <graphene/protocol/operations.hpp>
#include <graphene/chain/hardfork.hpp>
#include <fc/reflect/typelist.hpp>
#include <fc/reflect/typelist.hpp>
#include <type_traits>
#include <functional>

namespace graphene { namespace chain {
using namespace protocol;
namespace TL {
using namespace fc::typelist;
}

struct hardfork_visitor {
    using tag_type = int64_t;
    using result_type = bool;
    using first_unforked_op = custom_authority_create_operation;
    using BSIP_40_ops = TL::list<custom_authority_create_operation, custom_authority_update_operation, custom_authority_delete_operation>;
    using TNT_ops = TL::list<tank_create_operation, tank_update_operation, tank_delete_operation, tank_query_operation, tap_open_operation, tap_connect_operation, account_fund_connection_operation, connection_fund_account_operation>;
    using ticket_ops = TL::list<ticket_create_operation, ticket_update_operation>;
    using ico_ops = TL::list<ico_balance_claim_operation>;
    using nft_ops = TL::list<custom_permission_create_operation, custom_permission_update_operation, custom_permission_delete_operation, custom_account_authority_create_operation, custom_account_authority_update_operation,custom_account_authority_delete_operation, offer_operation,bid_operation,cancel_offer_operation,finalize_offer_operation, nft_metadata_create_operation,nft_metadata_update_operation,nft_mint_operation, nft_safe_transfer_from_operation,nft_approve_operation,nft_set_approval_for_all_operation, account_role_create_operation,account_role_update_operation,account_role_delete_operation, nft_lottery_token_purchase_operation,nft_lottery_reward_operation,nft_lottery_end_operation >;

    fc::time_point_sec now;
    hardfork_visitor(fc::time_point_sec now) : now(now) {}

    // --- FIX 1: Explicitly handle cases outside special lists ---
    template<typename Op>
    std::enable_if_t<!TL::contains<BSIP_40_ops, Op>() &&
                     !TL::contains<TNT_ops, Op>() &&
                     !TL::contains<ticket_ops, Op>() &&
                     !TL::contains<ico_ops, Op>() &&
                     !TL::contains<nft_ops, Op>() &&
                     !(operation::tag<Op>::value < operation::tag<first_unforked_op>::value), bool>
    visit() {
        // Default behavior for newer/unknown operations: Assume not hardforked
        // or add custom logic here.
        return true; 
    }

    // --- Original visitor implementations ---
    template<typename Op>
    std::enable_if_t<operation::tag<Op>::value < operation::tag<first_unforked_op>::value, bool> 
    visit() { return true; }

    template<typename Op>
    std::enable_if_t<TL::contains<BSIP_40_ops, Op>(), bool> 
    visit() { return HARDFORK_BSIP_40_PASSED(now); }

    template<typename Op>
    std::enable_if_t<TL::contains<TNT_ops, Op>(), bool> 
    visit() { return HARDFORK_BSIP_72_PASSED(now); }

    template<typename Op>
    std::enable_if_t<TL::contains<ticket_ops, Op>(), bool> 
    visit() { return true; }

    template<typename Op>
    std::enable_if_t<TL::contains<ico_ops, Op>(), bool> 
    visit() { return true; }

    template<typename Op>
    std::enable_if_t<TL::contains<nft_ops, Op>(), bool> 
    visit() { return true; }

    // --- Adaptors ---
    template<class W, class Op=typename W::type>
    bool operator()(W) { return visit<Op>(); }

    template<class Op>
    bool operator()(const Op& op) { return visit<Op>(); }

    bool visit(operation::tag_type tag) {
    // Cast the tag to int64_t to allow for safety checking
     TL::runtime::dispatch(operation::list(), static_cast<int64_t>(tag), *this);
     return ;
    }
    bool visit(const operation& op) {
        return op.visit(*this);
    }
};

} } // namespace graphene::chain
