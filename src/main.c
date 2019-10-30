#define FUSE_USE_VERSION 26
#define _FILE_OFFSET_BITS 64

#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>

#include <fuse.h>
#include <curl/curl.h>
#include <libfastjson/json.h>

#include "repo.h"

struct string {
	char *ptr;
	size_t len;
};

struct resp {
	struct string s;
	char   *hdr;
};

struct list *repos;

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
	struct resp *r = (struct resp *) data;
	size_t new_len = r->s.len + size*nmemb;
	r->s.ptr = realloc(r->s.ptr, new_len+1);
	if (r->s.ptr == NULL) {
		fprintf(stderr, "realloc() failed\n");
		exit(EXIT_FAILURE);
	}
	memcpy(r->s.ptr+r->s.len, ptr, size*nmemb);
	r->s.ptr[new_len] = '\0';
	r->s.len = new_len;

	return size*nmemb;
}

static size_t header_callback(char *buffer, size_t size, size_t nitems, void *data)
{
	if (strncmp("Link: ", buffer, 6) == 0) {
		struct resp *r = (struct resp *)data;
		r->hdr = malloc(nitems * size - 5);
		memset(r->hdr, 0, nitems * size - 5);
		memcpy(r->hdr, buffer + 6, nitems * size - 6);
		r->hdr[nitems * size - 6] = '\0';
		char* tok = strtok(strdup(r->hdr), ",");
		while (tok != NULL) {
			if (strstr(tok, "rel=\"next\"") != NULL) {
				char *tmp = trim(strdup(tok));
				tmp[strlen(tmp) - 13] = '\0';
				r->hdr = trim(strdup(tmp) + 1);
				break;
			}
			r->hdr = NULL;
			tok = strtok(NULL, ",");
		}
		//memset(tok, 0, strlen(tok));
	}
	return nitems * size;
}

int get(CURL *curl, char *url, char *upass, struct list *repos)
{
	struct resp r;
	init_string(&r.s);
	r.hdr = malloc(4096);
	memset(r.hdr, 0, 4096);
	CURLcode res;
	curl_easy_setopt(curl, CURLOPT_URL, url);

	struct curl_slist *list = NULL;
	list = curl_slist_append(list, "Accept: application/vnd.github.v3+json");
	list = curl_slist_append(list, "User-Agent: new-tool github check v1");
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list);
	curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, header_callback);
	curl_easy_setopt(curl, CURLOPT_HEADERDATA, &r);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writefunc);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &r);
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
	struct fjson_object* obj;

	obj = fjson_tokener_parse(r.s.ptr);
	int length = fjson_object_array_length(obj);
	for (int i = 0; i < length; i++) {
		fjson_object *elm = fjson_object_array_get_idx(obj, i);
		struct fjson_object_iterator it = fjson_object_iter_begin(elm);
		struct fjson_object_iterator itEnd = fjson_object_iter_end(elm);
		char *name = NULL;
		char *url = NULL;
		char *description = NULL;
		char *fullpath = NULL;
		repo *r;
		while (!fjson_object_iter_equal(&it, &itEnd)) {
			if (strcmp("name", fjson_object_iter_peek_name(&it)) == 0)
				name = (char *) fjson_object_get_string(fjson_object_iter_peek_value(&it));
			if (strcmp("ssh_url", fjson_object_iter_peek_name(&it)) == 0)
				url = (char *) fjson_object_get_string(fjson_object_iter_peek_value(&it));
			if (strcmp("full_name", fjson_object_iter_peek_name(&it)) == 0)
				fullpath = (char *) fjson_object_get_string(fjson_object_iter_peek_value(&it));
			if (strcmp("description", fjson_object_iter_peek_name(&it)) == 0)
				description = (char *) fjson_object_get_string(fjson_object_iter_peek_value(&it));
			if (name != NULL && url != NULL && description != NULL && fullpath != NULL) {
				r = new_repo(name, url, fullpath, description);
				add_el(repos, new_el(r));
				break;
			}
			fjson_object_iter_next(&it);
		}
	}
	if (!fjson_object_iter_equal(&it, &itEnd)) {
		//do nothing
	}

	free(r.s.ptr);
	if (r.hdr != NULL)
		get(curl, r.hdr, upass, repos);

	return 0;
}

char **dup;
int   dup_length = 1;

void dedup_list(struct list *li)
{
	struct el *elm = li->first;
	repo *r;
	while (elm != NULL) {
		r = elm->data;
		char *tmp = strdup(r->path[0]);
		tmp++;
		for (int i = 0; i < dup_length; i++) {
			if (i == 0 && dup_length == 1) {
				// list is uninitialized
				dup[i] = malloc(strlen(tmp));
				dup[i] = strdup(tmp);
				dup_length++;
				break;
			} else if (i == dup_length - 1) { // last element
				dup[i] = malloc(strlen(tmp));
				dup[i] = strdup(tmp);
				dup_length++;
				break;
			} else if (strcmp(tmp, dup[i]) == 0) {
				break;
			}
		}
		elm = elm->next;
	}
	dup_length--;
}

static int readdir_callback(const char *path, void *buf, fuse_fill_dir_t filler,
	off_t offset, __attribute__((unused)) struct fuse_file_info *fi)
{
	(void) offset;
	(void) fi;

	filler(buf, ".", NULL, 0);
	filler(buf, "..", NULL, 0);

	if (strncmp(path, "/", 2) == 0) {
		for (int i = 0; i < dup_length; i++) {
			filler(buf, dup[i], NULL, 0);
		}
	} else {
		struct el *elm = repos->first;
		repo *r;
		while (elm != NULL) {
			r = elm->data;
			if (strcmp(path, r->path[0]) == 0) {
				filler(buf, r->name, NULL, 0);
			}
			elm = elm->next;
		}
	}

	return 0;
}

static int getattr_callback(const char *path, struct stat *stbuf)
{
	memset(stbuf, 0, sizeof(struct stat));

	if (strncmp(path, "/", 2) == 0) {
		stbuf->st_mode = S_IFDIR | 0755;
		stbuf->st_nlink = 2;
		return 0;
	}

	struct el *elm = repos->first;
	repo *r;
	while (elm != NULL) {
		r = elm->data;
		if (strcmp(path, r->path[0]) == 0) {
			stbuf->st_mode = S_IFDIR | 0755;
			stbuf->st_nlink = 2;
			return 0;
		}
		elm = elm->next;
	}

	elm = repos->first;
	while (elm != NULL) {
		r = elm->data;
		if (strcmp(path, r->path[1]) == 0) {
			stbuf->st_mode = S_IFREG | 0444;
			stbuf->st_nlink = 1;
			stbuf->st_size = strlen(repo_string(r));
			return 0;
		}
		elm = elm->next;
	}

	return -ENOENT;
}

static int open_callback(__attribute__((unused)) const char *path, __attribute__((unused)) struct fuse_file_info *fi) {
	return 0;
}

static int read_callback(const char *path, char *buf, size_t size, off_t offset,
	__attribute__((unused)) struct fuse_file_info *fi)
{
	struct el *elm = repos->first;
	repo *r;
	while (elm != NULL) {
		r = elm->data;
		if (strncmp(path, r->path[1], strlen(r->path[1]) + 1) == 0) {
			char *content = repo_string(r);
			size_t len = strlen(content);
			if ((size_t)offset >= len) {
				return 0;
			}

			if (offset + size > len) {
				memcpy(buf, content + offset, len - offset);
				return len - offset;
			}

			memcpy(buf, content + offset, size);
			return size;
		}
		elm = elm->next;
	}
	return -ENOENT;
}

static struct fuse_operations fuse_fetcher_opts = {
	.getattr = getattr_callback,
	.open    = open_callback,
	.read    = read_callback,
	.readdir = readdir_callback,
};


int main(int argc, char *argv[])
{

	char *user = getenv("GH_USER");
	char *pass = getenv("GH_TOKEN");

	char *upass = malloc(strlen(user) + strlen(pass) + 3);
	memset(upass, 0, strlen(user) + strlen(pass) + 3);

	char *trunc = "https://api.github.com/users//starred";
	char *url = malloc(strlen(trunc)+ strlen(user) + 2);
	memset(url, 0, strlen(trunc) + strlen(user) + 2);

	snprintf(upass, strlen(pass) + strlen(user) + 3, "%s:%s", user, pass);
	snprintf(url, strlen(trunc) + strlen(user) + 2, "https://api.github.com/users/%s/starred", user);
	printf("fetching stars...\n");
	repos = malloc(sizeof(struct list));
	memset(repos, 0, sizeof(struct list));
	CURL *curl = curl_easy_init();
	if (curl) {
		get(curl, url, upass, repos);
		curl_easy_cleanup(curl);
	} else {
		curl_easy_cleanup(curl);
		exit(1);
	}
	printf("  finished\n");
	printf("deduping orgs\n");
	dup = malloc(sizeof(char **) * list_len(repos));
	dedup_list(repos);
	printf("finished\n");
	printf("mounting filesystem\n");

	return fuse_main(argc, argv, &fuse_fetcher_opts, NULL);
}
