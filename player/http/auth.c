/**
 * @file auth.c
 * @brief Authenticates the users for HTTP services
 * 
 * @copyright Copyright (c) 2021 Khant Kyaw Khaung
 * 
 * @license{This project is released under the GPL License.}
 */


#include "auth.h"

#include <string.h>
#include <stdlib.h>

#ifndef _WIN32
#include <gcrypt.h>
#endif

#define DEFAULT_PASSWORD "password"




static char *load_file(const char *file, const char *mode, size_t *len) {
    FILE *fp = fopen(file, mode);
    if(fp == NULL)
        return NULL;
    fseek(fp, 0L, SEEK_END);
    size_t file_size = ftell(fp);
    fseek(fp, 0L, SEEK_SET);
    char *buffer = malloc(file_size+1);
    size_t i = 0;

    if(strncmp(mode, "r", 2) == 0) {
        char c;
        while((c = getc(fp)) != EOF)
            buffer[i++] = c;
    }
    else {
        while(i < file_size)
            buffer[i++] = getc(fp);
    }
    buffer[i] = '\0';
    fclose(fp);
    
    if(len != NULL)
        *len = i;

    return buffer;
}




#ifdef _WIN32
static unsigned int crypt_hash_algorithm = 0;
#endif
static unsigned int crypt_dlen = 0;

static void crypt_hash_init() {
  #ifdef _WIN32
  if(crypt_hash_algorithm != 0)
      return;

  HCRYPTPROV hProv = 0;
  HCRYPTHASH hHash = 0;
  BOOL res;

  res = CryptAcquireContext(&hProv, NULL, 0, PROV_RSA_FULL,
                            CRYPT_VERIFYCONTEXT);
  if(!res) {
      printf("Wincrypt context failed.\n");
      return;
  }

  res = CryptCreateHash(hProv, CALG_SHA_256, 0, 0, &hHash);
  if(res) {
      crypt_hash_algorithm = CALG_SHA_256;
      crypt_dlen = 32;
      CryptDestroyHash(hHash);
      CryptReleaseContext(hProv, 0);
      return;
  }

  res = CryptCreateHash(hProv, CALG_SHA1, 0, 0, &hHash);
  if(res) {
      crypt_hash_algorithm = CALG_SHA1;
      crypt_dlen = 20;
      CryptDestroyHash(hHash);
      CryptReleaseContext(hProv, 0);
      return;
  }

  CryptReleaseContext(hProv, 0);
  printf("No hash algorithm for Wincrypt available.\n");

  #else
  if(crypt_dlen == 0)
      crypt_dlen = gcry_md_get_algo_dlen(GCRY_MD_SHA256);
  #endif
}




#ifdef _WIN32
static void wincrypt_hash(const char *in, char *out) {
    const DWORD dlen = crypt_dlen;
    HCRYPTPROV hProv = 0;
    HCRYPTHASH hHash = 0;

    CryptAcquireContext(&hProv, NULL, 0, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT);
    CryptCreateHash(hProv, CALG_SHA1, 0, 0, &hHash);

    DWORD len = dlen;
    unsigned char* x = malloc(dlen);
    CryptHashData(hHash, in, strlen(in), 0);
    CryptGetHashParam(hHash, HP_HASHVAL, out, &len, 0);

    CryptDestroyHash(hHash);
    CryptReleaseContext(hProv, 0);
}
#else
static void gcrypt_hash(const char *in, char *out) {
    unsigned int dlen = gcry_md_get_algo_dlen(GCRY_MD_SHA256);
    gcry_md_hd_t h;
    gcry_md_open(&h, GCRY_MD_SHA256, GCRY_MD_FLAG_SECURE);
    
    gcry_md_write(h, in, strlen(in));
    unsigned char* x = gcry_md_read(h, GCRY_MD_SHA256);
    memcpy(out, x, dlen);

    gcry_md_close(h);
}
#endif

#ifdef _WIN32
#define crypt_hash wincrypt_hash
#else
#define crypt_hash gcrypt_hash
#endif




static uint32_t admin_addr = 0;


static int authenticate(const char *pswd) {
    unsigned int dlen;
    crypt_hash_init();
    if(crypt_dlen == 0)
        return 0;
    dlen = crypt_dlen;

    size_t file_length = 0;
    unsigned char* registered = load_file("http/password", "rb", &file_length);

    // If the hash does not exist or the hash is invalid
    if(registered == NULL || file_length != dlen) {
        if(registered != NULL)
            free(registered);
        // Rewrites a hash from a default password
        FILE* fp = fopen("http/password", "wb");
        registered = malloc(dlen);
        crypt_hash(DEFAULT_PASSWORD, registered);
        for(unsigned int i=0; i<dlen; i++)
            fputc(registered[i], fp);
        fclose(fp);
    }

    // Hashes the input password and compares with the registered one
    unsigned char* x = malloc(dlen);
    crypt_hash(pswd, x);
    int auth = (strncmp(x, registered, dlen) == 0);

    return auth;
}

/**
 * @brief Authenticates the access to special services
 *
 * @param con_info The user
 * @param pswd The password string
 *
 * @return 1 if authenticated and 0 otherwise
 */
int remote_http_authenticate(struct RemoteConnection* con_info,
                             const char *pswd)
{
    if(authenticate(pswd)) {
        admin_addr = con_info->address;
        return 1;
    }
    else
        return 0;
}

/**
 * @brief To check if the user if authenticated
 *
 * @param con_info The user
 *
 * @return 1 if authenticated and 0 otherwise
 */
int remote_http_is_authenticated(struct RemoteConnection* con_info) {
    return con_info->address == admin_addr;
}

/**
 * @brief Changes the password
 *
 * @param old_pswd The old password
 * @param new_pswd The new password
 *
 * @return 1 if the old password is correct and 0 otherwise
 */
int remote_http_change_password(const char* old_pswd, const char* new_pswd) {
    unsigned int dlen;
    crypt_hash_init();
    if(crypt_dlen == 0)
        return 0;
    dlen = crypt_dlen;

    if(authenticate(old_pswd)) {
        FILE* fp = fopen("http/password", "wb");
        char* x = malloc(dlen);
        crypt_hash(new_pswd, x);
        for(unsigned int i=0; i<dlen; i++)
            fputc(x[i], fp);
        fclose(fp);
        return 1;
    }
    else
        return 0;
}
