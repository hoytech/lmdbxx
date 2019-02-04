    #include <iostream>
    #include <lmdb++.h>

    int main() {
        /* Create and open the LMDB environment: */
        auto env = lmdb::env::create();
        env.set_mapsize(1UL * 1024UL * 1024UL * 1024UL); /* 1 GiB */
        env.open("./example.mdb", 0, 0664);

        // Inserting some key/value pairs in a write transaction:   
        {
            auto wtxn = lmdb::txn::begin(env);
            auto dbi = lmdb::dbi::open(wtxn, nullptr);

            dbi.put(wtxn, "username", "jhacker");
            dbi.put(wtxn, "email", "jhacker@example.org");
            dbi.put(wtxn, "fullname", "J. Random Hacker");

            wtxn.commit();
       }

       // In a read-only transaction, print out all the values using a cursor:
       {
           auto rtxn = lmdb::txn::begin(env, nullptr, MDB_RDONLY);
           auto dbi = lmdb::dbi::open(rtxn, nullptr);

           {
               auto cursor = lmdb::cursor::open(rtxn, dbi);

               std::string_view key, value;
               if (cursor.get(key, value, MDB_FIRST)) {
                   do {
                       std::cout << "key: " << key << "  value: " << value << std::endl;
                   } while (cursor.get(key, value, MDB_NEXT));
               }
           } // destroying cursor before committing/aborting transaction (see below)

           rtxn.abort();
       }

        /* The enviroment is closed automatically. */

        return 0;
    }
