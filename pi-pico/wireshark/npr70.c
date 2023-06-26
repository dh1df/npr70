/* packet-npr70.c
 * Routines for NPR70 and NPR70-Key IEEE 802.1X-2010 PDU dissection
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
 * Copyright 1998 Gerald Combs
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */


#define WS_BUILD_DLL
#include <stdio.h>
#include <ws_version.h>

#include "config.h"

#include <epan/packet.h>
#include <epan/tvbuff.h>
#include <epan/etypes.h>
#include <epan/proto_data.h>
#include <epan/reassemble.h>
#include <epan/expert.h>
#include "fec.h"

#ifndef VERSION
#define VERSION "0.0.0"
#endif

WS_DLL_PUBLIC_DEF const gchar plugin_version[] = VERSION;
WS_DLL_PUBLIC_DEF const int plugin_want_major = WIRESHARK_VERSION_MAJOR;
WS_DLL_PUBLIC_DEF const int plugin_want_minor = WIRESHARK_VERSION_MINOR;
WS_DLL_PUBLIC void plugin_register(void);

wmem_tree_t *npr70_hf_fragments;
wmem_tree_t *npr70_data_fragments;

void proto_register_npr70(void);
void proto_reg_handoff_npr70(void);
static int dissect_npr70fec(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data _U_);

static dissector_handle_t ip_handle;
static int npr70_last_seq = -1;

static expert_field ei_packet_missing = EI_INIT;

int proto_npr70 = -1;
int proto_npr70fec = -1;
static int hf_npr70_type = -1;
static int hf_npr70_seq = -1;
static int hf_npr70_us = -1;
static int hf_npr70_timestamp = -1;
static int hf_npr70_rssi = -1;
static int hf_npr70_len = -1;
static int hf_npr70_tdma_parity = -1;
static int hf_npr70_tdma_downlink = -1;
static int hf_npr70_tdma_synchro = -1;
static int hf_npr70_tdma_frame_counter = -1;
static int hf_npr70_tdma_uplink_buffer = -1;
static int hf_npr70_errors = -1;
static int hf_npr70_client_parity = -1;
static int hf_npr70_client_id = -1;
static int hf_npr70_proto = -1;
static int hf_npr70_signal = -1;
static int hf_npr70_slen = -1;
static int hf_npr70_id = -1;
static int hf_npr70_mac = -1;
static int hf_npr70_callsign = -1;
static int hf_npr70_ip_start = -1;
static int hf_npr70_ip_length = -1;
static int hf_npr70_static_alloc = -1;

static int hf_npr70_master_mac = -1;
static int hf_npr70_master_callsign = -1;
static int hf_npr70_modem_ip = -1;
static int hf_npr70_subnet_mask = -1;
static int hf_npr70_default_route_active = -1;
static int hf_npr70_default_route = -1;
static int hf_npr70_dns_active = -1;
static int hf_npr70_dns = -1;

static int hf_npr70_pkt_counter = -1;
static int hf_npr70_is_last_seg = -1;
static int hf_npr70_seg_counter = -1;

static int hf_npr70_rssi_uplink = -1;
static int hf_npr70_ber = -1;
static int hf_npr70_ta = -1;

static int hf_npr70_tdma_offset = -1;
static int hf_npr70_tdma_slot_length = -1;
static int hf_npr70_tdma_multiframe = -1;

static gint ett_npr70 = -1;
static gint ett_npr70fe = -1;

static dissector_handle_t npr70_handle;
static dissector_handle_t npr70fec_handle;

static reassembly_table npr70_reassembly_table;
static reassembly_table ip_reassembly_table;
static int reassembly_id;
static int data_reassembly_id;
static int npr70_len;
static int fragments_size;

static gint hf_hf_fragments = -1;
static gint hf_hf_fragment = -1;
static gint hf_hf_fragment_overlap = -1;
static gint hf_hf_fragment_overlap_conflicts = -1;
static gint hf_hf_fragment_multiple_tails = -1;
static gint hf_hf_fragment_too_long_fragment = -1;
static gint hf_hf_fragment_error = -1;
static gint hf_hf_fragment_count = -1;
static gint hf_hf_reassembled_in = -1;
static gint hf_hf_reassembled_length = -1;

static gint ett_hf_fragment = -1;
static gint ett_hf_fragments = -1;

static const fragment_items hf_frag_items = {
    /* Fragment subtrees */
    &ett_hf_fragment,
    &ett_hf_fragments,
    /* Fragment fields */
    &hf_hf_fragments,
    &hf_hf_fragment,
    &hf_hf_fragment_overlap,
    &hf_hf_fragment_overlap_conflicts,
    &hf_hf_fragment_multiple_tails,
    &hf_hf_fragment_too_long_fragment,
    &hf_hf_fragment_error,
    &hf_hf_fragment_count,
    /* Reassembled in field */
    &hf_hf_reassembled_in,
    /* Reassembled length field */
    &hf_hf_reassembled_length,
    /* Reassembled data field */
    NULL,
    /* Tag */
    "Message fragments"
};

static gint hf_data_fragments = -1;
static gint hf_data_fragment = -1;
static gint hf_data_fragment_overlap = -1;
static gint hf_data_fragment_overlap_conflicts = -1;
static gint hf_data_fragment_multiple_tails = -1;
static gint hf_data_fragment_too_long_fragment = -1;
static gint hf_data_fragment_error = -1;
static gint hf_data_fragment_count = -1;
static gint hf_data_reassembled_in = -1;
static gint hf_data_reassembled_length = -1;

static gint ett_data_fragment = -1;
static gint ett_data_fragments = -1;

static const fragment_items data_frag_items = {
    /* Fragment subtrees */
    &ett_data_fragment,
    &ett_data_fragments,
    /* Fragment fields */
    &hf_data_fragments,
    &hf_data_fragment,
    &hf_data_fragment_overlap,
    &hf_data_fragment_overlap_conflicts,
    &hf_data_fragment_multiple_tails,
    &hf_data_fragment_too_long_fragment,
    &hf_data_fragment_error,
    &hf_data_fragment_count,
    /* Reassembled in field */
    &hf_data_reassembled_in,
    /* Reassembled length field */
    &hf_data_reassembled_length,
    /* Reassembled data field */
    NULL,
    /* Tag */
    "Message fragments"
};



#define NPR70_HDR_LEN   4

static const value_string npr70_type_vals[] = {
  { 0, "RX Start" },
  { 1, "RX Cont" },
  { 2, "Tx Start" },
  { 3, "Tx Cont" },
  { 0, NULL }
};

static const value_string npr70_proto_vals[] = {
  { 0, "Null" },
  { 2, "IP" },
  { 0x1e, "Signal" },
  { 0x1f, "Alloc" },
};

static const value_string npr70_signal_vals[] = {
  { 0, "NULL" },
  { 1, "WHO" },
  { 5, "CONN REQ" },
  { 6, "CONN ACK" },
  { 7, "CONN NACK" },
  { 0xb, "DISCONN REQ" },
  { 0xc, "DISCONN ACK" },
  { 0xff, "END" },
  { 0, NULL }
};

static int
dissect_npr70(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data _U_)
{
  int         offset = 0;
  int         remaining,missing;
  guint8      npr70_type,npr70_seq,npr70_tdma;
  proto_item *e;
  proto_tree *ti;
  proto_tree *npr70_tree;
  tvbuff_t   *next_tvb;
  fragment_head *frag_msg = NULL;

#if 1
  pinfo->srcport=0;
  pinfo->destport=0;
  memset(&pinfo->src, 0, sizeof(pinfo->src));
  memset(&pinfo->dst, 0, sizeof(pinfo->dst));
#endif

  col_set_str(pinfo->cinfo, COL_PROTOCOL, "NPR70");
  col_clear(pinfo->cinfo, COL_INFO);

  ti = proto_tree_add_item(tree, proto_npr70, tvb, 0, -1, ENC_NA);
  npr70_tree = proto_item_add_subtree(ti, ett_npr70);

  npr70_type = tvb_get_guint8(tvb, offset);
  proto_tree_add_item(npr70_tree, hf_npr70_type, tvb, offset, 1, ENC_NA);
  col_add_str(pinfo->cinfo, COL_INFO, val_to_str(npr70_type, npr70_type_vals, "Unknown Type (0x%02X)"));
  offset++;

  npr70_seq = tvb_get_guint8(tvb, offset);
  e = proto_tree_add_uint(npr70_tree, hf_npr70_seq, tvb, offset, 1, npr70_seq);
  offset+=1;
  missing=npr70_last_seq != -1 ? (npr70_seq-(npr70_last_seq+1))%256:0;
  npr70_last_seq=npr70_seq;
  if (missing)
    expert_add_info_format(pinfo, e, &ei_packet_missing, "%d Packets missing",missing);
  

  proto_tree_add_item(npr70_tree, hf_npr70_us, tvb, offset, 4, ENC_LITTLE_ENDIAN);
  offset+=4;

  if (npr70_type == 0) {
    proto_tree_add_item(npr70_tree, hf_npr70_timestamp, tvb, offset, 3, ENC_LITTLE_ENDIAN);
    offset+=3;

    proto_tree_add_item(npr70_tree, hf_npr70_rssi, tvb, offset, 1, ENC_NA);
    offset+=1;
  }
  if (npr70_type == 0 || npr70_type == 2) {

    npr70_len = tvb_get_guint8(tvb, offset)+90;
    proto_tree_add_uint(npr70_tree, hf_npr70_len, tvb, offset, 1, npr70_len);
    offset+=1;

    npr70_tdma = tvb_get_guint8(tvb, offset);
    proto_tree_add_boolean(npr70_tree, hf_npr70_tdma_parity, tvb, offset, 1, npr70_tdma & 0x80);
    proto_tree_add_boolean(npr70_tree, hf_npr70_tdma_downlink, tvb, offset, 1, npr70_tdma & 0x40);
    proto_tree_add_boolean(npr70_tree, hf_npr70_tdma_synchro, tvb, offset, 1, npr70_tdma & 0x20);
    if (npr70_tdma & 0x40)
      proto_tree_add_uint(npr70_tree, hf_npr70_tdma_frame_counter, tvb, offset, 1, npr70_tdma & 0x1f);
    else
      proto_tree_add_uint(npr70_tree, hf_npr70_tdma_uplink_buffer, tvb, offset, 1, npr70_tdma & 0x1f);
    offset+=1;
    fragments_size = 0;
    reassembly_id=pinfo->num;
    npr70_len--;
  }
  remaining=tvb_captured_length_remaining(tvb, offset);
  if (!pinfo->fd->visited) {
    gboolean more=fragments_size+remaining < npr70_len;
    wmem_tree_insert32(npr70_hf_fragments, pinfo->num, (void *)(long)reassembly_id);
    frag_msg = fragment_add(&npr70_reassembly_table, tvb, offset, pinfo, reassembly_id, NULL, fragments_size, remaining, more);
  }
  frag_msg = fragment_get(&npr70_reassembly_table, pinfo, (int)(long)wmem_tree_lookup32(npr70_hf_fragments, pinfo->num), NULL);
  fragments_size += remaining;
  if (frag_msg) {
    next_tvb = process_reassembled_data(tvb, 0, pinfo, "Reassembled Message", frag_msg, &hf_frag_items, NULL, NULL);
    if (next_tvb) {
      guchar *fec_buffer = (guchar*)wmem_alloc(pinfo->pool, npr70_len);
      const unsigned char *buf = tvb_get_ptr(next_tvb, 0, -1);
      unsigned int errors;
      int len=FEC_decode(buf, -1, 0, fec_buffer, tvb_reported_length(next_tvb), &errors);
      proto_tree_add_uint(npr70_tree, hf_npr70_errors, tvb, 0, 0, 0);
      if (len) {
        next_tvb = tvb_new_child_real_data(next_tvb, fec_buffer, len, len);
        proto_item *frag_tree_item;
        show_fragment_tree(frag_msg, &hf_frag_items, tree, pinfo, next_tvb, &frag_tree_item);
        add_new_data_source(pinfo, next_tvb, "FECed Message");
        dissect_npr70fec(next_tvb, pinfo, tree, NULL);
      }
    } else {
       col_append_fstr(pinfo->cinfo, COL_INFO, " (NPR70 reassembled in packet %u)", frag_msg->reassembled_in);

    }
  }
  return tvb_captured_length(tvb);
}

static int
dissect_npr70_signal_single(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, int offset)
{
  guint8      npr70_signal, npr70_len, id;
  int saved_offset;

  if (tvb_reported_length_remaining(tvb, offset) < 2)
    return -1;
  npr70_signal = tvb_get_guint8(tvb, offset);
  proto_tree_add_item(tree, hf_npr70_signal, tvb, offset, 1, ENC_NA);
  offset++;
  if (npr70_signal == 0xff) {
        col_append_fstr(pinfo->cinfo, COL_INFO, " END");
        return -1;
  }
  npr70_len = tvb_get_guint8(tvb, offset);
  proto_tree_add_item(tree, hf_npr70_slen, tvb, offset, 1, ENC_NA);
  offset++;
  if (tvb_reported_length_remaining(tvb, offset) < npr70_len)
    return -1;
  saved_offset=offset;
#if 0
  printf("npr70fec proto=%d\n",npr70_proto);
#endif
  switch (npr70_signal) {
  case 1:
    if (tvb_reported_length_remaining(tvb, offset) < 32)
      return -1;
    id = tvb_get_guint8(tvb, offset);
    col_append_fstr(pinfo->cinfo, COL_INFO, " WHO %d %s",id,tvb_get_ptr(tvb, offset+3, -1));
    proto_tree_add_item(tree, hf_npr70_id, tvb, offset, 1, ENC_NA);
    offset++;
    proto_tree_add_item(tree, hf_npr70_mac, tvb, offset, 2, ENC_BIG_ENDIAN);
    offset+=2;
    proto_tree_add_item(tree, hf_npr70_callsign, tvb, offset, 14, ENC_NA);
    offset+=14;
    proto_tree_add_item(tree, hf_npr70_ip_start, tvb, offset, 4, ENC_BIG_ENDIAN);
    offset+=4;
    proto_tree_add_item(tree, hf_npr70_ip_length, tvb, offset, 4, ENC_BIG_ENDIAN);
    offset+=4;
    proto_tree_add_item(tree, hf_npr70_rssi_uplink, tvb, offset, 1, ENC_NA);
    offset+=1;
    proto_tree_add_item(tree, hf_npr70_ber, tvb, offset, 2, ENC_LITTLE_ENDIAN);
    offset+=2;
    proto_tree_add_item(tree, hf_npr70_ta, tvb, offset, 2, ENC_LITTLE_ENDIAN);
    offset+=2;
    break;
  case 5:
    col_append_fstr(pinfo->cinfo, COL_INFO, " CONN_REQ %s", tvb_get_ptr(tvb, offset+2, -1));
    proto_tree_add_item(tree, hf_npr70_mac, tvb, offset, 2, ENC_BIG_ENDIAN);
    offset+=2;
    proto_tree_add_item(tree, hf_npr70_callsign, tvb, offset, 14, ENC_NA);
    offset+=14;
    proto_tree_add_item(tree, hf_npr70_ip_length, tvb, offset, 4, ENC_BIG_ENDIAN);
    offset+=4;
    proto_tree_add_item(tree, hf_npr70_static_alloc, tvb, offset, 1, ENC_NA);
    offset+=1;
    break;
  case 6:
    id = tvb_get_guint8(tvb, offset);
    col_append_fstr(pinfo->cinfo, COL_INFO, " CONN_ACK %d %s",id,tvb_get_ptr(tvb, offset+3, -1));
    proto_tree_add_item(tree, hf_npr70_id, tvb, offset, 1, ENC_NA);
    offset++;
    proto_tree_add_item(tree, hf_npr70_mac, tvb, offset, 2, ENC_BIG_ENDIAN);
    offset+=2;
    proto_tree_add_item(tree, hf_npr70_callsign, tvb, offset, 14, ENC_NA);
    offset+=14;
    proto_tree_add_item(tree, hf_npr70_ip_start, tvb, offset, 4, ENC_BIG_ENDIAN);
    offset+=4;
    proto_tree_add_item(tree, hf_npr70_ip_length, tvb, offset, 4, ENC_BIG_ENDIAN);
    offset+=4;
    proto_tree_add_item(tree, hf_npr70_master_mac, tvb, offset, 2, ENC_BIG_ENDIAN);
    offset+=2;
    proto_tree_add_item(tree, hf_npr70_master_callsign, tvb, offset, 14, ENC_NA);
    offset+=14;
    proto_tree_add_item(tree, hf_npr70_modem_ip, tvb, offset, 4, ENC_BIG_ENDIAN);
    offset+=4;
    proto_tree_add_item(tree, hf_npr70_subnet_mask, tvb, offset, 4, ENC_BIG_ENDIAN);
    offset+=4;
    proto_tree_add_item(tree, hf_npr70_default_route_active, tvb, offset, 1, ENC_NA);
    offset++;
    proto_tree_add_item(tree, hf_npr70_default_route, tvb, offset, 4, ENC_BIG_ENDIAN);
    offset+=4;
    proto_tree_add_item(tree, hf_npr70_dns_active, tvb, offset, 1, ENC_NA);
    offset++;
    proto_tree_add_item(tree, hf_npr70_dns, tvb, offset, 4, ENC_BIG_ENDIAN);
    offset+=4;
    break;
  default:
    col_set_str(pinfo->cinfo, COL_PROTOCOL, "Signal ???");
    return -1;
    break;
  }
  return saved_offset+npr70_len;
}

static int
dissect_npr70_signal(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, int offset)
{
  col_set_str(pinfo->cinfo, COL_PROTOCOL, "SIGNAL");
  do {
    offset=dissect_npr70_signal_single(tvb, pinfo, tree, offset);
  } while (offset != -1);
  return tvb_captured_length(tvb);
}


static int
dissect_npr70_alloc_client(tvbuff_t *tvb,  packet_info *pinfo, proto_tree *tree, int offset)
{
  guint8 client;
  if (tvb_reported_length_remaining(tvb, offset) < 5)
    return -1;
  client  = tvb_get_guint8(tvb, offset);
  proto_tree_add_item(tree, hf_npr70_id, tvb, offset, 1, ENC_NA);
  offset++;
  if (client == 0xff) {
    col_append_fstr(pinfo->cinfo, COL_INFO, " END");
    return -1;
  }
  col_append_fstr(pinfo->cinfo, COL_INFO, " %d",client);
  proto_tree_add_item(tree, hf_npr70_tdma_offset, tvb, offset, 2, ENC_LITTLE_ENDIAN);
  offset+=2;
  proto_tree_add_item(tree, hf_npr70_tdma_slot_length, tvb, offset, 1, ENC_NA);
  offset++;
  proto_tree_add_item(tree, hf_npr70_tdma_multiframe, tvb, offset, 1, ENC_NA);
  offset++;
  return offset;
}

static int
dissect_npr70_alloc(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, int offset)
{
  col_set_str(pinfo->cinfo, COL_PROTOCOL, "ALLOC");
  col_set_str(pinfo->cinfo, COL_INFO, "ALLOC");
  do {
    offset=dissect_npr70_alloc_client(tvb, pinfo, tree, offset);
  } while (offset != -1);
  return tvb_captured_length(tvb);
}

static int
dissect_npr70fec(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data _U_)
{
  int         offset = 0;
  guint8      npr70_seg, npr70_protocol, id, npr70_pkt_counter, npr70_seg_counter, npr70_is_last_seg;
  proto_tree *ti;
  proto_tree *npr70_tree;
  fragment_head *frag_msg;

  col_set_str(pinfo->cinfo, COL_PROTOCOL, "NPR70 FEC");
  col_clear(pinfo->cinfo, COL_INFO);

  ti = proto_tree_add_item(tree, proto_npr70fec, tvb, 0, -1, ENC_NA);
  npr70_tree = proto_item_add_subtree(ti, ett_npr70fe);

  id = tvb_get_guint8(tvb, offset);
  proto_tree_add_boolean(npr70_tree, hf_npr70_client_parity, tvb, offset, 1, id & 0x80);
  proto_tree_add_uint(npr70_tree, hf_npr70_client_id, tvb, offset, 1, id & 0x7f);
  offset++;

  proto_tree_add_item(npr70_tree, hf_npr70_proto, tvb, offset, 1, ENC_NA);
  npr70_protocol = tvb_get_guint8(tvb, offset);
  offset++;
  switch(npr70_protocol) {
  case 0:
    col_set_str(pinfo->cinfo, COL_PROTOCOL, "NULL");
    col_add_fstr(pinfo->cinfo, COL_INFO, "NULL %d",id);
    break;
  case 2:
    col_set_str(pinfo->cinfo, COL_PROTOCOL, "IP");
    npr70_seg = tvb_get_guint8(tvb, offset);
    npr70_pkt_counter=(npr70_seg & 0xf0) >> 4;
    npr70_is_last_seg=(npr70_seg & 0x08) >> 3;
    npr70_seg_counter=npr70_seg & 0x07;
    proto_tree_add_uint(npr70_tree, hf_npr70_pkt_counter, tvb, offset, 1, npr70_pkt_counter);
    proto_tree_add_uint(npr70_tree, hf_npr70_is_last_seg, tvb, offset, 1, npr70_is_last_seg);
    proto_tree_add_uint(npr70_tree, hf_npr70_seg_counter, tvb, offset, 1, npr70_seg_counter);
    offset++;
    if (!pinfo->fd->visited) {
      if (!npr70_seg_counter)
        data_reassembly_id=pinfo->num;
#if 0
      printf("%d %d\n",pinfo->num,data_reassembly_id);
#endif
      wmem_tree_insert32(npr70_data_fragments, pinfo->num, (void *)(long)data_reassembly_id);
      fragment_add_seq(&ip_reassembly_table, tvb, offset, pinfo, data_reassembly_id, NULL, npr70_seg_counter, tvb_captured_length_remaining(tvb, offset), !npr70_is_last_seg, 0);
    }
    frag_msg = fragment_get(&ip_reassembly_table, pinfo,  (int)(long)wmem_tree_lookup32(npr70_data_fragments, pinfo->num), NULL);
    if (frag_msg) {
      tvbuff_t *next_tvb = process_reassembled_data(tvb, 0, pinfo, "Reassembled Data", frag_msg, &data_frag_items, NULL, NULL);
      if (next_tvb) {
        proto_item *frag_tree_item;
        show_fragment_tree(frag_msg, &data_frag_items, tree, pinfo, next_tvb, &frag_tree_item);
        call_dissector(ip_handle, next_tvb, pinfo, tree);
      } else {
        col_append_fstr(pinfo->cinfo, COL_INFO, "(Data reassembled in packet %u)", frag_msg->reassembled_in);
      }
    }
    break;
  case 0x1e:
    col_add_fstr(pinfo->cinfo, COL_INFO, "SIGNAL %d",id);
    return dissect_npr70_signal(tvb, pinfo, npr70_tree, offset);
  case 0x1f:
    return dissect_npr70_alloc(tvb, pinfo, npr70_tree, offset);
  default:
    col_set_str(pinfo->cinfo, COL_PROTOCOL, "Proto ???");
    break;
  }
  return tvb_captured_length(tvb);
}

void
proto_register_npr70(void)
{
  static hf_register_info hf[] = {
    { &hf_npr70_type, {
        "Type", "npr70.type",
        FT_UINT8, BASE_DEC, VALS(npr70_type_vals), 0x0,
        NULL, HFILL }},

    { &hf_npr70_seq, {
        "Seq", "npr70.seq",
        FT_UINT8, BASE_DEC, NULL, 0x0,
        NULL, HFILL }},

    { &hf_npr70_us, {
        "us", "npr70.us",
        FT_UINT24, BASE_DEC, NULL, 0x0,
        NULL, HFILL }},

    { &hf_npr70_rssi, {
        "RSSI", "npr70.rssi",
        FT_UINT8, BASE_DEC, NULL, 0x0,
        NULL, HFILL }},

    { &hf_npr70_len, {
        "Length", "npr70.len",
        FT_UINT8, BASE_DEC, NULL, 0x0,
        NULL, HFILL }},

    { &hf_npr70_timestamp, {
        "Timestamp", "npr70.timestamp",
        FT_UINT24, BASE_DEC, NULL, 0x0,
        NULL, HFILL }},

    { &hf_npr70_tdma_parity, {
        "TDMA Parity", "npr70.tdma.parity",
        FT_BOOLEAN, BASE_NONE, NULL, 0x0,
        NULL, HFILL }},

    { &hf_npr70_tdma_downlink, {
        "TDMA Downlink", "npr70.tdma.downlink",
        FT_BOOLEAN, BASE_NONE, NULL, 0x0,
        NULL, HFILL }},

    { &hf_npr70_tdma_synchro, {
        "TDMA Synchro", "npr70.tdma.synchro",
        FT_BOOLEAN, BASE_NONE, NULL, 0x0,
        NULL, HFILL }},

    { &hf_npr70_tdma_frame_counter, {
        "TDMA Frame counter", "npr70.tdma.frame_counter",
        FT_UINT8, BASE_DEC, NULL, 0x0,
        NULL, HFILL }},

    { &hf_npr70_tdma_uplink_buffer, {
        "TDMA Uplink Buffer", "npr70.tdma.uplink_buffer",
        FT_UINT8, BASE_DEC, NULL, 0x0,
        NULL, HFILL }},

    { &hf_npr70_errors, {
        "Errors", "npr70.errors",
        FT_UINT8, BASE_DEC, NULL, 0x0,
        NULL, HFILL }},

     { &hf_hf_fragments,
            {"Message fragments", "hf.fragments", FT_NONE, BASE_NONE,
                NULL, 0x00, NULL, HFILL }},
      { &hf_hf_fragment,
          {"Message fragment", "hf.fragment", FT_FRAMENUM, BASE_NONE,
                NULL, 0x00, NULL, HFILL }},
      { &hf_hf_fragment_overlap,
          {"Message fragment overlap", "hf.fragment.overlap",
                FT_BOOLEAN, BASE_NONE,
                NULL, 0x00, NULL, HFILL }},
      { &hf_hf_fragment_overlap_conflicts,
          {"Message fragment overlapping with conflicting data", "hf.fragment.overlap.conflicts",
                FT_BOOLEAN, BASE_NONE,
                NULL, 0x00, NULL, HFILL }},
      { &hf_hf_fragment_multiple_tails,
          {"Message has multiple tail fragments", "hf.fragment.multiple_tails",
                FT_BOOLEAN, BASE_NONE,
                NULL, 0x00, NULL, HFILL }},
      { &hf_hf_fragment_too_long_fragment,
          {"Message fragment too long", "hf.fragment.too_long_fragment",
                FT_BOOLEAN, BASE_NONE,
                NULL, 0x00, NULL, HFILL }},
      { &hf_hf_fragment_error,
          {"Message defragmentation error", "hf.fragment.error",
                FT_FRAMENUM, BASE_NONE,
                NULL, 0x00, NULL, HFILL }},
      { &hf_hf_fragment_count,
          {"Message fragment count", "hf.fragment.count", FT_UINT32, BASE_DEC,
                NULL, 0x00, NULL, HFILL }},
      { &hf_hf_reassembled_in,
          {"Reassembled in", "hf.reassembled.in", FT_FRAMENUM, BASE_NONE,
                NULL, 0x00, NULL, HFILL }},
      { &hf_hf_reassembled_length,
          {"Reassembled length", "hf.reassembled.length", FT_UINT32, BASE_DEC,
                NULL, 0x00, NULL, HFILL }}

  };

  static ei_register_info ei[] = {
      {
         &ei_packet_missing,
         { "npr70.packet_missing", PI_PROTOCOL, PI_WARN, "Missing Packet(s)", EXPFILL }
      },
  };

  static hf_register_info hffec[] = {
    { &hf_npr70_client_parity, {
        "Client Parity", "npr70fec.client.parity",
        FT_BOOLEAN, BASE_NONE, NULL, 0x0,
        NULL, HFILL }},

    { &hf_npr70_client_id, {
        "Client ID", "npr70fec.client.id",
        FT_UINT8, BASE_DEC, NULL, 0x0,
        NULL, HFILL }},

    { &hf_npr70_proto, {
        "Protocol", "npr70fec.proto",
        FT_UINT8, BASE_DEC, VALS(npr70_proto_vals), 0x0,
        NULL, HFILL }},

    { &hf_npr70_signal, {
        "Signal", "npr70fec.signal",
        FT_UINT8, BASE_DEC, VALS(npr70_signal_vals), 0x0,
        NULL, HFILL }},

    { &hf_npr70_slen, {
        "Signal_length", "npr70fec.slen",
        FT_UINT8, BASE_DEC, NULL, 0x0,
        NULL, HFILL }},

    { &hf_npr70_id, {
        "Id", "npr70fec.id",
        FT_UINT8, BASE_DEC, NULL, 0x0,
        NULL, HFILL }},

    { &hf_npr70_mac, {
        "MAC", "npr70fec.mac",
        FT_UINT8, BASE_HEX, NULL, 0x0,
        NULL, HFILL }},

    { &hf_npr70_callsign, {
        "Callsign", "npr70fec.callsign",
        FT_STRING, BASE_NONE, NULL, 0x0,
        NULL, HFILL }},

    { &hf_npr70_ip_start, {
        "IP Start", "npr70fec.ip_start",
        FT_IPv4, BASE_NONE, NULL, 0x0,
        NULL, HFILL }},

    { &hf_npr70_ip_length, {
        "IP Length", "npr70fec.ip_length",
        FT_UINT32, BASE_DEC, NULL, 0x0,
        NULL, HFILL }},

    { &hf_npr70_static_alloc, {
        "Static alloc", "npr70fec.static_alloc",
        FT_UINT8, BASE_DEC, NULL, 0x0,
        NULL, HFILL }},

    { &hf_npr70_master_mac, {
        "Master MAC", "npr70fec.master_mac",
        FT_UINT8, BASE_HEX, NULL, 0x0,
        NULL, HFILL }},

    { &hf_npr70_master_callsign, {
        "Master Callsign", "npr70fec.master_callsign",
        FT_STRING, BASE_NONE, NULL, 0x0,
        NULL, HFILL }},

    { &hf_npr70_modem_ip, {
        "Modem IP", "npr70fec.modem_ip",
        FT_IPv4, BASE_NONE, NULL, 0x0,
        NULL, HFILL }},

    { &hf_npr70_subnet_mask, {
        "Subnet Mask", "npr70fec.subnet_mask",
        FT_IPv4, BASE_NONE, NULL, 0x0,
        NULL, HFILL }},

    { &hf_npr70_default_route_active, {
        "Default Route active", "npr70fec.default_route_active",
        FT_UINT8, BASE_DEC, NULL, 0x0,
        NULL, HFILL }},

    { &hf_npr70_default_route, {
        "Default Route", "npr70fec.default_route",
        FT_IPv4, BASE_NONE, NULL, 0x0,
        NULL, HFILL }},

    { &hf_npr70_dns_active, {
        "DNS active", "npr70fec.dns_active",
        FT_UINT8, BASE_DEC, NULL, 0x0,
        NULL, HFILL }},

    { &hf_npr70_dns, {
        "DNS", "npr70fec.dns",
        FT_IPv4, BASE_NONE, NULL, 0x0,
        NULL, HFILL }},

    { &hf_npr70_pkt_counter, {
        "Packet counter", "npr70fec.pkt_counter",
        FT_UINT8, BASE_DEC, NULL, 0x0,
        NULL, HFILL }},

    { &hf_npr70_is_last_seg, {
        "Is last SEG", "npr70fec.is_last_seq",
        FT_UINT8, BASE_DEC, NULL, 0x0,
        NULL, HFILL }},

    { &hf_npr70_seg_counter, {
        "SEG counter", "npr70fec.seg_counter",
        FT_UINT8, BASE_DEC, NULL, 0x0,
        NULL, HFILL }},

    { &hf_npr70_rssi_uplink, {
        "RSSI Uplink", "npr70fec.rssi_uplink",
        FT_UINT8, BASE_DEC, NULL, 0x0,
        NULL, HFILL }},

    { &hf_npr70_ber, {
        "BER", "npr70fec.berr",
        FT_UINT16, BASE_DEC, NULL, 0x0,
        NULL, HFILL }},

    { &hf_npr70_ta, {
        "Timing Advance", "npr70fec.ta",
        FT_UINT16, BASE_DEC, NULL, 0x0,
        NULL, HFILL }},

    { &hf_npr70_tdma_offset, {
        "TDMA Offset", "npr70fec.tdma_offset",
        FT_UINT16, BASE_DEC, NULL, 0x0,
        NULL, HFILL }},

    { &hf_npr70_tdma_slot_length, {
        "TDMA Slot Length", "npr70fec.tdma_slot_length",
        FT_UINT8, BASE_DEC, NULL, 0x0,
        NULL, HFILL }},

    { &hf_npr70_tdma_multiframe, {
        "TDMA Multiframe", "npr70fec.tdma_multiframe",
        FT_UINT8, BASE_DEC, NULL, 0x0,
        NULL, HFILL }},

     { &hf_data_fragments,
            {"Message fragments", "data.fragments", FT_NONE, BASE_NONE,
                NULL, 0x00, NULL, HFILL }},
      { &hf_data_fragment,
          {"Message fragment", "data.fragment", FT_FRAMENUM, BASE_NONE,
                NULL, 0x00, NULL, HFILL }},
      { &hf_data_fragment_overlap,
          {"Message fragment overlap", "data.fragment.overlap",
                FT_BOOLEAN, BASE_NONE,
                NULL, 0x00, NULL, HFILL }},
      { &hf_data_fragment_overlap_conflicts,
          {"Message fragment overlapping with conflicting data", "data.fragment.overlap.conflicts",
                FT_BOOLEAN, BASE_NONE,
                NULL, 0x00, NULL, HFILL }},
      { &hf_data_fragment_multiple_tails,
          {"Message has multiple tail fragments", "data.fragment.multiple_tails",
                FT_BOOLEAN, BASE_NONE,
                NULL, 0x00, NULL, HFILL }},
      { &hf_data_fragment_too_long_fragment,
          {"Message fragment too long", "data.fragment.too_long_fragment",
                FT_BOOLEAN, BASE_NONE,
                NULL, 0x00, NULL, HFILL }},
      { &hf_data_fragment_error,
          {"Message defragmentation error", "data.fragment.error",
                FT_FRAMENUM, BASE_NONE,
                NULL, 0x00, NULL, HFILL }},
      { &hf_data_fragment_count,
          {"Message fragment count", "data.fragment.count", FT_UINT32, BASE_DEC,
                NULL, 0x00, NULL, HFILL }},
      { &hf_data_reassembled_in,
          {"Reassembled in", "data.reassembled.in", FT_FRAMENUM, BASE_NONE,
                NULL, 0x00, NULL, HFILL }},
      { &hf_data_reassembled_length,
          {"Reassembled length", "data.reassembled.length", FT_UINT32, BASE_DEC,
                NULL, 0x00, NULL, HFILL }}

  };

  static gint *ett[] = {
    &ett_npr70,
    &ett_npr70fe,
    &ett_hf_fragment,
    &ett_hf_fragments,
    &ett_data_fragment,
    &ett_data_fragments
  };

  proto_npr70 = proto_register_protocol("New Packet Radio 70", "NPR70", "npr70");
  proto_npr70fec = proto_register_protocol("NPR70 FEC Data", "NPR70FEC", "npr70fec");
  npr70_handle = register_dissector("npr70", dissect_npr70, proto_npr70);
  npr70fec_handle = register_dissector("npr70fec", dissect_npr70fec, proto_npr70fec);

  expert_module_t *expert_npr70 = expert_register_protocol(proto_npr70);
  expert_register_field_array(expert_npr70, ei, array_length(ei));

  proto_register_field_array(proto_npr70, hf, array_length(hf));
  proto_register_field_array(proto_npr70fec, hffec, array_length(hffec));
  proto_register_subtree_array(ett, array_length(ett));

  reassembly_table_register(&npr70_reassembly_table, &addresses_ports_reassembly_table_functions);
  reassembly_table_register(&ip_reassembly_table, &addresses_ports_reassembly_table_functions);
  npr70_hf_fragments = wmem_tree_new(wmem_epan_scope());
  npr70_data_fragments = wmem_tree_new(wmem_epan_scope());

}

void
proto_reg_handoff_npr70(void)
{
  dissector_add_uint("ethertype", 0xffff, npr70_handle);
  ip_handle = find_dissector("ip");
}

void
plugin_register(void)
{
  static proto_plugin plug;

  plug.register_protoinfo = proto_register_npr70;
  plug.register_handoff = proto_reg_handoff_npr70; /* or NULL */
  proto_register_plugin(&plug);
}

/*
 * Editor modelines
 *
 * Local Variables:
 * c-basic-offset: 2
 * tab-width: 8
 * indent-tabs-mode: nil
 * End:
 *
 * ex: set shiftwidth=2 tabstop=8 expandtab:
 * :indentSize=2:tabSize=8:noTabs=true:
 */
