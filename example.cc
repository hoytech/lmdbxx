    #include <iostream>
    #include <lmdb++.h>

    int main() {
        /* Create and open the LMDB environment: */
        auto env = lmdb::env::create();
        env.set_mapsize(1UL * 1024UL * 1024UL * 1024UL); /* 1 GiB */
        env.open("./example.mdb/", 0, 0664);
        lmdb::dbi dbi;

        // Get the dbi handle, and insert some key/value pairs in a write transaction:
        {
            auto wtxn = lmdb::txn::begin(env);
            dbi = lmdb::dbi::open(wtxn, nullptr);

            dbi.put(wtxn, "username", "jhacker");
            dbi.put(wtxn, "email",    std::string("jhacker@example.org"));
            dbi.put(wtxn, "fullname", std::string_view("J. Random Hacker"));

            wtxn.commit();
       }

       // In a read-only transaction, get and print one of the values:
       {
           auto rtxn = lmdb::txn::begin(env, nullptr, MDB_RDONLY);

           std::string_view email;
           if (dbi.get(rtxn, "email", email)) {
               std::cout << "The email is: " << email << std::endl;
           } else {
               std::cout << "email not found!" << std::endl;
           }
       } // rtxn aborted automatically

       // Print out all the values using a cursor:
       {
           auto rtxn = lmdb::txn::begin(env, nullptr, MDB_RDONLY);

           {
               auto cursor = lmdb::cursor::open(rtxn, dbi);

               std::string_view key, value;
               if (cursor.get(key, value, MDB_FIRST)) {
                   do {
                       std::cout << "key: " << key << "  value: " << value << std::endl;
                   } while (cursor.get(key, value, MDB_NEXT));
               }
           } // destroying cursor before committing/aborting transaction (see below)
       }

        return 0;
    } // enviroment closed automatically
