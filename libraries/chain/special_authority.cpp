

#include <graphene/chain/special_authority.hpp>
#include <graphene/chain/database.hpp>

#include <fc/io/raw.hpp>

namespace graphene { namespace chain {

struct special_authority_validate_visitor
{
   typedef void result_type;

   void operator()( const no_special_authority& a ) {}

   void operator()( const top_holders_special_authority& a )
   {
      FC_ASSERT( a.num_top_holders > 0 );
   }
};

void validate_special_authority( const special_authority& a )
{
   special_authority_validate_visitor vtor;
   a.visit( vtor );
}

struct special_authority_evaluate_visitor
{
   typedef void result_type;

   special_authority_evaluate_visitor( const database& d ) : db(d) {}

   void operator()( const no_special_authority& a ) {}

   void operator()( const top_holders_special_authority& a )
   {
      a.asset(db);     // require asset to exist
   }

   const database& db;
};

void evaluate_special_authority( const database& db, const special_authority& a )
{
   special_authority_evaluate_visitor vtor( db );
   a.visit( vtor );
}

} } // graphene::chain


GRAPHENE_EXTERNAL_SERIALIZATION( /*not extern*/, graphene::chain::top_holders_special_authority )
