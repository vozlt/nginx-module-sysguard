
/*
 * Copyright (C) 2010-2015 Alibaba Group Holding Limited
 * Copyright (C) YoungJoo Kim (vozlt)
 */


#ifndef _NGX_SYSGUARD_MODULE_H_INCLUDED_
#define _NGX_SYSGUARD_MODULE_H_INCLUDED_


#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

#include "ngx_http_sysguard_sysinfo.h"
#include "ngx_http_sysguard_node.h"

#define NGX_HTTP_SYSGUARD_MODE_OR  0
#define NGX_HTTP_SYSGUARD_MODE_AND 1

#define ngx_http_sysguard_triangle(n) (unsigned) (  \
    n * (n + 1) / 2                                 \
)


typedef struct {
    ngx_flag_t                          enable;

    ngx_int_t                           load;
    ngx_str_t                           load_action;

    ngx_int_t                           swap;
    ngx_str_t                           swap_action;

    size_t                              free;
    ngx_str_t                           free_action;

    ngx_int_t                           rt;
    ngx_int_t                           rt_period;
    ngx_int_t                           rt_number;
    ngx_int_t                           rt_method;
    ngx_str_t                           rt_action;

    time_t                              interval;
    ngx_uint_t                          log_level;
    ngx_uint_t                          mode;

    ngx_http_sysguard_sysinfo_t         sysinfo;
    ngx_http_sysguard_node_time_ring_t  request_times;
} ngx_http_sysguard_conf_t;


ngx_msec_int_t ngx_http_sysguard_request_time(ngx_http_request_t *r);

extern ngx_module_t ngx_http_sysguard_module;


#endif /* _NGX_SYSGUARD_MODULE_H_INCLUDED_ */

/* vi:set ft=c ts=4 sw=4 et fdm=marker: */
