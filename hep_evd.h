//
// hep_evd.h
//
// Copyright (c) 2023 Ryan Cross. All rights reserved.
// MIT License
//

#ifndef HEP_EVD_H
#define HEP_EVD_H

#if __has_include(<windows.h>)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

// Include everything...
#include "include/config.h"
#include "include/geometry.h"
#include "include/hits.h"
#include "include/marker.h"
#include "include/server.h"
#include "include/utils.h"

#ifdef HEP_EVD_BASE_HELPER
#include "include/base_helper.h"
#endif

#ifdef HEP_EVD_PANDORA_HELPERS
#include "include/pandora_helpers.h"
#endif

#ifdef HEP_EVD_LARSOFT_HELPERS
#include "include/larsoft_helpers.h"
#endif

#ifdef HEP_EVD_TRACCC_HELPERS
#include "include/traccc_helpers.h"
#endif

#endif // HEP_EVD_H
