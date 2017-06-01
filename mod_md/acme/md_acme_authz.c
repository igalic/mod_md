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

#include <stdio.h>

#include <apr_lib.h>
#include <apr_file_info.h>
#include <apr_file_io.h>
#include <apr_fnmatch.h>
#include <apr_hash.h>
#include <apr_strings.h>
#include <apr_tables.h>

#include "../md_json.h"
#include "../md_log.h"
#include "../md_jws.h"
#include "../md_util.h"

#include "md_acme.h"
#include "md_acme_acct.h"
#include "md_acme_authz.h"

static apr_status_t authz_create(md_acme_authz_t **pauthz, apr_pool_t *p, 
                                 const char *domain, md_acme_acct_t *acct)
{
    md_acme_authz_t *authz;
    
    md_log_perror(MD_LOG_MARK, MD_LOG_TRACE2, 0, p, "generating new authz for %s", domain);
    
    authz = apr_pcalloc(p, sizeof(*authz));
    authz->domain = apr_pstrdup(p, domain);
    authz->acct = acct;
    authz->challenges = apr_array_make(p, 5, sizeof(md_acme_challenge_t *));
    *pauthz = authz;
      
    return APR_SUCCESS;
}

/**************************************************************************************************/
/* Register a new authorization */

static apr_status_t on_init_authz(md_acme_req_t *req, void *baton)
{
    md_acme_authz_t *authz = baton;
    md_acme_acct_t *acct = authz->acct;
    md_json_t *jpayload;

    jpayload = md_json_create(req->pool);
    md_json_sets("new-authz", jpayload, "resource", NULL);
    md_json_sets("dns", jpayload, "identifier", "type", NULL);
    md_json_sets(authz->domain, jpayload, "identifier", "value", NULL);
    
    return md_acme_req_body_init(req, jpayload, acct->key);
} 

static apr_status_t on_success_authz(md_acme_t *acme, const apr_table_t *hdrs, md_json_t *body, void *baton)
{
    md_acme_authz_t *authz = baton;
    md_acme_acct_t *acct = authz->acct;
    const char *location = apr_table_get(hdrs, "location");
    
    if (location) {
        md_log_perror(MD_LOG_MARK, MD_LOG_TRACE1, 0, acct->pool, 
                      "authz_new success: url=%s\n%s", location, 
                      md_json_writep(body, MD_JSON_FMT_INDENT, acct->pool));
        authz->url = apr_pstrdup(acct->pool, location);
        return APR_SUCCESS;
    }
    else {
        md_log_perror(MD_LOG_MARK, MD_LOG_WARNING, APR_EINVAL, acct->pool, 
                      "new authz without location header");
        return APR_EINVAL;
    }
}

apr_status_t md_acme_authz_register(struct md_acme_authz_t **pauthz, const char *domain, 
                                    md_acme_acct_t *acct)
{
    md_acme_t *acme;
    md_acme_authz_t *authz;
    apr_status_t rv;
    
    md_log_perror(MD_LOG_MARK, MD_LOG_DEBUG, 0, acct->pool, "create new authz");
    rv = authz_create(&authz, acct->pool, domain, acct);
    if (APR_SUCCESS != rv) {
        return rv;
    }
    
     
    md_log_perror(MD_LOG_MARK, MD_LOG_DEBUG, 0, acct->pool, "agree to terms-of-service");
    
    if (APR_SUCCESS == (rv = md_acme_create(&acme, acct->pool, acct->ca_url, acct->store))
        && APR_SUCCESS == (rv = md_acme_setup(acme))) {
        
        rv = md_acme_req_do(acme, acme->new_authz, on_init_authz, on_success_authz, authz);
    }
    *pauthz = (APR_SUCCESS == rv)? authz : NULL;
    return rv;
} 

