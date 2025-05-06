/*
 * hse_api.h
 *
 *  Created on: May 2, 2025
 */

#ifndef HSE_API_H_
#define HSE_API_H_

#include <stdint.h>

/* HSE Signature Verification Parameters */
typedef struct {
    uint32_t keyIndex;
    uint32_t signatureType;
    uint32_t signatureAddr;
    uint32_t signatureSize;
    uint32_t dataAddr;
    uint32_t dataSize;
} HSE_SignatureVerifyParams_t;

/* Function prototypes for HSE API */
uint32_t HSE_SignatureVerify(HSE_SignatureVerifyParams_t *params);

#endif /* HSE_API_H_ */
