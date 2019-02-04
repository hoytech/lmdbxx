********************************
lmdb++: a C++11 wrapper for LMDB
********************************

.. image:: https://api.travis-ci.org/bendiken/lmdbxx.svg?branch=master
   :target: https://travis-ci.org/bendiken/lmdbxx
   :alt: Travis CI build status

.. image:: https://scan.coverity.com/projects/4900/badge.svg
   :target: https://scan.coverity.com/projects/4900
   :alt: Coverity Scan build status

This is a comprehensive C++ wrapper for the LMDB_ embedded database library,
offering both an error-checked procedural interface and an object-oriented
resource interface with RAII_ semantics.

.. _LMDB: http://symas.com/mdb/
.. _RAII: http://en.wikipedia.org/wiki/Resource_Acquisition_Is_Initialization

Example
=======

Here follows a simple motivating example_ demonstrating basic use of the
object-oriented resource interface::

   #include <cstdio>
   #include <cstdlib>
   #include <lmdb++.h>

   int main() {
     /* Create and open the LMDB environment: */
     auto env = lmdb::env::create();
     env.set_mapsize(1UL * 1024UL * 1024UL * 1024UL); /* 1 GiB */
     env.open("./example.mdb", 0, 0664);

     /* Insert some key/value pairs in a write transaction: */
     auto wtxn = lmdb::txn::begin(env);
     auto dbi = lmdb::dbi::open(wtxn, nullptr);
     dbi.put(wtxn, "username", "jhacker");
     dbi.put(wtxn, "email", "jhacker@example.org");
     dbi.put(wtxn, "fullname", "J. Random Hacker");
     wtxn.commit();

     /* Fetch key/value pairs in a read-only transaction: */
     auto rtxn = lmdb::txn::begin(env, nullptr, MDB_RDONLY);
     auto cursor = lmdb::cursor::open(rtxn, dbi);
     std::string key, value;
     while (cursor.get(key, value, MDB_NEXT)) {
       std::printf("key: '%s', value: '%s'\n", key.c_str(), value.c_str());
     }
     cursor.close();
     rtxn.abort();

     /* The enviroment is closed automatically. */

     return EXIT_SUCCESS;
   }

Should any operation in the above fail, an ``lmdb::error`` exception will be
thrown and terminate the program since we don't specify an exception handler.
All resources will regardless get automatically cleaned up due to RAII
semantics.

.. note::

   In order to run this example, you must first manually create the
   ``./example.mdb`` directory. This is a basic characteristic of LMDB: the
   given environment path must already exist, as LMDB will not attempt to
   automatically create it.

.. _example: https://github.com/bendiken/lmdbxx/blob/master/example.cc#L1

Features
========

* Designed to be entirely self-contained as a single ``<lmdb++.h>`` header
  file that can be dropped into a project.
* Implements a straightforward mapping from C to C++, with consistent naming.
* Provides both a procedural interface and an object-oriented RAII interface.
* Simplifies error handling by translating error codes into C++ exceptions.
* Carefully differentiates logic errors, runtime errors, and fatal errors.
* Exception strings include the name of the LMDB function that failed.
* Plays nice with others: all symbols are placed into the ``lmdb`` namespace.
* 100% free and unencumbered `public domain <http://unlicense.org/>`_ software,
  usable in any context and for any purpose.

Requirements
============

The ``<lmdb++.h>`` header file requires a C++11 compiler and standard library.
Recent releases of Clang_ or GCC_ will work fine.

In addition, for your application to build and run, the underlying
``<lmdb.h>`` header file shipped with LMDB must be available in the
preprocessor's include path, and you must link with the ``liblmdb`` native
library. On Ubuntu Linux 14.04 and newer, these prerequisites can be
satisfied by installing the ``liblmdb-dev`` package.

.. _Clang: http://clang.llvm.org/
.. _GCC:   http://gcc.gnu.org/

Overview
========

This wrapper offers both an error-checked procedural interface and an
object-oriented resource interface with RAII semantics. The former will be
useful for easily retrofitting existing projects that currently use the raw
C interface, but we recommend the latter for all new projects due to the
exception safety afforded by RAII semantics.

Resource Interface
------------------

The high-level resource interface wraps LMDB handles in a loving RAII
embrace. This way, you can ensure e.g. that a transaction will get
automatically aborted when exiting a lexical scope, regardless of whether
the escape happened normally or by throwing an exception.

============================ ===================================================
C handle                     C++ wrapper class
============================ ===================================================
``MDB_env*``                 ``lmdb::env``
``MDB_txn*``                 ``lmdb::txn``
``MDB_dbi``                  ``lmdb::dbi``
``MDB_cursor*``              ``lmdb::cursor``
``MDB_val``                  ``lmdb::val``
============================ ===================================================

The methods available on these C++ classes are named consistently with the
procedural interface, below, with the obvious difference of omitting the
handle type prefix which is already implied by the class in question.

Procedural Interface
--------------------

The low-level procedural interface wraps LMDB functions with error-checking
code that will throw an instance of a corresponding C++ exception class in
case of failure. This interface doesn't offer any convenience overloads as
does the resource interface; the parameter types are exactly the same as for
the raw C interface offered by LMDB itself.  The return type is generally
``void`` for these functions since the wrapper eats the error code returned
by the underlying C function, throwing an exception in case of failure and
otherwise returning values in the same output parameters as the C interface.

============================ ===================================================
C function                   C++ wrapper function
============================ ===================================================
``mdb_version()``            N/A
``mdb_strerror()``           N/A
``mdb_env_create()``         ``lmdb::env_create()``
``mdb_env_open()``           ``lmdb::env_open()``
``mdb_env_copy()``           ``lmdb::env_copy()``                           [1]_
``mdb_env_copyfd()``         ``lmdb::env_copy_fd()``                        [1]_
``mdb_env_copy2()``          ``lmdb::env_copy()``                           [1]_
``mdb_env_copyfd2()``        ``lmdb::env_copy_fd()``                        [1]_
``mdb_env_stat()``           ``lmdb::env_stat()``
``mdb_env_info()``           ``lmdb::env_info()``
``mdb_env_sync()``           ``lmdb::env_sync()``
``mdb_env_close()``          ``lmdb::env_close()``
``mdb_env_set_flags()``      ``lmdb::env_set_flags()``
``mdb_env_get_flags()``      ``lmdb::env_get_flags()``
``mdb_env_get_path()``       ``lmdb::env_get_path()``
``mdb_env_get_fd()``         ``lmdb::env_get_fd()``
``mdb_env_set_mapsize()``    ``lmdb::env_set_mapsize()``
``mdb_env_set_maxreaders()`` ``lmdb::env_set_max_readers()``
``mdb_env_get_maxreaders()`` ``lmdb::env_get_max_readers()``
``mdb_env_set_maxdbs()``     ``lmdb::env_set_max_dbs()``
``mdb_env_get_maxkeysize()`` ``lmdb::env_get_max_keysize()``
``mdb_env_set_userctx()``    ``lmdb::env_set_userctx()``                    [2]_
``mdb_env_get_userctx()``    ``lmdb::env_get_userctx()``                    [2]_
``mdb_env_set_assert()``     N/A
``mdb_txn_begin()``          ``lmdb::txn_begin()``
``mdb_txn_env()``            ``lmdb::txn_env()``
``mdb_txn_id()``             ``lmdb::txn_id()``                             [3]_
``mdb_txn_commit()``         ``lmdb::txn_commit()``
``mdb_txn_abort()``          ``lmdb::txn_abort()``
``mdb_txn_reset()``          ``lmdb::txn_reset()``
``mdb_txn_renew()``          ``lmdb::txn_renew()``
``mdb_dbi_open()``           ``lmdb::dbi_open()``
``mdb_stat()``               ``lmdb::dbi_stat()``                           [4]_
``mdb_dbi_flags()``          ``lmdb::dbi_flags()``
``mdb_dbi_close()``          ``lmdb::dbi_close()``
``mdb_drop()``               ``lmdb::dbi_drop()``                           [4]_
``mdb_set_compare()``        ``lmdb::dbi_set_compare()``                    [4]_
``mdb_set_dupsort()``        ``lmdb::dbi_set_dupsort()``                    [4]_
``mdb_set_relfunc()``        ``lmdb::dbi_set_relfunc()``                    [4]_
``mdb_set_relctx()``         ``lmdb::dbi_set_relctx()``                     [4]_
``mdb_get()``                ``lmdb::dbi_get()``                            [4]_
``mdb_put()``                ``lmdb::dbi_put()``                            [4]_
``mdb_del()``                ``lmdb::dbi_del()``                            [4]_
``mdb_cursor_open()``        ``lmdb::cursor_open()``
``mdb_cursor_close()``       ``lmdb::cursor_close()``
``mdb_cursor_renew()``       ``lmdb::cursor_renew()``
``mdb_cursor_txn()``         ``lmdb::cursor_txn()``
``mdb_cursor_dbi()``         ``lmdb::cursor_dbi()``
``mdb_cursor_get()``         ``lmdb::cursor_get()``
``mdb_cursor_put()``         ``lmdb::cursor_put()``
``mdb_cursor_del()``         ``lmdb::cursor_del()``
``mdb_cursor_count()``       ``lmdb::cursor_count()``
``mdb_cmp()``                N/A
``mdb_dcmp()``               N/A
``mdb_reader_list()``        TODO
``mdb_reader_check()``       TODO
============================ ===================================================

.. rubric:: Footnotes

.. [1] Three-parameter signature available since LMDB 0.9.14 (2014/09/20).

.. [2] Only available since LMDB 0.9.11 (2014/01/15).

.. [3] Only available in LMDB HEAD, not yet in any 0.9.x release (as of 0.9.16).
       Define the ``LMDBXX_TXN_ID`` preprocessor symbol to unhide this.

.. [4] Note the difference in naming. (See below.)
