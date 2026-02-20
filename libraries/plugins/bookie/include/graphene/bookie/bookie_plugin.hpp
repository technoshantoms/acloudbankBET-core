
#pragma once

#include <graphene/chain/database.hpp>
#include <graphene/chain/event_object.hpp>

#include <graphene/app/plugin.hpp>

#include <fc/thread/future.hpp>

namespace graphene { namespace bookie {
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
enum spaces {
   bookie_objects = 6
};

namespace detail
{
   class bookie_plugin_impl;
}

class bookie_plugin : public graphene::app::plugin
{
   public:
      //bookie_plugin();
      //virtual ~bookie_plugin();

      std::string plugin_name()const override;
      virtual void plugin_set_program_options(boost::program_options::options_description& cli,
                                              boost::program_options::options_description& cfg) override;
      virtual void plugin_initialize(const boost::program_options::variables_map& options) override;
      virtual void plugin_startup() override;

      flat_set<account_id_type> tracked_accounts()const;
      asset get_total_matched_bet_amount_for_betting_market_group(betting_market_group_id_type group_id);
      std::vector<event_object> get_events_containing_sub_string(const std::string& sub_string, const std::string& language);

      friend class detail::bookie_plugin_impl;
      std::unique_ptr<detail::bookie_plugin_impl> my;
};

} } //graphene::bookie

