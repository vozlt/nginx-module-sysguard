
/*
 * Copyright (C) 2010-2015 Alibaba Group Holding Limited
 * Copyright (C) YoungJoo Kim (vozlt)
 */


#ifndef _NGX_SYSGUARD_SYSINFO_H_INCLUDED_
#define _NGX_SYSGUARD_SYSINFO_H_INCLUDED_


#if (NGX_HAVE_SYSINFO)
#include <sys/sysinfo.h>
#endif

#if (NGX_HAVE_VM_STATS)
#include <sys/sysctl.h>
#endif

/* in bytes */
typedef struct {
    size_t                        totalram;
    size_t                        freeram;
    size_t                        bufferram;
    size_t                        cachedram;
    size_t                        totalswap;
    size_t                        freeswap;
} ngx_http_sysguard_meminfo_t;


typedef struct {
    time_t                        cached_load_exptime;
    time_t                        cached_mem_exptime;
    time_t                        cached_rt_exptime;

    ngx_int_t                     cached_load;
    ngx_int_t                     cached_swapstat;
    size_t                        cached_free;
    ngx_int_t                     cached_rt;

    ngx_http_sysguard_meminfo_t   meminfo;
} ngx_http_sysguard_sysinfo_t;


ngx_int_t ngx_http_sysguard_getloadavg(ngx_int_t avg[], ngx_int_t nelem, ngx_log_t *log);
ngx_int_t ngx_http_sysguard_getmeminfo(ngx_http_sysguard_meminfo_t *meminfo, ngx_log_t *log);


#endif /* _NGX_SYSGUARD_SYSINFO_H_INCLUDED_ */

/* vi:set ft=c ts=4 sw=4 et fdm=marker: */
