/*
 * Copyright (c) 2015 Cryptonomex, Inc., and contributors.
 *
 * The MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#pragma once
#include <graphene/db/object.hpp>
#include <graphene/db/index.hpp>
#include <graphene/db/undo_database.hpp>
#include <graphene/db/safety_check_policy.hpp>

#include <fc/log/logger.hpp>

#include <map>

namespace graphene { namespace db {

   /**
    *   @class object_database
    *   @brief maintains a set of indexed objects that can be modified with multi-level rollback support
    */
   class object_database
   {
      public:
         object_database();
         ~object_database();

         void reset_indexes();

         void open(const fc::path& data_dir );

         /**
          * Saves the complete state of the object_database to disk, this could take a while
          */
         void flush();
         void wipe(const fc::path& data_dir); // remove from disk
         void close();

         /**
          * @brief Allocate an object space, setting a safety check policy for that object space
          * @param space_id The ID of the object space to allocate
          * @tparam SafetyCheckPolicy Specific type of the safety check policy for the object space
          * @tparam ConstructorArgs Variadic of arguments for the constructor of SafetyCheckPolicy
          * @return A non-owning pointer to the safety_check_policy object for the newly allocated object space
          *
          * Allocate a new object space in the database, setting the safety check policy for that object space. The
          * policy object's lifetime is managed by the database, and a non-owning pointer to the newly created policy
          * object is returned to the caller. The pointer will remain valid until the database is closed, or the
          * indexes are reset.
          */
         template<typename SafetyCheckPolicy, typename... ConstructorArgs>
         SafetyCheckPolicy* allocate_object_space(uint8_t space_id, ConstructorArgs&&... args) {
            FC_ASSERT(_safety_checks[space_id] == nullptr,
                      "Cannot allocate object space ${ID}: object space already allocated.", ("ID", space_id));
            _safety_checks[space_id] = std::make_unique<SafetyCheckPolicy>(std::forward<ConstructorArgs>(args)...);
            return static_cast<SafetyCheckPolicy*>(_safety_checks[space_id].get());
         }

         ///These methods are used to retrieve indexes on the object_database. All public index accessors are const-access only.
         /// @{
         template<typename IndexType>
         const IndexType& get_index_type()const {
            static_assert( std::is_base_of<index,IndexType>::value, "Type must be an index type" );
            return static_cast<const IndexType&>( get_index( IndexType::object_type::space_id, IndexType::object_type::type_id ) );
         }
         template<typename T>
         const index&  get_index()const { return get_index(T::space_id,T::type_id); }
         const index&  get_index(uint8_t space_id, uint8_t type_id)const;
         const index&  get_index(object_id_type id)const { return get_index(id.space(),id.type()); }
         template<typename T>
         const index* find_index()const { return find_index(T::space_id,T::type_id); }
         const index* find_index(object_id_type id)const { return find_index(id.space(), id.type()); }
         const index* find_index(uint8_t space_id, uint8_t type_id)const;
         /// @}

         const object& get_object( object_id_type id )const;
         const object* find_object( object_id_type id )const;

         /// These methods are mutators of the object_database. You must use these methods to make changes to the object_database,
         /// in order to maintain proper undo history.
         ///@{

         template<typename T, typename F>
         const T& create( F&& constructor )
         {
            auto& idx = get_mutable_index<T>();
            return static_cast<const T&>( idx.create( [&](object& o)
            {
               assert( dynamic_cast<T*>(&o) );
               constructor( static_cast<T&>(o) );
            } ));
         }

         const object& insert( object&& obj ) { return get_mutable_index(obj.id).insert( std::move(obj) ); }
         void          remove( const object& obj ) { get_mutable_index(obj.id).remove( obj ); }
         template<typename T, typename Lambda>
         void modify( const T& obj, const Lambda& m ) {
            get_mutable_index(obj.id).modify(obj,m);
         }

         ///@}

         template<typename T>
         static const T& cast( const object& obj )
         {
            assert( nullptr != dynamic_cast<const T*>(&obj) );
            return static_cast<const T&>(obj);
         }
         template<typename T>
         static T& cast( object& obj )
         {
            assert( nullptr != dynamic_cast<T*>(&obj) );
            return static_cast<T&>(obj);
         }

         template<typename T>
         const T& get( object_id_type id )const
         {
            const object& obj = get_object( id );
            assert( nullptr != dynamic_cast<const T*>(&obj) );
            return static_cast<const T&>(obj);
         }
         template<typename T>
         const T* find( object_id_type id )const
         {
            const object* obj = find_object( id );
            assert(  !obj || nullptr != dynamic_cast<const T*>(obj) );
            return static_cast<const T*>(obj);
         }

         template<uint8_t SpaceID, uint8_t TypeID>
         auto find( object_id<SpaceID,TypeID> id )const -> const object_downcast_t<decltype(id)>* {
             return find<object_downcast_t<decltype(id)>>(id);
         }

         template<uint8_t SpaceID, uint8_t TypeID>
         auto get( object_id<SpaceID,TypeID> id )const -> const object_downcast_t<decltype(id)>& {
             return get<object_downcast_t<decltype(id)>>(id);
         }

         template<typename IndexType>
         IndexType* add_index()
         {
            typedef typename IndexType::object_type ObjectType;
            uint8_t space_id = ObjectType::space_id;
            uint8_t type_id = ObjectType::type_id;
            if( _index[space_id].size() <= type_id )
                _index[space_id].resize( 255 );
            assert(!_index[space_id][type_id]);

            safety_check_policy* check = _safety_checks[space_id].get();
            FC_ASSERT(check != nullptr && check->allow_new_index(type_id),
                      "Safety Check: Addition of new index ${S}.${T} not allowed!",
                      ("S", space_id)("T", type_id));
            unique_ptr<index> indexptr( new IndexType(*this, *check) );
            _index[space_id][type_id] = std::move(indexptr);
            return static_cast<IndexType*>(_index[space_id][type_id].get());
         }

         template<typename IndexType, typename SecondaryIndexType, typename... Args>
         SecondaryIndexType* add_secondary_index( Args... args )
         {
            return get_mutable_index_type<IndexType>().template add_secondary_index<SecondaryIndexType, Args...>(args...);
         }
         template<typename SecondaryIndexType, typename PrimaryIndexType = typename SecondaryIndexType::watched_index>
         SecondaryIndexType* add_secondary_index()
         {
            uint8_t space_id = PrimaryIndexType::object_type::space_id;
            uint8_t type_id = PrimaryIndexType::object_type::type_id;
            auto new_index =
                    get_mutable_index_type<PrimaryIndexType>().template add_secondary_index<SecondaryIndexType>();
            safety_check_policy* check = _safety_checks[space_id].get();
            FC_ASSERT(check != nullptr &&
                      check->allow_new_secondary_index(type_id, *new_index),
                      "Safety Check: Addition of new secondary index on ${S}.${T} not allowed!",
                      ("S", space_id)("T", type_id));
            return new_index;
         }
         template<typename SecondaryIndexType>
         SecondaryIndexType* add_secondary_index(const uint8_t space_id, const uint8_t type_id)
         {
            auto* base_primary = dynamic_cast<base_primary_index*>(&get_mutable_index(space_id, type_id));
            FC_ASSERT(base_primary != nullptr, "Cannot add secondary index: index for space ID ${S} and type ID ${T} "
                                               "does not support secondary indexes.",
                      ("S", space_id)("T", type_id));

            auto new_index = base_primary->template add_secondary_index<SecondaryIndexType>();
            safety_check_policy* check = _safety_checks[space_id].get();
            FC_ASSERT(check != nullptr && check->allow_new_secondary_index(type_id, *new_index),
                      "Safety Check: Addition of new secondary index on ${S}.${T} not allowed!",
                      ("S", space_id)("T", type_id));
            return new_index;
         }

         template<typename PrimaryIndexType>
         void delete_secondary_index(const secondary_index& secondary) {
            uint8_t space_id = PrimaryIndexType::object_type::space_id;
            uint8_t type_id = PrimaryIndexType::object_type::type_id;
            safety_check_policy* check = _safety_checks[space_id].get();
            FC_ASSERT(check != nullptr &&
                      check->allow_delete_secondary_index(type_id, secondary),
                      "Safety Check: Deletion of secondary index on ${S}.${T} not allowed!",
                      ("S", space_id)("T", type_id));
            get_mutable_index_type<PrimaryIndexType>().delete_secondary_index(secondary);
         }
         void delete_secondary_index(const uint8_t space_id, const uint8_t type_id, const secondary_index& secondary) {
            safety_check_policy* check = _safety_checks[space_id].get();
            FC_ASSERT(check != nullptr && check->allow_delete_secondary_index(type_id, secondary),
                      "Safety Check: Deletion of secondary index on ${S}.${T} not allowed!",
                      ("S", space_id)("T", type_id));
            auto* base_primary = dynamic_cast<base_primary_index*>(&get_mutable_index(space_id, type_id));
            FC_ASSERT(base_primary != nullptr, "Cannot delete secondary index: index for space ID ${S} and type ID "
                                               "${T} does not support secondary indexes.",
                      ("S", space_id)("T", type_id));
            base_primary->delete_secondary_index(secondary);
         }

         // Inspect each index in an object space. F is a callable taking a const index&
         template<typename F>
         void inspect_all_indexes(uint8_t space_id, F&& f) const {
             FC_ASSERT(_index.size() > space_id, "Cannot inspect indexes in space ID ${ID}: no such object space",
                       ("ID", space_id));
             for (const auto& ptr : _index[space_id])
                 if (ptr != nullptr)
                     f((const index&)(*ptr));
         }

         void pop_undo();

         fc::path get_data_dir()const { return _data_dir; }

         /** public for testing purposes only... should be private in practice. */
         undo_database                          _undo_db;
     protected:
         template<typename IndexType>
         IndexType&    get_mutable_index_type() {
            static_assert( std::is_base_of<index,IndexType>::value, "Type must be an index type" );
            return static_cast<IndexType&>( get_mutable_index( IndexType::object_type::space_id, IndexType::object_type::type_id ) );
         }
         template<typename T>
         index& get_mutable_index()                   { return get_mutable_index(T::space_id,T::type_id); }
         index& get_mutable_index(object_id_type id)  { return get_mutable_index(id.space(),id.type());   }
         index& get_mutable_index(uint8_t space_id, uint8_t type_id);

     private:

         friend class base_primary_index;
         friend class undo_database;
         void save_undo( const object& obj );
         void save_undo_add( const object& obj );
         void save_undo_remove( const object& obj );

         fc::path                                                  _data_dir;
         vector< std::unique_ptr<safety_check_policy> >            _safety_checks;
         vector< vector< unique_ptr<index> > >                     _index;
   };

} } // graphene::db


