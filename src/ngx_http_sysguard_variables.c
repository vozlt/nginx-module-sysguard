
/*
 * Copyright (C) 2010-2015 Alibaba Group Holding Limited
 * Copyright (C) YoungJoo Kim (vozlt)
 */


#include "ngx_http_sysguard_module.h"
#include "ngx_http_sysguard_variables.h"


static ngx_http_variable_t  ngx_http_sysguard_vars[] = {

    { ngx_string("sysguard_load"), NULL,
      ngx_http_sysguard_sysinfo_variable,
      offsetof(ngx_http_sysguard_sysinfo_t, cached_load),
      NGX_HTTP_VAR_NOCACHEABLE, 0 },

    { ngx_string("sysguard_swapstat"), NULL,
      ngx_http_sysguard_sysinfo_variable,
      offsetof(ngx_http_sysguard_sysinfo_t, cached_swapstat),
      NGX_HTTP_VAR_NOCACHEABLE, 0 },

    { ngx_string("sysguard_free"), NULL,
      ngx_http_sysguard_sysinfo_variable,
      offsetof(ngx_http_sysguard_sysinfo_t, cached_free),
      NGX_HTTP_VAR_NOCACHEABLE, 0 },

    { ngx_string("sysguard_rt"), NULL,
      ngx_http_sysguard_sysinfo_variable,
      offsetof(ngx_http_sysguard_sysinfo_t, cached_rt),
      NGX_HTTP_VAR_NOCACHEABLE, 0 },

    { ngx_string("sysguard_meminfo_totalram"), NULL,
      ngx_http_sysguard_sysinfo_variable,
      offsetof(ngx_http_sysguard_sysinfo_t, meminfo)
      + offsetof(ngx_http_sysguard_meminfo_t, totalram),
      NGX_HTTP_VAR_NOCACHEABLE, 0 },

    { ngx_string("sysguard_meminfo_freeram"), NULL,
      ngx_http_sysguard_sysinfo_variable,
      offsetof(ngx_http_sysguard_sysinfo_t, meminfo)
      + offsetof(ngx_http_sysguard_meminfo_t, freeram),
      NGX_HTTP_VAR_NOCACHEABLE, 0 },

    { ngx_string("sysguard_meminfo_bufferram"), NULL,
      ngx_http_sysguard_sysinfo_variable,
      offsetof(ngx_http_sysguard_sysinfo_t, meminfo)
      + offsetof(ngx_http_sysguard_meminfo_t, bufferram),
      NGX_HTTP_VAR_NOCACHEABLE, 0 },

    { ngx_string("sysguard_meminfo_cachedram"), NULL,
      ngx_http_sysguard_sysinfo_variable,
      offsetof(ngx_http_sysguard_sysinfo_t, meminfo)
      + offsetof(ngx_http_sysguard_meminfo_t, cachedram),
      NGX_HTTP_VAR_NOCACHEABLE, 0 },

    { ngx_string("sysguard_meminfo_totalswap"), NULL,
      ngx_http_sysguard_sysinfo_variable,
      offsetof(ngx_http_sysguard_sysinfo_t, meminfo)
      + offsetof(ngx_http_sysguard_meminfo_t, totalswap),
      NGX_HTTP_VAR_NOCACHEABLE, 0 },

    { ngx_string("sysguard_meminfo_freeswap"), NULL,
      ngx_http_sysguard_sysinfo_variable,
      offsetof(ngx_http_sysguard_sysinfo_t, meminfo)
      + offsetof(ngx_http_sysguard_meminfo_t, freeswap),
      NGX_HTTP_VAR_NOCACHEABLE, 0 },

    { ngx_null_string, NULL, NULL, 0, 0, 0 }
};

    
ngx_int_t
ngx_http_sysguard_sysinfo_variable(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{

    u_char                    *p;
    ngx_http_sysguard_conf_t  *glcf;

    glcf = ngx_http_get_module_loc_conf(r, ngx_http_sysguard_module);

    p = ngx_pnalloc(r->pool, NGX_INT_T_LEN);
    if (p == NULL) {
        goto not_found;
    }

    v->len = ngx_sprintf(p, "%i", *((ngx_int_t *) ((char *) &glcf->sysinfo + data))) - p;
    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;
    v->data = p;

    goto done;

not_found:

    v->not_found = 1;

done:

    return NGX_OK;
}


ngx_int_t
ngx_http_sysguard_add_variables(ngx_conf_t *cf)
{
    ngx_http_variable_t  *var, *v;

    for (v = ngx_http_sysguard_vars; v->name.len; v++) {
        var = ngx_http_add_variable(cf, &v->name, v->flags);
        if (var == NULL) {
            return NGX_ERROR;
        }

        var->get_handler = v->get_handler;
        var->data = v->data;
    }

    return NGX_OK;
}

/* vi:set ft=c ts=4 sw=4 et fdm=marker: */
