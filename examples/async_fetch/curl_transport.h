/* SPDX-License-Identifier: MIT */
#ifndef EXAMPLE_CURL_TRANSPORT_H
#define EXAMPLE_CURL_TRANSPORT_H
#include "fetch.h"
/* Main thread before worker creation / after worker join. */
bool af_curl_initialize(void);
void af_curl_shutdown(void);
void af_curl_transport(void *, const char *, af_cancel_fn, void *, af_result *);
#endif
