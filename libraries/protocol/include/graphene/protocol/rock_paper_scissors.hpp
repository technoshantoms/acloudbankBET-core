
#pragma once

#include <cassert>
#include <cstdint>
#include <string>

#include <fc/container/flat.hpp>
#include <fc/reflect/reflect.hpp>
#include <fc/crypto/sha256.hpp>

namespace graphene { namespace protocol {

   struct rock_paper_scissors_game_options
   {
      /// If true and a user fails to commit their move before the time_per_commit_move expires, 
      /// the blockchain will randomly choose a move for the user
      bool insurance_enabled;
      /// The number of seconds users are given to commit their next move, counted from the beginning
      /// of the hand (during the game, a hand begins immediately on the block containing the 
      /// second player's reveal or where the time_per_reveal move has expired).
      /// Note, if these times aren't an even multiple of the block interval, they will be rounded
      /// up.
      uint32_t time_per_commit_move;

      /// The number of seconds users are given to reveal their move, counted from the time of the
      /// block containing the second commit or the where the time_per_commit_move expired
      uint32_t time_per_reveal_move;

      /// The number of allowed gestures, must be either 3 or 5.  If 3, the game is
      /// standard rock-paper-scissors, if 5, it's 
      /// rock-paper-scissors-lizard-spock.
      uint8_t number_of_gestures;
   };

   enum class rock_paper_scissors_gesture
   {
      rock,
      paper,
      scissors,
      spock,
      lizard
   };

   struct rock_paper_scissors_throw
   {
      uint64_t nonce1;
      uint64_t nonce2;
      rock_paper_scissors_gesture gesture;
      // fc::sha256 calculate_hash() const {
      //    std::vector<char> full_throw_packed(fc::raw::pack(*this));
      //    return fc::sha256::hash(full_throw_packed.data(), full_throw_packed.size());
      // }
   };

   struct rock_paper_scissors_throw_commit
   {
      uint64_t nonce1;
      fc::sha256 throw_hash;
      bool operator<(const graphene::protocol::rock_paper_scissors_throw_commit& rhs) const
      {
         return std::tie(nonce1, throw_hash) < std::tie(rhs.nonce1, rhs.throw_hash);
      }
   };

   

   struct rock_paper_scissors_throw_reveal
   {
      uint64_t nonce2;
      rock_paper_scissors_gesture gesture;
   };

} }

FC_REFLECT( graphene::protocol::rock_paper_scissors_game_options, (insurance_enabled)(time_per_commit_move)(time_per_reveal_move)(number_of_gestures) )

// FC_REFLECT_TYPENAME( graphene::protocol::rock_paper_scissors_gesture)
FC_REFLECT_ENUM( graphene::protocol::rock_paper_scissors_gesture,
                 (rock)
                 (paper)
                 (scissors)
                 (spock)
                 (lizard))

FC_REFLECT( graphene::protocol::rock_paper_scissors_throw,
            (nonce1)
            (nonce2)
            (gesture) )

FC_REFLECT( graphene::protocol::rock_paper_scissors_throw_commit,
            (nonce1)
            (throw_hash) )

FC_REFLECT( graphene::protocol::rock_paper_scissors_throw_reveal,
            (nonce2)(gesture) )

