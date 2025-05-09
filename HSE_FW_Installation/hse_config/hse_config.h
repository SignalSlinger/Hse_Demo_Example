/*
 * hse_config.h
 *
 *  Created on: May 2, 2025
 */

#ifndef HSE_CONFIG_H_
#define HSE_CONFIG_H_

#include <stdint.h>
#include "hse_dcm_register.h"
#include "hse_api.h"  /* Include the HSE API definitions */
#include "fallback_manager.h"
#include "flash_programming.h"

/* HSE Status Codes */
#define HSE_STATUS_SUCCESS        0x00000000U
#define HSE_STATUS_FAILURE        0x00000001U
#define HSE_STATUS_BUSY           0x00000002U
#define HSE_STATUS_READY          0x00000003U
#define HSE_STATUS_MASK           0x0000000FU
#define HSE_STATUS_INIT_ERROR     0x00000004U
#define HSE_STATUS_VERSION_ERROR  0x00000005U
#define HSE_STATUS_INTEGRITY_FAILURE 0x00000006U


/* HSE Error Codes */
#define HSE_ERR_NONE              0x00000000U
#define HSE_ERR_GENERAL           0x00000001U
#define HSE_ERR_AUTH_FAILED       0x00000002U
#define HSE_ERR_KEY_NOT_FOUND     0x00000003U

/* HSE Key Indexes */
#define HSE_ECC_PUBLIC_KEY_INDEX  0x00000001U

/* HSE Signature Schemes */
#define HSE_SIGNATURE_SCHEME_ECDSA_P256 0x00000001U

/* Function prototypes */
uint32_t HSE_CheckFirmwareStatus(void);
uint32_t HSE_VerifyAppSignature(void);
uint32_t HSE_SecureBoot_Init(void);
uint32_t HSE_PrepareSecureBoot(void);
uint32_t HSE_AttemptFallbackBoot(void);
uint32_t HSE_GetLastError(void);
uint32_t HSE_PerformIntegrityCheck(void);
uint32_t HSE_UpdateSignatures(const uint8_t* newSignature, uint32_t signatureSize);
uint32_t HSE_VerifyFallbackSignature(void);

/* External variables from signature files */
extern const uint8_t g_appSignature[];
extern const uint32_t g_appSignatureSize;

#endif /* HSE_CONFIG_H_ */
