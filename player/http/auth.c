/**
 * @file auth.c
 * @brief Authenticates the users for HTTP services
 * 
 * @copyright Copyright (c) 2020 Khant Kyaw Khaung
 * 
 * @license{This project is released under the MIT License.}
 */


#include "auth.h"

#include <string.h>
#include <stdlib.h>

#ifdef _WIN32
#include <Winsock2.h>
#else
#include <netinet/in.h>
#include <gcrypt.h>
#endif


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
static int authenticate_wincrypt(const char* pswd) {
    const DWORD dlen = 20;
    HCRYPTPROV hProv = 0;
    HCRYPTHASH hHash = 0;

    CryptAcquireContext(&hProv, NULL, 0, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT);
    CryptCreateHash(hProv, CALG_SHA1, 0, 0, &hHash);

    size_t file_length = 0;
    unsigned char* registered = load_file("http/password", "rb", &file_length);

    // If the hash does not exist or the hash is invalid
    if(registered == NULL || file_length != dlen) {
        if(registered != NULL)
            free(registered);
        // Rewrites a hash from a default password
        FILE* fp = fopen("http/password", "wb");
        DWORD len = dlen;
        registered = malloc(dlen);
        CryptHashData(hHash, "password", 8, 0);
        CryptGetHashParam(hHash, HP_HASHVAL, registered, &len, 0);
        for(DWORD i=0; i<dlen; i++)
            fputc(registered[i], fp);
        fclose(fp);
    }

    // Hashes the input password and compares with the registered one
    DWORD len = dlen;
    unsigned char* x = malloc(dlen);
    CryptHashData(hHash, pswd, strlen(pswd), 0);
    CryptGetHashParam(hHash, HP_HASHVAL, x, &len, 0);
    int auth = 1;
    for(DWORD i=0; i<dlen; i++) {
        if(x[i] != registered[i]) {
            auth = 0;
            break;
        }
    }

    CryptDestroyHash(hHash);
    CryptReleaseContext(hProv, 0);
    return auth;
}
#else
static int authenticate_gcrypt(const char* pswd) {
    unsigned int dlen = gcry_md_get_algo_dlen(GCRY_MD_SHA256);
    gcry_md_hd_t h;
    gcry_md_open(&h, GCRY_MD_SHA256, GCRY_MD_FLAG_SECURE);

    size_t len = 0;
    unsigned char* registered = load_file("http/password", "rb", &len);

    // If the hash does not exist or the hash is invalid
    if(registered == NULL || len != dlen) {
        if(registered != NULL)
            free(registered);
        // Rewrites a hash from a default password
        FILE* fp = fopen("http/password", "wb");
        gcry_md_write(h, "password", 8);
        unsigned char* x = gcry_md_read(h, GCRY_MD_SHA256);
        registered = malloc(dlen);
        memcpy(registered, x, dlen);
        for(size_t i=0; i<dlen; i++)
            fputc(registered[i], fp);
        fclose(fp);
        gcry_md_reset(h);
    }

    // Hashes the input password and compares with the registered one
    gcry_md_write(h, pswd, strlen(pswd));
    unsigned char* x = gcry_md_read(h, GCRY_MD_SHA256);
    int auth = 1;
    for(size_t i=0; i<dlen; i++) {
        if(x[i] != registered[i]) {
            auth = 0;
            break;
        }
    }

    gcry_md_close(h);
    return auth;
}
#endif




static uint32_t admin_addr = 0;


/**
 * @brief Authenticates the access to special services
 *
 * @param con_info The user
 * @param pswd The password string
 *
 * @return 1 if authenticated and 0 otherwise
 */
int remote_http_authenticate(struct RemoteConnection* con_info, const char *pswd) {
    int auth;
    #ifdef _WIN32
    auth = authenticate_wincrypt(pswd);
    #else
    auth = authenticate_gcrypt(pswd);
    #endif
    if(auth)
        admin_addr = con_info->address;
    return auth;
}

/**
 * @brief To check if the user if authenticated
 *
 * @param con_info The user
 *
 * @return 1 if authenticated and 0 otherwise
 */
int remote_http_isAuthenticated(struct RemoteConnection* con_info) {
    return con_info->address == admin_addr;
}