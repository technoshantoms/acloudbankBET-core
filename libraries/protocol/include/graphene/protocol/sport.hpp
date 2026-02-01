
#pragma once

#include <graphene/protocol/types.hpp>
#include <graphene/protocol/base.hpp>

namespace graphene { namespace protocol {

struct sport_create_operation : public base_operation
{
   struct fee_parameters_type { uint64_t fee = GRAPHENE_BLOCKCHAIN_PRECISION; };
   asset             fee;

   /**
    * The name of the sport
    */
   internationalized_string_type name;

   extensions_type   extensions;

   account_id_type fee_payer()const { return GRAPHENE_WITNESS_ACCOUNT; }
   void            validate()const;
};

struct sport_update_operation : public base_operation
{
   struct fee_parameters_type { uint64_t fee = GRAPHENE_BLOCKCHAIN_PRECISION; };
   asset             fee;

   sport_id_type   sport_id;

   optional<internationalized_string_type> new_name;

   extensions_type   extensions;

   account_id_type fee_payer()const { return GRAPHENE_WITNESS_ACCOUNT; }
   void            validate()const;
};
    
struct sport_delete_operation : public base_operation
{
    struct fee_parameters_type { uint64_t fee = GRAPHENE_BLOCKCHAIN_PRECISION; };
    asset             fee;
    
    sport_id_type   sport_id;
    
    extensions_type   extensions;
    
    account_id_type fee_payer()const { return GRAPHENE_WITNESS_ACCOUNT; }
    void            validate()const;
};

} }

FC_REFLECT( graphene::protocol::sport_create_operation::fee_parameters_type, (fee) )
FC_REFLECT( graphene::protocol::sport_create_operation,
            (fee)(name)(extensions) )

FC_REFLECT( graphene::protocol::sport_update_operation::fee_parameters_type, (fee) )
FC_REFLECT( graphene::protocol::sport_update_operation,
            (fee)(sport_id)(new_name)(extensions) )

FC_REFLECT( graphene::protocol::sport_delete_operation::fee_parameters_type, (fee) )
FC_REFLECT( graphene::protocol::sport_delete_operation,
            (fee)(sport_id)(extensions) )
