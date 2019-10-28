#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <curl/curl.h>
#include <libfastjson/json.h>

struct string {
	char *ptr;
	size_t len;
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

struct resp {
	struct string s;
	char   *hdr;
};

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
char *trim(char *str)
{
  char *end;

  // Trim leading space
  while(isspace((unsigned char)*str)) str++;

  if(*str == 0)  // All spaces?
    return str;

  // Trim trailing space
  end = str + strlen(str) - 1;
  while(end > str && isspace((unsigned char)*end)) end--;

  // Write new null terminator character
  end[1] = '\0';

  return str;
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

int get(CURL *curl, char *url, char *upass)
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
	curl_easy_cleanup(curl);
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
		while (!fjson_object_iter_equal(&it, &itEnd)) {
			if (strcmp("name", fjson_object_iter_peek_name(&it)) == 0)
				printf("%s: ", fjson_object_get_string(fjson_object_iter_peek_value(&it)));
			if (strcmp("ssh_url", fjson_object_iter_peek_name(&it)) == 0)
				printf("%s\n", fjson_object_get_string(fjson_object_iter_peek_value(&it)));
			fjson_object_iter_next(&it);
		}
	}
	if (!fjson_object_iter_equal(&it, &itEnd)) {
		fprintf(stderr, "there was an error processing the json object from: %s\n", url);
		exit(EXIT_FAILURE);
	}

	free(r.s.ptr);
	if (r.hdr != NULL) {
		curl = curl_easy_init();
		get(curl, r.hdr, upass);
	}

	return 0;
}
int main(void) {
	char *user = getenv("GH_USER");
	char *pass = getenv("GH_TOKEN");
	char *upass = malloc(strlen(user) + strlen(pass) + 3);
	memset(upass, 0, strlen(user) + strlen(pass) + 3);
	char *trunc = "https://api.github.com/users//starred";
	char *url = malloc(strlen(trunc)+ strlen(user) + 2);
	memset(url, 0, strlen(trunc) + strlen(user) + 2);

	snprintf(upass, strlen(pass) + strlen(user) + 2, "%s:%s", user, pass);
	snprintf(url, strlen(trunc) + strlen(user) + 2, "https://api.github.com/users/%s/starred", user);
	printf("url: %s\n", url);
	CURL *curl = curl_easy_init();
	if(curl) {
		get(curl, url, upass);
	} else
		exit(1);
	return 0;
}
