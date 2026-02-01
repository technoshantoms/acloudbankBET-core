/*
 *AcloudBank
 */
#pragma once

#include <graphene/app/plugin.hpp>
#include <graphene/chain/database.hpp>

namespace graphene { namespace es_objects {

using namespace chain;

namespace detail
{
    class es_objects_plugin_impl;
}

class es_objects_plugin : public graphene::app::plugin
{
   public:
      explicit es_objects_plugin(graphene::app::application& app);
      ~es_objects_plugin() override;

      std::string plugin_name()const override;
      std::string plugin_description()const override;
      void plugin_set_program_options(
         boost::program_options::options_description& cli,
         boost::program_options::options_description& cfg) override;
      void plugin_initialize(const boost::program_options::variables_map& options) override;
      void plugin_startup() override;

   private:
      std::unique_ptr<detail::es_objects_plugin_impl> my;
};

struct adaptor_struct {
    fc::mutable_variant_object adapt(const variant_object &obj) {
      fc::mutable_variant_object o(obj);
      vector<string> keys_to_rename;
      for (auto i = o.begin(); i != o.end(); ++i) {
         auto &element = (*i).value();
         if (element.is_object()) {
            const string &name = (*i).key();
            auto &vo = element.get_object();
            if (vo.contains(name.c_str()))
               keys_to_rename.emplace_back(name);
            element = adapt(vo);
         } else if (element.is_array())
            adapt(element.get_array());
      }
      for (const auto &i : keys_to_rename) {
         string new_name = i + "_";
         o[new_name] = variant(o[i]);
         o.erase(i);
      }
      if (o.find("owner") != o.end() && o["owner"].is_string())
      {
         o["owner_"] = o["owner"].as_string();
         o.erase("owner");
      }
      if (o.find("active_special_authority") != o.end())
      {
         o["active_special_authority"] = fc::json::to_string(o["active_special_authority"]);
      }
      if (o.find("owner_special_authority") != o.end())
      {
         o["owner_special_authority"] = fc::json::to_string(o["owner_special_authority"]);
      }
      if (o.find("feeds") != o.end())
      {
         o["feeds"] = fc::json::to_string(o["feeds"]);
      }
      if (o.find("operations") != o.end())
      {
         o["operations"] = fc::json::to_string(o["operations"]);
      }
      
      return o;
   }

   void adapt(fc::variants &v) {
      for (auto &array_element : v) {
         if (array_element.is_object())
            array_element = adapt(array_element.get_object());
         else if (array_element.is_array())
            adapt(array_element.get_array());
         else
            array_element = array_element.as_string();
      }
   }
};

} } //graphene::es_objects
