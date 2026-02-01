
#pragma once

#include <graphene/protocol/operations.hpp>
#include <graphene/chain/evaluator.hpp>
#include <graphene/chain/database.hpp>

namespace graphene { namespace chain {

   class sport_object;
    
   class sport_create_evaluator : public fee_handling_evaluator<sport_create_evaluator>
   {
      public:
         typedef sport_create_operation operation_type;

         void_result do_evaluate( const sport_create_operation& o );
         object_id_type do_apply( const sport_create_operation& o );
   };

   class sport_update_evaluator : public fee_handling_evaluator<sport_update_evaluator>
   {
      public:
         typedef sport_update_operation operation_type;

         void_result do_evaluate( const sport_update_operation& o );
         void_result do_apply( const sport_update_operation& o );
   };

   class sport_delete_evaluator : public fee_handling_evaluator<sport_delete_evaluator>
   {
   public:
       typedef sport_delete_operation operation_type;
       
       void_result do_evaluate( const sport_delete_operation& o );
       void_result do_apply( const sport_delete_operation& o );
       
   private:
       const sport_object* _sport = nullptr;
   };
    
} } // graphene::chain
