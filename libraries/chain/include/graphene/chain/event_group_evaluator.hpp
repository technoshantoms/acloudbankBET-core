
#pragma once

#include <graphene/protocol/operations.hpp>
#include <graphene/chain/evaluator.hpp>
#include <graphene/chain/database.hpp>

namespace graphene { namespace chain {
    
   class event_group_object;

   class event_group_create_evaluator : public evaluator<event_group_create_evaluator>
   {
      public:
         typedef event_group_create_operation operation_type;

         void_result do_evaluate( const event_group_create_operation& o );
         object_id_type do_apply( const event_group_create_operation& o );

      private:
         sport_id_type sport_id;
   };

   class event_group_update_evaluator : public evaluator<event_group_update_evaluator>
   {
      public:
         typedef event_group_update_operation operation_type;

         void_result do_evaluate( const event_group_update_operation& o );
         void_result do_apply( const event_group_update_operation& o );

      private:
         sport_id_type sport_id;
   };
    
   class event_group_delete_evaluator : public evaluator<event_group_delete_evaluator>
   {
   public:
       typedef event_group_delete_operation operation_type;
       
       void_result do_evaluate( const event_group_delete_operation& o );
       void_result do_apply( const event_group_delete_operation& o );
       
   private:
       const event_group_object* _event_group = nullptr;
   };
} } // graphene::chain
