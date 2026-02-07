
#pragma once

#include <graphene/chain/evaluator.hpp>
#include <graphene/chain/database.hpp>
#include <graphene/chain/event_object.hpp>

#include <graphene/protocol/operations.hpp>

namespace graphene { namespace chain {

   class event_create_evaluator : public evaluator<event_create_evaluator>
   {
      public:
         typedef event_create_operation operation_type;

         void_result do_evaluate( const event_create_operation& o );
         object_id_type do_apply( const event_create_operation& o );
      private:
         event_group_id_type event_group_id;
   };

   class event_update_evaluator : public evaluator<event_update_evaluator>
   {
      public:
         typedef event_update_operation operation_type;

         void_result do_evaluate( const event_update_operation& o );
         void_result do_apply( const event_update_operation& o );
      private:
         event_group_id_type event_group_id;
   };

   class event_update_status_evaluator : public evaluator<event_update_status_evaluator>
   {
      public:
         typedef event_update_status_operation operation_type;

         void_result do_evaluate( const event_update_status_operation& o );
         void_result do_apply( const event_update_status_operation& o );
      private:
         const event_object* _event_to_update = nullptr;
   };

} } // graphene::chain
