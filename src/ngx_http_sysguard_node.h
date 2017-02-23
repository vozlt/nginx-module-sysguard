
/*
 * Copyright (C) 2010-2015 Alibaba Group Holding Limited
 * Copyright (C) YoungJoo Kim (vozlt)
 */


#ifndef _NGX_SYSGUARD_NODE_H_INCLUDED_
#define _NGX_SYSGUARD_NODE_H_INCLUDED_


#define NGX_HTTP_SYSGUARD_AVERAGE_AMM 0
#define NGX_HTTP_SYSGUARD_AVERAGE_WMA 1


typedef struct {
    ngx_msec_t                      time;
    ngx_msec_int_t                  msec;
} ngx_http_sysguard_node_time_t;


typedef struct {
    ngx_http_sysguard_node_time_t  *times;
    ngx_int_t                       front;
    ngx_int_t                       rear;
    ngx_int_t                       len;
} ngx_http_sysguard_node_time_ring_t;


void ngx_http_sysguard_node_time_ring_zero(
    ngx_http_sysguard_node_time_ring_t *q);
void ngx_http_sysguard_node_time_ring_init(
    ngx_http_sysguard_node_time_ring_t *q,
    size_t len);
void ngx_http_sysguard_node_time_ring_insert(
    ngx_http_sysguard_node_time_ring_t *q,
    ngx_msec_int_t x);
ngx_int_t ngx_http_sysguard_node_time_ring_push(
    ngx_http_sysguard_node_time_ring_t *q,
    ngx_msec_int_t x);
ngx_int_t ngx_http_sysguard_node_time_ring_pop(
    ngx_http_sysguard_node_time_ring_t *q,
    ngx_http_sysguard_node_time_t *x);
ngx_msec_t ngx_http_sysguard_node_time_ring_average_amm(
    ngx_http_request_t *r,
    ngx_http_sysguard_node_time_ring_t *q);
ngx_msec_t ngx_http_sysguard_node_time_ring_average_wma(
    ngx_http_request_t *r,
    ngx_http_sysguard_node_time_ring_t *q);


#endif /* _NGX_SYSGUARD_NODE_H_INCLUDED_ */

/* vi:set ft=c ts=4 sw=4 et fdm=marker: */
