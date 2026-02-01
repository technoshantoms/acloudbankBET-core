/*
 *AcloudBank
 */

#pragma once
#include <graphene/chain/evaluator.hpp>
#include <graphene/chain/commit_reveal_object.hpp>

namespace graphene { namespace chain {

class commit_create_evaluator : public evaluator<commit_create_evaluator>
{
public:
   typedef commit_create_operation operation_type;

   void_result do_evaluate( const commit_create_operation& o );
   object_id_type do_apply( const commit_create_operation& o ) ;
};

class reveal_create_evaluator : public evaluator<reveal_create_evaluator>
{
public:
   typedef reveal_create_operation operation_type;

   void_result do_evaluate( const reveal_create_operation& o );
   object_id_type do_apply( const reveal_create_operation& o ) ;
};

} } // graphene::chain
