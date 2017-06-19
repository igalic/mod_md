/* Copyright 2017 greenbytes GmbH (https://www.greenbytes.de)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef mod_md_md_h
#define mod_md_md_h

struct apr_array_header_t;
struct apr_hash_t;
struct md_json_t;
struct md_cert_t;
struct md_pkey_t;
struct md_store_t;

typedef enum {
    MD_S_UNKNOWN,                   /* MD has not been analysed yet */
    MD_S_INCOMPLETE,                /* MD is missing necessary information, can go live */
    MD_S_COMPLETE,                  /* MD has all necessary information, can go live */
    MD_S_INACTIVE,                  /* MD is inactive, not to go live */
    MD_S_ERROR,                     /* MD data is flawed, unable to be processed as is */ 
} md_state_t;

typedef struct md_t md_t;
struct md_t {
    const char         *name;       /* unique name of this MD */
    struct apr_array_header_t *domains; /* all DNS names this MD includes */

    md_state_t state;               /* state of this MD */
    int proto_state;                /* state of renewal process, protocol specific */

    const char *ca_url;             /* url of CA certificate service */
    const char *ca_proto;           /* protocol used vs CA (e.g. ACME) */
    const char *ca_account;         /* account used at CA */
    const char *ca_agreement;       /* accepted agreement uri between CA and user */ 
    apr_array_header_t *contacts;   /* list of contact uris, e.g. mailto:xxx */

    const char *defn_name;          /* config file this MD was defined */
    unsigned defn_line_number;      /* line number of definition */
};

#define MD_KEY_ACCOUNT          "account"
#define MD_KEY_AGREEMENT        "agreement"
#define MD_KEY_CA               "ca"
#define MD_KEY_CA_URL           "ca-url"
#define MD_KEY_CHALLENGES       "challenges"
#define MD_KEY_CONTACT          "contact"
#define MD_KEY_CONTACTS         "contacts"
#define MD_KEY_DISABLED         "disabled"
#define MD_KEY_DOMAINS          "domains"
#define MD_KEY_ID               "id"
#define MD_KEY_IDENTIFIER       "identifier"
#define MD_KEY_KEYAUTHZ         "keyAuthorization"
#define MD_KEY_NAME             "name"
#define MD_KEY_PROTO            "proto"
#define MD_KEY_REGISTRATION     "registration"
#define MD_KEY_RESOURCE         "resource"
#define MD_KEY_STATE            "state"
#define MD_KEY_STATUS           "status"
#define MD_KEY_TOKEN            "token"
#define MD_KEY_TYPE             "type"
#define MD_KEY_URL              "url"
#define MD_KEY_URI              "uri"
#define MD_KEY_VALUE            "value"
#define MD_KEY_VERSION          "version"

#define MD_FN_MD                "md.json"
#define MD_FN_PKEY              "pkey.pem"
#define MD_FN_CERT              "cert.pem"
#define MD_FN_CHAIN             "chain.pem"


/**
 * Determine if the Managed Domain contains a specific domain name.
 */
int md_contains(const md_t *md, const char *domain);

/**
 * Determine if the names of the two managed domains overlap.
 */
int md_domains_overlap(const md_t *md1, const md_t *md2);

/**
 * Determine if the domain names are equal.
 */
int md_equal_domains(const md_t *md1, const md_t *md2);

/**
 * Determine if the domains in md1 contain all domains of md2.
 */
int md_contains_domains(const md_t *md1, const md_t *md2);

/**
 * Get one common domain name of the two managed domains or NULL.
 */
const char *md_common_name(const md_t *md1, const md_t *md2);

/**
 * Look up a managed domain by its name.
 */
md_t *md_get_by_name(struct apr_array_header_t *mds, const char *name);

/**
 * Find a managed domain, different from the given one, that has overlaps
 * in the domain list.
 */
md_t *md_get_by_dns_overlap(struct apr_array_header_t *mds, const md_t *md);

/**
 * Create and empty md record, structures initialized.
 */
md_t *md_create_empty(apr_pool_t *p);

/**
 * Create a managed domain, given a list of domain names.
 */
const char *md_create(md_t **pmd, apr_pool_t *p, struct apr_array_header_t *domains);

/**
 * Deep copy an md record into another pool.
 */
md_t *md_clone(apr_pool_t *p, const md_t *src);

/**
 * Shallow copy an md record into another pool.
 */
md_t *md_copy(apr_pool_t *p, const md_t *src);

/** 
 * Convert the managed domain into a JSON representation and vice versa. 
 *
 * This reads and writes the following information: name, domains, ca_url, ca_proto and state.
 */
struct md_json_t *md_to_json (const md_t *md, apr_pool_t *p);
md_t *md_from_json(struct md_json_t *json, apr_pool_t *p);

/* Storage handling of managed domains */
apr_status_t md_load(struct md_store_t *store, const char *name, md_t **pmd, apr_pool_t *p);
apr_status_t md_save(struct md_store_t *store, md_t *md, int create);
apr_status_t md_remove(struct md_store_t *store, const char *name, int force);
apr_status_t md_load_all(struct apr_array_header_t **pmds, struct md_store_t *store, apr_pool_t *p);

apr_status_t md_load_pkey(struct md_store_t *store, const char *name, 
                          struct md_pkey_t **ppkey, apr_pool_t *p);
apr_status_t md_load_cert(struct md_store_t *store, const char *name, 
                          struct md_cert_t **pcert, apr_pool_t *p);
apr_status_t md_load_chain(struct md_store_t *store, const char *name, 
                           struct apr_array_header_t **pchain, apr_pool_t *p);

/**************************************************************************************************/
/* domain credentials */

typedef struct md_creds_t md_creds_t;
struct md_creds_t {
    struct md_cert_t *cert;
    struct md_pkey_t *pkey;
    struct apr_array_header_t *chain;      /* list of md_cert* */
    int expired;
};

#endif /* mod_md_md_h */
