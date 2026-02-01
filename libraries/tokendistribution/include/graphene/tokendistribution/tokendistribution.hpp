/*
 * AcloudBank
 *
 */

#pragma once

#include <string>

namespace graphene { namespace tokendistribution {

void preparePubKey(std::string* pubKey);

void prepareSignature(std::string& pubKey);

std::string getAddress(std::string pubKey);

int verifyMessage (std::string pubKey, std::string msg, std::string sig);

} }
