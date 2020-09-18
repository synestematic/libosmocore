#include <stdio.h>

#include <osmocom/core/utils.h>
#include <osmocom/core/msgb.h>
#include <osmocom/gsm/gad.h>

void test_gad_lat_lon_dec_enc_stability()
{
	uint32_t lat_enc;
	uint32_t lon_enc;
	printf("--- %s\n", __func__);
	for (lat_enc = 0x0; lat_enc <= 0xffffff; lat_enc++) {
		int32_t lat_dec = osmo_gad_dec_lat(lat_enc);
		uint32_t enc2 = osmo_gad_enc_lat(lat_dec);
		uint32_t want_enc = lat_enc;
		/* "-0" == 0, because the highest bit is defined as a sign bit. */
		if (lat_enc == 0x800000)
			want_enc = 0;
		if (enc2 != want_enc) {
			printf("ERR: lat=%u --> %d --> %u\n", lat_enc, lat_dec, enc2);
			printf("%d -> %u\n", lat_dec + 1, osmo_gad_enc_lat(lat_dec + 1));
			OSMO_ASSERT(false);
		}
	}
	printf("osmo_gad_dec_lat() -> osmo_gad_enc_lat() of %u values successful\n", lat_enc);
	for (lon_enc = 0; lon_enc <= 0xffffff; lon_enc++) {
		int32_t lon_dec = osmo_gad_dec_lon(lon_enc);
		uint32_t enc2 = osmo_gad_enc_lon(lon_dec);
		uint32_t want_enc = lon_enc;
		if (enc2 != want_enc) {
			printf("ERR: lon=%u 0x%x --> %d --> %u\n", lon_enc, lon_enc, lon_dec, enc2);
			printf("%d -> %u\n", lon_dec + 1, osmo_gad_enc_lon(lon_dec + 1));
			printf("%d -> %u\n", lon_dec - 1, osmo_gad_enc_lon(lon_dec - 1));
			OSMO_ASSERT(false);
		}
	}
	printf("osmo_gad_dec_lon() -> osmo_gad_enc_lon() of %u values successful\n", lon_enc);
}

struct gad_pdu gad_test_pdus[] = {
	{
		.type = GAD_TYPE_ELL_POINT_UNC_CIRCLE,
		.ell_point_unc_circle = {
			/* Values rounded to the nearest encodable value, for test result matching */
			.lat = 23000006,
			.lon = 42000002,
			.unc = 442592,
		},
	},
};

void test_gad_enc_dec()
{
	struct gad_pdu *pdu;
	printf("--- %s\n", __func__);

	for (pdu = gad_test_pdus; (pdu - gad_test_pdus) < ARRAY_SIZE(gad_test_pdus); pdu++) {
		struct msgb *msg = msgb_alloc(1024, __func__);
		struct gad_pdu dec_pdu;
		int rc;
		struct osmo_gad_err *err;
		void *loop_ctx = msg;
		rc = osmo_gad_enc(msg, pdu);
		if (rc <= 0) {
			printf("[%ld] %s: ERROR: failed to encode pdu\n", (pdu - gad_test_pdus),
			       osmo_gad_type_name(pdu->type));
			goto loop_end;
		}
		if (rc != msg->len) {
			printf("[%ld] %s: ERROR: osmo_gad_enc() returned length %d but msgb has %d bytes\n",
			       (pdu - gad_test_pdus), osmo_gad_type_name(pdu->type),
			       rc, msg->len);
			goto loop_end;
		}

		memset(&dec_pdu, 0xff, sizeof(dec_pdu));
		rc = osmo_gad_dec(&dec_pdu, &err, loop_ctx, msg->data, msg->len);
		if (rc) {
			printf("[%ld] ERROR: failed to decode pdu: %s\n", (pdu - gad_test_pdus), err->logmsg);
			printf("     encoded data: %s\n", osmo_hexdump(msg->data, msg->len));
			goto loop_end;
		}

		if (memcmp(pdu, &dec_pdu, sizeof(dec_pdu))) {
			printf("[%ld] %s: ERROR: decoded PDU != encoded PDU\n", (pdu - gad_test_pdus),
			       osmo_gad_type_name(pdu->type));
			printf("     original struct: %s\n", osmo_hexdump((void*)pdu, sizeof(*pdu)));
			printf("      decoded struct: %s\n", osmo_hexdump((void*)&dec_pdu, sizeof(dec_pdu)));
			goto loop_end;
		}

		printf("[%ld] %s: ok\n", (pdu - gad_test_pdus), osmo_gad_type_name(pdu->type));

loop_end:
		msgb_free(msg);
	}
}

int main()
{
	test_gad_lat_lon_dec_enc_stability();
	test_gad_enc_dec();
	return 0;
}
