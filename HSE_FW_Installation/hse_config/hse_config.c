/*
 * hse_config.c
 *
 *  Created on: May 2, 2025
 */

#include "hse_config.h"
#include "public_key.h"
#include "signature.h"
#include "advanced_security.h"
#include "boot_recovery.h"
#include "Siul2_Port_Ip.h" // For Port initialization
#include "Siul2_Dio_Ip.h"  // For LED control

/* Define the start address of application binary in flash */
#define APP_BINARY_START_ADDR     0x00400000
#define APP_BINARY_SIZE           0x002E0000 // 2.875MB

/* HSE error tracking */
static uint32_t g_lastHseError = HSE_STATUS_FAILURE;

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
	else
	{
		/* Log HSE firmware status failure */
		Boot_LogStatus(BOOT_STATUS_HSE_FAILURE, firmwareStatus);
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
       uint32_t hseResult = HSE_SignatureVerify(&verifyParams);
       if (HSE_ERR_NONE == hseResult) {
           status = HSE_STATUS_SUCCESS;
       }
       else
       {
           /* Log signature verification failure */
           Boot_LogStatus(BOOT_STATUS_SIGNATURE_FAILURE, hseResult);
           g_lastHseError = hseResult;
       }

    return status;
}

/* Verify fallback firmware signature */
uint32_t HSE_VerifyFallbackSignature(void)
{
    uint32_t status = HSE_STATUS_FAILURE;
    HSE_SignatureVerifyParams_t verifyParams;

    /* Get fallback metadata to determine firmware size */
    const fallback_metadata_t* metadata = Fallback_GetMetadata();
    if (metadata == NULL) {
        /* Invalid metadata */
        Boot_LogStatus(BOOT_STATUS_METADATA_INVALID, 0);
        return HSE_STATUS_FAILURE;
    }

    /* Set up verification parameters for fallback firmware */
    verifyParams.keyIndex = HSE_ECC_PUBLIC_KEY_INDEX;
    verifyParams.signatureType = HSE_SIGNATURE_SCHEME_ECDSA_P256;
    verifyParams.signatureAddr = FALLBACK_FIRMWARE_SIGNATURE_ADDR;
    verifyParams.signatureSize = g_appSignatureSize;
    verifyParams.dataAddr = FALLBACK_FIRMWARE_ADDR;
    verifyParams.dataSize = metadata->firmwareSize;

    /* Call HSE API to verify the fallback signature */
    uint32_t hseResult = HSE_SignatureVerify(&verifyParams);
    if (HSE_ERR_NONE == hseResult) {
        status = HSE_STATUS_SUCCESS;

        /* Log successful verification */
        Boot_LogStatus(BOOT_STATUS_SIGNATURE_VALID, 0x8000); /* Mark as fallback */
    }
    else {
        /* Log fallback signature verification failure */
        Boot_LogStatus(BOOT_STATUS_SIGNATURE_FAILURE, hseResult | 0x8000); /* Mark as fallback error */
        g_lastHseError = hseResult;
    }

    return status;
}

/* Configure secure debug access policies */
uint32_t HSE_ConfigureDebugAccess(void)
{
    /* Create debug configuration */
    debug_config_t debugConfig;

    #ifdef DEBUG_BUILD
        debugConfig.debugLevel = DEBUG_LEVEL_LIMITED;
    #else
        debugConfig.debugLevel = DEBUG_LEVEL_DISABLED;
    #endif

    debugConfig.passwordProtected = 1;
    debugConfig.timeoutEnabled = 1;
    debugConfig.debugTimeout = 3600; /* 1 hour timeout */

    /* Apply debug configuration */
    return Security_ConfigureDebug(debugConfig);
}

/* Activate runtime integrity checking */
uint32_t HSE_SetupRuntimeIntegrity(void)
{
    /* Start periodic integrity monitoring */
    uint32_t status = Security_StartIntegrityMonitor();

    /* Perform initial integrity check */
    if (status == 0 && Security_CheckIntegrity() != 0) {
        /* Initial integrity check failed */
        Boot_LogStatus(BOOT_STATUS_MEMORY_FAILURE, 0);
        status = HSE_STATUS_INTEGRITY_FAILURE;
    }

    return status;
}

/* Set up memory protection for secure regions */
uint32_t HSE_ConfigureMemoryProtection(void)
{
    /* In a real implementation, this would configure memory protection units
       and set up execute never regions based on device-specific capabilities */

    /* Example placeholder implementation */
    return HSE_STATUS_SUCCESS;
}

/* Attempt recovery by loading fallback firmware */
uint32_t HSE_AttemptFallbackBoot(void)
{
    uint32_t status;

    /* Initialize fallback manager */
    status = Fallback_Init();
    if (status != FALLBACK_STATUS_SUCCESS) {
        /* Fallback manager initialization failed */
        Boot_LogStatus(BOOT_STATUS_FACTORY_RESET, 0);
        return HSE_STATUS_FAILURE;
    }

    /* Check if we have a valid fallback firmware */
    if (!Fallback_IsValid()) {
        /* No valid fallback firmware available */
        Boot_LogStatus(BOOT_STATUS_FACTORY_RESET, 1);

        /* Visual indicator for critical failure */
        for (uint32_t i = 0; i < 10; i++) {
            Siul2_Dio_Ip_TogglePins(LED_RED_PORT, (1U << LED_RED_PIN));
            Siul2_Dio_Ip_TogglePins(LED_GREEN_PORT, (1U << LED_GREEN_PIN));

            /* Delay */
            volatile uint32_t delay;
            for (delay = 0; delay < 200000; delay++) {}
        }

        return HSE_STATUS_FAILURE;
    }

    /* Log fallback boot activation */
    Boot_LogStatus(BOOT_STATUS_RECOVERY_ACTIVE, 0);

    /* Attempt to recover using fallback firmware */
    status = Fallback_RecoverMainFirmware();

    if (status != FALLBACK_STATUS_SUCCESS) {
        /* Recovery failed, try direct boot from fallback */
        Fallback_JumpToFallbackFirmware();
        /* Should not return */
        return HSE_STATUS_FAILURE;
    }

    /* Recovery successful, reset to boot from recovered firmware */
    volatile uint32_t* resetReg = (volatile uint32_t*)0xE000ED0C;
    *resetReg = 0x5FA0004;

    /* Should not reach here */
    return HSE_STATUS_SUCCESS;
}

/* Initialize Secure Boot Configuration */
uint32_t HSE_SecureBoot_Init(void)
{
    uint32_t status = HSE_STATUS_FAILURE;

    /* Initialize boot recovery system */
    Boot_RecoveryInit();

    /* Initialize anti-rollback protection */
    status = Security_InitAntiRollback();
    if (status != 0) {
        Boot_LogStatus(BOOT_STATUS_HSE_FAILURE, (uint16_t)status);
        return HSE_STATUS_INIT_ERROR;
    }

    /* Get current firmware version */
    firmware_version_t version = Security_GetFirmwareVersion();

    /* Check if version is allowed by anti-rollback policy */
    if (Security_CheckVersion(version) != 0) {
        /* Version is too old, violates anti-rollback policy */
        Boot_LogStatus(BOOT_STATUS_ROLLBACK_DETECTED, 0);
        return HSE_STATUS_VERSION_ERROR;
    }

    /* Check if HSE firmware is ready */
    if (HSE_STATUS_SUCCESS == HSE_CheckFirmwareStatus())
    {
        /* HSE is ready, verify application signature */
        if (HSE_STATUS_SUCCESS == HSE_VerifyAppSignature())
        {
            /* Configure secure debug access */
            HSE_ConfigureDebugAccess();

            /* Set up memory protection */
            HSE_ConfigureMemoryProtection();

            /* Enable runtime integrity checks */
            HSE_SetupRuntimeIntegrity();

            /* Signature verified, prepare secure boot configuration */
            status = HSE_PrepareSecureBoot();

            /* Log successful boot */
            if (status == HSE_STATUS_SUCCESS) {
                Boot_LogStatus(BOOT_STATUS_SUCCESS, 0);
            }
        }
        else {
            /* Signature verification failed - try recovery */
            boot_recovery_ctx_t* recoveryCtx = Boot_GetRecoveryContext();

            if (recoveryCtx->consecutiveFailures >= MAX_BOOT_ATTEMPTS) {
                /* Too many failures - try fallback firmware */
                status = HSE_AttemptFallbackBoot();
                if (status == HSE_STATUS_SUCCESS) {
                    /* Fallback boot successful - reset device */
                    /* Use device-specific reset mechanism */
                    volatile uint32_t* resetReg = (volatile uint32_t*)0xE000ED0C;
                    *resetReg = 0x5FA0004;
                }
            } else {
                /* Log failure and return error to trigger recovery */
                status = HSE_STATUS_FAILURE;
            }
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

/* Get most recent HSE error code */
uint32_t HSE_GetLastError(void)
{
    return g_lastHseError;
}

/* Perform runtime integrity verification */
uint32_t HSE_PerformIntegrityCheck(void)
{
    return Security_CheckIntegrity();
}

/* Update both main app and fallback signatures */
uint32_t HSE_UpdateSignatures(const uint8_t* newSignature, uint32_t signatureSize)
{
    uint32_t status;

    /* Validate parameters */
    if (newSignature == NULL || signatureSize == 0) {
        return HSE_STATUS_FAILURE;
    }

    /* Update main application signature */
    /* This would typically involve updating the signature area in flash */
    /* Placeholder implementation - depends on how signatures are stored */

    /* Update fallback signature */
    status = Flash_EraseSector(FALLBACK_FIRMWARE_SIGNATURE_ADDR, FALLBACK_FIRMWARE_SIGNATURE_SIZE);
    if (status != 0) {
        return HSE_STATUS_FAILURE;
    }

    status = Flash_Program(FALLBACK_FIRMWARE_SIGNATURE_ADDR, newSignature, signatureSize);
    if (status != 0) {
        return HSE_STATUS_FAILURE;
    }

    /* Verify the signature was written correctly */
    for (uint32_t i = 0; i < signatureSize; i++) {
        if (((const uint8_t*)FALLBACK_FIRMWARE_SIGNATURE_ADDR)[i] != newSignature[i]) {
            return HSE_STATUS_FAILURE;
        }
    }

    return HSE_STATUS_SUCCESS;
}
