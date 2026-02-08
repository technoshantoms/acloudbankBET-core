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
 *s
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
#pragma once

#include <graphene/protocol/types.hpp>

namespace graphene { namespace chain { using namespace protocol; } }

/// Object types in the Implementation Space (enum impl_object_type (2.x.x))
GRAPHENE_DEFINE_IDS(chain, implementation_ids, impl_,
                    /* 2.0.x  */ (global_property)
                    /* 2.1.x  */ (dynamic_global_property)
                    /* 2.2.x  */ (reserved0) // unused, but can not be simply deleted due to API compatibility
                    /* 2.3.x  */ (asset_dynamic_data)
                    /* 2.4.x  */ (asset_bitasset_data)
                    /* 2.5.x  */ (account_balance)
                    /* 2.6.x  */ (account_statistics)
                    /* 2.7.x  */ (transaction_history)
                    /* 2.8.x  */ (block_summary)
                    /* 2.9.x  */ (reserved1/*account_transaction_history, moved to api_ids*/)
                    /* 2.10.x */ (chain_property)
                    /* 2.11.x */ (witness_schedule)
                    /* 2.12.x */ (budget_record)
                    /* 2.13.x */ (special_authority)
                    /* 2.14.x */ (buyback)
                    /* 2.15.x */ (fba_accumulator)
                    (asset_dividend_data)
                    (pending_dividend_payout_balance_for_holder)
                    (total_distributed_dividend_balance)
                    (betting_market_position)
                    (global_betting_statistics)
                    /* 2.16.x */ (lottery_balance)
                    /* 2.17.x */ (nft_lottery_balance)
                    /* 2.18.x */(sweeps_vesting_balance)
                    /* 2.19.x */(offer_history)
                   )
//satia: please fix me!
GRAPHENE_DEFINE_IDS(chain, api_ids, api_,
                   (operation_history)
                   (account_transaction_history))