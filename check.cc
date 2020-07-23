/* This is free and unencumbered software released into the public domain. */

#include "lmdb++.h"

#include <iostream>
#include <stdexcept>


int main() {
  std::string longLivedValue;

  try {
    auto env = lmdb::env::create();
    env.set_max_dbs(64);
    env.open("testdb/");
    lmdb::dbi mydb;
    lmdb::dbi mydbdups;

    // Put some values in and read them back out

    {
        auto txn = lmdb::txn::begin(env);
        mydb = lmdb::dbi::open(txn, "mydb", MDB_CREATE);

        mydb.put(txn, "hello", "world");
        mydb.put(txn, "abc", std::string("Q\0X", 3));

        txn.commit();
    }

    {
        auto txn = lmdb::txn::begin(env, nullptr, MDB_RDONLY);

        std::string_view v;
        mydb.get(txn, "hello", v);
        if (v != "world") throw std::runtime_error("bad read");

        longLivedValue = v;
    }


    // Update one of the values

    {
        auto txn = lmdb::txn::begin(env);

        mydb.put(txn, "hello", "WORLD!");

        txn.commit();
    }

    {
        auto txn = lmdb::txn::begin(env, nullptr, MDB_RDONLY);

        std::string_view v;
        mydb.get(txn, "hello", v);
        if (v != "WORLD!") throw std::runtime_error("bad read 2");
    }


    // Iterate over the values with a cursor

    {
        auto txn = lmdb::txn::begin(env, nullptr, MDB_RDONLY);

        auto cursor = lmdb::cursor::open(txn, mydb);
        std::string_view key, val;

        if (!cursor.get(key, val, MDB_FIRST)) throw std::runtime_error("cursor err 1");
        if (key != "abc" || val != std::string("Q\0X", 3)) throw std::runtime_error("cursor err 2");

        if (!cursor.get(key, val, MDB_NEXT)) throw std::runtime_error("cursor err 3");
        if (key != "hello" || val != "WORLD!") throw std::runtime_error("cursor err 4");

        if (cursor.get(key, val, MDB_NEXT)) throw std::runtime_error("cursor err 5");

        cursor.close();
    }


    // Delete a value

    {
        auto txn = lmdb::txn::begin(env);

        mydb.del(txn, "hello");

        std::string_view v;
        bool found = mydb.get(txn, "hello", v);
        if (found) throw std::runtime_error("wasn't deleted");

        txn.commit();
    }



    // Sorted dups

    {
        auto txn = lmdb::txn::begin(env);
        mydbdups = lmdb::dbi::open(txn, "mydbdups", MDB_CREATE | MDB_DUPSORT);

        mydbdups.put(txn, "aaaa", "junk");

        mydbdups.put(txn, "blah", "abc2");
        mydbdups.put(txn, "blah", "abc1");
        mydbdups.put(txn, "blah", "abc3");

        mydbdups.put(txn, "cccc", "junk");

        txn.commit();
    }

    {
        auto txn = lmdb::txn::begin(env, nullptr, MDB_RDONLY);

        auto cursor = lmdb::cursor::open(txn, mydbdups);
        std::string_view key("blah"), val;

        if (!cursor.get(key, val, MDB_SET_KEY)) throw std::runtime_error("cursor err 1");
        if (!cursor.get(key, val, MDB_FIRST_DUP)) throw std::runtime_error("FIRST_DUP err");

        if (cursor.count() != 3) throw std::runtime_error("cursor.count error");

        if (key != "blah" || val != "abc1") throw std::runtime_error("cursor err 2");

        if (!cursor.get(key, val, MDB_NEXT_DUP)) throw std::runtime_error("cursor err 3");
        if (key != "blah" || val != "abc2") throw std::runtime_error("cursor err 4");

        if (cursor.count() != 3) throw std::runtime_error("cursor.count error");

        if (!cursor.get(key, val, MDB_NEXT_DUP)) throw std::runtime_error("cursor err 5");
        if (key != "blah" || val != "abc3") throw std::runtime_error("cursor err 6");

        if (cursor.get(key, val, MDB_NEXT_DUP)) throw std::runtime_error("cursor err 7");

        cursor.close();
        txn.commit();
    }


    // Deleting a dup

    {
        auto txn = lmdb::txn::begin(env);

        mydbdups.del(txn, "blah", "abc2");

        txn.commit();
    }

    {
        auto txn = lmdb::txn::begin(env, nullptr, MDB_RDONLY);

        auto cursor = lmdb::cursor::open(txn, mydbdups);
        std::string_view key("blah"), val;

        if (!cursor.get(key, val, MDB_SET_KEY)) throw std::runtime_error("cursor err 1");
        if (key != "blah" || val != "abc1") throw std::runtime_error("cursor err 2");

        if (cursor.count() != 2) throw std::runtime_error("cursor.count error");

        if (!cursor.get(key, val, MDB_NEXT_DUP)) throw std::runtime_error("cursor err 5");
        if (key != "blah" || val != "abc3") throw std::runtime_error("cursor err 6");

        if (cursor.get(key, val, MDB_NEXT_DUP)) throw std::runtime_error("cursor err 7");

        cursor.close();
        txn.commit();
    }



    // to_sv / from_sv

    {
        auto txn = lmdb::txn::begin(env);

        // DON'T DO THIS: the r-value is destroyed at the end of the full expression
        //auto my_sv = lmdb::to_sv<uint64_t>(0x1122334455667788);
        //mydb.put(txn, "to_sv_key", my_sv);

        // OK: The rvalue will stay alive until the end of the "full-expression" (mydb.put(...))
        mydb.put(txn, "to_sv_key", lmdb::to_sv<uint64_t>(0x1122334455667788));

        // OK: temp stays alive until the end of its scope
        uint64_t temp = 0x8877665544332211;
        auto my_sv = lmdb::to_sv(temp);
        mydb.put(txn, "to_sv_key2", my_sv);

        // OK: v stays alive until the end of its scope
        int16_t v = -19288;
        mydb.put(txn, "to_sv_key3", lmdb::ptr_to_sv(&v));

        txn.commit();
    }

    {
        auto txn = lmdb::txn::begin(env, nullptr, MDB_RDONLY);

        std::string_view v;
        if (!mydb.get(txn, "to_sv_key", v)) throw std::runtime_error("bad read to_sv 1");
        if (lmdb::from_sv<uint64_t>(v) != 0x1122334455667788) throw std::runtime_error("bad read to_sv 2");

        if (!mydb.get(txn, "to_sv_key2", v)) throw std::runtime_error("bad read to_sv 2");
        if (lmdb::from_sv<uint64_t>(v) != 0x8877665544332211) throw std::runtime_error("bad read to_sv 3");

        if (!mydb.get(txn, "to_sv_key3", v)) throw std::runtime_error("bad read to_sv 4");
        if (lmdb::from_sv<int16_t>(v) != -19288) throw std::runtime_error("bad read to_sv 5");

        int16_t *ptr = lmdb::ptr_from_sv<int16_t>(v);
        if (*ptr != -19288) throw std::runtime_error("bad read to_sv 6");
    }



    // Nested transactions

    {
        auto txn = lmdb::txn::begin(env);

        {
            auto txn2 = lmdb::txn::begin(env, txn);

            mydb.put(txn2, "junk1", "blah");

            {
                // Can't use the parent txn when child active
                bool caught = false;

                try {
                    std::string_view v;
                    mydb.get(txn, "junk1", v);
                } catch (std::exception &e) {
                    caught = true;
                }

                if (!caught) throw std::runtime_error("bad nested tx 0");
            }

            txn2.abort();
        }

        {
            std::string_view v;
            if (mydb.get(txn, "junk1", v)) throw std::runtime_error("bad nested tx 1");
        }

        {
            auto txn2 = lmdb::txn::begin(env, txn);

            mydb.put(txn2, "junk2", "bleh");

            txn2.commit();
        }

        {
            std::string_view v;
            if (!mydb.get(txn, "junk2", v)) throw std::runtime_error("bad nested tx 2");
        }

        {
            // Creating a read-only sub-transaction inside a read-write transaction
            bool caught = false;

            try {
                auto txn2 = lmdb::txn::begin(env, txn, MDB_RDONLY);
            } catch (std::exception &e) {
                caught = true;
            }

            if (!caught) throw std::runtime_error("bad nested tx 2.1");
        }

        txn.commit();
    }

    {
        auto txn = lmdb::txn::begin(env, nullptr, MDB_RDONLY);

        std::string_view v;

        if (mydb.get(txn, "junk1", v)) throw std::runtime_error("bad nested tx 3");

        if (!mydb.get(txn, "junk2", v)) throw std::runtime_error("bad nested tx 4");
        if (v != "bleh") throw std::runtime_error("bad nested tx 3");


        // trying to create a non-readonly sub-tx

        bool caught = false;

        try {
            auto txn2 = lmdb::txn::begin(env, txn);
        } catch (std::exception &e) {
            caught = true;
        }

        if (!caught) throw std::runtime_error("bad nested tx 5");
    }




    if (0) {
        // This test case is not enabled by default because it causes the process
        // to crash. See the "Cursor double-free issue" section in README.md
        std::cout << "Running optional test #1" << std::endl;

        auto txn = lmdb::txn::begin(env);

        auto cursor = lmdb::cursor::open(txn, mydb);
        std::string_view key, val;
        cursor.get(key, val, MDB_FIRST);

        //cursor.close(); // <-- UNCOMMENT TO FIX CRASH (https://github.com/drycpp/lmdbxx/issues/22)
        txn.commit();
    }
  }
  catch (const lmdb::error& error) {
    std::cerr << "Failed with error: " << error.what() << std::endl;
    return 1;
  }

  if (longLivedValue != "world") throw std::runtime_error("bad longLivedValue");



  if (0) {
      // Enable this to verify the fix for https://github.com/hoytech/lmdbxx/pull/3
      // This test is not run in the normal test-suite because the sizes chosen depend
      // on internal LMDB layout, which could change.
      std::cout << "Running optional test #2" << std::endl;

      if (system("rm testdb/data.mdb testdb/lock.mdb")) {
        std::cerr << "Unable to delete DB files during test" << std::endl;
        return 1;
      }

      auto env = lmdb::env::create();
      env.set_max_dbs(64);
      env.set_mapsize(30000);
      env.open("testdb/");

      lmdb::dbi mydb;

      {
          auto txn = lmdb::txn::begin(env);
          mydb = lmdb::dbi::open(txn, "mydb", MDB_CREATE);
          txn.commit();
      }

      {
          auto txn = lmdb::txn::begin(env);

          mydb.put(txn, "k", std::string(4000, '\x01'));

          bool got_map_full_error = false;

          try {
              txn.commit(); // <-- This throws MDB_MAP_FULL, which leaves the transaction "partially committed"
          } catch (const lmdb::map_full_error& error) {
              got_map_full_error = true;
          }

          if (!got_map_full_error) throw std::runtime_error("didn't get expected lmdb::map_full_error");

          // <-- The transaction is aborted here, which results in double-free
      }
  }

  return 0;
}
