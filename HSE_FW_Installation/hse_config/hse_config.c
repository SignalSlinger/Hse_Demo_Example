/*
 * hse_config.c
 *
 *  Created on: May 2, 2025
 */

#include "hse_config.h"
#include "public_key.h"
#include "signature.h"

/* Define the start address of application binary in flash */
#define APP_BINARY_START_ADDR     0x00400000
#define APP_BINARY_SIZE           0x00010000 // 64kb

/* Check if HSE Firmware is running correctly */
uint32_t HSE_CheckFirmwareStatus(void)
{
    uint32_t status = HSE_STATUS_FAILURE;
    uint32_t firmwareStatus;

    /* Read HSE firmware status */
    firmwareStatus = (HSE_DMC_GetStatus() & HSE_STATUS_MASK);

    /* Check if HSE firmware is running properly */
    if (HSE_STATUS_READY == firmwareStatus)
    {
        status = HSE_STATUS_SUCCESS;
    }

    return status;
}

/* Verify application signature using HSE */
uint32_t HSE_VerifyAppSignature(void)
{
    uint32_t status = HSE_STATUS_FAILURE;
    HSE_SignatureVerifyParams_t verifyParams;

    /* Set up verification parameters */
    verifyParams.keyIndex = HSE_ECC_PUBLIC_KEY_INDEX; /* Index where public key is stored in HSE */
    verifyParams.signatureType = HSE_SIGNATURE_SCHEME_ECDSA_P256;
    verifyParams.signatureAddr = (uint32_t) g_appSignature;
    verifyParams.signatureSize = g_appSignatureSize;
    verifyParams.dataAddr = APP_BINARY_START_ADDR;
    verifyParams.dataSize = APP_BINARY_SIZE;

    /* Call HSE API to verify the signature */
    if (HSE_ERR_NONE == HSE_SignatureVerify(&verifyParams)) {
        status = HSE_STATUS_SUCCESS;
    }

    return status;
}

/* Initialize Secure Boot Configuration */
uint32_t HSE_SecureBoot_Init(void)
{
    uint32_t status = HSE_STATUS_FAILURE;

    /* Check if HSE firmware is ready */
    if (HSE_STATUS_SUCCESS == HSE_CheckFirmwareStatus())
    {
        /* HSE is ready, verify application signature */
        if (HSE_STATUS_SUCCESS == HSE_VerifyAppSignature())
        {
            /* Signature verified, prepare secure boot configuration */
            status = HSE_PrepareSecureBoot();
        }
    }

    return status;
}

/* Prepare the device for secure boot */
uint32_t HSE_PrepareSecureBoot(void)
{
    uint32_t status = HSE_STATUS_SUCCESS;

    /* Additional secure boot initialization steps would go here */

    return status;
}
