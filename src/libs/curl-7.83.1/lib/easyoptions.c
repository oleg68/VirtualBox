/***************************************************************************
 *                                  _   _ ____  _
 *  Project                     ___| | | |  _ | |
 *                             / __| | | | |_) | |
 *                            | (__| |_| |  _ <| |___
 *                             ___|___/|_| ______|
 *
 * Copyright (C) 1998 - 2021, Daniel Stenberg, <daniel.se>, et al.
 *
 * This software is licensed as described in the file COPYING, which
 * you should have received as part of this distribution. The terms
 * are also available at https://curl.se/docs/copyright.html.
 *
 * You may opt to use, copy, modify, merge, publish, distribute and/or sell
 * copies of the Software, and permit persons to whom the Software is
 * furnished to do so, under the terms of the COPYING file.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ***************************************************************************/

/* This source code is generated by optiontable.pl - DO NOT EDIT BY HAND */

#include "curl_setup.h"
#include "easyoptions.h"

/* all easy setopt options listed in alphabetical order */
struct curl_easyoption Curl_easyopts[] = {
  {"ABSTRACT_UNIX_SOCKET", CURLOPT_ABSTRACT_UNIX_SOCKET, CURLOT_STRING, 0},
  {"ACCEPTTIMEOUT_MS", CURLOPT_ACCEPTTIMEOUT_MS, CURLOT_LONG, 0},
  {"ACCEPT_ENCODING", CURLOPT_ACCEPT_ENCODING, CURLOT_STRING, 0},
  {"ADDRESS_SCOPE", CURLOPT_ADDRESS_SCOPE, CURLOT_LONG, 0},
  {"ALTSVC", CURLOPT_ALTSVC, CURLOT_STRING, 0},
  {"ALTSVC_CTRL", CURLOPT_ALTSVC_CTRL, CURLOT_LONG, 0},
  {"APPEND", CURLOPT_APPEND, CURLOT_LONG, 0},
  {"AUTOREFERER", CURLOPT_AUTOREFERER, CURLOT_LONG, 0},
  {"AWS_SIGV4", CURLOPT_AWS_SIGV4, CURLOT_STRING, 0},
  {"BUFFERSIZE", CURLOPT_BUFFERSIZE, CURLOT_LONG, 0},
  {"CAINFO", CURLOPT_CAINFO, CURLOT_STRING, 0},
  {"CAINFO_BLOB", CURLOPT_CAINFO_BLOB, CURLOT_BLOB, 0},
  {"CAPATH", CURLOPT_CAPATH, CURLOT_STRING, 0},
  {"CERTINFO", CURLOPT_CERTINFO, CURLOT_LONG, 0},
  {"CHUNK_BGN_FUNCTION", CURLOPT_CHUNK_BGN_FUNCTION, CURLOT_FUNCTION, 0},
  {"CHUNK_DATA", CURLOPT_CHUNK_DATA, CURLOT_CBPTR, 0},
  {"CHUNK_END_FUNCTION", CURLOPT_CHUNK_END_FUNCTION, CURLOT_FUNCTION, 0},
  {"CLOSESOCKETDATA", CURLOPT_CLOSESOCKETDATA, CURLOT_CBPTR, 0},
  {"CLOSESOCKETFUNCTION", CURLOPT_CLOSESOCKETFUNCTION, CURLOT_FUNCTION, 0},
  {"CONNECTTIMEOUT", CURLOPT_CONNECTTIMEOUT, CURLOT_LONG, 0},
  {"CONNECTTIMEOUT_MS", CURLOPT_CONNECTTIMEOUT_MS, CURLOT_LONG, 0},
  {"CONNECT_ONLY", CURLOPT_CONNECT_ONLY, CURLOT_LONG, 0},
  {"CONNECT_TO", CURLOPT_CONNECT_TO, CURLOT_SLIST, 0},
  {"CONV_FROM_NETWORK_FUNCTION", CURLOPT_CONV_FROM_NETWORK_FUNCTION,
   CURLOT_FUNCTION, 0},
  {"CONV_FROM_UTF8_FUNCTION", CURLOPT_CONV_FROM_UTF8_FUNCTION,
   CURLOT_FUNCTION, 0},
  {"CONV_TO_NETWORK_FUNCTION", CURLOPT_CONV_TO_NETWORK_FUNCTION,
   CURLOT_FUNCTION, 0},
  {"COOKIE", CURLOPT_COOKIE, CURLOT_STRING, 0},
  {"COOKIEFILE", CURLOPT_COOKIEFILE, CURLOT_STRING, 0},
  {"COOKIEJAR", CURLOPT_COOKIEJAR, CURLOT_STRING, 0},
  {"COOKIELIST", CURLOPT_COOKIELIST, CURLOT_STRING, 0},
  {"COOKIESESSION", CURLOPT_COOKIESESSION, CURLOT_LONG, 0},
  {"COPYPOSTFIELDS", CURLOPT_COPYPOSTFIELDS, CURLOT_OBJECT, 0},
  {"CRLF", CURLOPT_CRLF, CURLOT_LONG, 0},
  {"CRLFILE", CURLOPT_CRLFILE, CURLOT_STRING, 0},
  {"CURLU", CURLOPT_CURLU, CURLOT_OBJECT, 0},
  {"CUSTOMREQUEST", CURLOPT_CUSTOMREQUEST, CURLOT_STRING, 0},
  {"DEBUGDATA", CURLOPT_DEBUGDATA, CURLOT_CBPTR, 0},
  {"DEBUGFUNCTION", CURLOPT_DEBUGFUNCTION, CURLOT_FUNCTION, 0},
  {"DEFAULT_PROTOCOL", CURLOPT_DEFAULT_PROTOCOL, CURLOT_STRING, 0},
  {"DIRLISTONLY", CURLOPT_DIRLISTONLY, CURLOT_LONG, 0},
  {"DISALLOW_USERNAME_IN_URL", CURLOPT_DISALLOW_USERNAME_IN_URL,
   CURLOT_LONG, 0},
  {"DNS_CACHE_TIMEOUT", CURLOPT_DNS_CACHE_TIMEOUT, CURLOT_LONG, 0},
  {"DNS_INTERFACE", CURLOPT_DNS_INTERFACE, CURLOT_STRING, 0},
  {"DNS_LOCAL_IP4", CURLOPT_DNS_LOCAL_IP4, CURLOT_STRING, 0},
  {"DNS_LOCAL_IP6", CURLOPT_DNS_LOCAL_IP6, CURLOT_STRING, 0},
  {"DNS_SERVERS", CURLOPT_DNS_SERVERS, CURLOT_STRING, 0},
  {"DNS_SHUFFLE_ADDRESSES", CURLOPT_DNS_SHUFFLE_ADDRESSES, CURLOT_LONG, 0},
  {"DNS_USE_GLOBAL_CACHE", CURLOPT_DNS_USE_GLOBAL_CACHE, CURLOT_LONG, 0},
  {"DOH_SSL_VERIFYHOST", CURLOPT_DOH_SSL_VERIFYHOST, CURLOT_LONG, 0},
  {"DOH_SSL_VERIFYPEER", CURLOPT_DOH_SSL_VERIFYPEER, CURLOT_LONG, 0},
  {"DOH_SSL_VERIFYSTATUS", CURLOPT_DOH_SSL_VERIFYSTATUS, CURLOT_LONG, 0},
  {"DOH_URL", CURLOPT_DOH_URL, CURLOT_STRING, 0},
  {"EGDSOCKET", CURLOPT_EGDSOCKET, CURLOT_STRING, 0},
  {"ENCODING", CURLOPT_ACCEPT_ENCODING, CURLOT_STRING, CURLOT_FLAG_ALIAS},
  {"ERRORBUFFER", CURLOPT_ERRORBUFFER, CURLOT_OBJECT, 0},
  {"EXPECT_100_TIMEOUT_MS", CURLOPT_EXPECT_100_TIMEOUT_MS, CURLOT_LONG, 0},
  {"FAILONERROR", CURLOPT_FAILONERROR, CURLOT_LONG, 0},
  {"FILE", CURLOPT_WRITEDATA, CURLOT_CBPTR, CURLOT_FLAG_ALIAS},
  {"FILETIME", CURLOPT_FILETIME, CURLOT_LONG, 0},
  {"FNMATCH_DATA", CURLOPT_FNMATCH_DATA, CURLOT_CBPTR, 0},
  {"FNMATCH_FUNCTION", CURLOPT_FNMATCH_FUNCTION, CURLOT_FUNCTION, 0},
  {"FOLLOWLOCATION", CURLOPT_FOLLOWLOCATION, CURLOT_LONG, 0},
  {"FORBID_REUSE", CURLOPT_FORBID_REUSE, CURLOT_LONG, 0},
  {"FRESH_CONNECT", CURLOPT_FRESH_CONNECT, CURLOT_LONG, 0},
  {"FTPAPPEND", CURLOPT_APPEND, CURLOT_LONG, CURLOT_FLAG_ALIAS},
  {"FTPLISTONLY", CURLOPT_DIRLISTONLY, CURLOT_LONG, CURLOT_FLAG_ALIAS},
  {"FTPPORT", CURLOPT_FTPPORT, CURLOT_STRING, 0},
  {"FTPSSLAUTH", CURLOPT_FTPSSLAUTH, CURLOT_VALUES, 0},
  {"FTP_ACCOUNT", CURLOPT_FTP_ACCOUNT, CURLOT_STRING, 0},
  {"FTP_ALTERNATIVE_TO_USER", CURLOPT_FTP_ALTERNATIVE_TO_USER,
   CURLOT_STRING, 0},
  {"FTP_CREATE_MISSING_DIRS", CURLOPT_FTP_CREATE_MISSING_DIRS,
   CURLOT_LONG, 0},
  {"FTP_FILEMETHOD", CURLOPT_FTP_FILEMETHOD, CURLOT_VALUES, 0},
  {"FTP_RESPONSE_TIMEOUT", CURLOPT_FTP_RESPONSE_TIMEOUT, CURLOT_LONG, 0},
  {"FTP_SKIP_PASV_IP", CURLOPT_FTP_SKIP_PASV_IP, CURLOT_LONG, 0},
  {"FTP_SSL", CURLOPT_USE_SSL, CURLOT_VALUES, CURLOT_FLAG_ALIAS},
  {"FTP_SSL_CCC", CURLOPT_FTP_SSL_CCC, CURLOT_LONG, 0},
  {"FTP_USE_EPRT", CURLOPT_FTP_USE_EPRT, CURLOT_LONG, 0},
  {"FTP_USE_EPSV", CURLOPT_FTP_USE_EPSV, CURLOT_LONG, 0},
  {"FTP_USE_PRET", CURLOPT_FTP_USE_PRET, CURLOT_LONG, 0},
  {"GSSAPI_DELEGATION", CURLOPT_GSSAPI_DELEGATION, CURLOT_VALUES, 0},
  {"HAPPY_EYEBALLS_TIMEOUT_MS", CURLOPT_HAPPY_EYEBALLS_TIMEOUT_MS,
   CURLOT_LONG, 0},
  {"HAPROXYPROTOCOL", CURLOPT_HAPROXYPROTOCOL, CURLOT_LONG, 0},
  {"HEADER", CURLOPT_HEADER, CURLOT_LONG, 0},
  {"HEADERDATA", CURLOPT_HEADERDATA, CURLOT_CBPTR, 0},
  {"HEADERFUNCTION", CURLOPT_HEADERFUNCTION, CURLOT_FUNCTION, 0},
  {"HEADEROPT", CURLOPT_HEADEROPT, CURLOT_VALUES, 0},
  {"HSTS", CURLOPT_HSTS, CURLOT_STRING, 0},
  {"HSTSREADDATA", CURLOPT_HSTSREADDATA, CURLOT_CBPTR, 0},
  {"HSTSREADFUNCTION", CURLOPT_HSTSREADFUNCTION, CURLOT_FUNCTION, 0},
  {"HSTSWRITEDATA", CURLOPT_HSTSWRITEDATA, CURLOT_CBPTR, 0},
  {"HSTSWRITEFUNCTION", CURLOPT_HSTSWRITEFUNCTION, CURLOT_FUNCTION, 0},
  {"HSTS_CTRL", CURLOPT_HSTS_CTRL, CURLOT_LONG, 0},
  {"HTTP09_ALLOWED", CURLOPT_HTTP09_ALLOWED, CURLOT_LONG, 0},
  {"HTTP200ALIASES", CURLOPT_HTTP200ALIASES, CURLOT_SLIST, 0},
  {"HTTPAUTH", CURLOPT_HTTPAUTH, CURLOT_VALUES, 0},
  {"HTTPGET", CURLOPT_HTTPGET, CURLOT_LONG, 0},
  {"HTTPHEADER", CURLOPT_HTTPHEADER, CURLOT_SLIST, 0},
  {"HTTPPOST", CURLOPT_HTTPPOST, CURLOT_OBJECT, 0},
  {"HTTPPROXYTUNNEL", CURLOPT_HTTPPROXYTUNNEL, CURLOT_LONG, 0},
  {"HTTP_CONTENT_DECODING", CURLOPT_HTTP_CONTENT_DECODING, CURLOT_LONG, 0},
  {"HTTP_TRANSFER_DECODING", CURLOPT_HTTP_TRANSFER_DECODING, CURLOT_LONG, 0},
  {"HTTP_VERSION", CURLOPT_HTTP_VERSION, CURLOT_VALUES, 0},
  {"IGNORE_CONTENT_LENGTH", CURLOPT_IGNORE_CONTENT_LENGTH, CURLOT_LONG, 0},
  {"INFILE", CURLOPT_READDATA, CURLOT_CBPTR, CURLOT_FLAG_ALIAS},
  {"INFILESIZE", CURLOPT_INFILESIZE, CURLOT_LONG, 0},
  {"INFILESIZE_LARGE", CURLOPT_INFILESIZE_LARGE, CURLOT_OFF_T, 0},
  {"INTERFACE", CURLOPT_INTERFACE, CURLOT_STRING, 0},
  {"INTERLEAVEDATA", CURLOPT_INTERLEAVEDATA, CURLOT_CBPTR, 0},
  {"INTERLEAVEFUNCTION", CURLOPT_INTERLEAVEFUNCTION, CURLOT_FUNCTION, 0},
  {"IOCTLDATA", CURLOPT_IOCTLDATA, CURLOT_CBPTR, 0},
  {"IOCTLFUNCTION", CURLOPT_IOCTLFUNCTION, CURLOT_FUNCTION, 0},
  {"IPRESOLVE", CURLOPT_IPRESOLVE, CURLOT_VALUES, 0},
  {"ISSUERCERT", CURLOPT_ISSUERCERT, CURLOT_STRING, 0},
  {"ISSUERCERT_BLOB", CURLOPT_ISSUERCERT_BLOB, CURLOT_BLOB, 0},
  {"KEEP_SENDING_ON_ERROR", CURLOPT_KEEP_SENDING_ON_ERROR, CURLOT_LONG, 0},
  {"KEYPASSWD", CURLOPT_KEYPASSWD, CURLOT_STRING, 0},
  {"KRB4LEVEL", CURLOPT_KRBLEVEL, CURLOT_STRING, CURLOT_FLAG_ALIAS},
  {"KRBLEVEL", CURLOPT_KRBLEVEL, CURLOT_STRING, 0},
  {"LOCALPORT", CURLOPT_LOCALPORT, CURLOT_LONG, 0},
  {"LOCALPORTRANGE", CURLOPT_LOCALPORTRANGE, CURLOT_LONG, 0},
  {"LOGIN_OPTIONS", CURLOPT_LOGIN_OPTIONS, CURLOT_STRING, 0},
  {"LOW_SPEED_LIMIT", CURLOPT_LOW_SPEED_LIMIT, CURLOT_LONG, 0},
  {"LOW_SPEED_TIME", CURLOPT_LOW_SPEED_TIME, CURLOT_LONG, 0},
  {"MAIL_AUTH", CURLOPT_MAIL_AUTH, CURLOT_STRING, 0},
  {"MAIL_FROM", CURLOPT_MAIL_FROM, CURLOT_STRING, 0},
  {"MAIL_RCPT", CURLOPT_MAIL_RCPT, CURLOT_SLIST, 0},
  {"MAIL_RCPT_ALLLOWFAILS", CURLOPT_MAIL_RCPT_ALLLOWFAILS, CURLOT_LONG, 0},
  {"MAXAGE_CONN", CURLOPT_MAXAGE_CONN, CURLOT_LONG, 0},
  {"MAXCONNECTS", CURLOPT_MAXCONNECTS, CURLOT_LONG, 0},
  {"MAXFILESIZE", CURLOPT_MAXFILESIZE, CURLOT_LONG, 0},
  {"MAXFILESIZE_LARGE", CURLOPT_MAXFILESIZE_LARGE, CURLOT_OFF_T, 0},
  {"MAXLIFETIME_CONN", CURLOPT_MAXLIFETIME_CONN, CURLOT_LONG, 0},
  {"MAXREDIRS", CURLOPT_MAXREDIRS, CURLOT_LONG, 0},
  {"MAX_RECV_SPEED_LARGE", CURLOPT_MAX_RECV_SPEED_LARGE, CURLOT_OFF_T, 0},
  {"MAX_SEND_SPEED_LARGE", CURLOPT_MAX_SEND_SPEED_LARGE, CURLOT_OFF_T, 0},
  {"MIMEPOST", CURLOPT_MIMEPOST, CURLOT_OBJECT, 0},
  {"MIME_OPTIONS", CURLOPT_MIME_OPTIONS, CURLOT_LONG, 0},
  {"NETRC", CURLOPT_NETRC, CURLOT_VALUES, 0},
  {"NETRC_FILE", CURLOPT_NETRC_FILE, CURLOT_STRING, 0},
  {"NEW_DIRECTORY_PERMS", CURLOPT_NEW_DIRECTORY_PERMS, CURLOT_LONG, 0},
  {"NEW_FILE_PERMS", CURLOPT_NEW_FILE_PERMS, CURLOT_LONG, 0},
  {"NOBODY", CURLOPT_NOBODY, CURLOT_LONG, 0},
  {"NOPROGRESS", CURLOPT_NOPROGRESS, CURLOT_LONG, 0},
  {"NOPROXY", CURLOPT_NOPROXY, CURLOT_STRING, 0},
  {"NOSIGNAL", CURLOPT_NOSIGNAL, CURLOT_LONG, 0},
  {"OPENSOCKETDATA", CURLOPT_OPENSOCKETDATA, CURLOT_CBPTR, 0},
  {"OPENSOCKETFUNCTION", CURLOPT_OPENSOCKETFUNCTION, CURLOT_FUNCTION, 0},
  {"PASSWORD", CURLOPT_PASSWORD, CURLOT_STRING, 0},
  {"PATH_AS_IS", CURLOPT_PATH_AS_IS, CURLOT_LONG, 0},
  {"PINNEDPUBLICKEY", CURLOPT_PINNEDPUBLICKEY, CURLOT_STRING, 0},
  {"PIPEWAIT", CURLOPT_PIPEWAIT, CURLOT_LONG, 0},
  {"PORT", CURLOPT_PORT, CURLOT_LONG, 0},
  {"POST", CURLOPT_POST, CURLOT_LONG, 0},
  {"POST301", CURLOPT_POSTREDIR, CURLOT_VALUES, CURLOT_FLAG_ALIAS},
  {"POSTFIELDS", CURLOPT_POSTFIELDS, CURLOT_OBJECT, 0},
  {"POSTFIELDSIZE", CURLOPT_POSTFIELDSIZE, CURLOT_LONG, 0},
  {"POSTFIELDSIZE_LARGE", CURLOPT_POSTFIELDSIZE_LARGE, CURLOT_OFF_T, 0},
  {"POSTQUOTE", CURLOPT_POSTQUOTE, CURLOT_SLIST, 0},
  {"POSTREDIR", CURLOPT_POSTREDIR, CURLOT_VALUES, 0},
  {"PREQUOTE", CURLOPT_PREQUOTE, CURLOT_SLIST, 0},
  {"PREREQDATA", CURLOPT_PREREQDATA, CURLOT_CBPTR, 0},
  {"PREREQFUNCTION", CURLOPT_PREREQFUNCTION, CURLOT_FUNCTION, 0},
  {"PRE_PROXY", CURLOPT_PRE_PROXY, CURLOT_STRING, 0},
  {"PRIVATE", CURLOPT_PRIVATE, CURLOT_OBJECT, 0},
  {"PROGRESSDATA", CURLOPT_XFERINFODATA, CURLOT_CBPTR, CURLOT_FLAG_ALIAS},
  {"PROGRESSFUNCTION", CURLOPT_PROGRESSFUNCTION, CURLOT_FUNCTION, 0},
  {"PROTOCOLS", CURLOPT_PROTOCOLS, CURLOT_LONG, 0},
  {"PROXY", CURLOPT_PROXY, CURLOT_STRING, 0},
  {"PROXYAUTH", CURLOPT_PROXYAUTH, CURLOT_VALUES, 0},
  {"PROXYHEADER", CURLOPT_PROXYHEADER, CURLOT_SLIST, 0},
  {"PROXYPASSWORD", CURLOPT_PROXYPASSWORD, CURLOT_STRING, 0},
  {"PROXYPORT", CURLOPT_PROXYPORT, CURLOT_LONG, 0},
  {"PROXYTYPE", CURLOPT_PROXYTYPE, CURLOT_VALUES, 0},
  {"PROXYUSERNAME", CURLOPT_PROXYUSERNAME, CURLOT_STRING, 0},
  {"PROXYUSERPWD", CURLOPT_PROXYUSERPWD, CURLOT_STRING, 0},
  {"PROXY_CAINFO", CURLOPT_PROXY_CAINFO, CURLOT_STRING, 0},
  {"PROXY_CAINFO_BLOB", CURLOPT_PROXY_CAINFO_BLOB, CURLOT_BLOB, 0},
  {"PROXY_CAPATH", CURLOPT_PROXY_CAPATH, CURLOT_STRING, 0},
  {"PROXY_CRLFILE", CURLOPT_PROXY_CRLFILE, CURLOT_STRING, 0},
  {"PROXY_ISSUERCERT", CURLOPT_PROXY_ISSUERCERT, CURLOT_STRING, 0},
  {"PROXY_ISSUERCERT_BLOB", CURLOPT_PROXY_ISSUERCERT_BLOB, CURLOT_BLOB, 0},
  {"PROXY_KEYPASSWD", CURLOPT_PROXY_KEYPASSWD, CURLOT_STRING, 0},
  {"PROXY_PINNEDPUBLICKEY", CURLOPT_PROXY_PINNEDPUBLICKEY, CURLOT_STRING, 0},
  {"PROXY_SERVICE_NAME", CURLOPT_PROXY_SERVICE_NAME, CURLOT_STRING, 0},
  {"PROXY_SSLCERT", CURLOPT_PROXY_SSLCERT, CURLOT_STRING, 0},
  {"PROXY_SSLCERTTYPE", CURLOPT_PROXY_SSLCERTTYPE, CURLOT_STRING, 0},
  {"PROXY_SSLCERT_BLOB", CURLOPT_PROXY_SSLCERT_BLOB, CURLOT_BLOB, 0},
  {"PROXY_SSLKEY", CURLOPT_PROXY_SSLKEY, CURLOT_STRING, 0},
  {"PROXY_SSLKEYTYPE", CURLOPT_PROXY_SSLKEYTYPE, CURLOT_STRING, 0},
  {"PROXY_SSLKEY_BLOB", CURLOPT_PROXY_SSLKEY_BLOB, CURLOT_BLOB, 0},
  {"PROXY_SSLVERSION", CURLOPT_PROXY_SSLVERSION, CURLOT_VALUES, 0},
  {"PROXY_SSL_CIPHER_LIST", CURLOPT_PROXY_SSL_CIPHER_LIST, CURLOT_STRING, 0},
  {"PROXY_SSL_OPTIONS", CURLOPT_PROXY_SSL_OPTIONS, CURLOT_LONG, 0},
  {"PROXY_SSL_VERIFYHOST", CURLOPT_PROXY_SSL_VERIFYHOST, CURLOT_LONG, 0},
  {"PROXY_SSL_VERIFYPEER", CURLOPT_PROXY_SSL_VERIFYPEER, CURLOT_LONG, 0},
  {"PROXY_TLS13_CIPHERS", CURLOPT_PROXY_TLS13_CIPHERS, CURLOT_STRING, 0},
  {"PROXY_TLSAUTH_PASSWORD", CURLOPT_PROXY_TLSAUTH_PASSWORD,
   CURLOT_STRING, 0},
  {"PROXY_TLSAUTH_TYPE", CURLOPT_PROXY_TLSAUTH_TYPE, CURLOT_STRING, 0},
  {"PROXY_TLSAUTH_USERNAME", CURLOPT_PROXY_TLSAUTH_USERNAME,
   CURLOT_STRING, 0},
  {"PROXY_TRANSFER_MODE", CURLOPT_PROXY_TRANSFER_MODE, CURLOT_LONG, 0},
  {"PUT", CURLOPT_PUT, CURLOT_LONG, 0},
  {"QUOTE", CURLOPT_QUOTE, CURLOT_SLIST, 0},
  {"RANDOM_FILE", CURLOPT_RANDOM_FILE, CURLOT_STRING, 0},
  {"RANGE", CURLOPT_RANGE, CURLOT_STRING, 0},
  {"READDATA", CURLOPT_READDATA, CURLOT_CBPTR, 0},
  {"READFUNCTION", CURLOPT_READFUNCTION, CURLOT_FUNCTION, 0},
  {"REDIR_PROTOCOLS", CURLOPT_REDIR_PROTOCOLS, CURLOT_LONG, 0},
  {"REFERER", CURLOPT_REFERER, CURLOT_STRING, 0},
  {"REQUEST_TARGET", CURLOPT_REQUEST_TARGET, CURLOT_STRING, 0},
  {"RESOLVE", CURLOPT_RESOLVE, CURLOT_SLIST, 0},
  {"RESOLVER_START_DATA", CURLOPT_RESOLVER_START_DATA, CURLOT_CBPTR, 0},
  {"RESOLVER_START_FUNCTION", CURLOPT_RESOLVER_START_FUNCTION,
   CURLOT_FUNCTION, 0},
  {"RESUME_FROM", CURLOPT_RESUME_FROM, CURLOT_LONG, 0},
  {"RESUME_FROM_LARGE", CURLOPT_RESUME_FROM_LARGE, CURLOT_OFF_T, 0},
  {"RTSPHEADER", CURLOPT_HTTPHEADER, CURLOT_SLIST, CURLOT_FLAG_ALIAS},
  {"RTSP_CLIENT_CSEQ", CURLOPT_RTSP_CLIENT_CSEQ, CURLOT_LONG, 0},
  {"RTSP_REQUEST", CURLOPT_RTSP_REQUEST, CURLOT_VALUES, 0},
  {"RTSP_SERVER_CSEQ", CURLOPT_RTSP_SERVER_CSEQ, CURLOT_LONG, 0},
  {"RTSP_SESSION_ID", CURLOPT_RTSP_SESSION_ID, CURLOT_STRING, 0},
  {"RTSP_STREAM_URI", CURLOPT_RTSP_STREAM_URI, CURLOT_STRING, 0},
  {"RTSP_TRANSPORT", CURLOPT_RTSP_TRANSPORT, CURLOT_STRING, 0},
  {"SASL_AUTHZID", CURLOPT_SASL_AUTHZID, CURLOT_STRING, 0},
  {"SASL_IR", CURLOPT_SASL_IR, CURLOT_LONG, 0},
  {"SEEKDATA", CURLOPT_SEEKDATA, CURLOT_CBPTR, 0},
  {"SEEKFUNCTION", CURLOPT_SEEKFUNCTION, CURLOT_FUNCTION, 0},
  {"SERVER_RESPONSE_TIMEOUT", CURLOPT_FTP_RESPONSE_TIMEOUT,
   CURLOT_LONG, CURLOT_FLAG_ALIAS},
  {"SERVICE_NAME", CURLOPT_SERVICE_NAME, CURLOT_STRING, 0},
  {"SHARE", CURLOPT_SHARE, CURLOT_OBJECT, 0},
  {"SOCKOPTDATA", CURLOPT_SOCKOPTDATA, CURLOT_CBPTR, 0},
  {"SOCKOPTFUNCTION", CURLOPT_SOCKOPTFUNCTION, CURLOT_FUNCTION, 0},
  {"SOCKS5_AUTH", CURLOPT_SOCKS5_AUTH, CURLOT_LONG, 0},
  {"SOCKS5_GSSAPI_NEC", CURLOPT_SOCKS5_GSSAPI_NEC, CURLOT_LONG, 0},
  {"SOCKS5_GSSAPI_SERVICE", CURLOPT_SOCKS5_GSSAPI_SERVICE, CURLOT_STRING, 0},
  {"SSH_AUTH_TYPES", CURLOPT_SSH_AUTH_TYPES, CURLOT_VALUES, 0},
  {"SSH_COMPRESSION", CURLOPT_SSH_COMPRESSION, CURLOT_LONG, 0},
  {"SSH_HOST_PUBLIC_KEY_MD5", CURLOPT_SSH_HOST_PUBLIC_KEY_MD5,
   CURLOT_STRING, 0},
  {"SSH_HOST_PUBLIC_KEY_SHA256", CURLOPT_SSH_HOST_PUBLIC_KEY_SHA256,
   CURLOT_STRING, 0},
  {"SSH_KEYDATA", CURLOPT_SSH_KEYDATA, CURLOT_CBPTR, 0},
  {"SSH_KEYFUNCTION", CURLOPT_SSH_KEYFUNCTION, CURLOT_FUNCTION, 0},
  {"SSH_KNOWNHOSTS", CURLOPT_SSH_KNOWNHOSTS, CURLOT_STRING, 0},
  {"SSH_PRIVATE_KEYFILE", CURLOPT_SSH_PRIVATE_KEYFILE, CURLOT_STRING, 0},
  {"SSH_PUBLIC_KEYFILE", CURLOPT_SSH_PUBLIC_KEYFILE, CURLOT_STRING, 0},
  {"SSLCERT", CURLOPT_SSLCERT, CURLOT_STRING, 0},
  {"SSLCERTPASSWD", CURLOPT_KEYPASSWD, CURLOT_STRING, CURLOT_FLAG_ALIAS},
  {"SSLCERTTYPE", CURLOPT_SSLCERTTYPE, CURLOT_STRING, 0},
  {"SSLCERT_BLOB", CURLOPT_SSLCERT_BLOB, CURLOT_BLOB, 0},
  {"SSLENGINE", CURLOPT_SSLENGINE, CURLOT_STRING, 0},
  {"SSLENGINE_DEFAULT", CURLOPT_SSLENGINE_DEFAULT, CURLOT_LONG, 0},
  {"SSLKEY", CURLOPT_SSLKEY, CURLOT_STRING, 0},
  {"SSLKEYPASSWD", CURLOPT_KEYPASSWD, CURLOT_STRING, CURLOT_FLAG_ALIAS},
  {"SSLKEYTYPE", CURLOPT_SSLKEYTYPE, CURLOT_STRING, 0},
  {"SSLKEY_BLOB", CURLOPT_SSLKEY_BLOB, CURLOT_BLOB, 0},
  {"SSLVERSION", CURLOPT_SSLVERSION, CURLOT_VALUES, 0},
  {"SSL_CIPHER_LIST", CURLOPT_SSL_CIPHER_LIST, CURLOT_STRING, 0},
  {"SSL_CTX_DATA", CURLOPT_SSL_CTX_DATA, CURLOT_CBPTR, 0},
  {"SSL_CTX_FUNCTION", CURLOPT_SSL_CTX_FUNCTION, CURLOT_FUNCTION, 0},
  {"SSL_EC_CURVES", CURLOPT_SSL_EC_CURVES, CURLOT_STRING, 0},
  {"SSL_ENABLE_ALPN", CURLOPT_SSL_ENABLE_ALPN, CURLOT_LONG, 0},
  {"SSL_ENABLE_NPN", CURLOPT_SSL_ENABLE_NPN, CURLOT_LONG, 0},
  {"SSL_FALSESTART", CURLOPT_SSL_FALSESTART, CURLOT_LONG, 0},
  {"SSL_OPTIONS", CURLOPT_SSL_OPTIONS, CURLOT_VALUES, 0},
  {"SSL_SESSIONID_CACHE", CURLOPT_SSL_SESSIONID_CACHE, CURLOT_LONG, 0},
  {"SSL_VERIFYHOST", CURLOPT_SSL_VERIFYHOST, CURLOT_LONG, 0},
  {"SSL_VERIFYPEER", CURLOPT_SSL_VERIFYPEER, CURLOT_LONG, 0},
  {"SSL_VERIFYSTATUS", CURLOPT_SSL_VERIFYSTATUS, CURLOT_LONG, 0},
  {"STDERR", CURLOPT_STDERR, CURLOT_OBJECT, 0},
  {"STREAM_DEPENDS", CURLOPT_STREAM_DEPENDS, CURLOT_OBJECT, 0},
  {"STREAM_DEPENDS_E", CURLOPT_STREAM_DEPENDS_E, CURLOT_OBJECT, 0},
  {"STREAM_WEIGHT", CURLOPT_STREAM_WEIGHT, CURLOT_LONG, 0},
  {"SUPPRESS_CONNECT_HEADERS", CURLOPT_SUPPRESS_CONNECT_HEADERS,
   CURLOT_LONG, 0},
  {"TCP_FASTOPEN", CURLOPT_TCP_FASTOPEN, CURLOT_LONG, 0},
  {"TCP_KEEPALIVE", CURLOPT_TCP_KEEPALIVE, CURLOT_LONG, 0},
  {"TCP_KEEPIDLE", CURLOPT_TCP_KEEPIDLE, CURLOT_LONG, 0},
  {"TCP_KEEPINTVL", CURLOPT_TCP_KEEPINTVL, CURLOT_LONG, 0},
  {"TCP_NODELAY", CURLOPT_TCP_NODELAY, CURLOT_LONG, 0},
  {"TELNETOPTIONS", CURLOPT_TELNETOPTIONS, CURLOT_SLIST, 0},
  {"TFTP_BLKSIZE", CURLOPT_TFTP_BLKSIZE, CURLOT_LONG, 0},
  {"TFTP_NO_OPTIONS", CURLOPT_TFTP_NO_OPTIONS, CURLOT_LONG, 0},
  {"TIMECONDITION", CURLOPT_TIMECONDITION, CURLOT_VALUES, 0},
  {"TIMEOUT", CURLOPT_TIMEOUT, CURLOT_LONG, 0},
  {"TIMEOUT_MS", CURLOPT_TIMEOUT_MS, CURLOT_LONG, 0},
  {"TIMEVALUE", CURLOPT_TIMEVALUE, CURLOT_LONG, 0},
  {"TIMEVALUE_LARGE", CURLOPT_TIMEVALUE_LARGE, CURLOT_OFF_T, 0},
  {"TLS13_CIPHERS", CURLOPT_TLS13_CIPHERS, CURLOT_STRING, 0},
  {"TLSAUTH_PASSWORD", CURLOPT_TLSAUTH_PASSWORD, CURLOT_STRING, 0},
  {"TLSAUTH_TYPE", CURLOPT_TLSAUTH_TYPE, CURLOT_STRING, 0},
  {"TLSAUTH_USERNAME", CURLOPT_TLSAUTH_USERNAME, CURLOT_STRING, 0},
  {"TRAILERDATA", CURLOPT_TRAILERDATA, CURLOT_CBPTR, 0},
  {"TRAILERFUNCTION", CURLOPT_TRAILERFUNCTION, CURLOT_FUNCTION, 0},
  {"TRANSFERTEXT", CURLOPT_TRANSFERTEXT, CURLOT_LONG, 0},
  {"TRANSFER_ENCODING", CURLOPT_TRANSFER_ENCODING, CURLOT_LONG, 0},
  {"UNIX_SOCKET_PATH", CURLOPT_UNIX_SOCKET_PATH, CURLOT_STRING, 0},
  {"UNRESTRICTED_AUTH", CURLOPT_UNRESTRICTED_AUTH, CURLOT_LONG, 0},
  {"UPKEEP_INTERVAL_MS", CURLOPT_UPKEEP_INTERVAL_MS, CURLOT_LONG, 0},
  {"UPLOAD", CURLOPT_UPLOAD, CURLOT_LONG, 0},
  {"UPLOAD_BUFFERSIZE", CURLOPT_UPLOAD_BUFFERSIZE, CURLOT_LONG, 0},
  {"URL", CURLOPT_URL, CURLOT_STRING, 0},
  {"USERAGENT", CURLOPT_USERAGENT, CURLOT_STRING, 0},
  {"USERNAME", CURLOPT_USERNAME, CURLOT_STRING, 0},
  {"USERPWD", CURLOPT_USERPWD, CURLOT_STRING, 0},
  {"USE_SSL", CURLOPT_USE_SSL, CURLOT_VALUES, 0},
  {"VERBOSE", CURLOPT_VERBOSE, CURLOT_LONG, 0},
  {"WILDCARDMATCH", CURLOPT_WILDCARDMATCH, CURLOT_LONG, 0},
  {"WRITEDATA", CURLOPT_WRITEDATA, CURLOT_CBPTR, 0},
  {"WRITEFUNCTION", CURLOPT_WRITEFUNCTION, CURLOT_FUNCTION, 0},
  {"WRITEHEADER", CURLOPT_HEADERDATA, CURLOT_CBPTR, CURLOT_FLAG_ALIAS},
  {"XFERINFODATA", CURLOPT_XFERINFODATA, CURLOT_CBPTR, 0},
  {"XFERINFOFUNCTION", CURLOPT_XFERINFOFUNCTION, CURLOT_FUNCTION, 0},
  {"XOAUTH2_BEARER", CURLOPT_XOAUTH2_BEARER, CURLOT_STRING, 0},
  {NULL, CURLOPT_LASTENTRY, 0, 0} /* end of table */
};

#ifdef DEBUGBUILD
/*
 * Curl_easyopts_check() is a debug-only function that returns non-zero
 * if this source file is not in sync with the options listed in curl/curl.h
 */
int Curl_easyopts_check(void)
{
  return ((CURLOPT_LASTENTRY%10000) != (315 + 1));
}
#endif
