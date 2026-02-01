/*
 *AcloudBank
 */
#pragma once

#include <graphene/chain/evaluator.hpp>
#include <graphene/chain/tnt/cow_db_wrapper.hpp>
#include <graphene/chain/tnt/query_evaluator.hpp>

#include <graphene/protocol/tnt/operations.hpp>

namespace graphene { namespace chain {
class tank_object;

class tank_create_evaluator : public evaluator<tank_create_evaluator> {
public:
   using operation_type = tank_create_operation;

   void_result do_evaluate(const operation_type& o);
   object_id_type do_apply(const operation_type&);

private:
   ptnt::tank_schematic new_tank;
};

class tank_update_evaluator : public evaluator<tank_update_evaluator> {
public:
   using operation_type = tank_update_operation;

   void_result do_evaluate(const operation_type& o);
   void_result do_apply(const operation_type& o);

private:
   const tank_object* old_tank = nullptr;
   fc::optional<ptnt::index_type> new_restrictor_ID;
   ptnt::tank_schematic updated_tank;
};

class tank_delete_evaluator : public evaluator<tank_delete_evaluator> {
public:
   using operation_type = tank_delete_operation;

   void_result do_evaluate(const operation_type& o);
   void_result do_apply(const operation_type& o);

private:
   const tank_object* old_tank;
};

class tank_query_evaluator : public evaluator<tank_query_evaluator> {
public:
   using operation_type = tank_query_operation;

   void_result do_evaluate(const operation_type& o);
   void_result do_apply(const operation_type&);

private:
   const tank_object* query_tank = nullptr;
   tnt::query_evaluator evaluator;
};

class tap_open_evaluator : public evaluator<tap_open_evaluator> {
public:
   using operation_type = tap_open_operation;

   void_result do_evaluate(const operation_type& o);
   void_result do_apply(const operation_type&);

private:
   const tank_object* tank = nullptr;
   std::unique_ptr<cow_db_wrapper> db_wrapper = nullptr;
   std::list<connection_fund_account_operation> accounts_to_pay;
   bool delete_tank = false;
};

class tap_connect_evaluator : public evaluator<tap_connect_evaluator> {
public:
   using operation_type = tap_connect_operation;

   void_result do_evaluate(const operation_type& o);
   void_result do_apply(const operation_type&);

private:
   const tank_object* tank = nullptr;
};

class account_fund_connection_evaluator : public evaluator<account_fund_connection_evaluator> {
public:
   using operation_type = account_fund_connection_operation;

   void_result do_evaluate(const operation_type& o);
   void_result do_apply(const operation_type&);

private:
   std::unique_ptr<cow_db_wrapper> db_wrapper = nullptr;
   std::list<connection_fund_account_operation> accounts_to_pay;
};
} } // namespace graphene::chain
