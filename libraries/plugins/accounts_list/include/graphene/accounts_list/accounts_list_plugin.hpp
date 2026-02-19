#pragma once

#include <graphene/app/plugin.hpp>
#include <graphene/chain/database.hpp>
#include <graphene/chain/account_object.hpp>

#include <fc/thread/future.hpp>

namespace graphene { namespace accounts_list {
using namespace chain;

namespace detail
{
    class accounts_list_plugin_impl;
}

class accounts_list_plugin : public graphene::app::plugin
{
   public:
     // accounts_list_plugin();
     // virtual ~accounts_list_plugin();

      std::string plugin_name()const override;
      virtual void plugin_set_program_options(
         boost::program_options::options_description& cli,
         boost::program_options::options_description& cfg) override;
      virtual void plugin_initialize(const boost::program_options::variables_map& options) override;
      virtual void plugin_startup() override;

      vector<account_balance_object>list_accounts()const;

      friend class detail::accounts_list_plugin_impl;
      std::unique_ptr<detail::accounts_list_plugin_impl> my;
};

} } //graphene::accounts_list

