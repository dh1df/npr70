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
#include "fec.h"

#ifndef VERSION
#define VERSION "0.0.0"
#endif

WS_DLL_PUBLIC_DEF const gchar plugin_version[] = VERSION;
WS_DLL_PUBLIC_DEF const int plugin_want_major = WIRESHARK_VERSION_MAJOR;
WS_DLL_PUBLIC_DEF const int plugin_want_minor = WIRESHARK_VERSION_MINOR;
WS_DLL_PUBLIC void plugin_register(void);


void proto_register_npr70(void);
void proto_reg_handoff_npr70(void);

static dissector_handle_t foo_handle;

int proto_npr70 = -1;
int proto_npr70fec = -1;
static int hf_npr70_type = -1;
static int hf_npr70_seq = -1;
static int hf_npr70_us = -1;
static int hf_npr70_timestamp = -1;
static int hf_npr70_rssi = -1;
static int hf_npr70_len = -1;
static int hf_npr70_tdma = -1;
static int hf_npr70_errors = -1;
static int hf_npr70_client = -1;
static int hf_npr70_proto = -1;
static int hf_npr70_signal = -1;
static int hf_npr70_slen = -1;
static int hf_npr70_id = -1;
static int hf_npr70_mac = -1;
static int hf_npr70_callsign = -1;
static int hf_npr70_ip_start = -1;
static int hf_npr70_ip_length = -1;
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
static int npr70_len;
static int fragments_size;

static gint hf_mp_fragments = -1;
static gint hf_mp_fragment = -1;
static gint hf_mp_fragment_overlap = -1;
static gint hf_mp_fragment_overlap_conflicts = -1;
static gint hf_mp_fragment_multiple_tails = -1;
static gint hf_mp_fragment_too_long_fragment = -1;
static gint hf_mp_fragment_error = -1;
static gint hf_mp_fragment_count = -1;
static gint hf_mp_reassembled_in = -1;
static gint hf_mp_reassembled_length = -1;

static int ett_mp = -1;
static int ett_mp_flags = -1;
static gint ett_mp_fragment = -1;
static gint ett_mp_fragments = -1;

static const fragment_items mp_frag_items = {
    /* Fragment subtrees */
    &ett_mp_fragment,
    &ett_mp_fragments,
    /* Fragment fields */
    &hf_mp_fragments,
    &hf_mp_fragment,
    &hf_mp_fragment_overlap,
    &hf_mp_fragment_overlap_conflicts,
    &hf_mp_fragment_multiple_tails,
    &hf_mp_fragment_too_long_fragment,
    &hf_mp_fragment_error,
    &hf_mp_fragment_count,
    /* Reassembled in field */
    &hf_mp_reassembled_in,
    /* Reassembled length field */
    &hf_mp_reassembled_length,
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
  { 0x1e, "Signal" },
  { 0x1f, "Alloc" },
};

static const value_string npr70_signal_vals[] = {
  { 0, "NULL" },
  { 1, "WHOIS" },
  { 5, "CONN REQ" },
  { 6, "CONN ACK" },
  { 7, "CONN NACK" },
  { 0xb, "DISCONN REQ" },
  { 0xc, "DISCONN ACK" },
  { 0, NULL }
};

#if 1
static void
print_fragment_table_chain(gpointer k _U_, gpointer v, gpointer ud _U_) {
        printf("%p %p\n",k,v);
}
#endif

static int
dissect_npr70(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data _U_)
{
  int         offset = 0;
  int         remaining;
  gboolean    more;
  guint8      npr70_type;
  proto_tree *ti;
  proto_tree *npr70_tree;
  tvbuff_t   *next_tvb;
  fragment_head *frag_msg;

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

  proto_tree_add_item(npr70_tree, hf_npr70_seq, tvb, offset, 1, ENC_NA);
  offset+=1;

  proto_tree_add_item(npr70_tree, hf_npr70_us, tvb, offset, 4, ENC_LITTLE_ENDIAN);
  offset+=4;

  if (npr70_type == 0) {
    proto_tree_add_item(npr70_tree, hf_npr70_timestamp, tvb, offset, 3, ENC_LITTLE_ENDIAN);
    offset+=3;

    proto_tree_add_item(npr70_tree, hf_npr70_rssi, tvb, offset, 1, ENC_NA);
    offset+=1;

    npr70_len = tvb_get_guint8(tvb, offset)+90;
    proto_tree_add_uint(npr70_tree, hf_npr70_len, tvb, offset, 1, npr70_len);
    offset+=1;

    proto_tree_add_item(npr70_tree, hf_npr70_tdma, tvb, offset, 1, ENC_NA);
    offset+=1;

    fragments_size = 0;
    reassembly_id=pinfo->num;
    npr70_len--;
  }
  remaining=tvb_captured_length_remaining(tvb, offset);
  more=fragments_size+remaining < npr70_len;
  frag_msg = fragment_add(&npr70_reassembly_table, tvb, offset, pinfo, reassembly_id, NULL, fragments_size, remaining, more);
  fragments_size += remaining;
  if (!more && !frag_msg) {
        g_hash_table_foreach(npr70_reassembly_table.fragment_table, print_fragment_table_chain, NULL);
        printf("error\n");
        exit(0);
  }
  if (frag_msg) {
    next_tvb = process_reassembled_data(tvb, 0, pinfo, "Reassembled Message", frag_msg, &mp_frag_items, NULL, NULL);
    if (next_tvb) {
      guchar *fec_buffer = (guchar*)wmem_alloc(pinfo->pool, npr70_len);
      const unsigned char *buf = tvb_get_ptr(next_tvb, 0, -1);
      unsigned int errors;
      int len=FEC_decode(buf, -1, 0, fec_buffer, npr70_len, &errors);
      proto_tree_add_uint(npr70_tree, hf_npr70_errors, tvb, 0, 0, 0);
#if 0
    int i;
    printf("(%d)",npr70_len);
    for (i = 0 ; i < npr70_len ; i++)
        printf(" %02x",buf[i]);
    printf("\n");
#endif
#if 0
    proto_tree_add_uint(npr70_tree, hf_npr70_errors, tvb, 0, 0, errors);
#endif
      next_tvb = tvb_new_child_real_data(next_tvb, fec_buffer, len, len);
      add_new_data_source(pinfo, next_tvb, "FECed Message");
      call_dissector(npr70fec_handle, next_tvb, pinfo, npr70_tree);
    }
  }
  return tvb_captured_length(tvb);
}

static int
dissect_npr70_signal_single(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, int offset)
{
  guint8      npr70_signal, npr70_len;
  int saved_offset;

  if (tvb_reported_length_remaining(tvb, offset) < 2)
    return -1;
  npr70_signal = tvb_get_guint8(tvb, offset);
  if (npr70_signal == 0xff)
        return -1;
  proto_tree_add_item(tree, hf_npr70_signal, tvb, offset, 1, ENC_NA);
  offset++;
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
  case 0:
    col_set_str(pinfo->cinfo, COL_PROTOCOL, "NULL");
    break;
  case 1:
    if (tvb_reported_length_remaining(tvb, offset) < 32)
      return -1;
    col_set_str(pinfo->cinfo, COL_PROTOCOL, "WHO");
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
    proto_tree_add_item(tree, hf_npr70_ber, tvb, offset, 2, ENC_BIG_ENDIAN);
    offset+=2;
    proto_tree_add_item(tree, hf_npr70_ta, tvb, offset, 2, ENC_BIG_ENDIAN);
    offset+=2;
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
  col_set_str(pinfo->cinfo, COL_PROTOCOL, "Signal");
  do {
    offset=dissect_npr70_signal_single(tvb, pinfo, tree, offset);
  } while (offset != -1);
  return tvb_captured_length(tvb);
}


static int
dissect_npr70_alloc_client(tvbuff_t *tvb,  proto_tree *tree, int offset)
{
  guint8 client;
  if (tvb_reported_length_remaining(tvb, offset) < 5)
    return -1;
  client  = tvb_get_guint8(tvb, offset);
  if (client == 0xff)
    return -1;
  proto_tree_add_item(tree, hf_npr70_id, tvb, offset, 1, ENC_NA);
  offset++;
  proto_tree_add_item(tree, hf_npr70_tdma_offset, tvb, offset, 2, ENC_BIG_ENDIAN);
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
  col_set_str(pinfo->cinfo, COL_PROTOCOL, "Alloc");
  do {
    offset=dissect_npr70_alloc_client(tvb, tree, offset);
  } while (offset != -1);
  return tvb_captured_length(tvb);
}

static int
dissect_npr70fec(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data _U_)
{
  int         offset = 0;
  guint8      npr70_protocol;
  proto_tree *ti;
  proto_tree *npr70_tree;

  col_set_str(pinfo->cinfo, COL_PROTOCOL, "NPR70 FEC");
  col_clear(pinfo->cinfo, COL_INFO);

  ti = proto_tree_add_item(tree, proto_npr70fec, tvb, 0, -1, ENC_NA);
  npr70_tree = proto_item_add_subtree(ti, ett_npr70fe);

  proto_tree_add_item(npr70_tree, hf_npr70_client, tvb, offset, 1, ENC_NA);
  offset++;

  proto_tree_add_item(npr70_tree, hf_npr70_proto, tvb, offset, 1, ENC_NA);
  npr70_protocol = tvb_get_guint8(tvb, offset);
  offset++;
  switch(npr70_protocol) {
  case 0:
    col_set_str(pinfo->cinfo, COL_PROTOCOL, "NULL");
    break;
  case 0x1e:
    return dissect_npr70_signal(tvb, pinfo, npr70_tree, offset);
  case 0x1f:
    return dissect_npr70_alloc(tvb, pinfo, npr70_tree, offset);
  default:
    col_set_str(pinfo->cinfo, COL_PROTOCOL, "Proto ???");
    break;
  }
  col_clear(pinfo->cinfo, COL_INFO);
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

    { &hf_npr70_tdma, {
        "TDMA", "npr70.tdma",
        FT_UINT8, BASE_DEC, NULL, 0x0,
        NULL, HFILL }},

    { &hf_npr70_errors, {
        "Errors", "npr70.errors",
        FT_UINT8, BASE_DEC, NULL, 0x0,
        NULL, HFILL }},

     { &hf_mp_fragments,
            {"Message fragments", "mp.fragments", FT_NONE, BASE_NONE,
                NULL, 0x00, NULL, HFILL }},
      { &hf_mp_fragment,
          {"Message fragment", "mp.fragment", FT_FRAMENUM, BASE_NONE,
                NULL, 0x00, NULL, HFILL }},
      { &hf_mp_fragment_overlap,
          {"Message fragment overlap", "mp.fragment.overlap",
                FT_BOOLEAN, BASE_NONE,
                NULL, 0x00, NULL, HFILL }},
      { &hf_mp_fragment_overlap_conflicts,
          {"Message fragment overlapping with conflicting data", "mp.fragment.overlap.conflicts",
                FT_BOOLEAN, BASE_NONE,
                NULL, 0x00, NULL, HFILL }},
      { &hf_mp_fragment_multiple_tails,
          {"Message has multiple tail fragments", "mp.fragment.multiple_tails",
                FT_BOOLEAN, BASE_NONE,
                NULL, 0x00, NULL, HFILL }},
      { &hf_mp_fragment_too_long_fragment,
          {"Message fragment too long", "mp.fragment.too_long_fragment",
                FT_BOOLEAN, BASE_NONE,
                NULL, 0x00, NULL, HFILL }},
      { &hf_mp_fragment_error,
          {"Message defragmentation error", "mp.fragment.error",
                FT_FRAMENUM, BASE_NONE,
                NULL, 0x00, NULL, HFILL }},
      { &hf_mp_fragment_count,
          {"Message fragment count", "mp.fragment.count", FT_UINT32, BASE_DEC,
                NULL, 0x00, NULL, HFILL }},
      { &hf_mp_reassembled_in,
          {"Reassembled in", "mp.reassembled.in", FT_FRAMENUM, BASE_NONE,
                NULL, 0x00, NULL, HFILL }},
      { &hf_mp_reassembled_length,
          {"Reassembled length", "mp.reassembled.length", FT_UINT32, BASE_DEC,
                NULL, 0x00, NULL, HFILL }}

  };
  static hf_register_info hffec[] = {
    { &hf_npr70_client, {
        "Client", "npr70.client",
        FT_UINT8, BASE_DEC, NULL, 0x0,
        NULL, HFILL }},

    { &hf_npr70_proto, {
        "Protocol", "npr70.proto",
        FT_UINT8, BASE_DEC, VALS(npr70_proto_vals), 0x0,
        NULL, HFILL }},

    { &hf_npr70_signal, {
        "Signal", "npr70.signal",
        FT_UINT8, BASE_DEC, VALS(npr70_signal_vals), 0x0,
        NULL, HFILL }},

    { &hf_npr70_slen, {
        "Signal_length", "npr70.slen",
        FT_UINT8, BASE_DEC, NULL, 0x0,
        NULL, HFILL }},

    { &hf_npr70_id, {
        "Id", "npr70.id",
        FT_UINT8, BASE_DEC, NULL, 0x0,
        NULL, HFILL }},

    { &hf_npr70_mac, {
        "mac", "npr70.mac",
        FT_UINT8, BASE_HEX, NULL, 0x0,
        NULL, HFILL }},

    { &hf_npr70_callsign, {
        "Callsign", "npr70.callsign",
        FT_STRING, BASE_NONE, NULL, 0x0,
        NULL, HFILL }},

    { &hf_npr70_ip_start, {
        "IP Start", "npr70.ip_start",
        FT_IPv4, BASE_NONE, NULL, 0x0,
        NULL, HFILL }},

    { &hf_npr70_ip_length, {
        "IP Length", "npr70.ip_length",
        FT_UINT32, BASE_DEC, NULL, 0x0,
        NULL, HFILL }},

    { &hf_npr70_rssi_uplink, {
        "RSSI Uplink", "npr70.rssi_uplink",
        FT_UINT8, BASE_DEC, NULL, 0x0,
        NULL, HFILL }},

    { &hf_npr70_ber, {
        "BER", "npr70.berr",
        FT_UINT16, BASE_DEC, NULL, 0x0,
        NULL, HFILL }},

    { &hf_npr70_ta, {
        "Timing Advance", "npr70.ta",
        FT_UINT16, BASE_DEC, NULL, 0x0,
        NULL, HFILL }},

    { &hf_npr70_tdma_offset, {
        "TDMA Offset", "npr70.tdma_offset",
        FT_UINT16, BASE_DEC, NULL, 0x0,
        NULL, HFILL }},

    { &hf_npr70_tdma_slot_length, {
        "TDMA Slot Length", "npr70.tdma_slot_length",
        FT_UINT8, BASE_DEC, NULL, 0x0,
        NULL, HFILL }},

    { &hf_npr70_tdma_multiframe, {
        "TDMA Multiframe", "npr70.tdma_multiframe",
        FT_UINT8, BASE_DEC, NULL, 0x0,
        NULL, HFILL }},

  };

  static gint *ett[] = {
    &ett_npr70,
    &ett_npr70fe,
  };

  proto_npr70 = proto_register_protocol("New Packet Radio 70", "NPR70", "npr70");
  proto_npr70fec = proto_register_protocol("NPR70 FEC Data", "NPR70FEC", "npr70fec");
  npr70_handle = register_dissector("npr70", dissect_npr70, proto_npr70);
  npr70fec_handle = register_dissector("npr70fec", dissect_npr70fec, proto_npr70fec);

  proto_register_field_array(proto_npr70, hf, array_length(hf));
  proto_register_field_array(proto_npr70fec, hffec, array_length(hffec));
  proto_register_subtree_array(ett, array_length(ett));

  reassembly_table_register(&npr70_reassembly_table, &addresses_ports_reassembly_table_functions);
  reassembly_table_register(&ip_reassembly_table, &addresses_ports_reassembly_table_functions);

}

void
proto_reg_handoff_npr70(void)
{
  dissector_add_uint("ethertype", 0xffff, npr70_handle);
  foo_handle = find_dissector("ip");
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
