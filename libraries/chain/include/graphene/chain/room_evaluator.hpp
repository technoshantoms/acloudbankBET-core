/*
 * acloudbank
 */

#pragma once

#include <graphene/chain/evaluator.hpp>
#include <graphene/chain/room_object.hpp>

namespace graphene { namespace chain {

class room_create_evaluator : public evaluator<room_create_evaluator>
{
public:
   typedef room_create_operation operation_type;

   void_result do_evaluate( const room_create_operation& o );
   object_id_type do_apply( const room_create_operation& o );
};

class room_update_evaluator : public evaluator<room_update_evaluator>
{
public:
   typedef room_update_operation operation_type;

   void_result do_evaluate( const room_update_operation& o );
   object_id_type do_apply( const room_update_operation& o );

private:
   const room_object* _room = nullptr;
};

class room_add_participant_evaluator : public evaluator<room_add_participant_evaluator>
{
public:
   typedef room_add_participant_operation operation_type;

   void_result do_evaluate( const room_add_participant_operation& o );
   object_id_type do_apply( const room_add_participant_operation& o );

private:
   const room_object* _room = nullptr;
};

class room_remove_participant_evaluator : public evaluator<room_remove_participant_evaluator>
{
public:
   typedef room_remove_participant_operation operation_type;

   void_result do_evaluate( const room_remove_participant_operation& o );
   object_id_type do_apply( const room_remove_participant_operation& o );

private:
   const room_participant_object* _participant = nullptr;
   const room_object* _room = nullptr;
};

} } // graphene::chain
