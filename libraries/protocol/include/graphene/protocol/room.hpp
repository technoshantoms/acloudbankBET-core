/*
 * acloudbank
 */

#pragma once

#include <graphene/protocol/base.hpp>
#include <graphene/protocol/asset.hpp>
#include <graphene/protocol/authority.hpp>

namespace graphene { namespace protocol {

   /**
    * @brief Create a room (encrypted thread)
    *
    * This operation creates a new room with an encrypted room_key.
    * The owner is automatically added as the first participant.
    */
   struct room_create_operation : public base_operation
   {
      struct fee_parameters_type {
         uint64_t fee = 20 * GRAPHENE_BLOCKCHAIN_PRECISION;
      };

      asset           fee;
      account_id_type owner;      // Room owner
      string          name;       // Room name (max 256 chars)
      string          room_key;   // Encrypted room key (for owner)

      account_id_type fee_payer()const { return owner; }
      void            validate()const;
      share_type      calculate_fee(const fee_parameters_type& )const;

      void get_required_active_authorities( flat_set<account_id_type>& a )const
      {
         a.insert( owner );
      }
   };

   /**
    * @brief Update room name (owner only)
    *
    * This operation allows the owner to change the room name.
    */
   struct room_update_operation : public base_operation
   {
      struct fee_parameters_type {
         uint64_t fee = 5 * GRAPHENE_BLOCKCHAIN_PRECISION;
      };

      asset           fee;
      account_id_type owner;
      room_id_type    room;
      string          name;       // New room name

      account_id_type fee_payer()const { return owner; }
      void            validate()const;
      share_type      calculate_fee(const fee_parameters_type& )const;

      void get_required_active_authorities( flat_set<account_id_type>& a )const
      {
         a.insert( owner );
      }
   };

   /**
    * @brief Add participant to room
    *
    * This operation adds a new participant to the room with their encrypted content_key.
    * Only the room owner can add participants.
    */
   struct room_add_participant_operation : public base_operation
   {
      struct fee_parameters_type {
         uint64_t fee = 5 * GRAPHENE_BLOCKCHAIN_PRECISION;
         uint32_t price_per_kbyte = 10 * GRAPHENE_BLOCKCHAIN_PRECISION;
      };

      asset           fee;
      account_id_type owner;           // Only owner can add participants
      room_id_type    room;
      account_id_type participant;     // Participant to add
      string          content_key;     // Room key encrypted for participant
      flat_map<uint32_t, string>  epoch_keys;  // Optional: historical epoch keys encrypted for participant

      account_id_type fee_payer()const { return owner; }
      void            validate()const;
      share_type      calculate_fee(const fee_parameters_type& )const;

      void get_required_active_authorities( flat_set<account_id_type>& a )const
      {
         a.insert( owner );
      }
   };

   /**
    * @brief Remove participant from room
    *
    * This operation removes a participant from the room.
    * Only the room owner can remove participants.
    * The owner cannot be removed from the room.
    */
   struct room_remove_participant_operation : public base_operation
   {
      struct fee_parameters_type {
         uint64_t fee = GRAPHENE_BLOCKCHAIN_PRECISION;
      };

      asset                       fee;
      account_id_type             owner;           // Only owner can remove
      room_participant_id_type    participant_id;  // Participant object to remove

      account_id_type fee_payer()const { return owner; }
      void            validate()const;
      share_type      calculate_fee(const fee_parameters_type& )const;

      void get_required_active_authorities( flat_set<account_id_type>& a )const
      {
         a.insert( owner );
      }
   };

   /**
    * @brief Rotate room key (create new epoch)
    *
    * This operation rotates the room key, creating a new epoch.
    * All current participants receive the new key.
    * Only the room owner can rotate the key.
    */
   struct room_rotate_key_operation : public base_operation
   {
      struct fee_parameters_type {
         uint64_t fee = 10 * GRAPHENE_BLOCKCHAIN_PRECISION;
         uint32_t price_per_kbyte = 10 * GRAPHENE_BLOCKCHAIN_PRECISION;
      };

      asset                                fee;
      account_id_type                      owner;
      room_id_type                         room;
      string                               new_room_key;      // New room key encrypted for owner
      flat_map<account_id_type, string>    participant_keys;   // New key encrypted for each remaining participant

      account_id_type fee_payer()const { return owner; }
      void            validate()const;
      share_type      calculate_fee(const fee_parameters_type& )const;

      void get_required_active_authorities( flat_set<account_id_type>& a )const
      {
         a.insert( owner );
      }
   };

} } // graphene::protocol

FC_REFLECT( graphene::protocol::room_create_operation::fee_parameters_type, (fee) )
FC_REFLECT( graphene::protocol::room_create_operation,
            (fee)(owner)(name)(room_key)
          )

FC_REFLECT( graphene::protocol::room_update_operation::fee_parameters_type, (fee) )
FC_REFLECT( graphene::protocol::room_update_operation,
            (fee)(owner)(room)(name)
          )

FC_REFLECT( graphene::protocol::room_add_participant_operation::fee_parameters_type, (fee)(price_per_kbyte) )
FC_REFLECT( graphene::protocol::room_add_participant_operation,
            (fee)(owner)(room)(participant)(content_key)(epoch_keys)
          )

FC_REFLECT( graphene::protocol::room_remove_participant_operation::fee_parameters_type, (fee) )
FC_REFLECT( graphene::protocol::room_remove_participant_operation,
            (fee)(owner)(participant_id)
          )

FC_REFLECT( graphene::protocol::room_rotate_key_operation::fee_parameters_type, (fee)(price_per_kbyte) )
FC_REFLECT( graphene::protocol::room_rotate_key_operation,
            (fee)(owner)(room)(new_room_key)(participant_keys)
          )

GRAPHENE_DECLARE_EXTERNAL_SERIALIZATION( graphene::protocol::room_create_operation )
GRAPHENE_DECLARE_EXTERNAL_SERIALIZATION( graphene::protocol::room_update_operation )
GRAPHENE_DECLARE_EXTERNAL_SERIALIZATION( graphene::protocol::room_add_participant_operation )
GRAPHENE_DECLARE_EXTERNAL_SERIALIZATION( graphene::protocol::room_remove_participant_operation )
GRAPHENE_DECLARE_EXTERNAL_SERIALIZATION( graphene::protocol::room_rotate_key_operation )
