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
