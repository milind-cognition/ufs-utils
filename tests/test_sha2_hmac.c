// SPDX-License-Identifier: GPL-2.0-or-later
/* Unit tests for SHA2 and HMAC-SHA2 implementations */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "test_framework.h"
#include "sha2.h"
#include "hmac_sha2.h"

/* Known test vectors from FIPS 180-2 and RFC 4231 */

/* ---- SHA-256 tests ---- */

static void test_sha256_empty(void)
{
	unsigned char digest[SHA256_DIGEST_SIZE];
	/* SHA-256("") = e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855 */
	unsigned char expected[] = {
		0xe3, 0xb0, 0xc4, 0x42, 0x98, 0xfc, 0x1c, 0x14,
		0x9a, 0xfb, 0xf4, 0xc8, 0x99, 0x6f, 0xb9, 0x24,
		0x27, 0xae, 0x41, 0xe4, 0x64, 0x9b, 0x93, 0x4c,
		0xa4, 0x95, 0x99, 0x1b, 0x78, 0x52, 0xb8, 0x55
	};

	sha256((const unsigned char *)"", 0, digest);
	TEST_ASSERT_MEM_EQ(digest, expected, SHA256_DIGEST_SIZE,
			   "SHA-256 of empty string");
}

static void test_sha256_abc(void)
{
	unsigned char digest[SHA256_DIGEST_SIZE];
	/* SHA-256("abc") = ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad */
	unsigned char expected[] = {
		0xba, 0x78, 0x16, 0xbf, 0x8f, 0x01, 0xcf, 0xea,
		0x41, 0x41, 0x40, 0xde, 0x5d, 0xae, 0x22, 0x23,
		0xb0, 0x03, 0x61, 0xa3, 0x96, 0x17, 0x7a, 0x9c,
		0xb4, 0x10, 0xff, 0x61, 0xf2, 0x00, 0x15, 0xad
	};

	sha256((const unsigned char *)"abc", 3, digest);
	TEST_ASSERT_MEM_EQ(digest, expected, SHA256_DIGEST_SIZE,
			   "SHA-256 of 'abc'");
}

static void test_sha256_long_message(void)
{
	unsigned char digest[SHA256_DIGEST_SIZE];
	const char *msg = "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq";
	/* SHA-256 of the above = 248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1 */
	unsigned char expected[] = {
		0x24, 0x8d, 0x6a, 0x61, 0xd2, 0x06, 0x38, 0xb8,
		0xe5, 0xc0, 0x26, 0x93, 0x0c, 0x3e, 0x60, 0x39,
		0xa3, 0x3c, 0xe4, 0x59, 0x64, 0xff, 0x21, 0x67,
		0xf6, 0xec, 0xed, 0xd4, 0x19, 0xdb, 0x06, 0xc1
	};

	sha256((const unsigned char *)msg, strlen(msg), digest);
	TEST_ASSERT_MEM_EQ(digest, expected, SHA256_DIGEST_SIZE,
			   "SHA-256 of NIST test vector");
}

static void test_sha256_incremental(void)
{
	unsigned char digest_one[SHA256_DIGEST_SIZE];
	unsigned char digest_inc[SHA256_DIGEST_SIZE];
	sha256_ctx ctx;
	const char *msg = "Hello, World!";

	sha256((const unsigned char *)msg, strlen(msg), digest_one);

	sha256_init(&ctx);
	sha256_update(&ctx, (const unsigned char *)"Hello, ", 7);
	sha256_update(&ctx, (const unsigned char *)"World!", 6);
	sha256_final(&ctx, digest_inc);

	TEST_ASSERT_MEM_EQ(digest_one, digest_inc, SHA256_DIGEST_SIZE,
			   "incremental SHA-256 should match one-shot");
}

/* ---- SHA-224 tests ---- */

static void test_sha224_abc(void)
{
	unsigned char digest[SHA224_DIGEST_SIZE];
	/* SHA-224("abc") = 23097d223405d8228642a477bda255b32aadbce4bda0b3f7e36c9da7 */
	unsigned char expected[] = {
		0x23, 0x09, 0x7d, 0x22, 0x34, 0x05, 0xd8, 0x22,
		0x86, 0x42, 0xa4, 0x77, 0xbd, 0xa2, 0x55, 0xb3,
		0x2a, 0xad, 0xbc, 0xe4, 0xbd, 0xa0, 0xb3, 0xf7,
		0xe3, 0x6c, 0x9d, 0xa7
	};

	sha224((const unsigned char *)"abc", 3, digest);
	TEST_ASSERT_MEM_EQ(digest, expected, SHA224_DIGEST_SIZE,
			   "SHA-224 of 'abc'");
}

/* ---- SHA-384 tests ---- */

static void test_sha384_abc(void)
{
	unsigned char digest[SHA384_DIGEST_SIZE];
	/* SHA-384("abc") = cb00753f45a35e8bb5a03d699ac65007272c32ab0eded1631a8b605a43ff5bed
	 *                  8086072ba1e7cc2358baeca134c825a7 */
	unsigned char expected[] = {
		0xcb, 0x00, 0x75, 0x3f, 0x45, 0xa3, 0x5e, 0x8b,
		0xb5, 0xa0, 0x3d, 0x69, 0x9a, 0xc6, 0x50, 0x07,
		0x27, 0x2c, 0x32, 0xab, 0x0e, 0xde, 0xd1, 0x63,
		0x1a, 0x8b, 0x60, 0x5a, 0x43, 0xff, 0x5b, 0xed,
		0x80, 0x86, 0x07, 0x2b, 0xa1, 0xe7, 0xcc, 0x23,
		0x58, 0xba, 0xec, 0xa1, 0x34, 0xc8, 0x25, 0xa7
	};

	sha384((const unsigned char *)"abc", 3, digest);
	TEST_ASSERT_MEM_EQ(digest, expected, SHA384_DIGEST_SIZE,
			   "SHA-384 of 'abc'");
}

/* ---- SHA-512 tests ---- */

static void test_sha512_abc(void)
{
	unsigned char digest[SHA512_DIGEST_SIZE];
	/* SHA-512("abc") */
	unsigned char expected[] = {
		0xdd, 0xaf, 0x35, 0xa1, 0x93, 0x61, 0x7a, 0xba,
		0xcc, 0x41, 0x73, 0x49, 0xae, 0x20, 0x41, 0x31,
		0x12, 0xe6, 0xfa, 0x4e, 0x89, 0xa9, 0x7e, 0xa2,
		0x0a, 0x9e, 0xee, 0xe6, 0x4b, 0x55, 0xd3, 0x9a,
		0x21, 0x92, 0x99, 0x2a, 0x27, 0x4f, 0xc1, 0xa8,
		0x36, 0xba, 0x3c, 0x23, 0xa3, 0xfe, 0xeb, 0xbd,
		0x45, 0x4d, 0x44, 0x23, 0x64, 0x3c, 0xe8, 0x0e,
		0x2a, 0x9a, 0xc9, 0x4f, 0xa5, 0x4c, 0xa4, 0x9f
	};

	sha512((const unsigned char *)"abc", 3, digest);
	TEST_ASSERT_MEM_EQ(digest, expected, SHA512_DIGEST_SIZE,
			   "SHA-512 of 'abc'");
}

static void test_sha512_incremental(void)
{
	unsigned char digest_one[SHA512_DIGEST_SIZE];
	unsigned char digest_inc[SHA512_DIGEST_SIZE];
	sha512_ctx ctx;
	const char *msg = "Hello, World!";

	sha512((const unsigned char *)msg, strlen(msg), digest_one);

	sha512_init(&ctx);
	sha512_update(&ctx, (const unsigned char *)"Hello, ", 7);
	sha512_update(&ctx, (const unsigned char *)"World!", 6);
	sha512_final(&ctx, digest_inc);

	TEST_ASSERT_MEM_EQ(digest_one, digest_inc, SHA512_DIGEST_SIZE,
			   "incremental SHA-512 should match one-shot");
}

/* ---- HMAC-SHA-256 tests (RFC 4231 Test Case 2) ---- */

static void test_hmac_sha256_rfc4231_tc2(void)
{
	unsigned char mac[SHA256_DIGEST_SIZE];
	const unsigned char key[] = "Jefe";
	const unsigned char data[] = "what do ya want for nothing?";
	unsigned char expected[] = {
		0x5b, 0xdc, 0xc1, 0x46, 0xbf, 0x60, 0x75, 0x4e,
		0x6a, 0x04, 0x24, 0x26, 0x08, 0x95, 0x75, 0xc7,
		0x5a, 0x00, 0x3f, 0x08, 0x9d, 0x27, 0x39, 0x83,
		0x9d, 0xec, 0x58, 0xb9, 0x64, 0xec, 0x38, 0x43
	};

	hmac_sha256(key, 4, data, 28, mac, SHA256_DIGEST_SIZE);
	TEST_ASSERT_MEM_EQ(mac, expected, SHA256_DIGEST_SIZE,
			   "HMAC-SHA-256 RFC 4231 Test Case 2");
}

static void test_hmac_sha256_incremental(void)
{
	unsigned char mac_one[SHA256_DIGEST_SIZE];
	unsigned char mac_inc[SHA256_DIGEST_SIZE];
	hmac_sha256_ctx ctx;
	const unsigned char key[] = "secret_key";
	const unsigned char msg[] = "Hello, World!";

	hmac_sha256(key, 10, msg, 13, mac_one, SHA256_DIGEST_SIZE);

	hmac_sha256_init(&ctx, key, 10);
	hmac_sha256_update(&ctx, (const unsigned char *)"Hello, ", 7);
	hmac_sha256_update(&ctx, (const unsigned char *)"World!", 6);
	hmac_sha256_final(&ctx, mac_inc, SHA256_DIGEST_SIZE);

	TEST_ASSERT_MEM_EQ(mac_one, mac_inc, SHA256_DIGEST_SIZE,
			   "incremental HMAC-SHA-256 should match one-shot");
}

static void test_hmac_sha256_reinit(void)
{
	unsigned char mac1[SHA256_DIGEST_SIZE];
	unsigned char mac2[SHA256_DIGEST_SIZE];
	hmac_sha256_ctx ctx;
	const unsigned char key[] = "key";
	const unsigned char msg[] = "data";

	hmac_sha256_init(&ctx, key, 3);
	hmac_sha256_update(&ctx, msg, 4);
	hmac_sha256_final(&ctx, mac1, SHA256_DIGEST_SIZE);

	hmac_sha256_reinit(&ctx);
	hmac_sha256_update(&ctx, msg, 4);
	hmac_sha256_final(&ctx, mac2, SHA256_DIGEST_SIZE);

	TEST_ASSERT_MEM_EQ(mac1, mac2, SHA256_DIGEST_SIZE,
			   "HMAC-SHA-256 reinit should produce same result");
}

/* ---- HMAC-SHA-512 test ---- */

static void test_hmac_sha512_rfc4231_tc2(void)
{
	unsigned char mac[SHA512_DIGEST_SIZE];
	const unsigned char key[] = "Jefe";
	const unsigned char data[] = "what do ya want for nothing?";
	/* RFC 4231 Test Case 2 HMAC-SHA-512 */
	unsigned char expected[] = {
		0x16, 0x4b, 0x7a, 0x7b, 0xfc, 0xf8, 0x19, 0xe2,
		0xe3, 0x95, 0xfb, 0xe7, 0x3b, 0x56, 0xe0, 0xa3,
		0x87, 0xbd, 0x64, 0x22, 0x2e, 0x83, 0x1f, 0xd6,
		0x10, 0x27, 0x0c, 0xd7, 0xea, 0x25, 0x05, 0x54,
		0x97, 0x58, 0xbf, 0x75, 0xc0, 0x5a, 0x99, 0x4a,
		0x6d, 0x03, 0x4f, 0x65, 0xf8, 0xf0, 0xe6, 0xfd,
		0xca, 0xea, 0xb1, 0xa3, 0x4d, 0x4a, 0x6b, 0x4b,
		0x63, 0x6e, 0x07, 0x0a, 0x38, 0xbc, 0xe7, 0x37
	};

	hmac_sha512(key, 4, data, 28, mac, SHA512_DIGEST_SIZE);
	TEST_ASSERT_MEM_EQ(mac, expected, SHA512_DIGEST_SIZE,
			   "HMAC-SHA-512 RFC 4231 Test Case 2");
}

/* ---- Large input test ---- */

static void test_sha256_1000_bytes(void)
{
	unsigned char digest1[SHA256_DIGEST_SIZE];
	unsigned char digest2[SHA256_DIGEST_SIZE];
	sha256_ctx ctx;
	unsigned char buf[1000];
	int i;

	memset(buf, 'a', sizeof(buf));

	sha256(buf, sizeof(buf), digest1);

	sha256_init(&ctx);
	for (i = 0; i < 10; i++)
		sha256_update(&ctx, buf + i * 100, 100);
	sha256_final(&ctx, digest2);

	TEST_ASSERT_MEM_EQ(digest1, digest2, SHA256_DIGEST_SIZE,
			   "SHA-256 chunked 1000 bytes should match one-shot");
}

int main(void)
{
	TEST_INIT();
	printf("\n[test_sha2_hmac] Running SHA2/HMAC tests\n");

	RUN_TEST(test_sha256_empty);
	RUN_TEST(test_sha256_abc);
	RUN_TEST(test_sha256_long_message);
	RUN_TEST(test_sha256_incremental);
	RUN_TEST(test_sha224_abc);
	RUN_TEST(test_sha384_abc);
	RUN_TEST(test_sha512_abc);
	RUN_TEST(test_sha512_incremental);
	RUN_TEST(test_hmac_sha256_rfc4231_tc2);
	RUN_TEST(test_hmac_sha256_incremental);
	RUN_TEST(test_hmac_sha256_reinit);
	RUN_TEST(test_hmac_sha512_rfc4231_tc2);
	RUN_TEST(test_sha256_1000_bytes);

	TEST_REPORT("test_sha2_hmac");
	return TEST_RESULT();
}
