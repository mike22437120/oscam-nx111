#include "globals.h"
#ifdef READER_VIDEOGUARD
#include "reader-common.h"
#include "reader-videoguard-common.h"

static int32_t vg11_do_cmd(struct s_reader *reader, const unsigned char *ins, const unsigned char *txbuff, unsigned char *rxbuff, unsigned char *cta_res)
{
  uint16_t cta_lr;
  unsigned char ins2[5];
  memcpy(ins2, ins, 5);
  unsigned char len = 0;
  len = ins2[4];

  if (txbuff == NULL) {
    if (!write_cmd_vg(ins2, NULL) || !status_ok(cta_res + len)) {
      return -1;
    }
    if(rxbuff != NULL){
	    memcpy(rxbuff, ins2, 5);
	    memcpy(rxbuff + 5, cta_res, len);
	    memcpy(rxbuff + 5 + len, cta_res + len, 2);
	  }
  } else {
    if (!write_cmd_vg(ins2, (uchar *) txbuff) || !status_ok(cta_res)) {
      return -2;
    }
    if(rxbuff != NULL){
	    memcpy(rxbuff, ins2, 5);
	    memcpy(rxbuff + 5, txbuff, len);
	    memcpy(rxbuff + 5 + len, cta_res, 2);
	  }
  }

  return len;
}

static void read_tiers(struct s_reader *reader)
{
  def_resp;
//  const unsigned char ins2a[5] = {  0x48, 0x2a, 0x00, 0x00, 0x00  };
  int32_t l;

//  return; // Not working at present so just do nothing

//  l = vg11_do_cmd(reader, ins2a, NULL, NULL, cta_res);
//  if (l < 0 || !status_ok(cta_res + l))
//  {
//    return;
//  }
  unsigned char ins76[5] = { 0x48, 0x76, 0x00, 0x00, 0x00 };
  ins76[3] = 0x7f;
  ins76[4] = 2;
  if (!write_cmd_vg(ins76, NULL) || !status_ok(cta_res + 2)) {
    return;
  }
  ins76[3] = 0;
  ins76[4] = 0x0a;
  int32_t num = cta_res[1];
  int32_t i;

  cs_clear_entitlement(reader); //reset the entitlements

  for (i = 0; i < num; i++) {
    ins76[2] = i;
    l = vg11_do_cmd(reader, ins76, NULL, NULL, cta_res);
    if (l < 0 || !status_ok(cta_res + l)) {
      return;
    }
    if (cta_res[2] == 0 && cta_res[3] == 0) {
      break;
    }
    int32_t y, m, d, H, M, S;
    rev_date_calc(&cta_res[4], &y, &m, &d, &H, &M, &S, reader->card_baseyear);
    uint16_t tier_id = (cta_res[2] << 8) | cta_res[3];


    // add entitlements to list
    struct tm timeinfo;
    memset(&timeinfo, 0, sizeof(struct tm));
    timeinfo.tm_year = y - 1900; //tm year starts with 1900
    timeinfo.tm_mon = m - 1; //tm month starts with 0
    timeinfo.tm_mday = d;
    cs_add_entitlement(reader, reader->caid, b2ll(4, reader->prid[0]), tier_id, 0, 0, mktime(&timeinfo), 4);

    char tiername[83];
    rdr_log(reader, "tier: %04x, expiry date: %04d/%02d/%02d-%02d:%02d:%02d %s", tier_id, y, m, d, H, M, S, get_tiername(tier_id, reader->caid, tiername));
  }
}

static int32_t videoguard11_card_init(struct s_reader *reader, ATR *newatr)
{

  get_hist;
  /* 40 B0 12 69 FF 4A 50  */
  if ((hist_size < 7) || (hist[1] != 0xB0) || (hist[4] != 0xFF) || (hist[5] != 0x4A) || (hist[6] != 0x50)){
       return ERROR;
  }

  get_atr;
  def_resp;

  /* set information on the card stored in reader-videoguard-common.c */
  set_known_card_info(reader,atr,&atr_size);

  if((reader->ndsversion != NDS11) && ((reader->card_system_version != NDS11) || (reader->ndsversion != NDSAUTO))) {
    /* known ATR and not NDS11
       or unknown ATR and not forced to NDS11
       or known NDS1 ATR and forced to another NDS version
       ... probably not NDS11 */
    return ERROR;
  }

  rdr_log(reader, "type: %s, baseyear: %i", reader->card_desc, reader->card_baseyear);
  if(reader->ndsversion == NDS11){
    rdr_log(reader, "forced to NDS1+");
  }

  /* NDS1 Class 48 only cards only need a very basic initialisation
     NDS1 Class 48 only cards do not respond to vg11_do_cmd(ins7416)
     nor do they return list of valid command therefore do not even try
     NDS1 Class 48 only cards need to be told the length as (48, ins, 00, 80, 01)
     does not return the length */

  unsigned char boxID[4];
  int32_t boxidOK = 0;


  /* the boxid is specified in the config */
  if (reader->boxid > 0) {
    int32_t i;
    for (i = 0; i < 4; i++) {
      boxID[i] = (reader->boxid >> (8 * (3 - i))) % 0x100;
    }
    boxidOK = 1;
    // rdr_log(reader, "config BoxID: %02X%02X%02X%02X", boxID[0], boxID[1], boxID[2], boxID[3]);
  }

  if (!boxidOK) {
    rdr_log(reader, "no boxID available");
    return ERROR;
  }

  // Send BoxID
  static const unsigned char ins4C[5] = { 0x48, 0x4C, 0x00, 0x00, 0x09 };
  unsigned char payload4C[9] = { 0, 0, 0, 0, 0x03, 0xC8, 0, 0, 0 };
  memcpy(payload4C, boxID, 4);
  if (!write_cmd_vg(ins4C, payload4C) || !status_ok(cta_res + cta_lr-2)) {
    rdr_log(reader, "class48 ins4C: sending boxid failed");
  //  return ERROR;
  }


  static const unsigned char ins0C[5] = { 0x48, 0x0C, 0x00, 0x00, 0x0A };
  if (!write_cmd_vg(ins0C,NULL) || !status_ok(cta_res+cta_lr-2)) {
    rdr_log(reader, "class48 ins0C: failed");
    //return ERROR;
  }

  static const unsigned char ins58[5] = { 0x48, 0x58, 0x00, 0x00, 0x17 };
  if (!write_cmd_vg(ins58,NULL) || !status_ok(cta_res+cta_lr-2)) {
    rdr_log(reader, "class48 ins58: failed");
  //  return ERROR;
  }

  memset(reader->hexserial, 0, 8);
  memcpy(reader->hexserial + 2, cta_res + 1, 4);
  memcpy(reader->sa, cta_res + 1, 3);
  //  reader->caid = cta_res[24] * 0x100 + cta_res[25];
  /* Force caid until can figure out how to get it */
  reader->caid = 0x9 * 0x100 + 0x9C;

  /* we have one provider, 0x0000 */
  reader->nprov = 1;
  memset(reader->prid, 0x00, sizeof(reader->prid));

  rdr_log_sensitive(reader, "type: VideoGuard, caid: %04X, serial: {%02X%02X%02X%02X}, BoxID: {%02X%02X%02X%02X}",
    reader->caid, reader->hexserial[2], reader->hexserial[3], reader->hexserial[4], reader->hexserial[5],
    boxID[0], boxID[1], boxID[2], boxID[3]);
  rdr_log(reader, "ready for requests - this is in testing please send -d 255 logs");

  return OK;
}

static int32_t videoguard11_do_ecm(struct s_reader * reader, const ECM_REQUEST *er, struct s_ecm_answer *ea)
{
  unsigned char cta_res[CTA_RES_LEN];
  unsigned char ins40[5] = { 0x48, 0x40, 0x20, 0x00, 0x52 };
  static const unsigned char ins54[5] = { 0x48, 0x54, 0x00, 0x00, 0x0D };
  int32_t posECMpart2 = er->ecm[6] + 7;
  int32_t lenECMpart2 = er->ecm[posECMpart2];
  unsigned char tbuff[264];
  unsigned char rbuff[264];
  memcpy(&tbuff[0], &(er->ecm[posECMpart2 + 1]), lenECMpart2);
  ins40[4] = lenECMpart2;
  int32_t l;
  l = vg11_do_cmd(reader, ins40, tbuff, NULL, cta_res);
  if (l > 0 && status_ok(cta_res)) {
    l = vg11_do_cmd(reader, ins54, NULL, rbuff, cta_res);
    if (l > 0 && status_ok(cta_res + l)) {
      if (!cw_is_valid(rbuff+5))    //sky cards report 90 00 = ok but send cw = 00 when channel not subscribed
      {
        rdr_log(reader, "class48 ins54 status 90 00 but cw=00 -> channel not subscribed");
        return ERROR;
      }

      if(er->ecm[0]&1) {
        memset(ea->cw+0, 0, 8);
        memcpy(ea->cw+8, rbuff + 5, 8);
      } else {
        memcpy(ea->cw+0, rbuff + 5, 8);
        memset(ea->cw+8, 0, 8);
      }
      return OK;
    }
  }
  rdr_log(reader, "class48 ins54 (%d) status not ok %02x %02x", l, cta_res[0], cta_res[1]);
  return ERROR;
}

static int32_t videoguard11_do_emm(struct s_reader *reader, EMM_PACKET * ep)
{
   return videoguard_do_emm(reader, ep, 0x48, read_tiers, vg11_do_cmd);
}

static int32_t videoguard11_card_info(struct s_reader *reader)
{
  /* info is displayed in init, or when processing info */
  rdr_log(reader, "card detected");
  rdr_log(reader, "type: %s", reader->card_desc);
  read_tiers(reader);
  return OK;
}

void reader_videoguard11(struct s_cardsystem *ph)
{
	ph->do_emm=videoguard11_do_emm;
	ph->do_ecm=videoguard11_do_ecm;
	ph->card_info=videoguard11_card_info;
	ph->card_init=videoguard11_card_init;
	ph->get_emm_type=videoguard_get_emm_type;
	ph->get_emm_filter=videoguard_get_emm_filter;
	ph->caids[0]=0x09;
	ph->desc="videoguard11";
}
#endif

