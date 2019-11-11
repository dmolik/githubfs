#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

#include <curl/curl.h>
#include <libfastjson/json.h>

#include "fetch.h"

struct string {
	char *ptr;
	size_t len;
};

struct post_data {
	const char *ptr;
	size_t      size;
};

void init_string(struct string *s) {
	s->len = 0;
	s->ptr = malloc(s->len+1);
	if (s->ptr == NULL) {
		fprintf(stderr, "malloc() failed\n");
		exit(EXIT_FAILURE);
	}
	s->ptr[0] = '\0';
}

char *trim(char *str)
{
	char *end;

	while(isspace((unsigned char)*str))
		str++;

	if (*str == 0) // All spaces?
		return str;

	end = str + strlen(str) - 1;
	while(end > str && isspace((unsigned char)*end))
		end--;

	end[1] = '\0';

	return str;
}

size_t writefunc(void *ptr, size_t size, size_t nmemb, void *data)
{
	struct string *s = (struct string *) data;
	size_t new_len = s->len + size*nmemb;
	s->ptr = realloc(s->ptr, new_len+1);
	if (s->ptr == NULL) {
		fprintf(stderr, "realloc() failed\n");
		exit(EXIT_FAILURE);
	}
	memcpy(s->ptr + s->len, ptr, size*nmemb);
	s->ptr[new_len] = '\0';
	s->len = new_len;

	return size*nmemb;
}

static size_t post_callback(void *buf, size_t size, size_t nmemb, void *data)
{
	struct post_data *pd = (struct post_data *) data;
	size_t buffer_size = size * nmemb;

	if (pd->size) {
		size_t copy_size = pd->size;
		if (copy_size > buffer_size)
			copy_size = buffer_size;
		memcpy(buf, pd->ptr, copy_size);

		pd->ptr  += copy_size;
		pd->size -= copy_size;
		return copy_size; /* we copied this many bytes */
	}

	return 0; /* no more data left to deliver */
}

int get(CURL *curl, char *url, char *post, char *upass, MDB_env *env)
{
	struct string s;
	init_string(&s);

	struct post_data pd;
	pd.ptr = post;
	pd.size = strlen(pd.ptr);
	CURLcode res;
	curl_easy_setopt(curl, CURLOPT_URL, url);

	struct curl_slist *list = NULL;
	list = curl_slist_append(list, "Accept: application/json");
	list = curl_slist_append(list, "Content-Type: application/json,application/graphql");
	list = curl_slist_append(list, "User-Agent: gitfs-fetcher v1");
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list);
	curl_easy_setopt(curl, CURLOPT_POST, 1L);
	curl_easy_setopt(curl, CURLOPT_READFUNCTION, post_callback);
	curl_easy_setopt(curl, CURLOPT_READDATA, &pd);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writefunc);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);
	curl_easy_setopt(curl, CURLOPT_USERPWD, upass);

	res = curl_easy_perform(curl);
	curl_slist_free_all(list); /* free the list again */
	if (res != CURLE_OK) {
		fprintf(stderr, "curl_easy_perform() failed: %s\n",
			curl_easy_strerror(res));
		curl_easy_cleanup(curl);
		return 1;
	}
	long response_code;
	curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
	if (response_code >= 400) {
		printf("response code: %ld\n", response_code);
		exit(1);
	}

	struct fjson_object_iterator it;
	struct fjson_object_iterator itEnd;
	struct fjson_object *obj;
	obj = fjson_tokener_parse(s.ptr);
	struct fjson_object *next_obj   = NULL;
	struct fjson_object *cursor_obj = NULL;
	struct fjson_object *nodes      = NULL;
	struct fjson_object *page_info  = NULL;
	struct fjson_object *tmp        = obj;
	if (!fjson_object_object_get_ex(tmp, "data", &tmp)) {
		fprintf(stderr, "%s\n", fjson_object_to_json_string(tmp));
		fprintf(stderr, "failed to get data json_object\n");
		exit(EXIT_FAILURE);
	}
	if (!fjson_object_object_get_ex(tmp, "viewer", &tmp)) {
		fprintf(stderr, "%s\n", fjson_object_to_json_string(tmp));
		fprintf(stderr, "failed to get viewer json_object\n");
		exit(EXIT_FAILURE);
	}
	if (!fjson_object_object_get_ex(tmp, "starredRepositories", &tmp)) {
		fprintf(stderr, "%s\n", fjson_object_to_json_string(tmp));
		fprintf(stderr, "failed to get starred repos json_object\n");
		exit(EXIT_FAILURE);
	}
	if (!fjson_object_object_get_ex(tmp, "pageInfo", &page_info)) {
		fprintf(stderr, "%s\n", fjson_object_to_json_string(tmp));
		fprintf(stderr, "failed to get pageInfo json_object\n");
		exit(EXIT_FAILURE);
	}
	if (!fjson_object_object_get_ex(page_info, "hasNextPage", &next_obj)) {
		fprintf(stderr, "%s\n", fjson_object_to_json_string(page_info));
		fprintf(stderr, "failed to get hasNext json_object\n");
		exit(EXIT_FAILURE);
	}
	bool has_next = fjson_object_get_boolean(next_obj);

	if (!fjson_object_object_get_ex(page_info, "endCursor", &cursor_obj)) {
		fprintf(stderr, "%s\n", fjson_object_to_json_string(page_info));
		fprintf(stderr, "failed to get endCursor json_object\n");
		exit(EXIT_FAILURE);
	}
	const char *cursor = fjson_object_get_string(cursor_obj);

	if (!fjson_object_object_get_ex(tmp, "nodes", &nodes)) {
		fprintf(stderr, "%s\n", fjson_object_to_json_string(tmp));
		fprintf(stderr, "failed to get nodes json_object\n");
		exit(EXIT_FAILURE);
	}

	int rc;
	MDB_txn *txn;
	MDB_dbi  dbir, dbio;
	if ((rc = mdb_txn_begin(env, NULL, 0, &txn)) != 0) {
		fprintf(stderr, "failed to get txn (%d) %s\n", rc, mdb_strerror(rc));
		return -1;
	}
	if ((rc = mdb_dbi_open(txn, "repos", MDB_CREATE, &dbir)) != 0) {
		fprintf(stderr, "failed to open repos dbi (%d) %s\n", rc, mdb_strerror(rc));
		return -1;
	}
	if ((rc = mdb_dbi_open(txn, "orgs", MDB_CREATE, &dbio)) != 0) {
		fprintf(stderr, "failed to open orgs dbi (%d) %s\n", rc, mdb_strerror(rc));
		return -1;
	}

	int length = fjson_object_array_length(nodes);
	for (int i = 0; i < length; i++) {
		fjson_object *elm = fjson_object_array_get_idx(nodes, i);
		struct fjson_object_iterator it = fjson_object_iter_begin(elm);
		struct fjson_object_iterator itEnd = fjson_object_iter_end(elm);
		char *name = NULL;
		char *surl = NULL;
		char *description = NULL;
		char *fullpath = NULL;
		while (!fjson_object_iter_equal(&it, &itEnd)) {
			if (strcmp("name", fjson_object_iter_peek_name(&it)) == 0)
				name = (char *) fjson_object_get_string(fjson_object_iter_peek_value(&it));
			if (strcmp("sshUrl", fjson_object_iter_peek_name(&it)) == 0)
				surl = (char *) fjson_object_get_string(fjson_object_iter_peek_value(&it));
			if (strcmp("nameWithOwner", fjson_object_iter_peek_name(&it)) == 0)
				fullpath = (char *) fjson_object_get_string(fjson_object_iter_peek_value(&it));
			if (strcmp("description", fjson_object_iter_peek_name(&it)) == 0)
				description = (char *) fjson_object_get_string(fjson_object_iter_peek_value(&it));
			if (name != NULL && surl != NULL && description != NULL && fullpath != NULL) {
				repo r = new_repo(name, surl, fullpath, description);
				MDB_val key = { 0 };
				MDB_val val = { 0 };
				key.mv_size = strlen(fullpath) + 1;
				key.mv_data = strdup(fullpath);
				val.mv_data = &r;
				val.mv_size = sizeof(repo);

				MDB_val korg = { 0 };
				char *org = strdup(r.path[0]);
				org++;
				korg.mv_size = strlen(org) + 1;
				korg.mv_data = org;
				MDB_val vorg = { 0 };
				vorg.mv_size = strlen(org) + 1;
				vorg.mv_data = strdup(org);
				if ((rc = mdb_put(txn, dbir, &key, &val, 0)) != 0) {
					fprintf(stderr, "failed to add key [%s], to repos db, (%d) %s\n", (char *) key.mv_data, rc, mdb_strerror(rc));
					return -1;
				}
				if ((rc = mdb_put(txn, dbio, &korg, &vorg, 0)) != 0) {
					fprintf(stderr, "failed to add key [%s], to orgs db, (%d) %s\n", (char *) korg.mv_data, rc, mdb_strerror(rc));
					return -1;
				}
				break;
			}
			fjson_object_iter_next(&it);
		}
	}
	if (!fjson_object_iter_equal(&it, &itEnd)) {
		//do nothing
	}

	if ((rc = mdb_txn_commit(txn)) != 0) {
		fprintf(stderr, "failed commit transaction to repos, (%d) %s\n", rc, mdb_strerror(rc));
		return -1;
	}
	if ((rc = mdb_env_sync(env, 1)) != 0) {
		fprintf(stderr, "failed to sync repos to disk, (%d) %s\n", rc, mdb_strerror(rc));
		return -1;
	}
	free(s.ptr);

	mdb_dbi_close(env, dbir);
	mdb_dbi_close(env, dbio);
	//mdb_txn_abort(txn);
	if (has_next) {
		char *q = "{\"query\": \"{ viewer { starredRepositories(first: 35, after: \\\"\\\") { nodes { name nameWithOwner sshUrl description } pageInfo { endCursor hasNextPage } } } } \"}";
		char *content = malloc(strlen(q) + strlen(cursor) + 3);
		memset(content, 0, strlen(q) + strlen(cursor) + 3);
		snprintf(content, strlen(q) + strlen(cursor) + 3, "{\"query\": \"{ viewer { starredRepositories(first: 35, after: \\\"%s\\\" ) { nodes { name nameWithOwner sshUrl description } pageInfo { endCursor hasNextPage } } } } \"}", cursor);
		get(curl, url, content, upass, env);
		free(content);
	}
	return 0;
}

