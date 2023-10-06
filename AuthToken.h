/*
 * Copyright (C) 2023, Chris Elford
 * 
 * SPDX-License-Identifier: MIT
 *
 */

#ifndef _AUTHTOKENH
#define _AUTHTOKENH

class Config;

class AuthToken {

private:
  char *authBuf;  // Auth buffer holding auth code from server
  char *authCode; // Subset of AUTH_BUF with the actual auth code
  const char *fetchNewToken(Config *myConfig);
  
public:
  AuthToken();
  const char *getAuthToken(Config *myConfig, bool forceNew);
};

#endif
