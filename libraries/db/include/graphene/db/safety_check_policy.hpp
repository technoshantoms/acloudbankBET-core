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

#include <graphene/protocol/object_id.hpp>

namespace graphene {
namespace db {

class secondary_index;

/**
 * @brief A polymorphic safety check policy for validating modifications to the database
 *
 * The safety check policy is used to catch bugs surrounding modifications to databases early in development. In
 * particular, it can be used to validate that modifications to a given database object space are made by specific
 * pathways of the code, and modifications made outside of these pathways can be blocked to preserve architectural
 * invariants of the system. A safety check policy can be installed on an object space when that space is allocated
 * to check modifications made to that space.
 *
 * Pursuant to this goal, the policy is used to validate modifications to the database, including new indexes (both
 * primary and secondary), new objects, modifications to objects, and deletion of objects. Additionally, as secondary
 * indexes represent transfer of execution to a pathway distinct from the pathway that led to the change being
 * notified about, the policy will also be notified before and after executing a secondary index.
 *
 * @note Safety checks are not queried when loading objects from persistence when the database is opened, but are
 * invoked when secondary indexes are notified of such.
 */
class safety_check_policy {
public:
   virtual ~safety_check_policy();

   virtual bool allow_new_index(uint8_t type_id) = 0;
   virtual bool allow_new_secondary_index(uint8_t type_id, const secondary_index& new_secondary_index) = 0;
   virtual bool allow_delete_secondary_index(uint8_t type_id, const secondary_index& secondary_index) = 0;
   virtual bool allow_object_creation(object_id_type new_object_id) = 0;
   virtual bool allow_object_modification(object_id_type modified_object_id) = 0;
   virtual bool allow_object_deletion(object_id_type deleted_object_id) = 0;

   virtual void pre_secondary_index_notification(uint8_t type_id, const secondary_index& notified_index) = 0;
   virtual void post_secondary_index_notification(uint8_t type_id, const secondary_index& notified_index) = 0;
};

/** @brief A safety check that allows all modifications and does nothing when secondary indexes are invoked */
class null_safety_check : public safety_check_policy {
public:
   null_safety_check() = default;
   null_safety_check(null_safety_check&&) = default;
   null_safety_check(const null_safety_check&) = delete;
   virtual ~null_safety_check();
   null_safety_check& operator=(const null_safety_check&) = delete;
   null_safety_check& operator=(null_safety_check&&) = default;

   bool allow_new_index(uint8_t) override { return true; }
   bool allow_new_secondary_index(uint8_t, const secondary_index&) override { return true; }
   bool allow_delete_secondary_index(uint8_t, const secondary_index&) override { return true; }
   bool allow_object_creation(object_id_type) override { return true; }
   bool allow_object_modification(object_id_type) override { return true; }
   bool allow_object_deletion(object_id_type) override { return true; }
   void pre_secondary_index_notification(uint8_t, const secondary_index&) override {}
   void post_secondary_index_notification(uint8_t, const secondary_index&) override {}
};

/**
 * @brief A safety check that allows the database object space to be locked and unlocked for changes
 *
 * This class implements a time-based database safety check using a lock, such that modifications made at times that
 * the database is unlocked are always allowed, but modifications made at times the database is locked are always
 * rejected. The database is initially unlocked, and is automatically locked while secondary indexes are being
 * notified of changes to the database, and unlocked after.
 *
 * Secondary indexes are always allowed to be added or deleted, regardless of lock state.
 */
class database_lock_safety_check : public safety_check_policy {
   bool locked;

public:
   database_lock_safety_check() : locked(false) {}
   database_lock_safety_check(database_lock_safety_check&&) = default;
   database_lock_safety_check(const database_lock_safety_check&) = delete;
   virtual ~database_lock_safety_check();
   database_lock_safety_check& operator=(const database_lock_safety_check&) = delete;
   database_lock_safety_check& operator=(database_lock_safety_check&&) = default;

   bool is_locked() const { return locked; }
   void lock() { locked = true; }
   void unlock() { locked = false; }

   bool allow_new_index(uint8_t) override { return !locked; }
   bool allow_new_secondary_index(uint8_t, const secondary_index&) override { return true; }
   bool allow_delete_secondary_index(uint8_t, const secondary_index&) override { return true; }
   bool allow_object_creation(object_id_type) override { return !locked; }
   bool allow_object_modification(object_id_type) override { return !locked; }
   bool allow_object_deletion(object_id_type) override { return !locked; }
   void pre_secondary_index_notification(uint8_t, const secondary_index&) override { locked = true; }
   void post_secondary_index_notification(uint8_t, const secondary_index&) override { locked = false; }
};

class scoped_database_unlocker {
   std::vector<database_lock_safety_check*> locks;
   std::vector<bool> original_states;

public:
   template<typename... Locks>
   scoped_database_unlocker(Locks&... locks) : locks{&locks...}, original_states{locks.is_locked()...} {
      for (const auto& lock : this->locks)
         lock->unlock();
   }

   scoped_database_unlocker(scoped_database_unlocker&&) = default;
   scoped_database_unlocker(const scoped_database_unlocker&) = delete;
   scoped_database_unlocker& operator=(const scoped_database_unlocker&) = delete;
   scoped_database_unlocker& operator=(scoped_database_unlocker&&) = default;

   ~scoped_database_unlocker() {
      for (unsigned int i = 0; i < locks.size(); ++i)
         if (original_states[i] == true) locks[i]->lock();
         else locks[i]->unlock();
   }
};

} } // namespace graphene::db
