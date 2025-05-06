@echo off
setlocal enabledelayedexpansion

REM Get formatted UTC date and time
for /f "tokens=2 delims==" %%a in ('wmic os get localdatetime /value') do set dt=%%a
set YYYY=%dt:~0,4%
set MM=%dt:~4,2%
set DD=%dt:~6,2%
set HH=%dt:~8,2%
set Min=%dt:~10,2%
set Sec=%dt:~12,2%
set FORMATTED_DATE=%YYYY%-%MM%-%DD% %HH%:%Min%:%Sec%

echo S32K344 Secure Boot Key Generator and Binary Signer
echo =================================================
echo.
echo Current Date and Time (UTC - YYYY-MM-DD HH:MM:SS formatted): %FORMATTED_DATE%

REM Set file names and paths
set DEBUG_FLASH_FOLDER=.
set PRIVATE_KEY=private_key.pem
set PUBLIC_KEY=public_key.pem
set PUBLIC_KEY_DER=public_key.der
set PUBLIC_KEY_HEADER=public_key.h
set BINARY_FILE=HSE_FW_Installation.bin
set APP_SECURE_BIN=Application_Secure.bin
set PUBLIC_KEY_BIN=public_key.bin
set SIGNATURE_FILE=HSE_FW_Installation.sig
set SIGNATURE_HEADER=signature.h
set PROJECT_HSE_CONFIG_FOLDER=..\hse_config

REM Check if binary file exists FIRST before proceeding
if not exist "%BINARY_FILE%" (
    echo ERROR: Application binary file %BINARY_FILE% not found.
    echo Make sure the binary is generated before running this script.
    exit /b 1
)

REM Create PowerShell script for binary to C array conversion
echo $inputFile = $args[0] > bin2c.ps1
echo $outputFile = $args[1] >> bin2c.ps1
echo $headerName = $args[2] >> bin2c.ps1
echo $arrayName = $args[3] >> bin2c.ps1
echo $description = $args[4] >> bin2c.ps1
echo. >> bin2c.ps1
echo $bytes = [System.IO.File]::ReadAllBytes($inputFile) >> bin2c.ps1
echo $sb = New-Object System.Text.StringBuilder >> bin2c.ps1
echo $sb.AppendLine('#ifndef ' + $headerName) >> bin2c.ps1
echo $sb.AppendLine('#define ' + $headerName) >> bin2c.ps1
echo $sb.AppendLine('') >> bin2c.ps1
echo $sb.AppendLine('#include ^<stdint.h^>') >> bin2c.ps1
echo $sb.AppendLine('') >> bin2c.ps1
echo $sb.AppendLine('/* ' + $description + ' */') >> bin2c.ps1
echo $sb.AppendLine('const uint8_t ' + $arrayName + '[] = {') >> bin2c.ps1

echo # Fixed formatting to ensure values at start of each line >> bin2c.ps1
echo $bytesPerLine = 8; >> bin2c.ps1
echo $lineCount = [Math]::Ceiling($bytes.Length / $bytesPerLine) >> bin2c.ps1
echo for ($line = 0; $line -lt $lineCount; $line++) { >> bin2c.ps1
echo     $sb.Append('    ') >> bin2c.ps1
echo     $start = $line * $bytesPerLine >> bin2c.ps1
echo     $end = [Math]::Min(($line + 1) * $bytesPerLine - 1, $bytes.Length - 1) >> bin2c.ps1
echo     for ($i = $start; $i -le $end; $i++) { >> bin2c.ps1
echo         $sb.Append('0x' + $bytes[$i].ToString('X2')) >> bin2c.ps1
echo         if ($i -lt $end) { >> bin2c.ps1
echo             $sb.Append(', ') >> bin2c.ps1
echo         } >> bin2c.ps1
echo     } >> bin2c.ps1
echo     if ($line -lt $lineCount - 1) { >> bin2c.ps1
echo         $sb.Append(',') >> bin2c.ps1
echo     } >> bin2c.ps1
echo     $sb.AppendLine('') >> bin2c.ps1
echo } >> bin2c.ps1

echo $sb.AppendLine('};') >> bin2c.ps1
echo $sb.AppendLine('') >> bin2c.ps1
echo $sb.AppendLine('const uint32_t ' + $arrayName + 'Size = sizeof(' + $arrayName + ');') >> bin2c.ps1
echo $sb.AppendLine('') >> bin2c.ps1
echo $sb.AppendLine('#endif /* ' + $headerName + ' */') >> bin2c.ps1
echo [System.IO.File]::WriteAllText($outputFile, $sb.ToString()) >> bin2c.ps1

echo Generating ECC key pair (NIST P-256)...
openssl ecparam -name prime256v1 -genkey -noout -out %PRIVATE_KEY%
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Failed to generate ECC key pair.
    pause
    exit /b 1
)

echo Extracting public key...
openssl ec -in %PRIVATE_KEY% -pubout -out %PUBLIC_KEY%
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Failed to extract public key.
    pause
    exit /b 1
)

echo Converting public key to DER format...
openssl ec -in %PUBLIC_KEY% -pubin -outform DER -out %PUBLIC_KEY_DER%
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Failed to convert public key to DER format.
    pause
    exit /b 1
)

echo Creating binary version of public key...
copy /Y "%PUBLIC_KEY_DER%" "%PUBLIC_KEY_BIN%"
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Failed to create binary version of public key.
    pause
    exit /b 1
)

echo Signing binary file...
openssl dgst -sha256 -sign %PRIVATE_KEY% -out %SIGNATURE_FILE% %BINARY_FILE%
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Failed to sign binary file.
    pause
    exit /b 1
)

echo Generating public key header file...
powershell -ExecutionPolicy Bypass -File bin2c.ps1 "%PUBLIC_KEY_DER%" "%PUBLIC_KEY_HEADER%" "PUBLIC_KEY_H" "g_eccPublicKey" "ECC P-256 Public Key"
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Failed to generate public key header.
    pause
    exit /b 1
)

echo Generating signature header file...
powershell -ExecutionPolicy Bypass -File bin2c.ps1 "%SIGNATURE_FILE%" "%SIGNATURE_HEADER%" "SIGNATURE_H" "g_appSignature" "ECDSA Signature for the application binary"
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Failed to generate signature header.
    pause
    exit /b 1
)

REM Create HSE config folder if it doesn't exist
if not exist "%PROJECT_HSE_CONFIG_FOLDER%" (
    mkdir "%PROJECT_HSE_CONFIG_FOLDER%"
    if %ERRORLEVEL% NEQ 0 (
        echo ERROR: Failed to create HSE config folder.
        pause
        exit /b 1
    )
)

REM Copy header files to project folder
copy /Y "%PUBLIC_KEY_HEADER%" "%PROJECT_HSE_CONFIG_FOLDER%\"
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Failed to copy public key header.
    pause
    exit /b 1
)

copy /Y "%SIGNATURE_HEADER%" "%PROJECT_HSE_CONFIG_FOLDER%\"
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Failed to copy signature header.
    pause
    exit /b 1
)

REM Create Application_Secure.bin from HSE_FW_Installation.bin
echo Creating %APP_SECURE_BIN% from application binary...
copy /Y "%BINARY_FILE%" "%APP_SECURE_BIN%"
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Failed to create %APP_SECURE_BIN%.
    pause
    exit /b 1
)

REM Verify all required files exist before completion
if not exist %APP_SECURE_BIN% (
    echo ERROR: Failed to create %APP_SECURE_BIN%.
    pause
    exit /b 1
)

if not exist %PUBLIC_KEY_BIN% (
    echo ERROR: Failed to create %PUBLIC_KEY_BIN%.
    pause
    exit /b 1
)

REM Display binary file info
echo.
echo ============================================================
echo BINARY FILES SUCCESSFULLY CREATED:
echo ============================================================
echo - %APP_SECURE_BIN%
echo - %PUBLIC_KEY_BIN%
echo.
echo ============================================================

REM Clean up temporary files
del bin2c.ps1

echo Key generation and file preparation completed successfully!
pause