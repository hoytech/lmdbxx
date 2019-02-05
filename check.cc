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

        if (!cursor.get(key, val, MDB_SET)) throw std::runtime_error("cursor err 1");
        if (!cursor.get(key, val, MDB_FIRST_DUP)) throw std::runtime_error("FIRST_DUP err");

        if (key != "blah" || val != "abc1") throw std::runtime_error("cursor err 2");

        if (!cursor.get(key, val, MDB_NEXT_DUP)) throw std::runtime_error("cursor err 3");
        if (key != "blah" || val != "abc2") throw std::runtime_error("cursor err 4");

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

        if (!cursor.get(key, val, MDB_SET)) throw std::runtime_error("cursor err 1");
        if (key != "blah" || val != "abc1") throw std::runtime_error("cursor err 2");

        if (!cursor.get(key, val, MDB_NEXT_DUP)) throw std::runtime_error("cursor err 5");
        if (key != "blah" || val != "abc3") throw std::runtime_error("cursor err 6");

        if (cursor.get(key, val, MDB_NEXT_DUP)) throw std::runtime_error("cursor err 7");

        cursor.close();
        txn.commit();
    }




    // This test case crashes. See README.md

    if (0) {
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

  return 0;
}
