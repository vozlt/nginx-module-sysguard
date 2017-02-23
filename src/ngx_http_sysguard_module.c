/*
 * Copyright (C) 2010-2015 Alibaba Group Holding Limited
 * Copyright (C) YoungJoo Kim (vozlt)
 */


#include "ngx_http_sysguard_module.h"
#include "ngx_http_sysguard_variables.h"


static void *ngx_http_sysguard_create_conf(ngx_conf_t *cf);
static char *ngx_http_sysguard_merge_conf(ngx_conf_t *cf, void *parent,
    void *child);
static char *ngx_http_sysguard_load(ngx_conf_t *cf, ngx_command_t *cmd,
    void *conf);
static char *ngx_http_sysguard_mem(ngx_conf_t *cf, ngx_command_t *cmd,
    void *conf);
static char *ngx_http_sysguard_rt(ngx_conf_t *cf,
    ngx_command_t *cmd, void *conf);
static ngx_int_t ngx_http_sysguard_init(ngx_conf_t *cf);


static ngx_conf_enum_t  ngx_http_sysguard_log_levels[] = {
    { ngx_string("info"), NGX_LOG_INFO },
    { ngx_string("notice"), NGX_LOG_NOTICE },
    { ngx_string("warn"), NGX_LOG_WARN },
    { ngx_string("error"), NGX_LOG_ERR },
    { ngx_null_string, 0 }
};


static ngx_conf_enum_t  ngx_http_sysguard_modes[] = {
    { ngx_string("or"), NGX_HTTP_SYSGUARD_MODE_OR },
    { ngx_string("and"), NGX_HTTP_SYSGUARD_MODE_AND },
    { ngx_null_string, 0 }
};


static ngx_command_t  ngx_http_sysguard_commands[] = {

    { ngx_string("sysguard"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_FLAG,
      ngx_conf_set_flag_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_sysguard_conf_t, enable),
      NULL },

    { ngx_string("sysguard_mode"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_FLAG,
      ngx_conf_set_enum_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_sysguard_conf_t, mode),
      &ngx_http_sysguard_modes },

    { ngx_string("sysguard_load"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE12,
      ngx_http_sysguard_load,
      NGX_HTTP_LOC_CONF_OFFSET,
      0,
      NULL },

    { ngx_string("sysguard_mem"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE12,
      ngx_http_sysguard_mem,
      NGX_HTTP_LOC_CONF_OFFSET,
      0,
      NULL },

    { ngx_string("sysguard_rt"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1234,
      ngx_http_sysguard_rt,
      NGX_HTTP_LOC_CONF_OFFSET,
      0,
      NULL },

    { ngx_string("sysguard_interval"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_sec_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_sysguard_conf_t, interval),
      NULL },

    { ngx_string("sysguard_log_level"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_enum_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_sysguard_conf_t, log_level),
      &ngx_http_sysguard_log_levels },

      ngx_null_command
};


static ngx_http_module_t  ngx_http_sysguard_module_ctx = {
    ngx_http_sysguard_add_variables,        /* preconfiguration */
    ngx_http_sysguard_init,                 /* postconfiguration */

    NULL,                                   /* create main configuration */
    NULL,                                   /* init main configuration */

    NULL,                                   /* create server configuration */
    NULL,                                   /* merge server configuration */

    ngx_http_sysguard_create_conf,          /* create location configuration */
    ngx_http_sysguard_merge_conf            /* merge location configuration */
};


ngx_module_t  ngx_http_sysguard_module = {
    NGX_MODULE_V1,
    &ngx_http_sysguard_module_ctx,          /* module context */
    ngx_http_sysguard_commands,             /* module directives */
    NGX_HTTP_MODULE,                        /* module type */
    NULL,                                   /* init master */
    NULL,                                   /* init module */
    NULL,                                   /* init process */
    NULL,                                   /* init thread */
    NULL,                                   /* exit thread */
    NULL,                                   /* exit process */
    NULL,                                   /* exit master */
    NGX_MODULE_V1_PADDING
};


static ngx_int_t
ngx_http_sysguard_update_load(ngx_http_request_t *r, time_t exptime)
{
    ngx_int_t                  load, rc;
    ngx_http_sysguard_conf_t  *glcf;

    glcf = ngx_http_get_module_loc_conf(r, ngx_http_sysguard_module);

    glcf->sysinfo.cached_load_exptime = ngx_time() + exptime;

    rc = ngx_http_sysguard_getloadavg(&load, 1, r->connection->log);
    if (rc == NGX_ERROR) {
        glcf->sysinfo.cached_load = 0;

        return NGX_ERROR;
    }

    glcf->sysinfo.cached_load = load;

    return NGX_OK;
}


static ngx_int_t
ngx_http_sysguard_update_mem(ngx_http_request_t *r, time_t exptime)
{
    ngx_int_t                     rc;
    ngx_http_sysguard_meminfo_t   m;
    ngx_http_sysguard_conf_t     *glcf;

    glcf = ngx_http_get_module_loc_conf(r, ngx_http_sysguard_module);

    glcf->sysinfo.cached_mem_exptime = ngx_time() + exptime;

    rc = ngx_http_sysguard_getmeminfo(&m, r->connection->log);
    if (rc == NGX_ERROR) {
        glcf->sysinfo.cached_swapstat = 0;
        glcf->sysinfo.cached_free = NGX_CONF_UNSET_SIZE;

        return NGX_ERROR;
    }

    glcf->sysinfo.meminfo = m;
    glcf->sysinfo.cached_swapstat = (m.totalswap == 0)
                                    ? 0
                                    : (m.totalswap - m.freeswap) * 100 / m.totalswap;
    glcf->sysinfo.cached_free = m.freeram + m.cachedram + m.bufferram;

    return NGX_OK;
}


static ngx_int_t
ngx_http_sysguard_update_rt(ngx_http_request_t *r, time_t exptime)
{
    ngx_http_sysguard_conf_t  *glcf;

    glcf = ngx_http_get_module_loc_conf(r, ngx_http_sysguard_module);

    glcf->sysinfo.cached_rt_exptime = ngx_time() + exptime;

    glcf->sysinfo.cached_rt = (glcf->rt_method == NGX_HTTP_SYSGUARD_AVERAGE_AMM)
                                        ? ngx_http_sysguard_node_time_ring_average_amm(
                                              r, &glcf->request_times)
                                        : ngx_http_sysguard_node_time_ring_average_wma(
                                              r, &glcf->request_times);

    return NGX_OK;
}


void
ngx_http_sysguard_update_rt_node(ngx_http_request_t *r)
{
    ngx_http_sysguard_conf_t  *glcf;

    glcf = ngx_http_get_module_loc_conf(r, ngx_http_sysguard_module);

    if (!glcf->enable) {
        return;
    }

    if (glcf->rt == NGX_CONF_UNSET) {
        return;
    }

    ngx_http_sysguard_node_time_ring_insert(&glcf->request_times,
                                            ngx_http_sysguard_request_time(r));
}


static ngx_int_t
ngx_http_sysguard_do_redirect(ngx_http_request_t *r, ngx_str_t *path)
{
    if (path->len == 0) {
        return NGX_HTTP_SERVICE_UNAVAILABLE;
    } else if (path->data[0] == '@') {
        (void) ngx_http_named_location(r, path);
    } else {
        (void) ngx_http_internal_redirect(r, path, &r->args);
    }

    ngx_http_finalize_request(r, NGX_DONE);

    return NGX_DONE;
}


static ngx_int_t
ngx_http_sysguard_handler(ngx_http_request_t *r)
{
    ngx_int_t                  load_log = 0, swap_log = 0,
                               free_log = 0, rt_log = 0;
    ngx_str_t                 *action = NULL;
    ngx_http_sysguard_conf_t  *glcf;


    /*
     * About ignoring the subrequests:
     * 
     *   The below is not guaranteed but it works well in general.
     *
     *   if (r->main->count != 1) {
     *     return NGX_DECLINED;
     *   }
     */

    glcf = ngx_http_get_module_loc_conf(r, ngx_http_sysguard_module);
    
    if (!glcf->enable) {
        return NGX_DECLINED;
    }

    /* load */

    if (glcf->load != NGX_CONF_UNSET) {

        if (glcf->sysinfo.cached_load_exptime < ngx_time()) {
            ngx_http_sysguard_update_load(r, glcf->interval);
        }

        ngx_log_debug4(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                       "http sysguard handler load: %1.3f %1.3f %V %V",
                       glcf->sysinfo.cached_load * 1.0 / 1000,
                       glcf->load * 1.0 / 1000,
                       &r->uri,
                       &glcf->load_action);

        if (glcf->sysinfo.cached_load > glcf->load) {

            if (glcf->mode == NGX_HTTP_SYSGUARD_MODE_OR) {

                ngx_log_error(glcf->log_level, r->connection->log, 0,
                              "sysguard load limited, current:%1.3f conf:%1.3f",
                              glcf->sysinfo.cached_load * 1.0 / 1000,
                              glcf->load * 1.0 / 1000);

                return ngx_http_sysguard_do_redirect(r, &glcf->load_action);
            } else {
                action = &glcf->load_action;
                load_log = 1;
            }
        } else {
            if (glcf->mode == NGX_HTTP_SYSGUARD_MODE_AND) {
                goto out;
            }
        }
    }

    /* swap */

    if (glcf->swap != NGX_CONF_UNSET) {

        if (glcf->sysinfo.cached_mem_exptime < ngx_time()) {
            ngx_http_sysguard_update_mem(r, glcf->interval);
        }

        ngx_log_debug4(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                       "http sysguard handler swap: %i %i %V %V",
                       glcf->sysinfo.cached_swapstat,
                       glcf->swap,
                       &r->uri,
                       &glcf->swap_action);

        if (glcf->sysinfo.cached_swapstat > glcf->swap) {

            if (glcf->mode == NGX_HTTP_SYSGUARD_MODE_OR) {

                ngx_log_error(glcf->log_level, r->connection->log, 0,
                              "sysguard swap limited, current:%i conf:%i",
                              glcf->sysinfo.cached_swapstat,
                              glcf->swap);

                return ngx_http_sysguard_do_redirect(r, &glcf->swap_action);
            } else {
                action = &glcf->swap_action;
                swap_log = 1;
            }
        } else {
            if (glcf->mode == NGX_HTTP_SYSGUARD_MODE_AND) {
                goto out;
            }
        }
    }

    /* mem free */

    if (glcf->free != NGX_CONF_UNSET_SIZE) {

        if (glcf->sysinfo.cached_mem_exptime < ngx_time()) {
            ngx_http_sysguard_update_mem(r, glcf->interval);
        }

        if (glcf->sysinfo.cached_free != NGX_CONF_UNSET_SIZE) {

            ngx_log_debug4(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                           "http sysguard handler free: %uz %uz %V %V",
                           glcf->sysinfo.cached_free,
                           glcf->free,
                           &r->uri,
                           &glcf->free_action);

            if (glcf->sysinfo.cached_free < glcf->free) {

                if (glcf->mode == NGX_HTTP_SYSGUARD_MODE_OR) {

                    ngx_log_error(glcf->log_level, r->connection->log, 0,
                                  "sysguard free limited, "
                                  "current:%uzM conf:%uzM",
                                  glcf->sysinfo.cached_free / 1024 / 1024,
                                  glcf->free / 1024 / 1024);

                    return ngx_http_sysguard_do_redirect(r, &glcf->free_action);
                } else {
                    action = &glcf->free_action;
                    free_log = 1;
                }
            } else {
                if (glcf->mode == NGX_HTTP_SYSGUARD_MODE_AND) {
                    goto out;
                }
            }
        }
    }

    /* response time */

    if (glcf->rt != NGX_CONF_UNSET) {

        if (glcf->sysinfo.cached_rt_exptime < ngx_time()) {
            ngx_http_sysguard_update_rt(r, glcf->interval);
        }

        ngx_log_debug2(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                       "http sysguard handler rt: %1.3f %1.3f",
                       glcf->sysinfo.cached_rt * 1.0 / 1000,
                       glcf->rt * 1.0 / 1000);

        if (glcf->sysinfo.cached_rt > glcf->rt) {

            if (glcf->mode == NGX_HTTP_SYSGUARD_MODE_OR) {

                ngx_log_error(glcf->log_level, r->connection->log, 0,
                              "sysguard rt limited, current:%1.3f conf:%1.3f",
                              glcf->sysinfo.cached_rt * 1.0 / 1000,
                              glcf->rt * 1.0 / 1000);

                return ngx_http_sysguard_do_redirect(r, &glcf->rt_action);
            } else {
                action = &glcf->rt_action;
                rt_log = 1;
            }
        } else {
            if (glcf->mode == NGX_HTTP_SYSGUARD_MODE_AND) {
                goto out;
            }
        }
    }

    if (glcf->mode == NGX_HTTP_SYSGUARD_MODE_AND && action) {

        if (load_log) {
            ngx_log_error(glcf->log_level, r->connection->log, 0,
                          "sysguard load limited, current:%1.3f conf:%1.3f",
                          glcf->sysinfo.cached_load * 1.0 / 1000,
                          glcf->load * 1.0 / 1000);
        }

        if (swap_log) {
            ngx_log_error(glcf->log_level, r->connection->log, 0,
                          "sysguard swap limited, current:%i conf:%i",
                          glcf->sysinfo.cached_swapstat,
                          glcf->swap);
        }

        if (free_log) {
            ngx_log_error(glcf->log_level, r->connection->log, 0,
                          "sysguard free limited, current:%uzM conf:%uzM",
                          glcf->sysinfo.cached_free / 1024 / 1024,
                          glcf->free / 1024 / 1024);
        }

        if (rt_log) {
            ngx_log_error(glcf->log_level, r->connection->log, 0,
                          "sysguard rt limited, current:%1.3f conf:%1.3f",
                          glcf->sysinfo.cached_rt * 1.0 / 1000,
                          glcf->rt * 1.0 / 1000);
        }

        return ngx_http_sysguard_do_redirect(r, action);
    }

out:
    return NGX_DECLINED;
}


ngx_msec_int_t
ngx_http_sysguard_request_time(ngx_http_request_t *r)
{
    ngx_time_t      *tp;
    ngx_msec_int_t   ms;

    tp = ngx_timeofday();

    ms = (ngx_msec_int_t)
             ((tp->sec - r->start_sec) * 1000 + (tp->msec - r->start_msec));
    return ngx_max(ms, 0);
}


static void *
ngx_http_sysguard_create_conf(ngx_conf_t *cf)
{
    ngx_http_sysguard_conf_t  *conf;

    conf = ngx_pcalloc(cf->pool, sizeof(ngx_http_sysguard_conf_t));
    if (conf == NULL) {
        return NGX_CONF_ERROR;
    }

    /*
     * set by ngx_pcalloc():
     *
     *     conf->load_action = {0, NULL};
     *     conf->swap_action = {0, NULL};
     *     conf->rt_action = {0, NULL};
     *     conf->ring = NULL;
     */

    conf->enable = NGX_CONF_UNSET;
    conf->load = NGX_CONF_UNSET;
    conf->swap = NGX_CONF_UNSET;
    conf->free = NGX_CONF_UNSET_SIZE;
    conf->rt = NGX_CONF_UNSET;
    conf->rt_period = NGX_CONF_UNSET;
    conf->rt_number = NGX_CONF_UNSET;
    conf->rt_method = NGX_CONF_UNSET;
    conf->interval = NGX_CONF_UNSET;
    conf->log_level = NGX_CONF_UNSET_UINT;
    conf->mode = NGX_CONF_UNSET_UINT;

    return conf;
}


static char *
ngx_http_sysguard_merge_conf(ngx_conf_t *cf, void *parent, void *child)
{
    ngx_http_sysguard_conf_t  *prev = parent;
    ngx_http_sysguard_conf_t  *conf = child;

    ngx_conf_merge_value(conf->enable, prev->enable, 0);

    ngx_conf_merge_str_value(conf->load_action, prev->load_action, "");
    ngx_conf_merge_str_value(conf->swap_action, prev->swap_action, "");
    ngx_conf_merge_str_value(conf->free_action, prev->free_action, "");
    ngx_conf_merge_str_value(conf->rt_action, prev->rt_action, "");

    ngx_conf_merge_value(conf->load, prev->load, NGX_CONF_UNSET);
    ngx_conf_merge_value(conf->swap, prev->swap, NGX_CONF_UNSET);
    ngx_conf_merge_size_value(conf->free, prev->free, NGX_CONF_UNSET_SIZE);
    ngx_conf_merge_value(conf->rt, prev->rt, NGX_CONF_UNSET);

    ngx_conf_merge_value(conf->rt_period, prev->rt_period, 1);
    ngx_conf_merge_value(conf->rt_number, prev->rt_number, conf->rt_period);
    ngx_conf_merge_value(conf->rt_method, prev->rt_method, NGX_HTTP_SYSGUARD_AVERAGE_AMM);

    ngx_conf_merge_value(conf->interval, prev->interval, 1);
    ngx_conf_merge_uint_value(conf->log_level, prev->log_level, NGX_LOG_ERR);
    ngx_conf_merge_uint_value(conf->mode, prev->mode, NGX_HTTP_SYSGUARD_MODE_OR);

    ngx_memzero(&conf->sysinfo, sizeof(conf->sysinfo));

    if (conf->rt != NGX_CONF_UNSET) {
        ngx_http_sysguard_node_time_ring_init(&conf->request_times, conf->rt_number);

        conf->request_times.times = ngx_pcalloc(cf->pool,
                                        sizeof(ngx_http_sysguard_node_time_t) * conf->rt_number);

        if (conf->request_times.times == NULL) {
            return NGX_CONF_ERROR;
        }
    }

    return NGX_CONF_OK;
}


static char *
ngx_http_sysguard_load(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_http_sysguard_conf_t  *glcf = conf;

    ngx_str_t   *value;
    ngx_uint_t   i, scale;

    value = cf->args->elts;
    i = 1;
    scale = 1;

    if (ngx_strncmp(value[i].data, "load=", 5) == 0) {

        if (glcf->load != NGX_CONF_UNSET) {
            return "is duplicate";
        }

        if (value[i].len == 5) {
            goto invalid;
        }

        value[i].data += 5;
        value[i].len -= 5;

        if (ngx_strncmp(value[i].data, "ncpu*", 5) == 0) {
            value[i].data += 5;
            value[i].len -= 5;
            scale = ngx_ncpu;
        }

        glcf->load = ngx_atofp(value[i].data, value[i].len, 3);
        if (glcf->load == NGX_ERROR) {
            goto invalid;
        }

        glcf->load = glcf->load * scale;

        if (cf->args->nelts == 2) {
            return NGX_CONF_OK;
        }

        i++;

        if (ngx_strncmp(value[i].data, "action=", 7) != 0) {
            goto invalid;
        }

        if (value[i].len == 7) {
            goto invalid;
        }

        if (value[i].data[7] != '/' && value[i].data[7] != '@') {
            goto invalid;
        }

        glcf->load_action.data = value[i].data + 7;
        glcf->load_action.len = value[i].len - 7;

        return NGX_CONF_OK;
    }

invalid:

    ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                       "invalid parameter \"%V\"", &value[i]);

    return NGX_CONF_ERROR;
}


static char *
ngx_http_sysguard_mem(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_http_sysguard_conf_t  *glcf = conf;

    ngx_str_t   *value, ss;
    ngx_uint_t   i;

    value = cf->args->elts;
    i = 1;

    if (ngx_strncmp(value[i].data, "swapratio=", 10) == 0) {

        if (glcf->swap != NGX_CONF_UNSET) {
            return "is duplicate";
        }

        if (value[i].data[value[i].len - 1] != '%') {
            goto invalid;
        }

        glcf->swap = ngx_atofp(value[i].data + 10, value[i].len - 11, 2);
        if (glcf->swap == NGX_ERROR) {
            goto invalid;
        }

        if (cf->args->nelts == 2) {
            return NGX_CONF_OK;
        }

        i++;

        if (ngx_strncmp(value[i].data, "action=", 7) != 0) {
            goto invalid;
        }

        if (value[i].len == 7) {
            goto invalid;
        }

        if (value[i].data[7] != '/' && value[i].data[7] != '@') {
            goto invalid;
        }

        glcf->swap_action.data = value[i].data + 7;
        glcf->swap_action.len = value[i].len - 7;

        return NGX_CONF_OK;

    } else if (ngx_strncmp(value[i].data, "free=", 5) == 0) {

        if (glcf->free != NGX_CONF_UNSET_SIZE) {
            return "is duplicate";
        }

        ss.data = value[i].data + 5;
        ss.len = value[i].len - 5;

        glcf->free = ngx_parse_size(&ss);
        if (glcf->free == (size_t) NGX_ERROR) {
            goto invalid;
        }

        if (cf->args->nelts == 2) {
            return NGX_CONF_OK;
        }

        i++;

        if (ngx_strncmp(value[i].data, "action=", 7) != 0) {
            goto invalid;
        }

        if (value[i].len == 7) {
            goto invalid;
        }

        if (value[i].data[7] != '/' && value[i].data[7] != '@') {
            goto invalid;
        }

        glcf->free_action.data = value[i].data + 7;
        glcf->free_action.len = value[i].len - 7;

        return NGX_CONF_OK;
    }

invalid:

    ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                       "invalid parameter \"%V\"", &value[i]);

    return NGX_CONF_ERROR;
}


static char *
ngx_http_sysguard_rt(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_http_sysguard_conf_t  *glcf = conf;

    u_char      *p;
    ngx_str_t   *value, ss;
    ngx_uint_t   i;

    value = cf->args->elts;

    for (i = 1; i < cf->args->nelts; i++) {
        if (ngx_strncmp(value[i].data, "rt=", 3) == 0) {

            if (glcf->rt != NGX_CONF_UNSET) {
                return "is duplicate";
            }

            glcf->rt = ngx_atofp(value[i].data + 3, value[i].len - 3, 3);
            if (glcf->rt == NGX_ERROR) {
                goto invalid;
            }

            continue;
        }

        if (ngx_strncmp(value[i].data, "period=", 7) == 0) {

            ss.data = value[i].data + 7;
            ss.len = value[i].len - 7;

            glcf->rt_period = ngx_parse_time(&ss, 1);
            if (glcf->rt_period == NGX_ERROR) {
                goto invalid;
            }

            continue;
        }

        if (ngx_strncmp(value[i].data, "method=", 7) == 0) {

            ss.data = value[i].data + 7;
            ss.len = value[i].len - 7;

            if (ngx_strncmp(ss.data, "WMA", 3) == 0) {
                glcf->rt_method = NGX_HTTP_SYSGUARD_AVERAGE_WMA;

            } else if (ngx_strncmp(ss.data, "AMM", 3) == 0) {
                glcf->rt_method = NGX_HTTP_SYSGUARD_AVERAGE_AMM;

            } else {
                goto invalid;
            }

            p = (u_char *) ngx_strchr(ss.data, ':');
            if (p == NULL) {
                continue;
            }

            ss.data = p + 1;
            ss.len = value[i].data + value[i].len - ss.data;
            glcf->rt_number = ngx_atoi(ss.data, ss.len);
            if (glcf->rt_number == NGX_ERROR) {
                goto invalid;
            }

            continue;
        }

        if (ngx_strncmp(value[i].data, "action=", 7) == 0) {

            if (value[i].len == 7) {
                goto invalid;
            }

            if (value[i].data[7] != '/' && value[i].data[7] != '@') {
                goto invalid;
            }

            glcf->rt_action.data = value[i].data + 7;
            glcf->rt_action.len = value[i].len - 7;

            continue;
        }
    }

    return NGX_CONF_OK;

invalid:

    ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                       "invalid parameter \"%V\"", &value[i]);

    return NGX_CONF_ERROR;
}


static ngx_int_t
ngx_http_sysguard_log_handler(ngx_http_request_t *r)
{
    ngx_http_sysguard_update_rt_node(r);

    return NGX_OK;
}


static ngx_int_t
ngx_http_sysguard_init(ngx_conf_t *cf)
{
    ngx_http_handler_pt        *h;
    ngx_http_core_main_conf_t  *cmcf;

    cmcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_core_module);

    h = ngx_array_push(&cmcf->phases[NGX_HTTP_PREACCESS_PHASE].handlers);
    if (h == NULL) {
        return NGX_ERROR;
    }

    *h = ngx_http_sysguard_handler;

    h = ngx_array_push(&cmcf->phases[NGX_HTTP_LOG_PHASE].handlers);
    if (h == NULL) {
        return NGX_ERROR;
    }

    *h = ngx_http_sysguard_log_handler;

    return NGX_OK;
}

/* vi:set ft=c ts=4 sw=4 et fdm=marker: */
