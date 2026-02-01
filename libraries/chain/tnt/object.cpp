/*
 *AcloudBank
 */
#include <graphene/protocol/asset.hpp>
#include <graphene/chain/tnt/object.hpp>

#include <fc/io/raw.hpp>

namespace graphene { namespace chain {

void tank_object::clear_tap_state(tnt::index_type tap_ID) {
   tnt::tap_id_type id;
   id.tap_id = tap_ID;
#if BOOST_VERSION >= 106800
   auto range = accessory_states.equal_range(id);
   accessory_states.erase(range.first, range.second);
#else
   // Workaround for outdated boost with non-C++14-compliant flat_map
   using std_map_type =
      std::map<stateful_accessory_address, tnt::tank_accessory_state,
               tnt::accessory_address_lt<stateful_accessory_address>>;
   std_map_type map_copy(std::make_move_iterator(accessory_states.begin()),
                         std::make_move_iterator(accessory_states.end()));
   accessory_states.clear();
   auto range = map_copy.equal_range(id);
   map_copy.erase(range.first, range.second);
   accessory_states.insert(std::make_move_iterator(map_copy.begin()), std::make_move_iterator(map_copy.end()));
#endif
}

void tank_object::clear_attachment_state(tnt::index_type attachment_ID) {
   // Type is ignored by accessory_states key search, so just choose an arbitrary type for the search
   accessory_states.erase(tnt::tank_accessory_address<tnt::asset_flow_meter>{attachment_ID});
}


} } // namespace graphene::chain

GRAPHENE_IMPLEMENT_EXTERNAL_SERIALIZATION(graphene::chain::tank_object)
