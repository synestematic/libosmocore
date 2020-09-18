/* 3GPP TS 23.032 GAD: Universal Geographical Area Description */
/*
 * (C) 2020 by sysmocom - s.f.m.c. GmbH <info@sysmocom.de>
 *
 * All Rights Reserved
 *
 * Author: Neels Hofmeyr <neels@hofmeyr.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <errno.h>
#include <inttypes.h>
#include <osmocom/core/msgb.h>
#include <osmocom/core/utils.h>
#include <osmocom/gsm/gad.h>

/*! \addtogroup gad
 *  @{
 *  \file gad.c
 *  Message encoding and decoding for 3GPP TS 23.032 GAD: Universal Geographical Area Description.
 */

const struct value_string osmo_gad_type_names[] = {
	{ GAD_TYPE_ELL_POINT, "Ellipsoid-point" },
	{ GAD_TYPE_ELL_POINT_UNC_CIRCLE, "Ellipsoid-point-with-uncertainty-circle" },
	{ GAD_TYPE_ELL_POINT_UNC_ELLIPSE, "Ellipsoid-point-with-uncertainty-ellipse" },
	{ GAD_TYPE_POLYGON, "Polygon" },
	{ GAD_TYPE_ELL_POINT_ALT, "Ellipsoid-point-with-altitude" },
	{ GAD_TYPE_ELL_POINT_ALT_UNC_ELL, "Ellipsoid-point-with-altitude-and-uncertainty-ellipsoid" },
	{ GAD_TYPE_ELL_ARC, "Ellipsoid-arc" },
	{ GAD_TYPE_HA_ELL_POINT_UNC_ELLIPSE, "High-accuracy-ellipsoid-point-with-uncertainty-ellipse" },
	{ GAD_TYPE_HA_ELL_POINT_ALT_UNC_ELL, "High-accuracy-ellipsoid-point-with-altitude-and-uncertainty-ellipsoid" },
	{}
};

static void put_u24be(struct msgb *msg, uint32_t val)
{
	uint8_t *pos = msgb_put(msg, 3);
	osmo_store32be_ext(val, pos, 3);
}

/*! Encode a latitude value according to 3GPP TS 23.032.
 * Normally, encoding and decoding is done via osmo_gad_enc() and osmo_gad_dec() for entire PDUs. But calling this
 * directly can be useful to clamp a latitude to an actually encodable accuracy:
 * int32_t set_lat = osmo_gad_dec_lat(osmo_gad_enc_lat(orig_lat));
 * \param[in] lat_deg_1e6  Latitude in micro degrees (degrees * 1e6), -90'000'000 (S) .. 90'000'000 (N).
 * \returns encoded latitude.
 */
uint32_t osmo_gad_enc_lat(int32_t lat_deg_1e6)
{
	/* N <= ((2**23)/90)*X < N+1
	 * N: encoded latitude
	 * X: latitude in degrees
	 */
	int32_t sign = 0;
	int64_t x;
	if (lat_deg_1e6 < 0) {
		sign = 1 << 23;
		lat_deg_1e6 = -lat_deg_1e6;
	}
	x = lat_deg_1e6;
	x <<= 23;
	x += (1 << 23) - 1;
	x /= 90 * 1000000;
	return sign | (x & 0x7fffff);
}

/*! Decode a latitude value according to 3GPP TS 23.032.
 * Normally, encoding and decoding is done via osmo_gad_enc() and osmo_gad_dec() for entire PDUs. But calling this
 * directly can be useful to clamp a latitude to an actually encodable accuracy:
 * int32_t set_lat = osmo_gad_dec_lat(osmo_gad_enc_lat(orig_lat));
 * \param[in] lat  encoded latitude.
 * \returns decoded latitude in micro degrees (degrees * 1e6), -90'000'000 (S) .. 90'000'000 (N).
 */
uint32_t osmo_gad_dec_lat(int32_t lat)
{
	int64_t sign = 1;
	int64_t x;
	if (lat & 0x800000) {
		sign = -1;
		lat &= 0x7fffff;
	}
	x = lat;
	x *= 90 * 1000000;
	x >>= 23;
	x *= sign;
	return x;
}

/*! Encode a longitude value according to 3GPP TS 23.032.
 * Normally, encoding and decoding is done via osmo_gad_enc() and osmo_gad_dec() for entire PDUs. But calling this
 * directly can be useful to clamp a longitude to an actually encodable accuracy:
 * int32_t set_lon = osmo_gad_dec_lon(osmo_gad_enc_lon(orig_lon));
 * \param[in] lon_deg_1e6  Longitude in micro degrees (degrees * 1e6), -180'000'000 (W) .. 180'000'000 (E).
 * \returns encoded longitude.
 */
uint32_t osmo_gad_enc_lon(int32_t lon_deg_1e6)
{
	/* -180 .. 180 degrees mapped to a signed 24 bit integer.
	 * N <= ((2**24)/360) * X < N+1
	 * N: encoded longitude
	 * X: longitude in degrees
	 */
	int64_t x = lon_deg_1e6;
	x *= (1 << 24);
	if (lon_deg_1e6 >= 0)
		x += (1 << 24) - 1;
	else
		x -= (1 << 24) - 1;
	x /= 360 * 1000000;
	return (uint32_t)(x & 0xffffff);
}

/*! Decode a longitude value according to 3GPP TS 23.032.
 * Normally, encoding and decoding is done via osmo_gad_enc() and osmo_gad_dec() for entire PDUs. But calling this
 * directly can be useful to clamp a longitude to an actually encodable accuracy:
 * int32_t set_lon = osmo_gad_dec_lon(osmo_gad_enc_lon(orig_lon));
 * \param[in] lon  Encoded longitude.
 * \returns Longitude in micro degrees (degrees * 1e6), -180'000'000 (W) .. 180'000'000 (E).
 */
uint32_t osmo_gad_dec_lon(int32_t lon)
{
	/* -180 .. 180 degrees mapped to a signed 24 bit integer.
	 * N <= ((2**24)/360) * X < N+1
	 * N: encoded longitude
	 * X: longitude in degrees
	 */
	int64_t x;
	if (lon & 0x800000) {
		/* make the 24bit negative number to a 32bit negative number */
		lon |= 0xff000000;
	}
	x = lon;
	x *= 360 * 1000000;
	x /= (1 << 24);
	return x;
}

/*
 * r = C((1+x)**K - 1)
 * C = 10, x = 0.1
 *
 * def r(k):
 *     return 10.*(((1+0.1)**k) -1 )
 * for k in range(128):
 *     print('%d,' % (r(k) * 1000.))
 */
static uint32_t table_uncertainty_1e3[128] = {
	0, 1000, 2100, 3310, 4641, 6105, 7715, 9487, 11435, 13579, 15937, 18531, 21384, 24522, 27974, 31772, 35949,
	40544, 45599, 51159, 57274, 64002, 71402, 79543, 88497, 98347, 109181, 121099, 134209, 148630, 164494, 181943,
	201137, 222251, 245476, 271024, 299126, 330039, 364043, 401447, 442592, 487851, 537636, 592400, 652640, 718904,
	791795, 871974, 960172, 1057189, 1163908, 1281299, 1410429, 1552472, 1708719, 1880591, 2069650, 2277615,
	2506377, 2758014, 3034816, 3339298, 3674227, 4042650, 4447915, 4893707, 5384077, 5923485, 6516834, 7169517,
	7887469, 8677216, 9545938, 10501531, 11552685, 12708953, 13980849, 15379933, 16918927, 18611820, 20474002,
	22522402, 24775642, 27254206, 29980627, 32979690, 36278659, 39907525, 43899277, 48290205, 53120226, 58433248,
	64277573, 70706330, 77777964, 85556760, 94113436, 103525780, 113879358, 125268293, 137796123, 151576735,
	166735409, 183409950, 201751945, 221928139, 244121953, 268535149, 295389664, 324929630, 357423593, 393166952,
	432484648, 475734112, 523308524, 575640376, 633205414, 696526955, 766180651, 842799716, 927080688, 1019789756,
	1121769732, 1233947705, 1357343476, 1493078824, 1642387706, 1806627477,
};

/*! Decode an uncertainty circle value according to 3GPP TS 23.032.
 * Normally, encoding and decoding is done via osmo_gad_enc() and osmo_gad_dec() for entire PDUs. But calling this
 * directly can be useful to clamp a value to an actually encodable accuracy:
 * uint32_t set_unc = osmo_gad_dec_unc(osmo_gad_enc_unc(orig_unc));
 * \param[in] unc  Encoded uncertainty value.
 * \returns Uncertainty value in millimeters.
 */
uint32_t osmo_gad_dec_unc(uint8_t unc)
{
	return table_uncertainty_1e3[unc & 0x7f];
}

/*! Encode an uncertainty circle value according to 3GPP TS 23.032.
 * Normally, encoding and decoding is done via osmo_gad_enc() and osmo_gad_dec() for entire PDUs. But calling this
 * directly can be useful to clamp a value to an actually encodable accuracy:
 * uint32_t set_unc = osmo_gad_dec_unc(osmo_gad_enc_unc(orig_unc));
 * \param[in] mm  Uncertainty value in millimeters.
 * \returns  Encoded uncertainty value.
 */
uint8_t osmo_gad_enc_unc(uint32_t mm)
{
	uint8_t unc;
	for (unc = 0; unc < ARRAY_SIZE(table_uncertainty_1e3); unc++) {
		if (table_uncertainty_1e3[unc] > mm)
			return unc - 1;
	}
	return 127;
}

/* So far we don't encode a high-accuracy uncertainty anywhere, so these static items would flag as compiler warnings
 * for unused items. As soon as any HA items get used, remove this ifdef. */
#ifdef GAD_FUTURE

/*
 * r = C((1+x)**K - 1)
 * C = 0.3, x = 0.02
 *
 * def r(k):
 *     return 0.3*(((1+0.02)**k) -1 )
 * for k in range(256):
 *     print('%d,' % (r(k) * 1000.))
 */
static uint32_t table_ha_uncertainty_1e3[256] = {
	0, 6, 12, 18, 24, 31, 37, 44, 51, 58, 65, 73, 80, 88, 95, 103, 111, 120, 128, 137, 145, 154, 163, 173, 182, 192,
	202, 212, 222, 232, 243, 254, 265, 276, 288, 299, 311, 324, 336, 349, 362, 375, 389, 402, 417, 431, 445, 460,
	476, 491, 507, 523, 540, 556, 574, 591, 609, 627, 646, 665, 684, 703, 724, 744, 765, 786, 808, 830, 853, 876,
	899, 923, 948, 973, 998, 1024, 1051, 1078, 1105, 1133, 1162, 1191, 1221, 1252, 1283, 1314, 1347, 1380, 1413,
	1447, 1482, 1518, 1554, 1592, 1629, 1668, 1707, 1748, 1788, 1830, 1873, 1916, 1961, 2006, 2052, 2099, 2147,
	2196, 2246, 2297, 2349, 2402, 2456, 2511, 2567, 2625, 2683, 2743, 2804, 2866, 2929, 2994, 3060, 3127, 3195,
	3265, 3336, 3409, 3483, 3559, 3636, 3715, 3795, 3877, 3961, 4046, 4133, 4222, 4312, 4404, 4498, 4594, 4692,
	4792, 4894, 4998, 5104, 5212, 5322, 5435, 5549, 5666, 5786, 5907, 6032, 6158, 6287, 6419, 6554, 6691, 6830,
	6973, 7119, 7267, 7418, 7573, 7730, 7891, 8055, 8222, 8392, 8566, 8743, 8924, 9109, 9297, 9489, 9685, 9884,
	10088, 10296, 10508, 10724, 10944, 11169, 11399, 11633, 11871, 12115, 12363, 12616, 12875, 13138, 13407, 13681,
	13961, 14246, 14537, 14834, 15136, 15445, 15760, 16081, 16409, 16743, 17084, 17431, 17786, 18148, 18517, 18893,
	19277, 19669, 20068, 20475, 20891, 21315, 21747, 22188, 22638, 23096, 23564, 24042, 24529, 25025, 25532, 26048,
	26575, 27113, 27661, 28220, 28791, 29372, 29966, 30571, 31189, 31818, 32461, 33116, 33784, 34466, 35161, 35871,
	36594, 37332, 38085, 38852, 39635, 40434, 41249, 42080, 42927, 43792, 44674, 45573, 46491,
};

static uint32_t osmo_gad_dec_ha_unc(uint8_t unc)
{
	return table_uncertainty_1e3[unc];
}

static uint8_t osmo_gad_enc_ha_unc(uint32_t mm)
{
	uint8_t unc;
	for (unc = 0; unc < ARRAY_SIZE(table_ha_uncertainty_1e3); unc++) {
		if (table_uncertainty_1e3[unc] > mm)
			return unc - 1;
	}
	return 255;
}

#endif /* GAD_FUTURE */

#define DEC_ERR(RC, TYPE, fmt, args...) do { \
		if (err) { \
			*err = talloc_zero(err_ctx, struct osmo_gad_err); \
			**err = (struct osmo_gad_err){ \
				.rc = (RC), \
				.type = (TYPE), \
				.logmsg = talloc_asprintf(*err, "Error decoding GAD%s%s: " fmt, \
							  (TYPE) >= 0 ? " " : "", \
							  (TYPE) >= 0 ? osmo_gad_type_name(TYPE) : "", ##args), \
			}; \
		} \
		return RC; \
	} while(0)

static int osmo_gad_enc_ell_point_unc_circle(struct msgb *msg, const struct gad_ell_point_unc_circle *v)
{
	uint8_t *old_tail = msg->tail;
	msgb_put_u8(msg, GAD_TYPE_ELL_POINT_UNC_CIRCLE << 4);
	put_u24be(msg, osmo_gad_enc_lat(v->lat));
	put_u24be(msg, osmo_gad_enc_lon(v->lon));
	msgb_put_u8(msg, osmo_gad_enc_unc(v->unc));
	return (msg->tail - old_tail);
}

static int osmo_gad_dec_ell_point_unc_circle(struct gad_ell_point_unc_circle *v,
					     struct osmo_gad_err **err, void *err_ctx,
					     const uint8_t *data, uint8_t len)
{
	uint32_t val;
	uint8_t unc;
	if (len != 8)
		DEC_ERR(-EINVAL, -1, "Expecting length of 8 bytes, got %u", len);

	/* Load a 24bit big endian integer from data[1] */
	val = osmo_load32be_ext_2(&data[1], 3);
	v->lat = osmo_gad_dec_lat(val);

	/* Load a 24bit big endian integer from data[4] */
	val = osmo_load32be_ext_2(&data[4], 3);
	v->lon = osmo_gad_dec_lon(val);

	unc = data[7];
	if (unc & 0x80)
		DEC_ERR(-EINVAL, -1, "Bit 8 of Uncertainty code should be zero (unc = 0x%x)", unc);
	v->unc = osmo_gad_dec_unc(unc);
	return 0;
}

/*! Encode a GAD PDU and append to the msgb.
 * \param[out] msg  Append to this msgb.
 * \param[in] gad  GAD values to encode.
 * \returns number of bytes appended to msgb, or negative on failure.
 */
int osmo_gad_enc(struct msgb *msg, const struct gad_pdu *gad)
{
	switch (gad->type) {
	case GAD_TYPE_ELL_POINT_UNC_CIRCLE:
		return osmo_gad_enc_ell_point_unc_circle(msg, &gad->ell_point_unc_circle);
	default:
		return -ENOTSUP;
	}
}

/*! Decode a GAD PDU.
 * \param[out] gad  Decoded GAD values are written here.
 * \param[out] err  Returned pointer to error info, dynamically allocated; NULL to not return any.
 * \param[in] err_ctx  Talloc context to allocate err from, if required.
 * \param[in] data  Encoded GAD bytes buffer.
 * \param[in] len  Length of data in bytes.
 * \returns NULL on success, human readable error message otherwise (string constant).
 */
int osmo_gad_dec(struct gad_pdu *gad, struct osmo_gad_err **err, void *err_ctx, const uint8_t *data, uint8_t len)
{
	int rc;
	if (err)
		*err = NULL;
	if (len < 1)
		DEC_ERR(-EINVAL, -1, "zero length");
	*gad = (struct gad_pdu){};
	gad->type = data[0] >> 4;
	switch (gad->type) {
	case GAD_TYPE_ELL_POINT_UNC_CIRCLE:
		rc = osmo_gad_dec_ell_point_unc_circle(&gad->ell_point_unc_circle, err, err_ctx, data, len);
		break;
	default:
		DEC_ERR(-ENOTSUP, gad->type, "unsupported GAD type");
	}

	if (err && *err)
		(*err)->type = gad->type;
	return rc;
}

/*! Return a human readable representation of GAD (location estimate) data.
 * \param[out] buf  Buffer to write string to.
 * \param[in] buflen  sizeof(buf).
 * \param[in] gad  Location data.
 * \returns number of chars that would be written, like snprintf().
 */
int osmo_gad_to_str_buf(char *buf, size_t buflen, const struct gad_pdu *gad)
{
	struct osmo_strbuf sb = { .buf = buf, .len = buflen };

	if (!gad) {
		OSMO_STRBUF_PRINTF(sb, "null");
		return sb.chars_needed;
	}

	OSMO_STRBUF_PRINTF(sb, "%s{", osmo_gad_type_name(gad->type));

	switch (gad->type) {
	case GAD_TYPE_ELL_POINT:
		OSMO_STRBUF_PRINTF(sb, "lat=");
		OSMO_STRBUF_APPEND(sb, osmo_micros_to_float_str_buf, gad->ell_point.lat);
		OSMO_STRBUF_PRINTF(sb, ",lon=");
		OSMO_STRBUF_APPEND(sb, osmo_micros_to_float_str_buf, gad->ell_point.lon);

	case GAD_TYPE_ELL_POINT_UNC_CIRCLE:
		OSMO_STRBUF_PRINTF(sb, "lat=");
		OSMO_STRBUF_APPEND(sb, osmo_micros_to_float_str_buf, gad->ell_point_unc_circle.lat);
		OSMO_STRBUF_PRINTF(sb, ",lon=");
		OSMO_STRBUF_APPEND(sb, osmo_micros_to_float_str_buf, gad->ell_point_unc_circle.lon);
		OSMO_STRBUF_PRINTF(sb, ",unc=%" PRIu32 "mm", gad->ell_point_unc_circle.unc);
		break;

	default:
		OSMO_STRBUF_PRINTF(sb, "to-str-not-implemented");
		break;
	}
	OSMO_STRBUF_PRINTF(sb, "}");
	return sb.chars_needed;
}

/*! Return a human readable representation of GAD (location estimate) data.
 * \param[in] ctx  Talloc ctx to allocate string buffer from.
 * \param[in] val  Value to convert to float.
 * \returns resulting string, dynamically allocated.
 */
char *osmo_gad_to_str_c(void *ctx, const struct gad_pdu *gad)
{
	OSMO_NAME_C_IMPL(ctx, 128, "ERROR", osmo_gad_to_str_buf, gad)
}

/*! @} */
