/* This is free and unencumbered software released into the public domain. */

#include "lmdb++.h"

#include <iostream>
#include <stdexcept>


int main() {
  try {
    auto env = lmdb::env::create();
    env.set_max_dbs(64);
    env.open("testdb/");


    // Put some values in and read them back out

    {
        auto txn = lmdb::txn::begin(env);
        auto mydb = lmdb::dbi::open(txn, "mydb", MDB_CREATE);

        mydb.put(txn, "hello", "world");
        mydb.put(txn, "abc", std::string("Q\0X", 3));

        txn.commit();
    }

    {
        auto txn = lmdb::txn::begin(env, nullptr, MDB_RDONLY);
        auto mydb = lmdb::dbi::open(txn, "mydb");

        std::string_view v;
        mydb.get(txn, "hello", v);
        if (v != "world") throw std::runtime_error("bad read");
    }


    // Update one of the values

    {
        auto txn = lmdb::txn::begin(env);
        auto mydb = lmdb::dbi::open(txn, "mydb", MDB_CREATE);

        mydb.put(txn, "hello", "WORLD!");

        txn.commit();
    }

    {
        auto txn = lmdb::txn::begin(env, nullptr, MDB_RDONLY);
        auto mydb = lmdb::dbi::open(txn, "mydb");

        std::string_view v;
        mydb.get(txn, "hello", v);
        if (v != "WORLD!") throw std::runtime_error("bad read 2");
    }


    // Iterate over the values with a cursor

    {
        auto txn = lmdb::txn::begin(env, nullptr, MDB_RDONLY);
        auto mydb = lmdb::dbi::open(txn, "mydb");

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
        auto mydb = lmdb::dbi::open(txn, "mydb");

        mydb.del(txn, "hello");

        std::string_view v;
        bool found = mydb.get(txn, "hello", v);
        if (found) throw std::runtime_error("wasn't deleted");

        txn.commit();
    }


    // This test case crashes. See README.md

    if (0) {
        auto txn = lmdb::txn::begin(env);
        auto mydb = lmdb::dbi::open(txn, "mydb");

        auto cursor = lmdb::cursor::open(txn, mydb);
        std::string_view key, val;
        cursor.get(key, val, MDB_FIRST);

        //cursor.close(); // <-- UNCOMMENT TO FIX CRASH (https://github.com/drycpp/lmdbxx/issues/22)
        txn.commit();
    }

    return 0;
  }
  catch (const lmdb::error& error) {
    std::cerr << "Failed with error: " << error.what() << std::endl;
    return 1;
  }
}
