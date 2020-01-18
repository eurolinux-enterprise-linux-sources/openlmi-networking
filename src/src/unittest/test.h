/*
 * Copyright (C) 2013 Red Hat, Inc.  All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Authors: Radek Novacek <rnovacek@redhat.com>
 */

#ifndef TEST_H
#define TEST_H

#include <check.h>

#define assert_string_eq(s1, s2) {                                            \
    if (s1 == NULL) {                                                         \
        fail_unless(s2 == NULL, "s1==\"%s\", s2==\"%s\"", s1, s2);            \
    } else {                                                                  \
        if (s2 == NULL) {                                                     \
            ck_abort_msg("s1==\"%s\", s2==\"%s\"", s1, s2);                   \
        } else {                                                              \
            ck_assert_str_eq(s1, s2);                                         \
        }                                                                     \
    }                                                                         \
}

#endif
