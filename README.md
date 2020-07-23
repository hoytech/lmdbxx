# lmdb++: a C++17 wrapper for LMDB

This is a comprehensive C++ wrapper for the [LMDB](http://symas.com/mdb/) embedded database library,
offering both an error-checked procedural interface and an object-oriented
resource interface with [RAII](http://en.wikipedia.org/wiki/Resource_Acquisition_Is_Initialization) semantics.

This library is a fork of [Arto Bendiken](https://ar.to/)'s [lmdbxx C++11 library](https://github.com/drycpp/lmdbxx).
The main difference from Arto's version is that the `lmdb::val` class has been removed.
Instead, all keys and values are [std::string_view](https://en.cppreference.com/w/cpp/string/basic_string_view)s.
See the [Fork Differences](#fork-differences) section for full details on what has been changed from Arto's version.

## Example

Here follows a simple motivating example demonstrating basic use of the object-oriented resource interface::

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

**NOTE:** In order to run this example, you must first manually create the
`./example.mdb/` directory. This is a basic characteristic of LMDB: the
given environment path must already exist, as LMDB will not attempt to
automatically create it.

Should any operation in the above fail, an `lmdb::error` exception will be
thrown and terminate the program since we don't specify an exception handler.
All resources will regardless get automatically cleaned up due to RAII
semantics.


## Features

* Designed to be entirely self-contained as a single `<lmdb++.h>` header file that can be dropped into a project.
* Implements a straightforward mapping to and from the LMDB C library, with consistent naming.
* Provides both a procedural interface and an object-oriented RAII interface.
* Simplifies error handling by translating error codes into C++ exceptions.
* Carefully differentiates logic errors, runtime errors, and fatal errors.
* Exception strings include the name of the LMDB function that failed.
* Plays nice with others: all symbols are placed into the `lmdb` namespace.
* 100% free and unencumbered [public domain](http://unlicense.org/) software, usable in any context and for any purpose.


## Requirements

The `<lmdb++.h>` header file requires a C++17 compiler and standard library.  Recent releases of Clang or GCC will work fine.

In addition, for your application to build and run, the underlying
`<lmdb.h>` header file shipped with LMDB must be available in the
preprocessor's include path, and you must link with the `liblmdb` native
library. On Ubuntu Linux 14.04 and newer, these prerequisites can be
satisfied by installing the `liblmdb-dev` package.




## string_view

LMDB uses a simple struct named `MDB_val` which contains only a `void *` and a `size_t`. This is what it uses to represent both keys and values in all functions. As of C++17, there is a standard type known as [std::string_view](https://en.cppreference.com/w/cpp/string/basic_string_view) which also contains only a pointer and a size. In the resource interface of this library, `std::string_view` is used for all keys and values.

The nice aspect about `std::string_view` objects is that they are compatible with many aspects of C++. You can easily construct `std::string`s from them, ie `std::string(my_stringview)`. Unfortunately, that involves copying the data from the LMDB memory map to a new allocation on the heap (unless your string is short, then a [short string optimisation](https://stackoverflow.com/questions/21694302/what-are-the-mechanics-of-short-string-optimization-in-libc) may apply).

However, with some care `std::string_view` lets you avoid copying in several cases. For example, you can take zero-copy substrings by using `substr()`. Many modern C++ libraries are now being designed to reduce or eliminate copying by accepting or returning `std::string_view` objects, for example the [TAO C++ JSON parser](https://github.com/taocpp/json) and the [flatbuffers serialisation system](http://google.github.io/flatbuffers/).

With `std::string_view` the standard LMDB caveats apply: If you need to keep the data around after closing the LMDB transaction (or after performing any write operation on the DB) then you need to make a copy. This is as easy as assigning the `std::string_view` to an `std::string`.

    std::string longLivedValue;

    {
        auto txn = lmdb::txn::begin(env);
        auto mydb = lmdb::dbi::open(txn, "mydb");

        std::string_view v;
        mydb.get(txn, "hello", v);

        longLivedValue = v;
    }

In the code above, note that `"hello"` was passed in as a key. This works because a `std::string_view` is implicitly constructed. This works for `char *`, `std::string`, etc.


### string_view Conversions

Arto's original version of this library had templated `get` and `put` convenience methods. These methods reduced type safety and [caused problems for some users](https://github.com/drycpp/lmdbxx/issues/1) so this fork has removed them in favour of explicit methods to convert to and from `std::string_view`s.

**Note:** These conversion functions described in this section are mostly designed for storing integers in `MDB_INTEGERKEY`/`MDB_INTEGERDUP` databases. Although you can use them for more complicated types, we do not recommend doing so. Instead, please look into zero-copy serialization schemes such as [flatbuffers](https://google.github.io/flatbuffers/) or [capn proto](https://capnproto.org/). With these you can get almost all the performance benefit of storing raw structs. In addition you will get more safety, the ability to access your database from languages other than C/C++, database portability across systems, and a way to upgrade your structures by adding new fields, deprecating old ones, etc.

If you do decide to store complex structs directly, you have to be very careful when using the following methods. If you have any pointers in your structures then you will almost certainly experience out-of-bounds memory accesses, and/or memory corruption.

It is **strongly** recommended that you develop and test using [address sanitizer](https://en.wikipedia.org/wiki/AddressSanitizer) when working with these routines (and in general). This will help you detect problems early on during development. The `Makefile` compiles the `check.cpp` test suite with `-fsanitize=address` for this reason.

#### Copying

For example, suppose you want to store raw `uint64_t` values in a DB. You can use the `to_sv` function to create a `string_view` which can then be passed to a `put` method:

      mydb.put(txn, "some_key", lmdb::to_sv<uint64_t>(123456));

**NOTE:** The above `to_sv` call will create a `std::string_view` pointing to a temporary object. You should ensure that you don't retain the `string_view` outside of the current [full expression](https://en.cppreference.com/w/cpp/language/lifetime), which in this case is the `mydb.put()`. Otherwise, you will encounter undefined behaviour.

Afterwards, you can `get` the value back out of the DB and extract the `uint64_t` with `from_sv`:

      std::string_view view;
      mydb.get(txn, "some_key", view);
      uint64_t val = lmdb::from_sv<uint64_t>(view);

This copies the memory from the database and returns this copy for you to use. In the case of simple data-types like `uint64_t` this doesn't make a difference, but for large structs you may want to use the pointer-based conversions described in the next section.

`from_sv` will throw an `MDB_BAD_VALSIZE` exception if the view isn't the expected size (in this case, 8 bytes). You should also use this method if you wish to ensure that your value is correctly aligned prior to accessing it since LMDB only guarantees 2-byte alignment of keys, unless you are [careful with the sizes of your keys and data](https://www.reddit.com/r/programming/comments/1daiu8/interesting_fast_and_small_keyvalue_data_store/c9p8ml1/).

#### Pointer-based

If you wish to avoid the copying and have the `string_view` point directly to an existing block of memory, you can use `ptr_to_sv` (note that the templated type is optional here since it can be inferred from the pointer type):

      uint64_t val = 123456;
      mydb.put(txn, "some_key", lmdb::ptr_to_sv(&val));

Note that you are responsible for managing the backing memory, and you should ensure that it is valid for as long as you need the constructed `string_view`.

Similarly, you can get a pointer pointing into the LMDB mapped memory by using `ptr_from_sv`:

      std::string_view view;
      mydb.get(txn, "some_key", view);
      uint64_t *ptr = lmdb::ptr_from_sv<uint64_t>(view);

Since the returned pointer is pointing into LMDB's mapped memory, you should not use this pointer after the transaction has been terminated, or after performing any write operations on the DB.

As with `from_sv`, `ptr_from_sv` will throw an `MDB_BAD_VALSIZE` exception if the view isn't the expected size (in this case, 8 bytes).


## Interfaces

This wrapper offers both an error-checked procedural interface and an
object-oriented resource interface with RAII semantics. The former will be
useful for easily retrofitting existing projects that currently use the raw
C interface, but **we recommend the resource interface** for all new projects due to the
exception safety afforded by RAII semantics.

### Resource Interface

The high-level resource interface wraps LMDB handles in a loving RAII
embrace. This way, you can ensure e.g. that a transaction will get
automatically aborted when exiting a lexical scope, regardless of whether
the escape happened normally or by throwing an exception.

| C handle       |             C++ wrapper class |
|----------------|-------------------------------|
|`MDB_env*`                 |`lmdb::env` |
|`MDB_txn*`                 |`lmdb::txn` |
|`MDB_dbi`                  |`lmdb::dbi` |
|`MDB_cursor*`              |`lmdb::cursor` |
|`MDB_val`                  |`std::string_view` |

The methods available on these C++ classes are named consistently with the
procedural interface, below, with the obvious difference of omitting the
handle type prefix which is already implied by the class in question.

### Procedural Interface

The low-level procedural interface wraps LMDB functions with error-checking
code that will throw an instance of a corresponding C++ exception class in
case of failure. This interface doesn't offer any convenience overloads as
does the resource interface; the parameter types are exactly the same as for
the raw C interface offered by LMDB itself.  The return type is generally
`void` for these functions since the wrapper eats the error code returned
by the underlying C function, throwing an exception in case of failure and
otherwise returning values in the same output parameters as the C interface.

This interface is implemented entirely using static inline functions, so
there are no hidden extra costs to using these wrapper functions so long as
you have a decent compiler capable of basic inlining optimization.

See the [FUNCTIONS.rst](FUNCTIONS.rst) file for a mapping of the procedural interface to the underlying LMDB C functions.

### Caveats

* The C++ procedural interface is more strictly and consistently grouped by
  handle type than is the LMDB native interface.  For instance,
  `mdb_put()` is wrapped as the C++ function `lmdb::dbi_put()`, not
  `lmdb::put()`.  These differences--a handful in number--all concern
  operations on database handles.

* The C++ interface takes some care to be const-correct for input-only
  parameters, something the original C interface largely ignores.
  Hence occasional use of `const_cast` in the wrapper code base.

* `lmdb::dbi_put()` does not throw an exception if LMDB returns the
  `MDB_KEYEXIST` error code; it instead just returns `false`.
  This is intended to simplify common usage patterns.

* `lmdb::dbi_get()`, `lmdb::dbi_del()`, and `lmdb::cursor_get()` do
  not throw an exception if LMDB returns the `MDB_NOTFOUND` error code;
  they instead just return `false`.
  This is intended to simplify common usage patterns.

* `lmdb::env_get_max_keysize()` returns an unsigned integer, instead of a
  signed integer as the underlying `mdb_env_get_maxkeysize()` function does.
  This conversion is done since the return value cannot in fact be negative.

### Cursor double-free issue

In a read-write transaction, you must make sure to call `.close()` on your cursors (or let them go out of scope) **before** committing or aborting your transaction.
Otherwise you will do a double-free which, if you are lucky, will crash your process. This is described in [this github issue](https://github.com/drycpp/lmdbxx/issues/22).

Consider this code:

    {
        auto txn = lmdb::txn::begin(env);
        auto mydb = lmdb::dbi::open(txn, "mydb");

        auto cursor = lmdb::cursor::open(txn, mydb);
        std::string_view key, val;
        cursor.get(key, val, MDB_FIRST);

        txn.commit();
    } // <-- BAD! cursor is destroyed here

The above code will result in a double free. You can uncomment a test case in `check.cc` if you want to verify this for yourself. When compiled with `-fsanitize=address` you will see the following:

    ==14400==ERROR: AddressSanitizer: attempting double-free on 0x614000000240 in thread T0:

To fix this, you should call `cursor.close()` before you call `txn.commit()`. Or, alternatively, do your cursor operations in a sub-scope so the cursor is destroyed before the transaction is committed:

    {  
        auto txn = lmdb::txn::begin(env);
        auto mydb = lmdb::dbi::open(txn, "mydb");

        {
            auto cursor = lmdb::cursor::open(txn, mydb);
            std::string_view key, val;
            cursor.get(key, val, MDB_FIRST);
        } // <-- GOOD! cursor is destroyed here

        txn.commit();
    }

Note that the double-free issue does not affect read-only transactions, but it is good practice to ensure closing/destruction of all cursors and transactions happen in the correct order, as shown in the motivating example. This is because you may change a read-only transaction to a read-write transaction in the future.


## Error Handling

This wrapper draws a careful distinction between three different classes of
possible LMDB error conditions:

* **Logic errors**, represented by `lmdb::logic_error`. Errors of this
  class are thrown due to programming errors where the function interfaces
  are used in violation of documented preconditions. A common strategy for
  handling this class of error conditions is to abort the program with a
  core dump, facilitating introspection to locate and remedy the bug.
* **Fatal errors**, represented by `lmdb::fatal_error`. Errors of this
  class are thrown due to the exhaustion of critical system resources, in
  particular available memory (`ENOMEM`), or due to attempts to exceed
  applicable system resource limits. A typical strategy for handling this
  class of error conditions is to terminate the program with a descriptive
  error message. More robust programs and shared libraries may wish to
  implement another strategy, such as retrying the operation after first
  letting most of the call stack unwind in order to free up scarce
  resources.
* **Runtime errors**, represented by `lmdb::runtime_error`. Errors of this
  class are thrown as a matter of course to indicate various exceptional
  conditions. These conditions are generally recoverable, and robust
  programs will take care to correctly handle them.

**NOTE:** The distinction between logic errors and runtime errors mirrors that
   found in the C++11 standard library, where the `<stdexcept>` header
   defines the standard exception base classes `std::logic_error` and
   `std::runtime_error`. The standard exception class `std::bad_alloc`,
   on the other hand, is a representative example of a fatal error.

| Error code            |    Exception class            |      Exception type |
|-----------------------|-------------------------------|---------------------|
|`MDB_KEYEXIST`         |`lmdb::key_exist_error`        | runtime             |
|`MDB_NOTFOUND`         |`lmdb::not_found_error`        | runtime             |
|`MDB_CORRUPTED`        |`lmdb::corrupted_error`        | fatal               |
|`MDB_PANIC`            |`lmdb::panic_error`            | fatal               |
|`MDB_VERSION_MISMATCH` |`lmdb::version_mismatch_error` | fatal               |
|`MDB_MAP_FULL`         |`lmdb::map_full_error`         | runtime             |
|`MDB_BAD_DBI`          |`lmdb::bad_dbi_error`          | runtime [4]         |
|(others)               |`lmdb::runtime_error`          | runtime             |

* [4] Available since LMDB 0.9.14 (2014/09/20).
* `MDB_KEYEXIST` and `MDB_NOTFOUND` are handled specially by some functions.


## Support

To report a bug or submit a patch for lmdb++, please file an issue in the [issue tracker on GitHub](https://github.com/hoytech/lmdbxx/issues).

Questions and discussions about LMDB itself should be directed to the [OpenLDAP mailing lists](http://www.openldap.org/lists/).

Also see Arto's original [github](https://github.com/bendiken/lmdbxx) (not maintained anymore?) and [sourceforge documentation](https://sourceforge.net/projects/lmdbxx/) (not up to date with this fork's changes).



## Fork Differences

This C++17 version is a fork of Arto Bendiken's C++11 version with the following changes:

* `lmdb::val` has been removed and replaced with `std::string_view`. See the [string::view section](#string_view) for more details.

* The templated versions of the `get` and `put` methods have been removed. See the conversion methods described in [string_view Conversions](#string_view-conversions) for an alternative.

* Changes to cursors:
    * The cursor interface has been completed. `put`, `del`, and `count` have been added, bringing us to parity with the LMDB API.
    * The cursor `find` method has been removed. This method did not correspond to any function in LMDB API. All it did was a `get` with a cursor op of `MDB_SET`. You should now do this directly, and consider the differences between `MDB_SET`, `MDB_SET_KEY`, and `MDB_GET_BOTH_RANGE`.
    * The option of passing `MDB_val*` in via the cursor resource interface has been removed. Now you must use `std::string_view`. Of course the procedural interface still lets you use raw `MDB_val*`s if you want.
    * `cursor_put` returns `bool` to propagate the condition that the key already exists and either `MDB_NODUPDATA` or `MDB_NOOVERWRITE` were set. This makes it consistent with `cursor_get`.

* A `del` method has been added to the `lmdb::dbi` resource interface that lets you pass in a value as well as a key so that you can delete sorted dup items via dbi objects.

* `lmdb::dbi` instances can now be constructed uninitialized. Attempting to use them in this state will result in an error. You should initialize them first, for example:

      lmdb::dbi mydb;

      // mydb is uninitialized, don't use it!

      {
          auto txn = lmdb::txn::begin(env);
          mydb = lmdb::dbi::open(txn, "mydb", MDB_CREATE);
          txn.commit();
      }

      // now mydb is safe to use

* `lmdb::dbi` instances can now be copied.

* Considerably expanded the test suite.

* Converted documentation to markdown.

* Added a section to the docs describing the [cursor double-free issue](#cursor-double-free-issue).

* If an exception was throw by `txn.commit()` (ie `MDB_MAP_FULL`), and this transaction was later aborted (because it went out of scope while unwinding the stack), then a double-free would occur. This was [fixed](https://github.com/hoytech/lmdbxx/pull/3) by Niklas Salmoukas.



## Author

[Arto Bendiken](https://ar.to/)

This fork maintained by [Doug Hoyte](https://hoytech.com)

## License

This is free and unencumbered public domain software. For more information,
see http://unlicense.org/ or the accompanying `UNLICENSE` file.
