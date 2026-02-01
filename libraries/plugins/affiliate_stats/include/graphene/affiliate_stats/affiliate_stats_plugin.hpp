#pragma once

#include <graphene/app/plugin.hpp>
#include <graphene/chain/database.hpp>

#include <fc/thread/future.hpp>

namespace graphene { namespace affiliate_stats {
   using namespace chain;

//
// Plugins should #define their SPACE_ID's so plugins with
// conflicting SPACE_ID assignments can be compiled into the
// same binary (by simply re-assigning some of the conflicting #defined
// SPACE_ID's in a build script).
//
// Assignment of SPACE_ID's cannot be done at run-time because
// various template automagic depends on them being known at compile
// time.
//
#ifndef AFFILIATE_STATS_SPACE_ID
#define AFFILIATE_STATS_SPACE_ID 7
#endif

namespace detail
{
    class affiliate_stats_plugin_impl;
}

class affiliate_stats_plugin : public graphene::app::plugin
{
   public:
      affiliate_stats_plugin();
      virtual ~affiliate_stats_plugin();

      std::string plugin_name()const override;
      virtual void plugin_set_program_options(
         boost::program_options::options_description& cli,
         boost::program_options::options_description& cfg) override;
      virtual void plugin_initialize(const boost::program_options::variables_map& options) override;
      virtual void plugin_startup() override;

      const std::set<graphene::chain::operation_history_id_type>& get_reward_history( account_id_type& affiliate )const;

      friend class detail::affiliate_stats_plugin_impl;
      std::unique_ptr<detail::affiliate_stats_plugin_impl> my;
};

} } //graphene::affiliate_stats
