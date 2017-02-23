
/*
 * Copyright (C) 2010-2015 Alibaba Group Holding Limited
 * Copyright (C) YoungJoo Kim (vozlt)
 */


#include "ngx_http_sysguard_module.h"


void
ngx_http_sysguard_node_time_ring_zero(
    ngx_http_sysguard_node_time_ring_t *q)
{
    ngx_memzero(q, sizeof(ngx_http_sysguard_node_time_ring_t));
}


void
ngx_http_sysguard_node_time_ring_init(
    ngx_http_sysguard_node_time_ring_t *q,
    size_t len)
{
    ngx_http_sysguard_node_time_ring_zero(q);
    q->rear = len - 1;
    q->len = len;
}


ngx_int_t
ngx_http_sysguard_node_time_ring_push(
    ngx_http_sysguard_node_time_ring_t *q,
    ngx_msec_int_t x)
{
    if ((q->rear + 1) % q->len == q->front) {
        return NGX_ERROR;
    }

    q->times[q->rear].time = ngx_current_msec;
    q->times[q->rear].msec = x;
    q->rear = (q->rear + 1) % q->len;

    return NGX_OK;
}


ngx_int_t
ngx_http_sysguard_node_time_ring_pop(
    ngx_http_sysguard_node_time_ring_t *q,
    ngx_http_sysguard_node_time_t *x)
{
    if (q->front == q->rear) {
        return NGX_ERROR;
    }

    *x = q->times[q->front];
    q->front = (q->front + 1) % q->len;

    return NGX_OK;
}


void
ngx_http_sysguard_node_time_ring_insert(
    ngx_http_sysguard_node_time_ring_t *q,
    ngx_msec_int_t x)
{
    ngx_int_t                      rc;
    ngx_http_sysguard_node_time_t  rx;
    rc = ngx_http_sysguard_node_time_ring_pop(q, &rx)
         | ngx_http_sysguard_node_time_ring_push(q, x);

    if (rc != NGX_OK) {
        ngx_http_sysguard_node_time_ring_init(q, q->len);
    }
}


ngx_msec_t
ngx_http_sysguard_node_time_ring_average_amm(
    ngx_http_request_t *r,
    ngx_http_sysguard_node_time_ring_t *q)
{
    ngx_int_t                  i, j, k;
    ngx_http_sysguard_conf_t  *glcf;

    glcf = ngx_http_get_module_loc_conf(r, ngx_http_sysguard_module);

    for (i = q->front, j = 1, k = 0; i != q->rear; i = (i + 1) % q->len, j++) {
        if (q->times[i].time + (glcf->rt_period * 1000) > ngx_current_msec) {
            k += (ngx_int_t) q->times[i].msec;
        }
    }

    if (j != q->len) {
        ngx_http_sysguard_node_time_ring_init(q, q->len);
    }

    return (ngx_msec_t) (k / (q->len - 1));
}


ngx_msec_t
ngx_http_sysguard_node_time_ring_average_wma(
    ngx_http_request_t *r,
    ngx_http_sysguard_node_time_ring_t *q)
{
    ngx_int_t                  i, j, k;
    ngx_http_sysguard_conf_t  *glcf;

    glcf = ngx_http_get_module_loc_conf(r, ngx_http_sysguard_module);

    for (i = q->front, j = 1, k = 0; i != q->rear; i = (i + 1) % q->len, j++) {
        if (q->times[i].time + (glcf->rt_period * 1000) > ngx_current_msec) {
            k += (ngx_int_t) q->times[i].msec * j;
        }
    }

    if (j != q->len) {
        ngx_http_sysguard_node_time_ring_init(q, q->len);
    }

    return (ngx_msec_t) (k / (ngx_int_t) ngx_http_sysguard_triangle((q->len - 1)));
}

/* vi:set ft=c ts=4 sw=4 et fdm=marker: */
