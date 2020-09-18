/*! \addtogroup gad
 *  @{
 *  \file gad.h
 *  Message encoding and decoding for 3GPP TS 23.032 GAD: Universal Geographical Area Description.
 */
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

#pragma once

#include <osmocom/gsm/protocol/gsm_23_032.h>
#include <osmocom/core/utils.h>

struct msgb;

struct osmo_gad_err {
	int rc;
	enum gad_type type;
	char *logmsg;
};

extern const struct value_string osmo_gad_type_names[];
static inline const char *osmo_gad_type_name(enum gad_type val)
{ return get_value_string(osmo_gad_type_names, val); }

int osmo_gad_enc(struct msgb *msg, const struct gad_pdu *gad);
int osmo_gad_dec(struct gad_pdu *gad, struct osmo_gad_err **err, void *err_ctx, const uint8_t *data, uint8_t len);

int osmo_gad_to_str_buf(char *buf, size_t buflen, const struct gad_pdu *gad);
char *osmo_gad_to_str_c(void *ctx, const struct gad_pdu *gad);

uint32_t osmo_gad_enc_lat(int32_t lat_deg_1e6);
uint32_t osmo_gad_dec_lat(int32_t lat);
uint32_t osmo_gad_enc_lon(int32_t lon_deg_1e6);
uint32_t osmo_gad_dec_lon(int32_t lon);
uint8_t osmo_gad_enc_unc(uint32_t mm);
uint32_t osmo_gad_dec_unc(uint8_t unc);

/*! @} */
