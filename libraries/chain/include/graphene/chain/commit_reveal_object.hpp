/*
 *AcloudBank
 */

#pragma once

#include <graphene/chain/types.hpp>
#include <graphene/db/generic_index.hpp>
#include <graphene/protocol/account.hpp>

#include <boost/multi_index/composite_key.hpp>

namespace graphene { namespace chain {
        class database;

        /**
         * @brief This class represents an commit-reveal data on the object graph
         * @ingroup object
         * @ingroup protocol
         *
         * Commit-reveal data the primary unit to give and store commit-reveal object.
         */
        class commit_reveal_object : public graphene::db::abstract_object<commit_reveal_object,protocol_ids,commit_reveal_object_type>
        {
        public:

            account_id_type account;
            string          hash;
            uint64_t        value;
            uint32_t        maintenance_time;
        };

        struct by_account;

        typedef multi_index_container<
               commit_reveal_object,
               indexed_by<
                     ordered_unique< tag<by_id>, member< object, object_id_type, &object::id > >,
                     ordered_unique< tag<by_account>,
                           composite_key< commit_reveal_object,
                                 member< commit_reveal_object, account_id_type, &commit_reveal_object::account>
                           >
                     >
               >
        > commit_reveal_multi_index_type;

        typedef generic_index<commit_reveal_object, commit_reveal_multi_index_type> commit_reveal_index;

    }}

MAP_OBJECT_ID_TO_TYPE(graphene::chain::commit_reveal_object)
FC_REFLECT_TYPENAME( graphene::chain::commit_reveal_object )
GRAPHENE_DECLARE_EXTERNAL_SERIALIZATION( graphene::chain::commit_reveal_object )
