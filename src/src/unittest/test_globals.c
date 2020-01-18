/*
 * Copyright (C) 2012 Red Hat, Inc.  All rights reserved.
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

#include "test.h"
#include "globals.h"
#include "test_globals.h"
#include <netinet/in.h>
#include <arpa/inet.h>
#include <glib.h>

void test_ip4(uint32_t i, const char *expected)
{
    char *ip = ip4ToString(i);
    assert_string_eq(ip, expected);
    uint32_t value = ip4FromString(ip);
    fail_unless(value == i, "Error converting ip address from string %s, result: %x, expected: %x", ip, value, i);
    free(ip);
}

void test_ip6(const struct in6_addr *addr, const char *expected)
{
    char *ip = ip6ToString(addr);
    assert_string_eq(ip, expected);
    struct in6_addr *value = ip6FromString(ip);
    for (unsigned int i = 0; i < 8; ++i) {
        fail_unless(value->s6_addr16[i] == addr->s6_addr16[i],
                    "Error converting ip address from string %s, group %d, result: %x, expected: %x", ip, i, value->s6_addr16[i], addr->s6_addr16[i]);
    }
    free(value);
    free(ip);
}

void test_prefix4(uint32_t prefix, const char *expected)
{
    char *mask = prefixToMask4(prefix);
    assert_string_eq(mask, expected);
    uint32_t prefix2 = netmaskToPrefix4(mask);
    fail_unless(prefix2 == prefix, "Error converting netmask %s to prefix, result: %d, expected: %d", mask, prefix2, prefix);
    free(mask);
}

START_TEST(test_ip4Conversions)
{
    test_ip4(0, "0.0.0.0");
    test_ip4(255, "255.0.0.0");
    test_ip4(256L*256*256*256 - 1, "255.255.255.255");
    test_ip4(192 + 168*256 + 147*256*256 + 100*256*256*256, "192.168.147.100");

    test_prefix4(8, "255.0.0.0");
    test_prefix4(16, "255.255.0.0");
    test_prefix4(24, "255.255.255.0");
    test_prefix4(28, "255.255.255.15");
    test_prefix4(31, "255.255.255.127");
}
END_TEST

START_TEST(test_ip6Conversions)
{
    test_ip6(&in6addr_any, "::");
    test_ip6(&in6addr_loopback, "::1");
    struct in6_addr addr = { { { 0x20,0x01, 0xd,0xb8, 0x85,0xa3, 0x0,0x0, 0x0,0x0, 0x8a,0x2e, 0x3,0x70, 0x73,0x34 } } };
    test_ip6(&addr, "2001:db8:85a3::8a2e:370:7334");

    struct in6_addr addr2 = { { { 0x20,0x01, 0x0d,0xb8, 0x80,0x00, 0x22,0x19, 0x02,0x10, 0x18,0xff, 0xfe,0x97, 0x22,0x22 } } };
    test_ip6(&addr2, "2001:db8:8000:2219:210:18ff:fe97:2222");

    GByteArray *ba = ip6ArrayFromString("2001:db8:85a3::8a2e:370:7334");
    ck_assert_int_eq(ba->len, 16);
    ck_assert_int_eq(ba->data[0], 0x20);
    ck_assert_int_eq(ba->data[1], 0x01);
    ck_assert_int_eq(ba->data[2], 0x0d);
    ck_assert_int_eq(ba->data[3], 0xb8);
    ck_assert_int_eq(ba->data[4], 0x85);
    ck_assert_int_eq(ba->data[5], 0xa3);
    ck_assert_int_eq(ba->data[6], 0x00);
    ck_assert_int_eq(ba->data[7], 0x00);
    ck_assert_int_eq(ba->data[8], 0x00);
    ck_assert_int_eq(ba->data[9], 0x00);
    ck_assert_int_eq(ba->data[10], 0x8a);
    ck_assert_int_eq(ba->data[11], 0x2e);
    ck_assert_int_eq(ba->data[12], 0x03);
    ck_assert_int_eq(ba->data[13], 0x70);
    ck_assert_int_eq(ba->data[14], 0x73);
    ck_assert_int_eq(ba->data[15], 0x34);
    g_byte_array_free(ba, true);
}
END_TEST

void test_mac_gbytearray_position(const GByteArray *mac, uint8_t bytes[], int i)
{
    fail_unless(mac->data[i] == bytes[i], "Wrong byte %.2X on position %d. Expected %.2X", mac->data[i], i, bytes[i]);
}

void test_mac_gbytearray(const GByteArray *mac, uint8_t bytes[])
{
    fail_unless(mac->len == 6, "GByteArray has wrong length: %d", mac->len);
    for (unsigned int i = 0; i < 6; ++i) {
        test_mac_gbytearray_position(mac, bytes, i);
    }
}

START_TEST(test_macConversions)
{
    uint8_t mac[6];
    const char *input;
    char *str;
    GByteArray *byteArray;

    input = "00:00:00:00:00:00";
    mac[0] = 0; mac[1] = 0; mac[2] = 0;
    mac[3] = 0; mac[4] = 0; mac[5] = 0;
    byteArray = macToGByteArray(input);
    test_mac_gbytearray(byteArray, mac);
    str = macFromGByteArray(byteArray);
    ck_assert_str_eq(input, str);
    g_byte_array_free(byteArray, true);
    free(str);

    byteArray = macToGByteArray("00:00:00:00:00:00:00");
    test_mac_gbytearray(byteArray, mac);
    g_byte_array_free(byteArray, true);

    input = "00:10:18:97:59:B3";
    mac[0] = 0; mac[1] = 16; mac[2] = 24;
    mac[3] = 151; mac[4] = 89; mac[5] = 179;
    byteArray = macToGByteArray(input);
    test_mac_gbytearray(byteArray, mac);
    str = macFromGByteArray(byteArray);
    ck_assert_str_eq(input, str);
    g_byte_array_free(byteArray, true);
    free(str);

    input = "ff:ff:FF:FF:ff:FF";
    mac[0] = 255; mac[1] = 255; mac[2] = 255;
    mac[3] = 255; mac[4] = 255; mac[5] = 255;
    byteArray = macToGByteArray(input);
    test_mac_gbytearray(byteArray, mac);
    str = macFromGByteArray(byteArray);
    fail_unless(strcasecmp(input, str) == 0);
    g_byte_array_free(byteArray, true);
    free(str);

    // Fails -- silence the errors
    int level = lmi_log_level();
    lmi_set_log_level(_LMI_DEBUG_NONE);

    fail_unless(macToGByteArray("") == NULL);
    fail_unless(macToGByteArray("00:00:00:00:00") == NULL);
    fail_unless(macToGByteArray(":::::") == NULL);
    fail_unless(macToGByteArray(NULL) == NULL);
    fail_unless(macToGByteArray("00:XX:00:00:00:00") == NULL);
    fail_unless(macToGByteArray("hello, world") == NULL);
    fail_unless(macToGByteArray("00:AA:00:BBB:00:00") == NULL);
    // int overflow
    asprintf(&str, "%X0:00:00:00:00:00", INT_MAX);
    fail_unless(macToGByteArray(str) == NULL);
    free(str);

    lmi_set_log_level(level);
}
END_TEST

START_TEST(test_get_system_name)
{
    char *name = (char *) get_system_name();
    fail_unless(name != NULL, "No system name found");
    free(name);
}
END_TEST

START_TEST(test_id_from_instanceid)
{
    // Valid uses

    // Without index
    char *s = id_from_instanceid(ORGID ":LMI_Test:1", "LMI_Test");
    assert_string_eq(s, "1");
    free(s);

    s = id_from_instanceid(ORGID ":LMI_Test:abc:cde:efg:LMI_Test", "LMI_Test");
    assert_string_eq(s, "abc:cde:efg:LMI_Test");
    free(s);

    // With one index
    size_t index;
    s = id_from_instanceid_with_index(ORGID ":LMI_Test:1_1", "LMI_Test", &index);
    assert_string_eq(s, "1");
    ck_assert_int_eq(index, 1);
    free(s);

    s = id_from_instanceid_with_index(ORGID ":LMI_Test:abc:cde:efg:LMI_1234", "LMI_Test", &index);
    assert_string_eq(s, "abc:cde:efg:LMI");
    ck_assert_int_eq(index, 1234);
    free(s);

    // With two indexes
    size_t index2;
    s = id_from_instanceid_with_index2(ORGID ":LMI_Test:1_1_2", "LMI_Test", &index, &index2);
    assert_string_eq(s, "1");
    ck_assert_int_eq(index, 1);
    ck_assert_int_eq(index2, 2);
    free(s);

    s = id_from_instanceid_with_index2(ORGID ":LMI_Test:abc:cde:efg:LMI_1234_3456", "LMI_Test", &index, &index2);
    assert_string_eq(s, "abc:cde:efg:LMI");
    ck_assert_int_eq(index, 1234);
    ck_assert_int_eq(index2, 3456);
    free(s);

    // Invalid uses
    fail_unless((s = id_from_instanceid("", "LMI_Test")) == NULL);
    free(s);
    fail_unless((s = id_from_instanceid(ORGID ":LMI_Test:1", "")) == NULL);
    free(s);
    fail_unless((s = id_from_instanceid(ORGID ":LMI_Test:1", "LMI_TestX")) == NULL);
    free(s);
    fail_unless((s = id_from_instanceid_with_index(ORGID ":LMI_Test:1", "LMI_Test", &index)) == NULL);
    free(s);
    fail_unless((s = id_from_instanceid_with_index2(ORGID ":LMI_Test:1", "LMI_Test", &index, &index2)) == NULL);
    free(s);
}
END_TEST

START_TEST(test_id_to_instanceid)
{
    char *instanceid;
    instanceid = id_to_instanceid("1", "LMI_Test");
    assert_string_eq(instanceid, ORGID ":LMI_Test:1");
    free(instanceid);

    instanceid = id_to_instanceid("abc:cde:efg:LMI_Test", "LMI_Test");
    assert_string_eq(instanceid, ORGID ":LMI_Test:abc:cde:efg:LMI_Test");
    free(instanceid);

    instanceid = id_to_instanceid("", "LMI_Test");
    assert_string_eq(instanceid, ORGID ":LMI_Test:");
    free(instanceid);
}
END_TEST

typedef struct Test {
    int id;
} Test;

Test *test_new(void)
{
    Test *test = malloc(sizeof(Test));
    test->id = 42;
    return test;
}

void test_free(Test *test)
{
    free(test);
}

typedef struct Tests Tests;

LIST_DECL(Test, test)

LIST_IMPL(Test, test)

START_TEST(test_list)
{
    Tests *tests = tests_new(1);
    fail_unless(tests_length(tests) == 0);
    Test *test;
    test = test_new();
    tests_add(tests, test);
    fail_unless(tests_length(tests) == 1);
    fail_unless(tests_index(tests, 0)->id == 42);

    test = test_new();
    test->id = 5;
    tests_add(tests, test);
    fail_unless(tests_length(tests) == 2);
    fail_unless(tests_index(tests, 0) != NULL);
    fail_unless(tests_index(tests, 0)->id == 42);
    fail_unless(tests_index(tests, 1) != NULL);
    fail_unless(tests_index(tests, 1)->id == 5);

    test_free(tests_pop(tests, 0));
    fail_unless(tests_length(tests) == 1);
    fail_unless(tests_index(tests, 0)->id == 5);

    tests_pop(tests, 0);
    fail_unless(tests_length(tests) == 0);

    tests_add(tests, test);
    fail_unless(tests_length(tests) == 1);
    fail_unless(tests_index(tests, 0)->id == 5);

    tests_free(tests, true);
}
END_TEST

START_TEST(test_key_value)
{
    // Valid input
    char *options = strdup("key1=value1,key2=value2,key3=value3");
    char *key, *value, *saveptr = NULL;
    fail_unless(key_value_parse(options, &key, &value, &saveptr) == true);
    assert_string_eq(key, "key1");
    assert_string_eq(value, "value1");

    fail_unless(key_value_parse(options, &key, &value, &saveptr) == true);
    assert_string_eq(key, "key2");
    assert_string_eq(value, "value2");

    fail_unless(key_value_parse(options, &key, &value, &saveptr) == true);
    assert_string_eq(key, "key3");
    assert_string_eq(value, "value3");

    fail_unless(key_value_parse(options, &key, &value, &saveptr) == false);
    free(options);

    // Valid input with one key without value
    options = strdup("key1=value1,key2,key3=value3");
    saveptr = NULL;
    fail_unless(key_value_parse(options, &key, &value, &saveptr) == true);
    assert_string_eq(key, "key1");
    assert_string_eq(value, "value1");

    fail_unless(key_value_parse(options, &key, &value, &saveptr) == true);
    assert_string_eq(key, "key2");
    fail_unless(value == NULL);

    fail_unless(key_value_parse(options, &key, &value, &saveptr) == true);
    assert_string_eq(key, "key3");
    assert_string_eq(value, "value3");

    fail_unless(key_value_parse(options, &key, &value, &saveptr) == false);
    free(options);

    // Valid input with one key without value but with equal sign
    options = strdup("key1=value1,key2=,key3=value3");
    saveptr = NULL;
    fail_unless(key_value_parse(options, &key, &value, &saveptr) == true);
    assert_string_eq(key, "key1");
    assert_string_eq(value, "value1");

    fail_unless(key_value_parse(options, &key, &value, &saveptr) == true);
    assert_string_eq(key, "key2");
    assert_string_eq(value, "");

    fail_unless(key_value_parse(options, &key, &value, &saveptr) == true);
    assert_string_eq(key, "key3");
    assert_string_eq(value, "value3");

    fail_unless(key_value_parse(options, &key, &value, &saveptr) == false);
    free(options);

    // Invalid input
    options = strdup("");
    saveptr = NULL;
    fail_unless(key_value_parse(options, &key, &value, &saveptr) == false);
    free(options);

    options = NULL;
    saveptr = NULL;
    fail_unless(key_value_parse(options, &key, &value, &saveptr) == false);

    options = strdup(",,,key1=value1");
    saveptr = NULL;
    fail_unless(key_value_parse(options, &key, &value, &saveptr) == true);
    assert_string_eq(key, "key1");
    assert_string_eq(value, "value1");
    fail_unless(key_value_parse(options, &key, &value, &saveptr) == false);
    free(options);
}
END_TEST

TCase *globals_tcase(void)
{
    TCase *tc_globals = tcase_create("globals");
    tcase_add_test(tc_globals, test_ip4Conversions);
    tcase_add_test(tc_globals, test_ip6Conversions);
    tcase_add_test(tc_globals, test_macConversions);
    tcase_add_test(tc_globals, test_get_system_name);
    tcase_add_test(tc_globals, test_list);
    tcase_add_test(tc_globals, test_key_value);
    tcase_add_test(tc_globals, test_id_from_instanceid);
    tcase_add_test(tc_globals, test_id_to_instanceid);
    return tc_globals;
}
