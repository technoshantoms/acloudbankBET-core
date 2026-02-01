

#pragma once

#include <string>

#include <graphene/chain/types.hpp>

#include <graphene/protocol/rock_paper_scissors.hpp>

#include <fc/crypto/sha256.hpp>
#include <fc/reflect/reflect.hpp>
#include <fc/optional.hpp>
#include <fc/static_variant.hpp>

namespace graphene { namespace chain {
   struct rock_paper_scissors_game_details
   {
      // note: I wanted to declare these as fixed arrays, but they don't serialize properly
      //fc::array<fc::optional<rock_paper_scissors_throw_commit>, 2> commit_moves;
      //fc::array<fc::optional<rock_paper_scissors_throw_reveal>, 2> reveal_moves;
      std::vector<fc::optional<rock_paper_scissors_throw_commit> > commit_moves;
      std::vector<fc::optional<rock_paper_scissors_throw_reveal> > reveal_moves;
      rock_paper_scissors_game_details() :
         commit_moves(2),
         reveal_moves(2)
      {
      }
   };

   typedef fc::static_variant<rock_paper_scissors_game_details> game_specific_details;
} }

FC_REFLECT( graphene::chain::rock_paper_scissors_game_details,
            (commit_moves)(reveal_moves) )
FC_REFLECT_TYPENAME( graphene::chain::game_specific_details )

