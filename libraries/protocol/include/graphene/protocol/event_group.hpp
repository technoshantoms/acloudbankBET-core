
#pragma once

#include <graphene/protocol/types.hpp>
#include <graphene/protocol/base.hpp>
#include <graphene/protocol/asset.hpp>

namespace graphene { namespace protocol {

struct event_group_create_operation : public base_operation
{
   struct fee_parameters_type { uint64_t fee = GRAPHENE_BLOCKCHAIN_PRECISION; };
   asset             fee;

   /**
    * The name of the event_group
    */
   internationalized_string_type name;

   /**
    * This can be a sport_id_type, or a
    * relative object id that resolves to a sport_id_type
    */
   object_id_type sport_id;

   extensions_type   extensions;

   account_id_type fee_payer()const { return GRAPHENE_WITNESS_ACCOUNT; }
   void            validate()const;
};

struct event_group_update_operation : public base_operation
{
   struct fee_parameters_type { uint64_t fee = GRAPHENE_BLOCKCHAIN_PRECISION; };
   asset             fee;

   event_group_id_type event_group_id;

    /**
    * This can be a sport_id_type, or a
    * relative object id that resolves to a sport_id_type
    */
   optional<object_id_type> new_sport_id;

   optional<internationalized_string_type> new_name;

   extensions_type   extensions;

   account_id_type fee_payer()const { return GRAPHENE_WITNESS_ACCOUNT; }
   void            validate()const;
};

struct event_group_delete_operation : public base_operation
{
   struct fee_parameters_type { uint64_t fee = GRAPHENE_BLOCKCHAIN_PRECISION; };
   asset             fee;
    
   event_group_id_type event_group_id;
    
   extensions_type   extensions;
   
   account_id_type fee_payer()const { return GRAPHENE_WITNESS_ACCOUNT; }
   void            validate()const;
};

} }

FC_REFLECT( graphene::protocol::event_group_create_operation::fee_parameters_type, (fee) )
FC_REFLECT( graphene::protocol::event_group_create_operation,
            (fee)(name)(sport_id)(extensions) )

FC_REFLECT( graphene::protocol::event_group_update_operation::fee_parameters_type, (fee) )
FC_REFLECT( graphene::protocol::event_group_update_operation,
            (fee)(new_sport_id)(new_name)(event_group_id)(extensions) )

FC_REFLECT( graphene::protocol::event_group_delete_operation::fee_parameters_type, (fee) )
FC_REFLECT( graphene::protocol::event_group_delete_operation,
            (fee)(event_group_id)(extensions) )
