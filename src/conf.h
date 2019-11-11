#ifndef _GITFS_CONF_H
#define _GITFS_CONF_H

typedef struct gfsconf {
	MDB_txn *txn;
	MDB_dbi  dbir;
	MDB_dbi  dbio;
} gfsconf;

#endif
