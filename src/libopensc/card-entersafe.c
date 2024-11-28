/*
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

/* Initially written by Weitao Sun (weitao@ftsafe.com) 2008 */

#if HAVE_CONFIG_H
#include "config.h"
#endif
#ifdef ENABLE_OPENSSL    /* empty file without openssl */

#include <stdlib.h>
#include <string.h>

#include <openssl/evp.h>

#include "internal.h"
#include "asn1.h"
#include "cardctl.h"
#ifndef _WIN32
#include <termios.h>
#else
#include <conio.h>
#endif

#define LENGTH_PART1_HARDCODED 110
#define LENGTH_PART1_FROM_PUBKEY 120
#define FINAL_PART1 230
#define LENGTH_PART2_HARDCODED 10
#define LENGTH_PART2_FROM_PUBKEY 136
#define FINAL_PART2 146
#define LENGTH_PRIVKEY_HARDCODED 119

extern int pkcs15_sec_operation;

static const struct sc_atr_table entersafe_atrs[] = {
    {   "3b:d5:18:ff:81:91:fe:1f:c3:80:73:c8:21:10:0a",
        NULL,
        "AmbiSecure/SecureDS",
        SC_CARD_TYPE_AMBIMAT_JCOS4_PKCS_T0_T1,
        0,
        NULL },
    {   "3b:d5:18:ff:81:91:fe:1f:c3:ae:19:04:82:00:31",
        NULL,
        "AmbiSecure/SecureDS",
        SC_CARD_TYPE_AMBIMAT_JCOS4_PKCS_T0_T1,
        0,
        NULL },
		/*
    {
        "3b:9f:95:81:31:fe:9f:00:65:46:53:05:30:06:71:df:00:00:00:80:6a:82:5e",
        "FF:FF:FF:FF:FF:FF:FF:FF:FF:FF:FF:FF:00:FF:FF:FF:FF:FF:FF:00:00:00:00",
        "FTCOS/PK-01C", SC_CARD_TYPE_ENTERSAFE_FTCOS_PK_01C, 0, NULL },
    {
        "3b:fc:18:00:00:81:31:80:45:90:67:46:4a:00:64:18:14:00:00:00:00:02",
        "ff:00:00:00:00:00:00:00:00:ff:ff:ff:ff:00:00:00:00:ff:ff:ff:ff:00",
        "EJAVA/PK-01C", SC_CARD_TYPE_ENTERSAFE_EJAVA_PK_01C, 0, NULL },
    {
        "3b:7c:18:00:00:90:67:46:4a:20:28:8c:58:00:00:00:00",
        "ff:00:00:00:00:ff:ff:ff:ff:00:00:00:00:ff:ff:ff:ff",
        "EJAVA/PK-01C-T0",SC_CARD_TYPE_ENTERSAFE_EJAVA_PK_01C_T0,0,NULL},
    {
        "3B:FC:18:00:00:81:31:80:45:90:67:46:4A:21:28:8C:58:00:00:00:00:B7",
        "ff:00:00:00:00:00:00:00:00:ff:ff:ff:ff:00:00:00:00:ff:ff:ff:ff:00",
        "EJAVA/H10CR/PK-01C-T1",SC_CARD_TYPE_ENTERSAFE_EJAVA_H10CR_PK_01C_T1,0,NULL},
    {
        "3B:FC:18:00:00:81:31:80:45:90:67:46:4A:20:25:c3:30:00:00:00:00",
        "ff:00:00:00:00:00:00:00:00:ff:ff:ff:ff:00:00:00:00:00:00:00:00",
        "EJAVA/D11CR/PK-01C-T1",SC_CARD_TYPE_ENTERSAFE_EJAVA_D11CR_PK_01C_T1,0,NULL},
    {
        "3B:FC:18:00:00:81:31:80:45:90:67:46:4A:00:6A:04:24:00:00:00:00:20",
        "ff:00:00:00:00:00:00:00:00:ff:ff:ff:ff:00:00:00:00:ff:ff:ff:ff:00",
        "EJAVA/C21C/PK-01C-T1",SC_CARD_TYPE_ENTERSAFE_EJAVA_C21C_PK_01C_T1,0,NULL},
    {
        "3B:FC:18:00:00:81:31:80:45:90:67:46:4A:00:68:08:04:00:00:00:00:0E",
        "ff:00:00:00:00:00:00:00:00:ff:ff:ff:ff:00:00:00:00:ff:ff:ff:ff:00",
        "EJAVA/A22CR/PK-01C-T1",SC_CARD_TYPE_ENTERSAFE_EJAVA_A22CR_PK_01C_T1,0,NULL},
    {
        "3B:FC:18:00:00:81:31:80:45:90:67:46:4A:10:27:61:30:00:00:00:00:0C",
        "ff:00:00:00:00:00:00:00:00:ff:ff:ff:ff:00:00:00:00:ff:ff:ff:ff:00",
        "EJAVA/A40CR/PK-01C-T1",SC_CARD_TYPE_ENTERSAFE_EJAVA_A40CR_PK_01C_T1,0,NULL},
    {
        "3b:fc:18:00:00:81:31:80:45:90:67:46:4a:00:68:08:06:00:00:00:00:0c",
        "FF:FF:FF:FF:FF:FF:FF:FF:FF:FF:FF:FF:00:FF:FF:FF:FF:FF:FF:00:00:00",
        "FTCOS/PK-01C", SC_CARD_TYPE_ENTERSAFE_FTCOS_PK_01C, 0, NULL },
        */
    { NULL, NULL, NULL, 0, 0, NULL }
};

static struct sc_card_operations entersafe_ops;
static struct sc_card_operations *iso_ops = NULL;

static struct sc_card_driver entersafe_drv = {
    "entersafe",
    "entersafe",
    &entersafe_ops,
    NULL, 0, NULL
};

static u8 trans_code_3k[] =
{
     0x01,0x02,0x03,0x04,
     0x05,0x06,0x07,0x08,
};

static u8 trans_code_ftcos_pk_01c[] =
{
     0x92,0x34,0x2E,0xEF,
     0x23,0x40,0x4F,0xD1,
};

static u8 init_key[] =
{
     1,  2,  3,  4,
     5,  6,  7,  8,
     9,  10, 11, 12,
     13, 14, 15, 16,
};

static u8 key_maintain[] =
{
     0x12, 0x34, 0x56, 0x78,
     0x21, 0x43, 0x65, 0x87,
     0x11, 0x22, 0xaa, 0xbb,
     0x33, 0x44, 0xcd, 0xef
};

static void entersafe_reverse_buffer(u8* buff,size_t size)
{
     u8 t;
     u8 * end=buff+size-1;

     while(buff<end)
     {
          t = *buff;
          *buff = *end;
          *end=t;
          ++buff;
          --end;
     }
}

static int entersafe_select_file(sc_card_t *card,
                                 const sc_path_t *in_path,
                                 sc_file_t **file_out);

/* the entersafe part */
static int entersafe_match_card(sc_card_t *card)
{
    int i;
    SC_FUNC_CALLED(card->ctx, SC_LOG_DEBUG_VERBOSE);

    i = _sc_match_atr(card, entersafe_atrs, &card->type);
    if (i < 0)
        return 0;

    return 1;
}

static int entersafe_init(sc_card_t *card)
{
    unsigned int flags;

    SC_FUNC_CALLED(card->ctx, SC_LOG_DEBUG_VERBOSE);

    card->name = "entersafe";
    card->cla  = 0x00;
    card->drv_data = NULL;

    flags =SC_ALGORITHM_ONBOARD_KEY_GEN
         | SC_ALGORITHM_RSA_RAW
         | SC_ALGORITHM_RSA_HASH_NONE;

    _sc_card_add_rsa_alg(card, 512, flags, 0);
    _sc_card_add_rsa_alg(card, 768, flags, 0);
    _sc_card_add_rsa_alg(card,1024, flags, 0);
    _sc_card_add_rsa_alg(card,2048, flags, 0);

    card->caps = SC_CARD_CAP_RNG;

    /* we need read_binary&friends with max 224 bytes per read */
    card->max_send_size = 224;
    card->max_recv_size = 224;
    SC_FUNC_RETURN(card->ctx, SC_LOG_DEBUG_VERBOSE,SC_SUCCESS);
}

static int entersafe_gen_random(sc_card_t *card,u8 *buff,size_t size)
{
     int r=SC_SUCCESS;
     u8 rbuf[SC_MAX_APDU_BUFFER_SIZE]={0};
     sc_apdu_t apdu;

    SC_FUNC_CALLED(card->ctx, SC_LOG_DEBUG_VERBOSE);

     sc_format_apdu(card,&apdu,SC_APDU_CASE_2_SHORT,0x84,0x00,0x00);
     apdu.resp=rbuf;
     apdu.le=size;
     apdu.resplen=sizeof(rbuf);

     r=sc_transmit_apdu(card,&apdu);
     LOG_TEST_RET(card->ctx, r, "entersafe gen random failed");

     if(apdu.resplen!=size)
          LOG_FUNC_RETURN(card->ctx, SC_ERROR_INTERNAL);
     memcpy(buff,rbuf,size);

     LOG_FUNC_RETURN(card->ctx, r);
}

static int entersafe_cipher_apdu(sc_card_t *card, sc_apdu_t *apdu,
                                 u8 *key, size_t keylen,
                                 u8 *buff, size_t buffsize)
{
     EVP_CIPHER_CTX * ctx = NULL;
    EVP_CIPHER *alg = NULL;

     u8 iv[8]={0};
     int len;

    SC_FUNC_CALLED(card->ctx, SC_LOG_DEBUG_VERBOSE);

     assert(card);
     assert(apdu);
     assert(key);
     assert(buff);

     /* padding as 0x80 0x00 0x00...... */
     memset(buff,0,buffsize);
     buff[0]=apdu->lc;
     memcpy(buff+1,apdu->data,apdu->lc);
     buff[apdu->lc+1]=0x80;

     ctx = EVP_CIPHER_CTX_new();
     if (ctx == NULL)
         LOG_FUNC_RETURN(card->ctx, SC_ERROR_OUT_OF_MEMORY);
     EVP_CIPHER_CTX_set_padding(ctx,0);

    if (keylen == 8) {
         alg = sc_evp_cipher(card->ctx, "DES-ECB");
        EVP_EncryptInit_ex(ctx, alg, NULL, key, iv);
    } else if (keylen == 16) {
         alg = sc_evp_cipher(card->ctx, "DES-EDE");
        EVP_EncryptInit_ex(ctx, alg, NULL, key, iv);
    } else {
        EVP_CIPHER_CTX_free(ctx);
          LOG_FUNC_RETURN(card->ctx, SC_ERROR_INTERNAL);
    }

     len = apdu->lc;
     if(!EVP_EncryptUpdate(ctx, buff, &len, buff, buffsize)){
        sc_evp_cipher_free(alg);
        EVP_CIPHER_CTX_free(ctx);
          sc_log(card->ctx,  "entersafe encryption error.");
          LOG_FUNC_RETURN(card->ctx, SC_ERROR_INTERNAL);
     }
     apdu->lc = len;

    sc_evp_cipher_free(alg);
     EVP_CIPHER_CTX_free(ctx);

     if(apdu->lc!=buffsize)
     {
          sc_log(card->ctx,  "entersafe build cipher apdu failed.");
          SC_FUNC_RETURN(card->ctx, SC_LOG_DEBUG_VERBOSE, SC_ERROR_INTERNAL);
     }

     apdu->data=buff;
     apdu->datalen=apdu->lc;

     SC_FUNC_RETURN(card->ctx, SC_LOG_DEBUG_VERBOSE, SC_SUCCESS);
}

static int entersafe_mac_apdu(sc_card_t *card, sc_apdu_t *apdu,
                              u8 * key,size_t keylen,
                              u8 * buff,size_t buffsize)
{
     int r;
     u8 iv[8];
     u8 *tmp=0,*tmp_rounded=NULL;
     size_t tmpsize=0,tmpsize_rounded=0;
     int outl=0;
     EVP_CIPHER_CTX * ctx = NULL;
    EVP_CIPHER *alg = NULL;

    SC_FUNC_CALLED(card->ctx, SC_LOG_DEBUG_VERBOSE);

     assert(card);
     assert(apdu);
     assert(key);
     assert(buff);

     if(apdu->cse != SC_APDU_CASE_3_SHORT)
          return SC_ERROR_INTERNAL;
     if(keylen!=8 && keylen!=16)
          return SC_ERROR_INTERNAL;

     r=entersafe_gen_random(card,iv,sizeof(iv));
     LOG_TEST_RET(card->ctx,r,"entersafe gen random failed");

     /* encode the APDU in the buffer */
     if ((r=sc_apdu_get_octets(card->ctx, apdu, &tmp, &tmpsize,SC_PROTO_RAW)) != SC_SUCCESS)
          goto out;

     /* round to 8 */
     tmpsize_rounded=(tmpsize/8+1)*8;

     tmp_rounded = malloc(tmpsize_rounded);
     if (tmp_rounded == NULL)
     {
          r =  SC_ERROR_OUT_OF_MEMORY;
          goto out;
     }

     /*build content and padded buffer by 0x80 0x00 0x00..... */
     memset(tmp_rounded,0,tmpsize_rounded);
     memcpy(tmp_rounded,tmp,tmpsize);
     tmp_rounded[4]+=4;
     tmp_rounded[tmpsize]=0x80;

     /* block_size-1 blocks*/
     ctx = EVP_CIPHER_CTX_new();
     if (ctx == NULL) {
        r =  SC_ERROR_OUT_OF_MEMORY;
        goto out;
     }
     EVP_CIPHER_CTX_set_padding(ctx,0);
    alg = sc_evp_cipher(card->ctx, "DES-CBC");
    EVP_EncryptInit_ex(ctx, alg, NULL, key, iv);

     if(tmpsize_rounded>8){
          if(!EVP_EncryptUpdate(ctx,tmp_rounded,&outl,tmp_rounded,tmpsize_rounded-8)){
               r = SC_ERROR_INTERNAL;
               goto out;
          }
     }
     /* last block */
     if(keylen==8)
     {
          if(!EVP_EncryptUpdate(ctx,tmp_rounded+outl,&outl,tmp_rounded+outl,8)){
               r = SC_ERROR_INTERNAL;
               goto out;
          }
     }
     else
     {
          EVP_EncryptInit_ex(ctx, EVP_des_ede_cbc(), NULL, key,tmp_rounded+outl-8);
          if(!EVP_EncryptUpdate(ctx,tmp_rounded+outl,&outl,tmp_rounded+outl,8)){
               r = SC_ERROR_INTERNAL;
               goto out;
          }
     }

     memcpy(buff,apdu->data,apdu->lc);
     /* use first 4 bytes of last block as mac value*/
     memcpy(buff+apdu->lc,tmp_rounded+tmpsize_rounded-8,4);
     apdu->data=buff;
     apdu->lc+=4;
     apdu->datalen=apdu->lc;

out:
     if(tmp)
          free(tmp);
     if(tmp_rounded)
          free(tmp_rounded);
     if (ctx) {
        sc_evp_cipher_free(alg);
        EVP_CIPHER_CTX_free(ctx);
    }

     SC_FUNC_RETURN(card->ctx, SC_LOG_DEBUG_VERBOSE, r);
}

static int entersafe_transmit_apdu(sc_card_t *card, sc_apdu_t *apdu,
                                   u8 * key, size_t keylen,
                                   int cipher,int mac)
{
     u8 *cipher_data=0,*mac_data=0;
     size_t cipher_data_size,mac_data_size;
     int blocks;
     int r=SC_SUCCESS;
     u8 *sbuf=NULL;
     size_t ssize=0;

     SC_FUNC_CALLED(card->ctx, SC_LOG_DEBUG_VERBOSE);

     assert(card);
     assert(apdu);

     if((cipher||mac) && (!key||(keylen!=8 && keylen!=16)))
          SC_FUNC_RETURN(card->ctx, SC_LOG_DEBUG_VERBOSE, SC_ERROR_INVALID_ARGUMENTS);

     r = sc_apdu_get_octets(card->ctx, apdu, &sbuf, &ssize, SC_PROTO_RAW);
     if (r == SC_SUCCESS)
          sc_apdu_log(card->ctx, sbuf, ssize, 1);
     if(sbuf)
          free(sbuf);

     if(cipher)
     {
          blocks=(apdu->lc+2)/8+1;
          cipher_data_size=blocks*8;
          cipher_data=malloc(cipher_data_size);
          if(!cipher_data)
          {
               r = SC_ERROR_OUT_OF_MEMORY;
               goto out;
          }

          if((r = entersafe_cipher_apdu(card,apdu,key,keylen,cipher_data,cipher_data_size))<0)
               goto out;
     }
     if(mac)
     {
          mac_data_size=apdu->lc+4;
          mac_data=malloc(mac_data_size);
          if(!mac_data)
          {
               r = SC_ERROR_OUT_OF_MEMORY;
               goto out;
          }
          r = entersafe_mac_apdu(card,apdu,key,keylen,mac_data,mac_data_size);
          if(r < 0)
               goto out;
     }

     r = sc_transmit_apdu(card,apdu);

out:
     if(cipher_data)
          free(cipher_data);
     if(mac_data)
          free(mac_data);

     SC_FUNC_RETURN(card->ctx, SC_LOG_DEBUG_VERBOSE, r);
}

static int entersafe_read_binary(sc_card_t *card,
                                 unsigned int idx, u8 *buf, size_t count,
                                 unsigned long *flags)
{
    sc_apdu_t apdu;
    u8 recvbuf[SC_MAX_APDU_BUFFER_SIZE];
    int r;

    SC_FUNC_CALLED(card->ctx, SC_LOG_DEBUG_VERBOSE);

    assert(count <= card->max_recv_size);
    sc_format_apdu(card, &apdu, SC_APDU_CASE_2_SHORT, 0xB0,
               (idx >> 8) & 0xFF, idx & 0xFF);

    apdu.cla=idx > 0x7fff ? 0x80:0x00;
    apdu.le = count;
    apdu.resplen = count;
    apdu.resp = recvbuf;

    r = entersafe_transmit_apdu(card, &apdu,0,0,0,0);
    LOG_TEST_RET(card->ctx, r, "APDU transmit failed");
    if (apdu.resplen == 0)
        SC_FUNC_RETURN(card->ctx, SC_LOG_DEBUG_VERBOSE, sc_check_sw(card, apdu.sw1, apdu.sw2));
    memcpy(buf, recvbuf, apdu.resplen);

    SC_FUNC_RETURN(card->ctx, SC_LOG_DEBUG_VERBOSE, apdu.resplen);
}

static int entersafe_update_binary(sc_card_t *card,
                                   unsigned int idx, const u8 *buf,
                                   size_t count, unsigned long flags)
{
    sc_apdu_t apdu;
    int r;

    SC_FUNC_CALLED(card->ctx, SC_LOG_DEBUG_VERBOSE);

    assert(count <= card->max_send_size);

    sc_format_apdu(card, &apdu, SC_APDU_CASE_3_SHORT, 0xD6,
               (idx >> 8) & 0xFF, idx & 0xFF);
    apdu.cla=idx > 0x7fff ? 0x80:0x00;
    apdu.lc = count;
    apdu.datalen = count;
    apdu.data = buf;

    r = entersafe_transmit_apdu(card, &apdu,0,0,0,0);
    LOG_TEST_RET(card->ctx, r, "APDU transmit failed");
    LOG_TEST_RET(card->ctx, sc_check_sw(card, apdu.sw1, apdu.sw2),
            "Card returned error");
    SC_FUNC_RETURN(card->ctx, SC_LOG_DEBUG_VERBOSE, count);
}


static int entersafe_process_fci(struct sc_card *card, struct sc_file *file,
                          const u8 *buf, size_t buflen)
{
     int r;

     assert(file);
     SC_FUNC_CALLED(card->ctx, SC_LOG_DEBUG_VERBOSE);

     r = iso_ops->process_fci(card,file,buf,buflen);
     LOG_TEST_RET(card->ctx, r, "Process fci failed");

     if(file->namelen)
     {
          file->type = SC_FILE_TYPE_DF;
          file->ef_structure = SC_FILE_EF_UNKNOWN;
     }
     else
     {
          file->type = SC_FILE_TYPE_WORKING_EF;
          file->ef_structure = SC_FILE_EF_TRANSPARENT;
     }

     SC_FUNC_RETURN(card->ctx, SC_LOG_DEBUG_VERBOSE, r);
}

static int entersafe_select_fid(sc_card_t *card,
                                unsigned int id_hi, unsigned int id_lo,
                                sc_file_t **file_out)
{
    int r;
    sc_file_t *file = NULL;
    sc_path_t path;

    SC_FUNC_CALLED(card->ctx, SC_LOG_DEBUG_VERBOSE);
    memset(&path, 0, sizeof(sc_path_t));

    path.type=SC_PATH_TYPE_FILE_ID;
    path.value[0]=id_hi;
    path.value[1]=id_lo;
    path.len=2;

    r = iso_ops->select_file(card,&path,&file);
    if (r < 0)
        sc_file_free(file);
    LOG_TEST_RET(card->ctx, r, "APDU transmit failed");

    /* update cache */
    if (file->type == SC_FILE_TYPE_DF) {
         card->cache.current_path.type = SC_PATH_TYPE_PATH;
         card->cache.current_path.value[0] = 0x3f;
         card->cache.current_path.value[1] = 0x00;
         if (id_hi == 0x3f && id_lo == 0x00){
              card->cache.current_path.len = 2;
         } else {
              card->cache.current_path.len = 4;
              card->cache.current_path.value[2] = id_hi;
              card->cache.current_path.value[3] = id_lo;
         }
    }

    if (file_out)
        *file_out = file;
    else
        sc_file_free(file);

    SC_FUNC_RETURN(card->ctx, SC_LOG_DEBUG_VERBOSE, SC_SUCCESS);
}

static int entersafe_select_aid(sc_card_t *card,
                                const sc_path_t *in_path,
                                sc_file_t **file_out)
{
    int r = 0;
    SC_FUNC_CALLED(card->ctx, SC_LOG_DEBUG_VERBOSE);
    if (card->cache.valid
        && card->cache.current_path.type == SC_PATH_TYPE_DF_NAME
        && card->cache.current_path.len == in_path->len
        && memcmp(card->cache.current_path.value, in_path->value, in_path->len)==0 )
    {
         if(file_out)
         {
              *file_out = sc_file_new();
              if(!file_out)
                   LOG_FUNC_RETURN(card->ctx, SC_ERROR_OUT_OF_MEMORY);
         }
    }
    else
    {
         r = iso_ops->select_file(card,in_path,file_out);
         LOG_TEST_RET(card->ctx, r, "APDU transmit failed");

         /* update cache */
         card->cache.current_path.type = SC_PATH_TYPE_DF_NAME;
         card->cache.current_path.len = in_path->len;
         memcpy(card->cache.current_path.value,in_path->value,in_path->len);
    }
    if (file_out) {
         sc_file_t *file = *file_out;
         assert(file);

         file->type = SC_FILE_TYPE_DF;
         file->ef_structure = SC_FILE_EF_UNKNOWN;
         file->path.len = 0;
         file->size = 0;
         /* AID */
         memcpy(file->name,in_path->value,in_path->len);
         file->namelen = in_path->len;
         file->id = 0x0000;
    }
    SC_FUNC_RETURN(card->ctx, SC_LOG_DEBUG_VERBOSE, r);
}

static int entersafe_select_path(sc_card_t *card,
                                const u8 pathbuf[16], const size_t len,
                                sc_file_t **file_out)
{
     u8 n_pathbuf[SC_MAX_PATH_SIZE];
     const u8 *path=pathbuf;
     size_t pathlen=len;
     int bMatch = -1;
     unsigned int i;
     int r;

     SC_FUNC_CALLED(card->ctx, SC_LOG_DEBUG_VERBOSE); //added by Neel

     if (pathlen%2 != 0 || pathlen > 6 || pathlen <= 0)
          SC_FUNC_RETURN(card->ctx, SC_LOG_DEBUG_VERBOSE, SC_ERROR_INVALID_ARGUMENTS);

     /* if pathlen == 6 then the first FID must be MF (== 3F00) */
     if (pathlen == 6 && ( path[0] != 0x3f || path[1] != 0x00 ))
          SC_FUNC_RETURN(card->ctx, SC_LOG_DEBUG_VERBOSE, SC_ERROR_INVALID_ARGUMENTS);

     /* unify path (the first FID should be MF) */
     if (path[0] != 0x3f || path[1] != 0x00)
     {
          n_pathbuf[0] = 0x3f;
          n_pathbuf[1] = 0x00;
          memcpy(n_pathbuf+2, path, pathlen);
          path = n_pathbuf;
          pathlen += 2;
     }

     /* check current working directory */
     if (card->cache.valid
         && card->cache.current_path.type == SC_PATH_TYPE_PATH
         && card->cache.current_path.len >= 2
         && card->cache.current_path.len <= pathlen )
     {
          bMatch = 0;
          for (i=0; i < card->cache.current_path.len; i+=2)
               if (card->cache.current_path.value[i] == path[i]
                   && card->cache.current_path.value[i+1] == path[i+1] )
                    bMatch += 2;
     }

     if ( card->cache.valid && bMatch > 2 )
     {
          if ( pathlen - bMatch == 2 )
          {
               /* we are in the right directory */
               return entersafe_select_fid(card, path[bMatch], path[bMatch+1], file_out);
          }
          else if ( pathlen - bMatch > 2 )
          {
               /* two more steps to go */
               sc_path_t new_path;

               /* first step: change directory */
               r = entersafe_select_fid(card, path[bMatch], path[bMatch+1], NULL);
               LOG_TEST_RET(card->ctx, r, "SELECT FILE (DF-ID) failed");

                  memset(&new_path, 0, sizeof(sc_path_t));

               new_path.type = SC_PATH_TYPE_PATH;
               new_path.len  = pathlen - bMatch-2;
               memcpy(new_path.value, &(path[bMatch+2]), new_path.len);
               /* final step: select file */
               return entersafe_select_file(card, &new_path, file_out);
          }
          else /* if (bMatch - pathlen == 0) */
          {
               /* done: we are already in the
                * requested directory */
               sc_log(card->ctx,
                "cache hit\n");
               /* copy file info (if necessary) */
               if (file_out) {
                    sc_file_t *file = sc_file_new();
                    if (!file)
                         LOG_FUNC_RETURN(card->ctx, SC_ERROR_OUT_OF_MEMORY);
                    file->id = (path[pathlen-2] << 8) +
                         path[pathlen-1];
                    file->path = card->cache.current_path;
                    file->type = SC_FILE_TYPE_DF;
                    file->ef_structure = SC_FILE_EF_UNKNOWN;
                    file->size = 0;
                    file->namelen = 0;
                    file->magic = SC_FILE_MAGIC;
                    *file_out = file;
               }
               /* nothing left to do */
               return SC_SUCCESS;
          }
     }
     else
     {
          /* no usable cache */
          for ( i=0; i<pathlen-2; i+=2 )
          {
               r = entersafe_select_fid(card, path[i], path[i+1], NULL);
               LOG_TEST_RET(card->ctx, r, "SELECT FILE (DF-ID) failed");
          }
          return entersafe_select_fid(card, path[pathlen-2], path[pathlen-1], file_out);
     }
}

static int entersafe_select_file(sc_card_t *card,
                                 const sc_path_t *in_path,
                                 sc_file_t **file_out)
{
     int r;
     char pbuf[SC_MAX_PATH_STRING_SIZE];
     assert(card);
     assert(in_path);
     SC_FUNC_CALLED(card->ctx, SC_LOG_DEBUG_VERBOSE);


      r = sc_path_print(pbuf, sizeof(pbuf), &card->cache.current_path);
      if (r != SC_SUCCESS)
         pbuf[0] = '\0';

      sc_log(card->ctx,
           "current path (%s, %s): %s (len: %"SC_FORMAT_LEN_SIZE_T"u)\n",
           card->cache.current_path.type == SC_PATH_TYPE_DF_NAME ?
           "aid" : "path",
           card->cache.valid ? "valid" : "invalid", pbuf,
           card->cache.current_path.len);

     switch(in_path->type)
     {
     case SC_PATH_TYPE_FILE_ID:
          if (in_path->len != 2)
               SC_FUNC_RETURN(card->ctx, SC_LOG_DEBUG_VERBOSE,SC_ERROR_INVALID_ARGUMENTS);
          return entersafe_select_fid(card,in_path->value[0],in_path->value[1], file_out);
     case SC_PATH_TYPE_DF_NAME:
          return entersafe_select_aid(card,in_path,file_out);
     case SC_PATH_TYPE_PATH:
          return entersafe_select_path(card,in_path->value,in_path->len,file_out);
     default:
          SC_FUNC_RETURN(card->ctx, SC_LOG_DEBUG_VERBOSE, SC_ERROR_INVALID_ARGUMENTS);
     }
}

static int entersafe_create_mf(sc_card_t *card, sc_entersafe_create_data * data)
{
    int r;
    sc_apdu_t apdu;

    SC_FUNC_CALLED(card->ctx, SC_LOG_DEBUG_VERBOSE);

    memcpy(data->data.df.init_key, init_key, sizeof(init_key));

    sc_format_apdu(card,&apdu,SC_APDU_CASE_3_SHORT,0xE0,0x00,0x00);
    apdu.cla=0x84;
    apdu.data=(u8*)&data->data.df;
    apdu.datalen=apdu.lc=sizeof(data->data.df);

    switch(card->type)
    {
    case SC_CARD_TYPE_ENTERSAFE_3K:
    {
         r = entersafe_transmit_apdu(card, &apdu,trans_code_3k,sizeof(trans_code_3k),0,1);
    }break;
    case SC_CARD_TYPE_ENTERSAFE_FTCOS_PK_01C:
    case SC_CARD_TYPE_ENTERSAFE_EJAVA_PK_01C:
    case SC_CARD_TYPE_ENTERSAFE_EJAVA_PK_01C_T0:
    case SC_CARD_TYPE_ENTERSAFE_EJAVA_H10CR_PK_01C_T1:
    case SC_CARD_TYPE_ENTERSAFE_EJAVA_D11CR_PK_01C_T1:
    case SC_CARD_TYPE_ENTERSAFE_EJAVA_C21C_PK_01C_T1:
    case SC_CARD_TYPE_ENTERSAFE_EJAVA_A22CR_PK_01C_T1:
    case SC_CARD_TYPE_ENTERSAFE_EJAVA_A40CR_PK_01C_T1:
    {
         r = entersafe_transmit_apdu(card, &apdu,trans_code_ftcos_pk_01c,sizeof(trans_code_ftcos_pk_01c),0,1);
    }break;
    default:
    {
         r = SC_ERROR_INTERNAL;
    }break;
    }

    LOG_TEST_RET(card->ctx, r, "APDU transmit failed");
    return sc_check_sw(card, apdu.sw1, apdu.sw2);
}
static int entersafe_create_df(sc_card_t *card, sc_entersafe_create_data * data)
{
    int r;
    sc_apdu_t apdu;

    SC_FUNC_CALLED(card->ctx, SC_LOG_DEBUG_VERBOSE);

    memcpy(data->data.df.init_key, init_key, sizeof(init_key));

    sc_format_apdu(card,&apdu,SC_APDU_CASE_3_SHORT,0xE0,0x01,0x00);
    apdu.cla=0x84;
    apdu.data=(u8*)&data->data.df;
    apdu.lc=apdu.datalen=sizeof(data->data.df);

    r = entersafe_transmit_apdu(card, &apdu,init_key,sizeof(init_key),0,1);
    LOG_TEST_RET(card->ctx, r, "APDU transmit failed");
    return sc_check_sw(card, apdu.sw1, apdu.sw2);
}

static int entersafe_create_ef(sc_card_t *card, sc_entersafe_create_data * data)
{
    int r;
    sc_apdu_t apdu;

    SC_FUNC_CALLED(card->ctx, SC_LOG_DEBUG_VERBOSE);

    sc_format_apdu(card, &apdu, SC_APDU_CASE_3_SHORT, 0xE0, 0x02, 0x00);
    apdu.cla = 0x84;
    apdu.data = (u8*)&data->data.ef;
    apdu.lc = apdu.datalen = sizeof(data->data.ef);

    r = entersafe_transmit_apdu(card, &apdu,init_key,sizeof(init_key),0,1);
    LOG_TEST_RET(card->ctx, r, "APDU transmit failed");
    return sc_check_sw(card, apdu.sw1, apdu.sw2);
}

static u8 process_acl_entry(sc_file_t *in, unsigned int method, unsigned int in_def)
{
    u8 def = (u8)in_def;
    const sc_acl_entry_t *entry = sc_file_get_acl_entry(in, method);
    if (!entry)
    {
        return def;
    }
    else if (entry->method & SC_AC_CHV)
    {
        unsigned int key_ref = entry->key_ref;
        if (key_ref == SC_AC_KEY_REF_NONE)
            return def;
        else
            return ENTERSAFE_AC_ALWAYS&0x04;
    }
    else if (entry->method & SC_AC_NEVER)
    {
        return ENTERSAFE_AC_NEVER;
    }
    else
    {
        return def;
    }
}

static int entersafe_create_file(sc_card_t *card, sc_file_t *file)
{
     SC_FUNC_CALLED(card->ctx, SC_LOG_DEBUG_VERBOSE);

     if (file->type == SC_FILE_TYPE_WORKING_EF) {
          sc_entersafe_create_data data;
          memset(&data,0,sizeof(data));

          data.data.ef.file_id[0] = (file->id>>8)&0xFF;
          data.data.ef.file_id[1] = file->id&0xFF;
          data.data.ef.size[0] = (file->size>>8)&0xFF;
          data.data.ef.size[1] = file->size&0xFF;
          memset(data.data.ef.ac,ENTERSAFE_AC_ALWAYS,sizeof(data.data.ef.ac));
          data.data.ef.ac[0] = process_acl_entry(file,SC_AC_OP_READ,ENTERSAFE_AC_ALWAYS);
          data.data.ef.ac[1] = process_acl_entry(file,SC_AC_OP_UPDATE,ENTERSAFE_AC_ALWAYS);

          return entersafe_create_ef(card, &data);
     } else
          return SC_ERROR_INVALID_ARGUMENTS;
}

/*static int entersafe_internal_set_security_env(sc_card_t *card,
                                               const sc_security_env_t *env,
                                               u8 ** data,size_t* size)
{
    sc_apdu_t apdu;
    u8 sbuf[SC_MAX_APDU_BUFFER_SIZE];
    u8 *p=sbuf;
    int r;

    SC_FUNC_CALLED(card->ctx, SC_LOG_DEBUG_VERBOSE);

    assert(card != NULL && env != NULL);

    switch (env->operation) {
    case SC_SEC_OPERATION_DECIPHER:
    case SC_SEC_OPERATION_SIGN:
         sc_format_apdu(card, &apdu, SC_APDU_CASE_3_SHORT, 0x22, 0, 0);
         apdu.p1 = 0x41;
         apdu.p2 = 0xB8;
         *p++ = 0x80;
         *p++ = 0x01;
         *p++ = 0x80;
         *p++ = 0x83;
         *p++ = 0x02;
         *p++ = env->key_ref[0];
         *p++ = 0x22;
         if(*size>1024/8)
         {
              if(*size == 2048/8)
              {
                   *p++ = 0x89;
                   *p++ = 0x40;
                   memcpy(p,*data,0x40);
                   p+=0x40;
                   *data+=0x40;
                   *size-=0x40;
              }
              else
              {
                   SC_FUNC_RETURN(card->ctx, SC_LOG_DEBUG_VERBOSE, SC_ERROR_INVALID_ARGUMENTS);
              }
         }
         break;
    default:
         SC_FUNC_RETURN(card->ctx, SC_LOG_DEBUG_VERBOSE, SC_ERROR_INVALID_ARGUMENTS);
    }

    apdu.le = 0;
    apdu.lc = apdu.datalen = p - sbuf;
    apdu.data = sbuf;
    apdu.resplen = 0;

    r = sc_transmit_apdu(card, &apdu);
    LOG_TEST_RET(card->ctx, r, "APDU transmit failed");
    return sc_check_sw(card, apdu.sw1, apdu.sw2);
}*/

/**
 * We don't really set the security environment,but cache it.It will be set when
 * security operation is performed later.Because we may transport partial of
 * the sign/decipher data within the security environment apdu.
 */
static int entersafe_set_security_env(sc_card_t *card,
                                      const sc_security_env_t *env,
                                      int se_num)
{
     assert(card);
     assert(env);

     SC_FUNC_CALLED(card->ctx, SC_LOG_DEBUG_VERBOSE);

     if(card->drv_data){
          free(card->drv_data);
          card->drv_data=0;
     }

     card->drv_data = calloc(1,sizeof(*env));
     if(!card->drv_data)
          SC_FUNC_RETURN(card->ctx, SC_LOG_DEBUG_VERBOSE, SC_ERROR_OUT_OF_MEMORY);

     memcpy(card->drv_data,env,sizeof(*env));
     SC_FUNC_RETURN(card->ctx, SC_LOG_DEBUG_VERBOSE, SC_SUCCESS);
}

static int entersafe_restore_security_env(sc_card_t *card, int se_num)
{
     SC_FUNC_CALLED(card->ctx, SC_LOG_DEBUG_VERBOSE);
     return SC_SUCCESS;
}


static int entersafe_compute_with_prkey(sc_card_t *card,
                                        const u8 * data, size_t datalen,
                                        u8 * out, size_t outlen)
{
    int r;
    sc_apdu_t apdu;
    u8 sbuf[256]={0x00};
    u8 rbuf[SC_MAX_APDU_BUFFER_SIZE];
    u8* p=sbuf;

    SC_FUNC_CALLED(card->ctx, SC_LOG_DEBUG_VERBOSE);

    if(!data)
         SC_FUNC_RETURN(card->ctx, SC_LOG_DEBUG_VERBOSE,SC_ERROR_INVALID_ARGUMENTS);

    memcpy(p,data,256);

    /* Data length is set to a maximum of 255 bytes by Neel and Anurag
     * TODO: Check Automatic length of input data and set it to apdu.lc and apdu.datalen
     */
    switch(pkcs15_sec_operation)
        {
            case SC_SEC_OPERATION_SIGN:
                sc_format_apdu(card, &apdu, SC_APDU_CASE_4_SHORT, 0x2A, 0x9E, 0x9A); //8086 for decryption and 9E9A for signing
                apdu.data=data;
                apdu.lc = datalen;
                apdu.datalen = datalen;
                apdu.resp = rbuf;
                apdu.resplen = sizeof(rbuf);
                apdu.le = 256;
                break;
            case SC_SEC_OPERATION_DECIPHER:
                sc_format_apdu(card, &apdu, SC_APDU_CASE_4_EXT, 0x2A, 0x80, 0x86); //8086 for decryption and 9E9A for signing
                apdu.data=p;
                apdu.lc = 256;
                apdu.datalen = 256;
                apdu.resp = rbuf;
                apdu.resplen = sizeof(rbuf);
                apdu.le = 256;
                break;
        }

    r = entersafe_transmit_apdu(card, &apdu,0,0,0,0);
    LOG_TEST_RET(card->ctx, r, "APDU transmit failed");

    if (apdu.sw1 == 0x90 && apdu.sw2 == 0x00) {
        size_t len = apdu.resplen > outlen ? outlen : apdu.resplen;
        memcpy(out, apdu.resp, len);
        SC_FUNC_RETURN(card->ctx, SC_LOG_DEBUG_VERBOSE, len);
    }
    SC_FUNC_RETURN(card->ctx, SC_LOG_DEBUG_VERBOSE, sc_check_sw(card, apdu.sw1, apdu.sw2));
}


static int entersafe_compute_signature(sc_card_t *card,
                                       const u8 * data, size_t datalen,
                                       u8 * out, size_t outlen)
{
     SC_FUNC_CALLED(card->ctx, SC_LOG_DEBUG_VERBOSE);
     return entersafe_compute_with_prkey(card,data,datalen,out,outlen);
}

static int entersafe_decipher(sc_card_t *card,
                              const u8 * crgram, size_t crgram_len,
                              u8 * out, size_t outlen)
{
     SC_FUNC_CALLED(card->ctx, SC_LOG_DEBUG_VERBOSE);
     return entersafe_compute_with_prkey(card,crgram,crgram_len,out,outlen);
}

static void entersafe_init_pin_info(struct sc_pin_cmd_pin *pin, unsigned int num)
{
    pin->encoding   = SC_PIN_ENCODING_ASCII;
    pin->min_length = 4;
    pin->max_length = 16;
    pin->pad_length = 16;
    pin->offset     = 5 + num * 16;
    pin->pad_char   = 0x00;
}

static int entersafe_pin_cmd(sc_card_t *card, struct sc_pin_cmd_data *data,
               int *tries_left)
{
     int r;
     SC_FUNC_CALLED(card->ctx, SC_LOG_DEBUG_VERBOSE);
     entersafe_init_pin_info(&data->pin1,0);
     entersafe_init_pin_info(&data->pin2,1);
     //data->flags |= SC_PIN_CMD_NEED_PADDING;

     if(data->cmd!=SC_PIN_CMD_UNBLOCK)
     {
          r = iso_ops->pin_cmd(card,data,tries_left);
          sc_log(card->ctx,  "Verify rv:%i", r);
     }
     else
     {
          {/*verify*/
               sc_apdu_t apdu;
               u8 sbuf[0x10]={0};

               memcpy(sbuf,data->pin1.data,data->pin1.len);
               sc_format_apdu(card, &apdu, SC_APDU_CASE_3_SHORT,0x20,0x00,data->pin_reference+1);
               apdu.lc = apdu.datalen = sizeof(sbuf);
               apdu.data = sbuf;

               r = entersafe_transmit_apdu(card, &apdu,0,0,0,0);
               LOG_TEST_RET(card->ctx, r, "APDU transmit failed");
          }

          {/*change*/
               sc_apdu_t apdu;
               u8 sbuf[0x12]={0};

               sbuf[0] = 0x33;
               sbuf[1] = 0x00;
               memcpy(sbuf+2,data->pin2.data,data->pin2.len);
               sc_format_apdu(card, &apdu, SC_APDU_CASE_3_SHORT,0xF4,0x0B,data->pin_reference);
               apdu.cla = 0x84;
               apdu.lc = apdu.datalen = sizeof(sbuf);
               apdu.data = sbuf;

               r = entersafe_transmit_apdu(card, &apdu,key_maintain,sizeof(key_maintain),1,1);
               LOG_TEST_RET(card->ctx, r, "APDU transmit failed");
          }
     }
     SC_FUNC_RETURN(card->ctx, SC_LOG_DEBUG_VERBOSE, r);
}

static int entersafe_erase_card(sc_card_t *card)
{
    int r;
    u8  sbuf[2];
    sc_apdu_t apdu;

    SC_FUNC_CALLED(card->ctx, SC_LOG_DEBUG_VERBOSE);

    sbuf[0] = 0x3f;
    sbuf[1] = 0x00;
    sc_format_apdu(card, &apdu, SC_APDU_CASE_3_SHORT, 0xA4, 0x00, 0x00);
    apdu.lc   = 2;
    apdu.datalen = 2;
    apdu.data = sbuf;

    r = entersafe_transmit_apdu(card, &apdu,0,0,0,0);
    LOG_TEST_RET(card->ctx, r, "APDU transmit failed");
    sc_invalidate_cache(card);

    sc_format_apdu(card, &apdu, SC_APDU_CASE_3_SHORT, 0xEE, 0x00, 0x00);
    apdu.cla=0x84;
    apdu.lc=2;
    apdu.datalen=2;
    apdu.data=sbuf;

    switch(card->type)
    {
    case SC_CARD_TYPE_ENTERSAFE_3K:
    {
         r = entersafe_transmit_apdu(card, &apdu,trans_code_3k,sizeof(trans_code_3k),0,1);
    }break;
    case SC_CARD_TYPE_ENTERSAFE_FTCOS_PK_01C:
    case SC_CARD_TYPE_ENTERSAFE_EJAVA_PK_01C:
    case SC_CARD_TYPE_ENTERSAFE_EJAVA_PK_01C_T0:
    case SC_CARD_TYPE_ENTERSAFE_EJAVA_H10CR_PK_01C_T1:
    case SC_CARD_TYPE_ENTERSAFE_EJAVA_D11CR_PK_01C_T1:
    case SC_CARD_TYPE_ENTERSAFE_EJAVA_C21C_PK_01C_T1:
    case SC_CARD_TYPE_ENTERSAFE_EJAVA_A22CR_PK_01C_T1:
    case SC_CARD_TYPE_ENTERSAFE_EJAVA_A40CR_PK_01C_T1:
    {
         r = entersafe_transmit_apdu(card, &apdu,trans_code_ftcos_pk_01c,sizeof(trans_code_ftcos_pk_01c),0,1);
    }break;
    default:
    {
         r = SC_ERROR_INTERNAL;
    }break;
    }

    LOG_TEST_RET(card->ctx, r, "APDU transmit failed");
    SC_FUNC_RETURN(card->ctx, SC_LOG_DEBUG_VERBOSE, sc_check_sw(card, apdu.sw1, apdu.sw2));
}

static void entersafe_encode_bignum(u8 tag,sc_pkcs15_bignum_t bignum,u8** ptr)
{
     u8 *p=*ptr;

     *p++=tag;
     if(bignum.len<128)
     {
          *p++=(u8)bignum.len;
     }
     else
     {
          u8 bytes=1;
          size_t len=bignum.len;
          while(len)
          {
               len=len>>8;
               ++bytes;
          }
          bytes&=0x0F;
          *p++=0x80|bytes;
          while(bytes)
          {
               *p++=bignum.len>>((bytes-1)*8);
               --bytes;
          }
     }
     memcpy(p,bignum.data,bignum.len);
     entersafe_reverse_buffer(p,bignum.len);
     p+=bignum.len;
     *ptr = p;
}

static int entersafe_write_small_rsa_key(sc_card_t *card,u8 key_id,struct sc_pkcs15_prkey_rsa *rsa)
{
     sc_apdu_t apdu;
     u8 sbuff[SC_MAX_APDU_BUFFER_SIZE];
     int r;
     u8 *p=sbuff;

    SC_FUNC_CALLED(card->ctx, SC_LOG_DEBUG_VERBOSE);

     {/* write prkey */
          *p++=0x00;            /* EC */
          *p++=0x00;            /* ver */
          entersafe_encode_bignum('E',rsa->exponent,&p);
          entersafe_encode_bignum('D',rsa->d,&p);

          sc_format_apdu(card,&apdu,SC_APDU_CASE_3_SHORT,0xF4,0x22,key_id);
          apdu.cla=0x84;
          apdu.data=sbuff;
          apdu.lc=apdu.datalen=p-sbuff;

          r=entersafe_transmit_apdu(card,&apdu,key_maintain,sizeof(key_maintain),1,1);
          LOG_TEST_RET(card->ctx, r, "APDU transmit failed");
          LOG_TEST_RET(card->ctx, sc_check_sw(card, apdu.sw1, apdu.sw2),"Write prkey failed");
     }

     p=sbuff;
     {/* write pukey */
          *p++=0x00;            /* EC */
          *p++=0x00;            /* ver */
          entersafe_encode_bignum('E',rsa->exponent,&p);
          entersafe_encode_bignum('N',rsa->modulus,&p);

          sc_format_apdu(card,&apdu,SC_APDU_CASE_3_SHORT,0xF4,0x2A,key_id);
          apdu.cla=0x84;
          apdu.data=sbuff;
          apdu.lc=apdu.datalen=p-sbuff;

          r=entersafe_transmit_apdu(card,&apdu,key_maintain,sizeof(key_maintain),1,1);
          LOG_TEST_RET(card->ctx, r, "APDU transmit failed");
          LOG_TEST_RET(card->ctx, sc_check_sw(card, apdu.sw1, apdu.sw2),"Write pukey failed");
     }

     SC_FUNC_RETURN(card->ctx, SC_LOG_DEBUG_VERBOSE,SC_SUCCESS);
}

static int entersafe_write_rsa_key_factor(sc_card_t *card,
                                          u8 key_id,u8 usage,
                                          u8 factor,
                                          sc_pkcs15_bignum_t data)
{
    int r;
    sc_apdu_t apdu;

    SC_FUNC_CALLED(card->ctx, SC_LOG_DEBUG_VERBOSE);

    {/* MSE */
         u8 sbuff[4];
         sbuff[0]=0x84;
         sbuff[1]=0x02;
         sbuff[2]=key_id;
         sbuff[3]=usage;

         sc_format_apdu(card,&apdu,SC_APDU_CASE_3_SHORT,0x22,0x01,0xB8);
         apdu.data=sbuff;
         apdu.lc=apdu.datalen=4;

         r=entersafe_transmit_apdu(card,&apdu,0,0,0,0);
         LOG_TEST_RET(card->ctx, r, "APDU transmit failed");
         LOG_TEST_RET(card->ctx, sc_check_sw(card, apdu.sw1, apdu.sw2),"Write prkey factor failed(MSE)");
    }

    {/* Write 'x'; */
        u8 sbuff[SC_MAX_APDU_BUFFER_SIZE];

         sc_format_apdu(card,&apdu,SC_APDU_CASE_3_SHORT,0x46,factor,0x00);

         memcpy(sbuff,data.data,data.len);
         entersafe_reverse_buffer(sbuff,data.len);
/*
 *  PK01C and PK13C smart card only support 1024 or 2048bit key .
 *  Size of exponent1 exponent2 coefficient of RSA private key keep the same as size of prime1
 *  So check factor is padded with zero or not
 */
         switch(factor){
             case 0x3:
             case 0x4:
             case 0x5:
                 {
                     if( data.len > 32 && data.len < 64 )
                     {
                         for(r = data.len ; r < 64 ; r ++)
                             sbuff[r] = 0;
                         data.len = 64;
                     }
                     else if( data.len > 64 && data.len < 128 )
                     {
                         for(r = data.len ; r < 128 ; r ++)
                             sbuff[r] = 0;
                         data.len = 128;
                     }
                 }
                 break;
             default:
                 break;
         }

         apdu.data=sbuff;
         apdu.lc=apdu.datalen=data.len;

         r = entersafe_transmit_apdu(card,&apdu,0,0,0,0);
         LOG_TEST_RET(card->ctx, r, "APDU transmit failed");
         LOG_TEST_RET(card->ctx, sc_check_sw(card, apdu.sw1, apdu.sw2),"Write prkey factor failed");
    }
    SC_FUNC_RETURN(card->ctx, SC_LOG_DEBUG_VERBOSE,SC_SUCCESS);
}

static int entersafe_write_large_rsa_key(sc_card_t *card,u8 key_id,struct sc_pkcs15_prkey_rsa *rsa)
{
     int r;

    SC_FUNC_CALLED(card->ctx, SC_LOG_DEBUG_VERBOSE);

     {/* write prkey */
          r = entersafe_write_rsa_key_factor(card,key_id,0x22,0x01,rsa->p);
          LOG_TEST_RET(card->ctx, r, "write p failed");
          r = entersafe_write_rsa_key_factor(card,key_id,0x22,0x02,rsa->q);
          LOG_TEST_RET(card->ctx, r, "write q failed");
          r = entersafe_write_rsa_key_factor(card,key_id,0x22,0x03,rsa->dmp1);
          LOG_TEST_RET(card->ctx, r, "write dmp1 failed");
          r = entersafe_write_rsa_key_factor(card,key_id,0x22,0x04,rsa->dmq1);
          LOG_TEST_RET(card->ctx, r, "write dmq1 failed");
          r = entersafe_write_rsa_key_factor(card,key_id,0x22,0x05,rsa->iqmp);
          LOG_TEST_RET(card->ctx, r, "write iqmp failed");
     }

     {/* write pukey */
          u8 sbuff[SC_MAX_APDU_BUFFER_SIZE];
          sc_apdu_t apdu;

          /* first 64(0x40) bytes of N */
          sbuff[0]=0x83;
          sbuff[1]=0x02;
          sbuff[2]=key_id;
          sbuff[3]=0x2A;
          sbuff[4]=0x89;
          sbuff[5]=0x40;
          memcpy(sbuff+6,rsa->modulus.data,0x40);

          sc_format_apdu(card,&apdu,SC_APDU_CASE_3_SHORT,0x22,0x01,0xB8);
          apdu.data=sbuff;
          apdu.lc=apdu.datalen=0x46;

          r=entersafe_transmit_apdu(card,&apdu,0,0,0,0);
          LOG_TEST_RET(card->ctx, r, "APDU transmit failed");
          LOG_TEST_RET(card->ctx, sc_check_sw(card, apdu.sw1, apdu.sw2),"Write pukey N(1) failed");

          /* left 192(0xC0) bytes of N */
          sc_format_apdu(card,&apdu,SC_APDU_CASE_3_SHORT,0x46,0x0B,0x00);
          apdu.data=rsa->modulus.data+0x40;
          apdu.lc=apdu.datalen=0xC0;

          r=entersafe_transmit_apdu(card,&apdu,0,0,0,0);
          LOG_TEST_RET(card->ctx, r, "APDU transmit failed");
          LOG_TEST_RET(card->ctx, sc_check_sw(card, apdu.sw1, apdu.sw2),"Write pukey N(2) failed");

          /* E */
          r = entersafe_write_rsa_key_factor(card,key_id,0x2A,0x0D,rsa->exponent);
          LOG_TEST_RET(card->ctx, r, "write exponent failed");
     }
    SC_FUNC_RETURN(card->ctx, SC_LOG_DEBUG_VERBOSE,SC_SUCCESS);
}

static int entersafe_write_symmetric_key(sc_card_t *card,
                                         u8 key_id,u8 usage,
                                         u8 EC,u8 ver,
                                         u8 *data,size_t len)
{
     sc_apdu_t apdu;
     u8 sbuff[SC_MAX_APDU_BUFFER_SIZE]={0};
     int r;

    SC_FUNC_CALLED(card->ctx, SC_LOG_DEBUG_VERBOSE);

     if(len>240)
          SC_FUNC_RETURN(card->ctx, SC_LOG_DEBUG_VERBOSE,SC_ERROR_INCORRECT_PARAMETERS);

     sbuff[0]=EC;
     sbuff[1]=ver;
     memcpy(&sbuff[2],data,len);

     sc_format_apdu(card,&apdu,SC_APDU_CASE_3_SHORT,0xF4,usage,key_id);
     apdu.cla=0x84;
     apdu.data=sbuff;
     apdu.lc=apdu.datalen=len+2;

     r=entersafe_transmit_apdu(card,&apdu,key_maintain,sizeof(key_maintain),1,1);
     LOG_TEST_RET(card->ctx, r, "APDU transmit failed");
     LOG_TEST_RET(card->ctx, sc_check_sw(card, apdu.sw1, apdu.sw2),"Write prkey failed");
     SC_FUNC_RETURN(card->ctx, SC_LOG_DEBUG_VERBOSE,r);
}

static int entersafe_write_key(sc_card_t *card, sc_entersafe_wkey_data *data)
{
     struct sc_pkcs15_prkey_rsa* rsa=data->key_data.rsa;

    SC_FUNC_CALLED(card->ctx, SC_LOG_DEBUG_VERBOSE);

     switch(data->usage)
     {
     case 0x22:
          if(rsa->modulus.len < 256)
               return entersafe_write_small_rsa_key(card,data->key_id,rsa);
          else
               return entersafe_write_large_rsa_key(card,data->key_id,rsa);
          break;
     case 0x2A:
          SC_FUNC_RETURN(card->ctx, SC_LOG_DEBUG_VERBOSE,SC_ERROR_NOT_SUPPORTED);
          break;
     default:
          return entersafe_write_symmetric_key(card,data->key_id,data->usage,
                                               data->key_data.symmetric.EC,
                                               data->key_data.symmetric.ver,
                                               data->key_data.symmetric.key_val,
                                               data->key_data.symmetric.key_len);
          break;
     }
     SC_FUNC_RETURN(card->ctx, SC_LOG_DEBUG_VERBOSE,SC_SUCCESS);
}

int util_getpass (char **lineptr, size_t *len, FILE *stream)
{
#define MAX_PASS_SIZE    128
    char *buf;
    size_t i;
    int ch = 0;
#ifndef _WIN32
    struct termios old, new;

    fflush(stdout);
    if (tcgetattr (fileno (stdout), &old) != 0)
        return -1;
    new = old;
    new.c_lflag &= ~ECHO;
    if (tcsetattr (fileno (stdout), TCSAFLUSH, &new) != 0)
        return -1;
#endif

    buf = calloc(1, MAX_PASS_SIZE);
    if (!buf)
        return -1;

    for (i = 0; i < MAX_PASS_SIZE - 1; i++) {
#ifndef _WIN32
        ch = getchar();
#else
        ch = getchar();
#endif
        if (ch == 0 || ch == 3)
            break;
        if (ch == '\n' || ch == '\r')
            break;

        buf[i] = (char) ch;
    }
#ifndef _WIN32
    tcsetattr (fileno (stdout), TCSAFLUSH, &old);
    fputs("\n", stdout);
#endif
    if (ch == 0 || ch == 3) {
        free(buf);
        return -1;
    }

    if (*lineptr && (!len || *len < i+1)) {
        free(*lineptr);
        *lineptr = NULL;
    }

    if (*lineptr) {
        memcpy(*lineptr,buf,i+1);
        memset(buf, 0, MAX_PASS_SIZE);
        free(buf);
    } else {
        *lineptr = buf;
        if (len)
            *len = 8;
    }
    return i;
}

static int entersafe_gen_key(sc_card_t *card, sc_entersafe_gen_key_data *data)
{
        int    r;
        sc_apdu_t apdu;
        u8 rbuf[300];
//        u8 sbuf[8];
//        u8 pubkey_part1_hardcoded[LENGTH_PART1_HARDCODED];
//        u8 pubkey_part1_final[FINAL_PART1];
//        u8 pubkey_part2_hardcoded[LENGTH_PART2_HARDCODED];
//        u8 pubkey_part2_final[FINAL_PART2];
//        u8 privkey_buf[LENGTH_PRIVKEY_HARDCODED];
        u8 *p;
        char        *pin = NULL;
        size_t        len = 0;

        SC_FUNC_CALLED(card->ctx, SC_LOG_DEBUG_VERBOSE);

        printf("Re-Authenticating prior to Generate Key Pair:\r\n");
        printf("Enter User PIN again\r\n");
        r = util_getpass(&pin, &len, stdin);

        sc_format_apdu(card, &apdu, SC_APDU_CASE_3_SHORT, 0x20,  0x00, 0x81);

        apdu.le     = 0;
        apdu.data    = (unsigned char *)pin;
        apdu.lc      = 8;
        apdu.datalen = 8;
        apdu.resp    = rbuf;
        apdu.resplen = 0;
        r = entersafe_transmit_apdu(card, &apdu,0,0,0,0);

        /* generate key pair*/
        sc_format_apdu(card, &apdu, SC_APDU_CASE_2_SHORT, 0x46,  0x00, 0x10);

        apdu.le      = 256;
        apdu.data    = 0;
        apdu.lc      = 0;
        apdu.datalen = 0;
        apdu.resp    = rbuf;
        apdu.resplen = sizeof(rbuf);


        r = entersafe_transmit_apdu(card, &apdu,0,0,0,0);
        LOG_TEST_RET(card->ctx, r, "APDU transmit failed");
        LOG_TEST_RET(card->ctx, sc_check_sw(card,apdu.sw1,apdu.sw2),"EnterSafe generate key pair failed");

        len=apdu.resplen;
        p=rbuf;
        data->modulus = malloc(len);
        if (!data->modulus)
            SC_FUNC_RETURN(card->ctx, SC_LOG_DEBUG_VERBOSE, SC_ERROR_OUT_OF_MEMORY);
        //entersafe_reverse_buffer(p, len);
        memcpy(data->modulus, p, len);

        /* write public key in 4101 */
        //Create data to be placed in public key file

//        pubkey_part1_hardcoded[0]=0x30;
//        pubkey_part1_hardcoded[1]=0x82;
//        pubkey_part1_hardcoded[2]=0x01;
//        pubkey_part1_hardcoded[3]=0x73;
//        pubkey_part1_hardcoded[4]=0x30;
//        pubkey_part1_hardcoded[5]=0x30;
//        pubkey_part1_hardcoded[6]=0x0C;
//        pubkey_part1_hardcoded[7]=0x00;
//        pubkey_part1_hardcoded[8]=0x03;
//        pubkey_part1_hardcoded[9]=0x02;
//        pubkey_part1_hardcoded[10]=0x06;
//        pubkey_part1_hardcoded[11]=0x40;
//        pubkey_part1_hardcoded[12]=0x30;
//        pubkey_part1_hardcoded[13]=0x28;
//        pubkey_part1_hardcoded[14]=0x30;
//        pubkey_part1_hardcoded[15]=0x06;
//        pubkey_part1_hardcoded[16]=0x03;
//        pubkey_part1_hardcoded[17]=0x02;
//        pubkey_part1_hardcoded[18]=0x07;
//        pubkey_part1_hardcoded[19]=0x80;
//        pubkey_part1_hardcoded[20]=0x05;
//        pubkey_part1_hardcoded[21]=0x00;
//        pubkey_part1_hardcoded[22]=0x30;
//        pubkey_part1_hardcoded[23]=0x0E;
//        pubkey_part1_hardcoded[24]=0x03;
//        pubkey_part1_hardcoded[25]=0x02;
//        pubkey_part1_hardcoded[26]=0x06;
//        pubkey_part1_hardcoded[27]=0x40;
//        pubkey_part1_hardcoded[28]=0x04;
//        pubkey_part1_hardcoded[29]=0x08;
//        pubkey_part1_hardcoded[30]=0x43;
//        pubkey_part1_hardcoded[31]=0x61;
//        pubkey_part1_hardcoded[32]=0x72;
//        pubkey_part1_hardcoded[33]=0x64;
//        pubkey_part1_hardcoded[34]=0x20;
//        pubkey_part1_hardcoded[35]=0x50;
//        pubkey_part1_hardcoded[36]=0x49;
//        pubkey_part1_hardcoded[37]=0x4E;
//        pubkey_part1_hardcoded[38]=0x30;
//        pubkey_part1_hardcoded[39]=0x0E;
//        pubkey_part1_hardcoded[40]=0x03;
//        pubkey_part1_hardcoded[41]=0x02;
//        pubkey_part1_hardcoded[42]=0x04;
//        pubkey_part1_hardcoded[43]=0x10;
//        pubkey_part1_hardcoded[44]=0x04;
//        pubkey_part1_hardcoded[45]=0x08;
//        pubkey_part1_hardcoded[46]=0x43;
//        pubkey_part1_hardcoded[47]=0x61;
//        pubkey_part1_hardcoded[48]=0x72;
//        pubkey_part1_hardcoded[49]=0x64;
//        pubkey_part1_hardcoded[50]=0x20;
//        pubkey_part1_hardcoded[51]=0x50;
//        pubkey_part1_hardcoded[52]=0x49;
//        pubkey_part1_hardcoded[53]=0x4E;
//        pubkey_part1_hardcoded[54]=0x30;
//        pubkey_part1_hardcoded[55]=0x22;
//        pubkey_part1_hardcoded[56]=0x04;
//        pubkey_part1_hardcoded[57]=0x14;
//        pubkey_part1_hardcoded[58]=0x13;
//        pubkey_part1_hardcoded[59]=0xFA;
//        pubkey_part1_hardcoded[60]=0x26;
//        pubkey_part1_hardcoded[61]=0x66;
//        pubkey_part1_hardcoded[62]=0x4C;
//        pubkey_part1_hardcoded[63]=0xEE;
//        pubkey_part1_hardcoded[64]=0xEC;
//        pubkey_part1_hardcoded[65]=0x00;
//        pubkey_part1_hardcoded[66]=0xD6;
//        pubkey_part1_hardcoded[67]=0x00;
//        pubkey_part1_hardcoded[68]=0xCC;
//        pubkey_part1_hardcoded[69]=0x01;
//        pubkey_part1_hardcoded[70]=0xFD;
//        pubkey_part1_hardcoded[71]=0xA6;
//        pubkey_part1_hardcoded[72]=0x57;
//        pubkey_part1_hardcoded[73]=0x4D;
//        pubkey_part1_hardcoded[74]=0x9B;
//        pubkey_part1_hardcoded[75]=0xFB;
//        pubkey_part1_hardcoded[76]=0x11;
//        pubkey_part1_hardcoded[77]=0xE1;
//        pubkey_part1_hardcoded[78]=0x03;
//        pubkey_part1_hardcoded[79]=0x02;
//        pubkey_part1_hardcoded[80]=0x00;
//        pubkey_part1_hardcoded[81]=0x8B;
//        pubkey_part1_hardcoded[82]=0x03;
//        pubkey_part1_hardcoded[83]=0x02;
//        pubkey_part1_hardcoded[84]=0x03;
//        pubkey_part1_hardcoded[85]=0x48;
//        pubkey_part1_hardcoded[86]=0x02;
//        pubkey_part1_hardcoded[87]=0x02;
//        pubkey_part1_hardcoded[88]=0x00;
//        pubkey_part1_hardcoded[89]=0x90;
//        pubkey_part1_hardcoded[90]=0xA1;
//        pubkey_part1_hardcoded[91]=0x82;
//        pubkey_part1_hardcoded[92]=0x01;
//        pubkey_part1_hardcoded[93]=0x19;
//        pubkey_part1_hardcoded[94]=0x30;
//        pubkey_part1_hardcoded[95]=0x82;
//        pubkey_part1_hardcoded[96]=0x01;
//        pubkey_part1_hardcoded[97]=0x15;
//        pubkey_part1_hardcoded[98]=0xA0;
//        pubkey_part1_hardcoded[99]=0x82;
//        pubkey_part1_hardcoded[100]=0x01;
//        pubkey_part1_hardcoded[101]=0x0D;
//        pubkey_part1_hardcoded[102]=0x30;
//        pubkey_part1_hardcoded[103]=0x82;
//        pubkey_part1_hardcoded[104]=0x01;
//        pubkey_part1_hardcoded[105]=0x09;
//        pubkey_part1_hardcoded[106]=0x02;
//        pubkey_part1_hardcoded[107]=0x82;
//        pubkey_part1_hardcoded[108]=0x01;
//        pubkey_part1_hardcoded[109]=0x00;
//
//        //020301000102020800FF
//        pubkey_part2_hardcoded[0]=0x02;
//        pubkey_part2_hardcoded[1]=0x03;
//        pubkey_part2_hardcoded[2]=0x01;
//        pubkey_part2_hardcoded[3]=0x00;
//        pubkey_part2_hardcoded[4]=0x01;
//        pubkey_part2_hardcoded[5]=0x02;
//        pubkey_part2_hardcoded[6]=0x02;
//        pubkey_part2_hardcoded[7]=0x08;
//        pubkey_part2_hardcoded[8]=0x00;
//        pubkey_part2_hardcoded[9]=0xFF;
//
//        memcpy(pubkey_part1_final,pubkey_part1_hardcoded,LENGTH_PART1_HARDCODED);
//        memcpy(pubkey_part1_final+LENGTH_PART1_HARDCODED,rbuf,LENGTH_PART1_FROM_PUBKEY);
//        sc_apdu_log(card->ctx, pubkey_part1_final, sizeof(pubkey_part1_final), 0);
//
//        memcpy(pubkey_part2_final,(rbuf+LENGTH_PART1_FROM_PUBKEY),LENGTH_PART2_FROM_PUBKEY);
//        memcpy(pubkey_part2_final+LENGTH_PART2_FROM_PUBKEY,pubkey_part2_hardcoded,10);
//        sc_apdu_log(card->ctx, pubkey_part2_final, sizeof(pubkey_part2_final), 0);
//
//        //Select Public Key File
//        sc_format_apdu(card, &apdu, SC_APDU_CASE_3_SHORT, 0xA4,  0x00, 0x00);
//
//        apdu.le      = 0;
//        sbuf[0]     = 0x41;
//        sbuf[1]     = 0x01;
//        apdu.data    = sbuf;
//        apdu.lc      = 2;
//        apdu.datalen = 2;
//        apdu.resp    = rbuf;
//        apdu.resplen = sizeof(rbuf);
//
//        r = entersafe_transmit_apdu(card, &apdu,0,0,0,0);
//        LOG_TEST_RET(card->ctx, r, "APDU transmit failed");
//        LOG_TEST_RET(card->ctx, sc_check_sw(card,apdu.sw1,apdu.sw2),"FAILED TO SELECT public KEY FILE");
//
//        //ADDING 255 BYTES
//
//        sc_format_apdu(card, &apdu, SC_APDU_CASE_3_SHORT, 0xD6,  0x00, 0x00);
//        apdu.le     = 0;
//        apdu.data    = pubkey_part1_final;
//        apdu.lc      = 0xE6;
//        apdu.datalen = sizeof(pubkey_part1_final);
//        apdu.resp    = rbuf;
//        apdu.resplen = 0;
//        r = entersafe_transmit_apdu(card, &apdu,0,0,0,0);
//
//        //ADDING LAST BYTE
//
//        sc_format_apdu(card, &apdu, SC_APDU_CASE_3_SHORT, 0xD6,  0x00, 0xE6);
//        apdu.le     = 0;
//        apdu.data    = pubkey_part2_final;
//        apdu.lc      = 0x92;
//        apdu.datalen = sizeof(pubkey_part2_final);
//        apdu.resp    = rbuf;
//        apdu.resplen = 0;
//        r = entersafe_transmit_apdu(card, &apdu,0,0,0,0);
//
//        /* write private key in 4100 */
//        //Select Private Key file
//        sc_format_apdu(card, &apdu, SC_APDU_CASE_3_SHORT, 0xA4,  0x00, 0x00);
//
//        apdu.le      = 0;
//        sbuf[0]     = 0x41;
//        sbuf[1]     = 0x00;
//        apdu.data    = sbuf;
//        apdu.lc      = 2;
//        apdu.datalen = 2;
//        apdu.resp    = rbuf;
//        apdu.resplen = sizeof(rbuf);
//
//
//        r = entersafe_transmit_apdu(card, &apdu,0,0,0,0);
//        LOG_TEST_RET(card->ctx, r, "APDU transmit failed");
//        LOG_TEST_RET(card->ctx, sc_check_sw(card,apdu.sw1,apdu.sw2),"FAILED TO SELECT PRIVATE KEY FILE");



        //ASSOCIATE PRIVATE KEY FILE WITH PRIVATE KEY DATA @ KEY ID 10
        /*sc_format_apdu(card, &apdu, SC_APDU_CASE_3_SHORT, 0xD6,  0x00, 0x69);

        apdu.le          = 0;
        sbuf[0]         = 0x10; //key id
        apdu.data        = sbuf;
        apdu.lc          = 1;
        apdu.datalen     = 1;
        apdu.resp        = rbuf;
        apdu.resplen     = sizeof(rbuf);

        r = entersafe_transmit_apdu(card, &apdu,0,0,0,0);
        LOG_TEST_RET(card->ctx, r, "APDU transmit failed");
        LOG_TEST_RET(card->ctx, sc_check_sw(card,apdu.sw1,apdu.sw2),"FAILED TO PLACE PRIVATE KEY INSIDE CONTAINER");
        */
        //00D6000077
        //307430420C00030206C00408436172642050494E3030300E030205200408436172642050494E300E030206400408436172642050494E300E030204100408436172642050494E3022041413FA26664CEEEC00D600CC01FDA6574D9BFB11E103020274030203B802020010A10A30083002040002020800FF

//        privkey_buf[0]=0x30;
//        privkey_buf[1]=0x74;
//        privkey_buf[2]=0x30;
//        privkey_buf[3]=0x42;
//        privkey_buf[4]=0x0C;
//        privkey_buf[5]=0x00;
//        privkey_buf[6]=0x03;
//        privkey_buf[7]=0x02;
//        privkey_buf[8]=0x06;
//        privkey_buf[9]=0xC0;
//        privkey_buf[10]=0x04;
//        privkey_buf[11]=0x08;
//        privkey_buf[12]=0x43;
//        privkey_buf[13]=0x61;
//        privkey_buf[14]=0x72;
//        privkey_buf[15]=0x64;
//        privkey_buf[16]=0x20;
//        privkey_buf[17]=0x50;
//        privkey_buf[18]=0x49;
//        privkey_buf[19]=0x4E;
//        privkey_buf[20]=0x30;
//        privkey_buf[21]=0x30;
//        privkey_buf[22]=0x30;
//        privkey_buf[23]=0x0E;
//        privkey_buf[24]=0x03;
//        privkey_buf[25]=0x02;
//        privkey_buf[26]=0x05;
//        privkey_buf[27]=0x20;
//        privkey_buf[28]=0x04;
//        privkey_buf[29]=0x08;
//        privkey_buf[30]=0x43;
//        privkey_buf[31]=0x61;
//        privkey_buf[32]=0x72;
//        privkey_buf[33]=0x64;
//        privkey_buf[34]=0x20;
//        privkey_buf[35]=0x50;
//        privkey_buf[36]=0x49;
//        privkey_buf[37]=0x4E;
//        privkey_buf[38]=0x30;
//        privkey_buf[38]=0x0E;
//        privkey_buf[40]=0x03;
//        privkey_buf[41]=0x02;
//        privkey_buf[42]=0x06;
//        privkey_buf[43]=0x40;
//        privkey_buf[44]=0x04;
//        privkey_buf[45]=0x08;
//        privkey_buf[46]=0x43;
//        privkey_buf[47]=0x61;
//        privkey_buf[48]=0x72;
//        privkey_buf[49]=0x64;
//        privkey_buf[50]=0x20;
//        privkey_buf[51]=0x50;
//        privkey_buf[52]=0x49;
//        privkey_buf[53]=0x4E;
//        privkey_buf[54]=0x30;
//        privkey_buf[55]=0x0E;
//        privkey_buf[56]=0x03;
//        privkey_buf[57]=0x02;
//        privkey_buf[58]=0x04;
//        privkey_buf[59]=0x10;
//        privkey_buf[60]=0x04;
//        privkey_buf[61]=0x08;
//        privkey_buf[62]=0x43;
//        privkey_buf[63]=0x61;
//        privkey_buf[64]=0x72;
//        privkey_buf[65]=0x64;
//        privkey_buf[66]=0x20;
//        privkey_buf[67]=0x50;
//        privkey_buf[68]=0x49;
//        privkey_buf[69]=0x4E;
//        privkey_buf[70]=0x30;
//        privkey_buf[71]=0x22;
//        privkey_buf[72]=0x04;
//        privkey_buf[73]=0x14;
//        privkey_buf[74]=0x13;
//        privkey_buf[75]=0xFA;
//        privkey_buf[76]=0x26;
//        privkey_buf[77]=0x66;
//        privkey_buf[78]=0x4C;
//        privkey_buf[79]=0xEE;
//        privkey_buf[80]=0xEC;
//        privkey_buf[81]=0x00;
//        privkey_buf[82]=0xD6;
//        privkey_buf[83]=0x00;
//        privkey_buf[84]=0xCC;
//        privkey_buf[85]=0x01;
//        privkey_buf[86]=0xFD;
//        privkey_buf[87]=0xA6;
//        privkey_buf[88]=0x57;
//        privkey_buf[89]=0x4D;
//        privkey_buf[90]=0x9B;
//        privkey_buf[91]=0xFB;
//        privkey_buf[92]=0x11;
//        privkey_buf[93]=0xE1;
//        privkey_buf[94]=0x03;
//        privkey_buf[95]=0x02;
//        privkey_buf[96]=0x02;
//        privkey_buf[97]=0x74;
//        privkey_buf[98]=0x03;
//        privkey_buf[99]=0x02;
//        privkey_buf[100]=0x03;
//        privkey_buf[101]=0xB8;
//        privkey_buf[102]=0x02;
//        privkey_buf[103]=0x02;
//        privkey_buf[104]=0x00;
//        privkey_buf[105]=0x10;
//        privkey_buf[106]=0xA1;
//        privkey_buf[107]=0x0A;
//        privkey_buf[108]=0x30;
//        privkey_buf[109]=0x08;
//        privkey_buf[110]=0x30;
//        privkey_buf[111]=0x02;
//        privkey_buf[112]=0x04;
//        privkey_buf[113]=0x00;
//        privkey_buf[114]=0x02;
//        privkey_buf[115]=0x02;
//        privkey_buf[116]=0x08;
//        privkey_buf[117]=0x00;
//        privkey_buf[118]=0xFF;
//
//        sc_format_apdu(card, &apdu, SC_APDU_CASE_3_SHORT, 0xD6,  0x00, 0x00);
//
//        apdu.le          = 0;
//        apdu.data        = privkey_buf;
//        apdu.lc          = 0x77;
//        apdu.datalen     = sizeof(privkey_buf);
//        apdu.resp        = rbuf;
//        apdu.resplen     = sizeof(rbuf);
//
//        r = entersafe_transmit_apdu(card, &apdu,0,0,0,0);
//        LOG_TEST_RET(card->ctx, r, "APDU transmit failed");
//        LOG_TEST_RET(card->ctx, sc_check_sw(card,apdu.sw1,apdu.sw2),"FAILED TO PLACE PRIVATE KEY INSIDE CONTAINER");



        SC_FUNC_RETURN(card->ctx, SC_LOG_DEBUG_VERBOSE,SC_SUCCESS);
}

static int entersafe_get_serialnr(sc_card_t *card, sc_serial_number_t *serial)
{
    int    r;
    sc_apdu_t apdu;
    u8 rbuf[SC_MAX_APDU_BUFFER_SIZE];

    SC_FUNC_CALLED(card->ctx, SC_LOG_DEBUG_VERBOSE);
    assert(serial);

    sc_format_apdu(card, &apdu, SC_APDU_CASE_2_SHORT,0xEA,0x00,0x00);
    apdu.cla=0x80;
    apdu.resp=rbuf;
    apdu.resplen=sizeof(rbuf);
    apdu.le=0x08;

    r=entersafe_transmit_apdu(card, &apdu,0,0,0,0);
    LOG_TEST_RET(card->ctx, r, "APDU transmit failed");
    LOG_TEST_RET(card->ctx, sc_check_sw(card,apdu.sw1,apdu.sw2),"EnterSafe get SN failed");

    card->serialnr.len=serial->len=8;
    memcpy(card->serialnr.value,rbuf,8);
    memcpy(serial->value,rbuf,8);

    SC_FUNC_RETURN(card->ctx, SC_LOG_DEBUG_VERBOSE,SC_SUCCESS);
}

static int entersafe_preinstall_rsa_2048(sc_card_t *card,u8 key_id)
{
    u8 sbuf[SC_MAX_APDU_BUFFER_SIZE];
    sc_apdu_t apdu;
    int ret=0;
    static u8 const rsa_key_e[] =
    {
        'E', 0x04, 0x01, 0x00, 0x01, 0x00
    };

    SC_FUNC_CALLED(card->ctx, SC_LOG_DEBUG_VERBOSE);

    /*  create rsa item in IKF */
    sbuf[0] = 0x04; /* key len extern */
    sbuf[1] = 0x0a; /* key len */
    sbuf[2] = 0x22;    /* USAGE */
    sbuf[3] = 0x34;    /* user ac */
    sbuf[4] = 0x04;    /* change ac */
    sbuf[5] = 0x34;    /* UPDATE AC */
    sbuf[6] = 0x40;    /* ALGO */
    sbuf[7] = 0x00;    /* EC */
    sbuf[8] = 0x00;    /* VER */
    memcpy(&sbuf[9], rsa_key_e, sizeof(rsa_key_e));
    sbuf[9 + sizeof(rsa_key_e) + 0] = 'C'+'R'+'T';
    sbuf[9 + sizeof(rsa_key_e) + 1] = 0x82;
    sbuf[9 + sizeof(rsa_key_e) + 2] = 0x04;
    sbuf[9 + sizeof(rsa_key_e) + 3] = 0x00;

    sc_format_apdu(card, &apdu, SC_APDU_CASE_3_SHORT,0xF0,0x00,key_id);
    apdu.cla=0x84;
    apdu.data=sbuf;
    apdu.lc=apdu.datalen=9 + sizeof(rsa_key_e) + 4;

    ret = entersafe_transmit_apdu(card,&apdu,init_key,sizeof(init_key),0,1);
    LOG_TEST_RET(card->ctx, ret, "Preinstall rsa failed");

    /*  create rsa item in PKF */
    sbuf[0] = 0x01;    /* key len extern */
    sbuf[1] = 0x0A;    /* key len */
    sbuf[2] = 0x2A;    /* USAGE */
    sbuf[3] = ENTERSAFE_AC_ALWAYS;    /* user ac */
    sbuf[4] = 0x04;    /* change ac */
    sbuf[5] = ENTERSAFE_AC_ALWAYS;    /* UPDATE AC */
    sbuf[6] = 0x40;    /* ALGO */
    sbuf[7] = 0x00;    /* EC */
    sbuf[8] = 0x00;    /* VER */
    memcpy(&sbuf[9], rsa_key_e, sizeof(rsa_key_e));
    sbuf[9 + sizeof(rsa_key_e) + 0] = 'N';
    sbuf[9 + sizeof(rsa_key_e) + 1] = 0x82;
    sbuf[9 + sizeof(rsa_key_e) + 2] = 0x01;
    sbuf[9 + sizeof(rsa_key_e) + 3] = 0x00;

    sc_format_apdu(card,&apdu,SC_APDU_CASE_3_SHORT,0xF0,0x00,key_id);
    apdu.cla=0x84;
    apdu.data=sbuf;
    apdu.lc=apdu.datalen=9 + sizeof(rsa_key_e) + 4;

    ret=entersafe_transmit_apdu(card,&apdu,init_key,sizeof(init_key),0,1);
    LOG_TEST_RET(card->ctx, ret, "Preinstall rsa failed");

    SC_FUNC_RETURN(card->ctx, SC_LOG_DEBUG_VERBOSE,SC_SUCCESS);
}

static int entersafe_preinstall_keys(sc_card_t *card,int (*install_rsa)(sc_card_t *,u8))
{
     int r;
     u8 sbuf[SC_MAX_APDU_BUFFER_SIZE];
     sc_apdu_t apdu;

    SC_FUNC_CALLED(card->ctx, SC_LOG_DEBUG_VERBOSE);

     {/* RSA */
          u8 rsa_index;
          for(rsa_index=ENTERSAFE_MIN_KEY_ID;
              rsa_index<=ENTERSAFE_MAX_KEY_ID;
              ++rsa_index)
          {
               r=install_rsa(card,rsa_index);
               LOG_TEST_RET(card->ctx, r, "Preinstall rsa key failed");
          }
     }

     {/* key maintain */
          /* create key maintain*/
          sbuf[0] = 0;    /* key len extern */
          sbuf[1] = sizeof(key_maintain);    /* key len */
          sbuf[2] = 0x03;    /* USAGE */
          sbuf[3] = ENTERSAFE_AC_ALWAYS;    /* use AC    */
          sbuf[4] = ENTERSAFE_AC_ALWAYS;    /* CHANGE AC */
          sbuf[5] = ENTERSAFE_AC_NEVER;    /* UPDATE AC */
          sbuf[6] = 0x01;    /* ALGO */
          sbuf[7] = 0x00;    /* EC */
          sbuf[8] = 0x00;    /* VER */
          memcpy(&sbuf[9], key_maintain, sizeof(key_maintain));

          sc_format_apdu(card,&apdu,SC_APDU_CASE_3_SHORT,0xF0,0x00,0x00);
          apdu.cla=0x84;
          apdu.data=sbuf;
          apdu.lc=apdu.datalen=0x19;

          r = entersafe_transmit_apdu(card,&apdu,init_key,sizeof(init_key),0,1);
          LOG_TEST_RET(card->ctx, r, "Preinstall key maintain failed");
     }

     {/* user PIN */
          memset(sbuf,0,sizeof(sbuf));
          sbuf[0] = 0;    /* key len extern */
          sbuf[1] = 16;    /* key len */
          sbuf[2] = 0x0B;    /* USAGE */
          sbuf[3] = ENTERSAFE_AC_ALWAYS;    /* use AC */
          sbuf[4] = 0X04;    /* CHANGE AC */
          sbuf[5] = 0x38;    /* UPDATE AC */
          sbuf[6] = 0x01;    /* ALGO */
          sbuf[7] = 0xFF;    /* EC */
          sbuf[8] = 0x00;    /* VER */

          sc_format_apdu(card,&apdu,SC_APDU_CASE_3_SHORT,0xF0,0x00,ENTERSAFE_USER_PIN_ID);
          apdu.cla=0x84;
          apdu.data=sbuf;
          apdu.lc=apdu.datalen=0x19;

          r = entersafe_transmit_apdu(card,&apdu,init_key,sizeof(init_key),0,1);
          LOG_TEST_RET(card->ctx, r, "Preinstall user PIN failed");
     }

     {/* user PUK */
          memset(sbuf,0,sizeof(sbuf));
          sbuf[0] = 0;    /* key len extern */
          sbuf[1] = 16;    /* key len */
          sbuf[2] = 0x0B;    /* USAGE */
          sbuf[3] = ENTERSAFE_AC_ALWAYS;    /* use AC */
          sbuf[4] = 0X08;    /* CHANGE AC */
          sbuf[5] = 0xC0;    /* UPDATE AC */
          sbuf[6] = 0x01;    /* ALGO */
          sbuf[7] = 0xFF;    /* EC */
          sbuf[8] = 0x00;    /* VER */

          sc_format_apdu(card,&apdu,SC_APDU_CASE_3_SHORT,0xF0,0x00,ENTERSAFE_USER_PIN_ID+1);
          apdu.cla=0x84;
          apdu.data=sbuf;
          apdu.lc=apdu.datalen=0x19;

          r = entersafe_transmit_apdu(card,&apdu,init_key,sizeof(init_key),0,1);
          LOG_TEST_RET(card->ctx, r, "Preinstall user PUK failed");
     }


     SC_FUNC_RETURN(card->ctx, SC_LOG_DEBUG_VERBOSE,SC_SUCCESS);
}

static int entersafe_card_ctl_2048(sc_card_t *card, unsigned long cmd, void *ptr)
{
    sc_entersafe_create_data *tmp = (sc_entersafe_create_data *)ptr;

    SC_FUNC_CALLED(card->ctx, SC_LOG_DEBUG_VERBOSE);

    switch (cmd)
    {
    case SC_CARDCTL_ENTERSAFE_CREATE_FILE:
        if (tmp->type == SC_ENTERSAFE_MF_DATA)
            return entersafe_create_mf(card, tmp);
        else if (tmp->type == SC_ENTERSAFE_DF_DATA)
            return entersafe_create_df(card, tmp);
        else if (tmp->type == SC_ENTERSAFE_EF_DATA)
            return entersafe_create_ef(card, tmp);
        else
            return SC_ERROR_INTERNAL;
    case SC_CARDCTL_ENTERSAFE_WRITE_KEY:
        return entersafe_write_key(card, (sc_entersafe_wkey_data *)ptr);
    case SC_CARDCTL_ENTERSAFE_GENERATE_KEY:
        return entersafe_gen_key(card, (sc_entersafe_gen_key_data *)ptr);
    case SC_CARDCTL_ERASE_CARD:
        return entersafe_erase_card(card);
    case SC_CARDCTL_GET_SERIALNR:
        return entersafe_get_serialnr(card, (sc_serial_number_t *)ptr);
    case SC_CARDCTL_ENTERSAFE_PREINSTALL_KEYS:
         return entersafe_preinstall_keys(card,entersafe_preinstall_rsa_2048);
    default:
        return SC_ERROR_NOT_SUPPORTED;
    }
}

static struct sc_card_driver * sc_get_driver(void)
{
    struct sc_card_driver *iso_drv = sc_get_iso7816_driver();

    if (iso_ops == NULL)
        iso_ops = iso_drv->ops;
  
    entersafe_ops = *iso_drv->ops;
    entersafe_ops.match_card = entersafe_match_card;
    entersafe_ops.init   = entersafe_init;
    entersafe_ops.read_binary = entersafe_read_binary;
    entersafe_ops.write_binary = NULL;
    entersafe_ops.update_binary = entersafe_update_binary;
    entersafe_ops.select_file = entersafe_select_file;
    entersafe_ops.restore_security_env = entersafe_restore_security_env;
    entersafe_ops.set_security_env  = entersafe_set_security_env;
    entersafe_ops.decipher = entersafe_decipher;
    entersafe_ops.compute_signature = entersafe_compute_signature;
    entersafe_ops.create_file = entersafe_create_file;
    entersafe_ops.delete_file = NULL;
    entersafe_ops.pin_cmd = entersafe_pin_cmd;
    entersafe_ops.card_ctl    = entersafe_card_ctl_2048;
    entersafe_ops.process_fci = entersafe_process_fci;
    return &entersafe_drv;
}

struct sc_card_driver * sc_get_entersafe_driver(void)
{
    return sc_get_driver();
}
#endif
