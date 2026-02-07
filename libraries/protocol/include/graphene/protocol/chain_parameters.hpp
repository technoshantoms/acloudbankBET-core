/*
 * AcloudBank
 *
 */
#pragma once

#include <memory>
#include <graphene/protocol/base.hpp>
#include <graphene/protocol/types.hpp>
#include <graphene/protocol/tnt/parameters.hpp>
#include <graphene/protocol/config.hpp>

namespace graphene { namespace protocol {
   struct fee_schedule;

   struct htlc_options
   {
      uint32_t max_timeout_secs;
      uint32_t max_preimage_size;
   };

   struct custom_authority_options_type
   {
      uint32_t max_custom_authority_lifetime_seconds = GRAPHENE_DEFAULT_MAX_CUSTOM_AUTHORITY_LIFETIME_SECONDS;
      uint32_t max_custom_authorities_per_account = GRAPHENE_DEFAULT_MAX_CUSTOM_AUTHORITIES_PER_ACCOUNT;
      uint32_t max_custom_authorities_per_account_op = GRAPHENE_DEFAULT_MAX_CUSTOM_AUTHORITIES_PER_ACCOUNT_OP;
      uint32_t max_custom_authority_restrictions = GRAPHENE_DEFAULT_MAX_CUSTOM_AUTHORITY_RESTRICTIONS;
   };

struct parameter_extension
   {
      optional< bet_multiplier_type > min_bet_multiplier;
      optional< bet_multiplier_type > max_bet_multiplier;
      optional< uint16_t >            betting_rake_fee_percentage;
      optional< flat_map<bet_multiplier_type, bet_multiplier_type> > permitted_betting_odds_increments;
      optional< uint16_t >            live_betting_delay_time;

      optional< uint16_t >            sweeps_distribution_percentage    = SWEEPS_DEFAULT_DISTRIBUTION_PERCENTAGE;
      optional< asset_id_type >       sweeps_distribution_asset         = SWEEPS_DEFAULT_DISTRIBUTION_ASSET;
      optional< account_id_type >     sweeps_vesting_accumulator_account= SWEEPS_ACCUMULATOR_ACCOUNT;
   /* rbac parameters */
      optional < uint16_t >           rbac_max_permissions_per_account    = RBAC_MAX_PERMISSIONS_PER_ACCOUNT;
      optional < uint32_t >           rbac_max_account_authority_lifetime = RBAC_MAX_ACCOUNT_AUTHORITY_LIFETIME;
      optional < uint16_t >           rbac_max_authorities_per_permission = RBAC_MAX_AUTHS_PER_PERMISSION;
      /* Account Roles - Permissions Parameters */
      optional < uint16_t >           account_roles_max_per_account    = ACCOUNT_ROLES_MAX_PER_ACCOUNT;
      optional < uint32_t >           account_roles_max_lifetime       = ACCOUNT_ROLES_MAX_LIFETIME;

      optional < asset_id_type >      btc_asset                         = asset_id_type();
      optional < asset_id_type >      hbd_asset                         = asset_id_type();
      optional < asset_id_type >      hive_asset                        = asset_id_type();
   };
   struct chain_parameters
   {
      /** using a shared_ptr breaks the circular dependency created between operations and the fee schedule */
      std::shared_ptr<const fee_schedule> current_fees;                  ///< current schedule of fees
      const fee_schedule& get_current_fees() const { FC_ASSERT(current_fees); return *current_fees; }
      fee_schedule& get_mutable_fees() { FC_ASSERT(current_fees); return const_cast<fee_schedule&>(*current_fees); }

      uint8_t                 block_interval                      = GRAPHENE_DEFAULT_BLOCK_INTERVAL; ///< interval in seconds between blocks
      uint32_t                maintenance_interval                = GRAPHENE_DEFAULT_MAINTENANCE_INTERVAL; ///< interval in sections between blockchain maintenance events
      uint8_t                 maintenance_skip_slots              = GRAPHENE_DEFAULT_MAINTENANCE_SKIP_SLOTS; ///< number of block_intervals to skip at maintenance time
      uint32_t                committee_proposal_review_period    = GRAPHENE_DEFAULT_COMMITTEE_PROPOSAL_REVIEW_PERIOD_SEC; ///< minimum time in seconds that a proposed transaction requiring committee authority may not be signed, prior to expiration
      uint32_t                maximum_transaction_size            = GRAPHENE_DEFAULT_MAX_TRANSACTION_SIZE; ///< maximum allowable size in bytes for a transaction
      uint32_t                maximum_block_size                  = GRAPHENE_DEFAULT_MAX_BLOCK_SIZE; ///< maximum allowable size in bytes for a block
      uint32_t                maximum_time_until_expiration       = GRAPHENE_DEFAULT_MAX_TIME_UNTIL_EXPIRATION; ///< maximum lifetime in seconds for transactions to be valid, before expiring
      uint32_t                maximum_proposal_lifetime           = GRAPHENE_DEFAULT_MAX_PROPOSAL_LIFETIME_SEC; ///< maximum lifetime in seconds for proposed transactions to be kept, before expiring
      uint8_t                 maximum_asset_whitelist_authorities = GRAPHENE_DEFAULT_MAX_ASSET_WHITELIST_AUTHORITIES; ///< maximum number of accounts which an asset may list as authorities for its whitelist OR blacklist
      uint8_t                 maximum_asset_feed_publishers       = GRAPHENE_DEFAULT_MAX_ASSET_FEED_PUBLISHERS; ///< the maximum number of feed publishers for a given asset
      uint16_t                maximum_witness_count               = GRAPHENE_DEFAULT_MAX_WITNESSES; ///< maximum number of active witnesses
      uint16_t                maximum_committee_count             = GRAPHENE_DEFAULT_MAX_COMMITTEE; ///< maximum number of active committee_members
      uint16_t                maximum_authority_membership        = GRAPHENE_DEFAULT_MAX_AUTHORITY_MEMBERSHIP; ///< largest number of keys/accounts an authority can have
      uint16_t                network_percent_of_fee              = GRAPHENE_DEFAULT_NETWORK_PERCENT_OF_FEE; ///< percent of transaction fees paid to network
      uint16_t                lifetime_referrer_percent_of_fee    = GRAPHENE_DEFAULT_LIFETIME_REFERRER_PERCENT_OF_FEE; ///< percent of transaction fees paid to network
      uint32_t                cashback_vesting_period_seconds     = GRAPHENE_DEFAULT_CASHBACK_VESTING_PERIOD_SEC; ///< time after cashback rewards are accrued before they become liquid
      share_type              cashback_vesting_threshold          = GRAPHENE_DEFAULT_CASHBACK_VESTING_THRESHOLD; ///< the maximum cashback that can be received without vesting
      bool                    count_non_member_votes              = true; ///< set to false to restrict voting privlegages to member accounts
      bool                    allow_non_member_whitelists         = false; ///< true if non-member accounts may set whitelists and blacklists; false otherwise
      share_type              witness_pay_per_block               = GRAPHENE_DEFAULT_WITNESS_PAY_PER_BLOCK; ///< CORE to be allocated to witnesses (per block)
      uint32_t                witness_pay_vesting_seconds         = GRAPHENE_DEFAULT_WITNESS_PAY_VESTING_SECONDS; ///< vesting_seconds parameter for witness VBO's
      bool                    worker_budget                       = GRAPHENE_DEFAULT_WORKER_BUDGET_PER_DAY; ///< Enabling the working proposal mechanism
      uint16_t                max_predicate_opcode                = GRAPHENE_DEFAULT_MAX_ASSERT_OPCODE; ///< predicate_opcode must be less than this number
      uint16_t                accounts_per_fee_scale              = GRAPHENE_DEFAULT_ACCOUNTS_PER_FEE_SCALE; ///< number of accounts between fee scalings
      uint8_t                 account_fee_scale_bitshifts         = GRAPHENE_DEFAULT_ACCOUNT_FEE_SCALE_BITSHIFTS; ///< number of times to left bitshift account registration fee at each scaling
      uint8_t                 max_authority_depth                 = GRAPHENE_MAX_SIG_CHECK_DEPTH;
      uint8_t                 rsquared_witnesses_top_max            = RSQUARED_WITNESSES_TOP_MAX; ///< limit witnesses top list to max 63 (consensus algorithm)
      uint8_t                 rsquared_witnesses_active_max         = RSQUARED_WITNESSES_ACTIVE_MAX; ///< randomly choose max 21 active witnesses (consensus algorithm)

      /* rps tournament parameters constraints */
      uint32_t                min_round_delay                     = TOURNAMENT_MIN_ROUND_DELAY; ///< miniaml delay between games
      uint32_t                max_round_delay                     = TOURNAMENT_MAX_ROUND_DELAY; ///< maximal delay between games
      uint32_t                min_time_per_commit_move            = TOURNAMENT_MIN_TIME_PER_COMMIT_MOVE; ///< minimal time to commit the next move
      uint32_t                max_time_per_commit_move            = TOURNAMENT_MAN_TIME_PER_COMMIT_MOVE; ///< maximal time to commit the next move
      uint32_t                min_time_per_reveal_move            = TOURNAMENT_MIN_TIME_PER_REVEAL_MOVE; ///< minimal time to reveal move
      uint32_t                max_time_per_reveal_move            = TOURNAMENT_MAX_TIME_PER_REVEAL_MOVE; ///< maximal time to reveal move
      uint16_t                rake_fee_percentage                 = TOURNAMENT_DEFAULT_RAKE_FEE_PERCENTAGE; ///< part of prize paid into the dividend account for the core token holders
      uint32_t                maximum_registration_deadline       = TOURNAMENT_MAXIMAL_REGISTRATION_DEADLINE; ///< value registration deadline must be before
      uint16_t                maximum_players_in_tournament       = TOURNAMENT_MAX_PLAYERS_NUMBER; ///< maximal count of players in tournament
      uint16_t                maximum_tournament_whitelist_length = TOURNAMENT_MAX_WHITELIST_LENGTH; ///< maximal tournament whitelist length
      uint32_t                maximum_tournament_start_time_in_future = TOURNAMENT_MAX_START_TIME_IN_FUTURE;
      uint32_t                maximum_tournament_start_delay      = TOURNAMENT_MAX_START_DELAY;
      uint16_t                maximum_tournament_number_of_wins   = TOURNAMENT_MAX_NUMBER_OF_WINS;

      struct ext
      {
         optional< htlc_options > updatable_htlc_options;
         optional< custom_authority_options_type > custom_authority_options;
         optional< tnt::parameters_type > updatable_tnt_options;
         optional< parameter_extension > updatable_nft_options;
         optional< uint16_t > market_fee_network_percent;
         optional< uint16_t > maker_fee_discount_percent;
         optional< uint16_t > electoral_threshold;
      };

      extension<ext> extensions;

       /** defined in fee_schedule.cpp */
      void validate()const;
      
      chain_parameters();
      chain_parameters(const chain_parameters& other);
      chain_parameters(chain_parameters&& other);
      chain_parameters& operator=(const chain_parameters& other);
      chain_parameters& operator=(chain_parameters&& other);

      /// If @ref market_fee_network_percent is valid, return the value it contains, otherwise return 0
      uint16_t get_market_fee_network_percent() const;

      /// If @ref maker_fee_discount_percent is valid, return the value it contains, otherwise return 0
      uint16_t get_maker_fee_discount_percent() const;

      /// If @ref electoral_threshold is valid, return the value it contains, otherwise return 0
      uint16_t get_electoral_threshold() const;

    inline uint16_t sweeps_distribution_percentage()const {
         return  SWEEPS_DEFAULT_DISTRIBUTION_PERCENTAGE;
      }
      inline asset_id_type sweeps_distribution_asset()const {
         return  SWEEPS_DEFAULT_DISTRIBUTION_ASSET;
      }
      inline account_id_type sweeps_vesting_accumulator_account()const {
         return  SWEEPS_ACCUMULATOR_ACCOUNT;
      }
      inline uint16_t rbac_max_permissions_per_account()const {
         return  RBAC_MAX_PERMISSIONS_PER_ACCOUNT;
      }
      inline uint32_t rbac_max_account_authority_lifetime()const {
         return  RBAC_MAX_ACCOUNT_AUTHORITY_LIFETIME;
      }
      inline uint16_t rbac_max_authorities_per_permission()const {
         return  RBAC_MAX_AUTHS_PER_PERMISSION;
      }
      inline uint16_t account_roles_max_per_account()const {
         return  ACCOUNT_ROLES_MAX_PER_ACCOUNT;
      }
      inline uint32_t account_roles_max_lifetime()const {
         return  ACCOUNT_ROLES_MAX_LIFETIME;
      }

      inline bet_multiplier_type min_bet_multiplier()const {
         return  GRAPHENE_DEFAULT_MIN_BET_MULTIPLIER;
      }
      inline bet_multiplier_type max_bet_multiplier()const {
         return GRAPHENE_DEFAULT_MAX_BET_MULTIPLIER;
      }
      inline uint16_t betting_rake_fee_percentage()const {
         return  GRAPHENE_DEFAULT_RAKE_FEE_PERCENTAGE;
      }
      inline const flat_map<bet_multiplier_type, bet_multiplier_type>& permitted_betting_odds_increments()const {
         static const flat_map<bet_multiplier_type, bet_multiplier_type> _default = GRAPHENE_DEFAULT_PERMITTED_BETTING_ODDS_INCREMENTS;
         return  _default;
      }
      inline uint16_t live_betting_delay_time()const {
         return  GRAPHENE_DEFAULT_LIVE_BETTING_DELAY_TIME;
      }

      /*inline uint16_t sweeps_distribution_percentage()const {
         return extensions.value.sweeps_distribution_percentage.valid() ? *extensions.value.sweeps_distribution_percentage : SWEEPS_DEFAULT_DISTRIBUTION_PERCENTAGE;
      }
      inline asset_id_type sweeps_distribution_asset()const {
         return extensions.value.sweeps_distribution_asset.valid() ? *extensions.value.sweeps_distribution_asset : SWEEPS_DEFAULT_DISTRIBUTION_ASSET;
      }
      inline account_id_type sweeps_vesting_accumulator_account()const {
         return extensions.value.sweeps_vesting_accumulator_account.valid() ? *extensions.value.sweeps_vesting_accumulator_account : SWEEPS_ACCUMULATOR_ACCOUNT;
      }
      inline uint16_t rbac_max_permissions_per_account()const {
         return extensions.value.rbac_max_permissions_per_account.valid() ? *extensions.value.rbac_max_permissions_per_account : RBAC_MAX_PERMISSIONS_PER_ACCOUNT;
      }
      inline uint32_t rbac_max_account_authority_lifetime()const {
         return extensions.value.rbac_max_account_authority_lifetime.valid() ? *extensions.value.rbac_max_account_authority_lifetime : RBAC_MAX_ACCOUNT_AUTHORITY_LIFETIME;
      }
      inline uint16_t rbac_max_authorities_per_permission()const {
         return extensions.value.rbac_max_authorities_per_permission.valid() ? *extensions.value.rbac_max_authorities_per_permission : RBAC_MAX_AUTHS_PER_PERMISSION;
      }
      inline uint16_t account_roles_max_per_account()const {
         return extensions.value.account_roles_max_per_account.valid() ? *extensions.value.account_roles_max_per_account : ACCOUNT_ROLES_MAX_PER_ACCOUNT;
      }
      inline uint32_t account_roles_max_lifetime()const {
         return extensions.value.account_roles_max_lifetime.valid() ? *extensions.value.account_roles_max_lifetime : ACCOUNT_ROLES_MAX_LIFETIME;
      }*/
      
      private:
      static void safe_copy(chain_parameters& to, const chain_parameters& from);
   };

} }  // graphene::protocol

FC_REFLECT( graphene::protocol::htlc_options,
      (max_timeout_secs)
      (max_preimage_size)
)

FC_REFLECT( graphene::protocol::custom_authority_options_type,
      (max_custom_authority_lifetime_seconds)
      (max_custom_authorities_per_account)
      (max_custom_authorities_per_account_op)
      (max_custom_authority_restrictions)
)

FC_REFLECT( graphene::protocol::parameter_extension,
   (min_bet_multiplier)
   (max_bet_multiplier)
   (betting_rake_fee_percentage)
   (live_betting_delay_time)
   (rbac_max_permissions_per_account)
   (rbac_max_account_authority_lifetime)
   (rbac_max_authorities_per_permission)
   (account_roles_max_per_account)
   (account_roles_max_lifetime)
   (sweeps_distribution_percentage)
   (sweeps_distribution_asset)
   (sweeps_vesting_accumulator_account)
)

FC_REFLECT( graphene::protocol::chain_parameters::ext,
      (updatable_htlc_options)
      (custom_authority_options)
      (updatable_tnt_options)
      (updatable_nft_options)
      (market_fee_network_percent)
      (maker_fee_discount_percent)
      (electoral_threshold)
)

FC_REFLECT( graphene::protocol::chain_parameters,
            (current_fees)
            (block_interval)
            (maintenance_interval)
            (maintenance_skip_slots)
            (committee_proposal_review_period)
            (maximum_transaction_size)
            (maximum_block_size)
            (maximum_time_until_expiration)
            (maximum_proposal_lifetime)
            (maximum_asset_whitelist_authorities)
            (maximum_asset_feed_publishers)
            (maximum_witness_count)
            (maximum_committee_count)
            (maximum_authority_membership)
            (network_percent_of_fee)
            (lifetime_referrer_percent_of_fee)
            (cashback_vesting_period_seconds)
            (cashback_vesting_threshold)
            (count_non_member_votes)
            (allow_non_member_whitelists)
            (witness_pay_per_block)
            (worker_budget)
            (max_predicate_opcode)
            (accounts_per_fee_scale)
            (account_fee_scale_bitshifts)
            (max_authority_depth)
            (min_round_delay)
            (max_round_delay)
            (min_time_per_commit_move)
            (max_time_per_commit_move)
            (min_time_per_reveal_move)
            (max_time_per_reveal_move)
            (rake_fee_percentage)
            (maximum_players_in_tournament)
            (maximum_tournament_whitelist_length)
            (maximum_tournament_start_time_in_future)
            (maximum_tournament_start_delay)
            (maximum_tournament_number_of_wins)
            (rsquared_witnesses_top_max)
            (rsquared_witnesses_active_max)
            (extensions)
          )

GRAPHENE_DECLARE_EXTERNAL_SERIALIZATION( graphene::protocol::chain_parameters )
