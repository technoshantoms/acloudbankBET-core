/*
 *AcloudBank
 */

#include <graphene/chain/database.hpp>
#include <graphene/chain/hardfork.hpp>

namespace graphene
{
   namespace chain
   {

      uint64_t database::get_commit_reveal_seed(const vector<account_id_type> &accounts) const
      {
         const auto &cr_idx = get_index_type<commit_reveal_index>();
         const auto &by_op_idx = cr_idx.indices().get<by_account>();

         uint64_t seed = 0;
         uint32_t maintenance_time = get_dynamic_global_properties().next_maintenance_time.sec_since_epoch();
         uint32_t prev_maintenance_time = maintenance_time - get_global_properties().parameters.maintenance_interval;
         for (const auto &acc : accounts)
         {
            auto itr = by_op_idx.lower_bound(acc);
            if (itr != by_op_idx.end() && itr->account == acc && prev_maintenance_time <= itr->maintenance_time && itr->maintenance_time < maintenance_time)
            {
               seed += itr->value;
            }
         }
         return seed;
      }

      vector<account_id_type> database::filter_commit_reveal_participant(const vector<account_id_type> &accounts) const
      {
         const auto &cr_idx = get_index_type<commit_reveal_index>();
         const auto &by_op_idx = cr_idx.indices().get<by_account>();

         vector<account_id_type> result;
         uint32_t maintenance_time = get_dynamic_global_properties().next_maintenance_time.sec_since_epoch();
         uint32_t prev_maintenance_time = maintenance_time - get_global_properties().parameters.maintenance_interval;
         for (const auto &acc : accounts)
         {
            auto itr = by_op_idx.lower_bound(acc);
            if (itr != by_op_idx.end() && itr->account == acc && itr->value != 0 && prev_maintenance_time <= itr->maintenance_time && itr->maintenance_time < maintenance_time)

            {
               result.push_back(itr->account);
            }
         }
         return result;
      }

   }
}
