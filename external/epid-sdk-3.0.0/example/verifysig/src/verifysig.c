/*############################################################################
  # Copyright 2016 Intel Corporation
  #
  # Licensed under the Apache License, Version 2.0 (the "License");
  # you may not use this file except in compliance with the License.
  # You may obtain a copy of the License at
  #
  #     http://www.apache.org/licenses/LICENSE-2.0
  #
  # Unless required by applicable law or agreed to in writing, software
  # distributed under the License is distributed on an "AS IS" BASIS,
  # WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  # See the License for the specific language governing permissions and
  # limitations under the License.
  ############################################################################*/

/*!
 * \file
 * \brief Signature verification implementation.
 */

#include "src/verifysig.h"

#include <stdlib.h>

#include "util/buffutil.h"
#include "util/envutil.h"
#include "epid/verifier/api.h"
#include "epid/common/file_parser.h"

bool IsCaCertAuthorizedByRootCa(void const* data, size_t size) {
  // Implementation of this function is out of scope of the sample.
  // In an actual implementation Issuing CA certificate must be validated
  // with CA Root certificate before using it in parse functions.
  (void)data;
  (void)size;
  return true;
}

EpidStatus Verify(EpidSignature const* sig, size_t sig_len, void const* msg,
                  size_t msg_len, void const* basename, size_t basename_len,
                  void const* signed_priv_rl, size_t signed_priv_rl_size,
                  void const* signed_sig_rl, size_t signed_sig_rl_size,
                  void const* signed_grp_rl, size_t signed_grp_rl_size,
                  VerifierRl const* ver_rl, size_t ver_rl_size,
                  void const* signed_pub_key, size_t signed_pub_key_size,
                  EpidCaCertificate const* cacert, HashAlg hash_alg,
                  VerifierPrecomp* verifier_precomp,
                  bool verifier_precomp_is_input) {
  EpidStatus result = kEpidErr;
  VerifierCtx* ctx = NULL;
  PrivRl* priv_rl = NULL;
  SigRl* sig_rl = NULL;
  GroupRl* grp_rl = NULL;

  do {
    GroupPubKey pub_key = {0};
    // authenticate and extract group public key
    result = EpidParseGroupPubKeyFile(signed_pub_key, signed_pub_key_size,
                                      cacert, &pub_key);
    if (kEpidNoErr != result) {
      break;
    }

    // create verifier
    result = EpidVerifierCreate(
        &pub_key, verifier_precomp_is_input ? verifier_precomp : NULL, &ctx);
    if (kEpidNoErr != result) {
      break;
    }

    // serialize verifier pre-computation blob
    result = EpidVerifierWritePrecomp(ctx, verifier_precomp);
    if (kEpidNoErr != result) {
      break;
    }

    // set hash algorithm used for signing
    result = EpidVerifierSetHashAlg(ctx, hash_alg);
    if (kEpidNoErr != result) {
      break;
    }

    // set the basename used for signing
    result = EpidVerifierSetBasename(ctx, basename, basename_len);
    if (kEpidNoErr != result) {
      break;
    }

    if (signed_priv_rl) {
      // authenticate and determine space needed for RL
      size_t priv_rl_size = 0;
      result = EpidParsePrivRlFile(signed_priv_rl, signed_priv_rl_size, cacert,
                                   NULL, &priv_rl_size);
      if (kEpidSigInvalid == result) {
        // authentication failure
        break;
      }
      if (kEpidNoErr != result) {
        break;
      }

      priv_rl = AllocBuffer(priv_rl_size);
      if (!priv_rl) {
        result = kEpidMemAllocErr;
        break;
      }

      // fill the rl
      result = EpidParsePrivRlFile(signed_priv_rl, signed_priv_rl_size, cacert,
                                   priv_rl, &priv_rl_size);
      if (kEpidNoErr != result) {
        break;
      }

      // set private key based revocation list
      result = EpidVerifierSetPrivRl(ctx, priv_rl, priv_rl_size);
      if (kEpidNoErr != result) {
        break;
      }
    }  // if (signed_priv_rl)

    if (signed_sig_rl) {
      // authenticate and determine space needed for RL
      size_t sig_rl_size = 0;
      result = EpidParseSigRlFile(signed_sig_rl, signed_sig_rl_size, cacert,
                                  NULL, &sig_rl_size);
      if (kEpidSigInvalid == result) {
        // authentication failure
        break;
      }
      if (kEpidNoErr != result) {
        break;
      }

      sig_rl = AllocBuffer(sig_rl_size);
      if (!sig_rl) {
        result = kEpidMemAllocErr;
        break;
      }

      // fill the rl
      result = EpidParseSigRlFile(signed_sig_rl, signed_sig_rl_size, cacert,
                                  sig_rl, &sig_rl_size);
      if (kEpidNoErr != result) {
        break;
      }

      // set signature based revocation list
      result = EpidVerifierSetSigRl(ctx, sig_rl, sig_rl_size);
      if (kEpidNoErr != result) {
        break;
      }
    }  // if (signed_sig_rl)

    if (signed_grp_rl) {
      // authenticate and determine space needed for RL
      size_t grp_rl_size = 0;
      result = EpidParseGroupRlFile(signed_grp_rl, signed_grp_rl_size, cacert,
                                    NULL, &grp_rl_size);
      if (kEpidSigInvalid == result) {
        // authentication failure
        break;
      }
      if (kEpidNoErr != result) {
        break;
      }

      grp_rl = AllocBuffer(grp_rl_size);
      if (!grp_rl) {
        result = kEpidMemAllocErr;
        break;
      }

      // fill the rl
      result = EpidParseGroupRlFile(signed_grp_rl, signed_grp_rl_size, cacert,
                                    grp_rl, &grp_rl_size);
      if (kEpidNoErr != result) {
        break;
      }
      // set group revocation list
      result = EpidVerifierSetGroupRl(ctx, grp_rl, grp_rl_size);
      if (kEpidNoErr != result) {
        break;
      }
    }  // if (signed_grp_rl)

    if (ver_rl) {
      // set verifier based revocation list
      result = EpidVerifierSetVerifierRl(ctx, ver_rl, ver_rl_size);
      if (kEpidNoErr != result) {
        break;
      }
    }

    // verify signature
    result = EpidVerify(ctx, sig, sig_len, msg, msg_len);
    if (kEpidNoErr != result) {
      break;
    }
  } while (0);

  // delete verifier
  EpidVerifierDelete(&ctx);

  if (priv_rl) free(priv_rl);
  if (sig_rl) free(sig_rl);
  if (grp_rl) free(grp_rl);

  return result;
}
