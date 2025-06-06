/*****************************************************************************
 *
 * Copyright 2020-2022 NXP
 * All Rights Reserved
 *
 *****************************************************************************
 *
 * THIS SOFTWARE IS PROVIDED BY NXP "AS IS" AND ANY EXPRESSED OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL NXP OR ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/*
* GCC Linker Command File:
* 0x00400000    0x007FFFFF  4194304 Program Flash (last 64K sBAF)
* 0x10000000    0x1003FFFF  262144  Data Flash (last 32K HSE_NVM)
* 0x20400000    0x20408000  32768   Standby RAM_0 (32K)
* 0x20400000    0x20427FFF  163840  SRAM_0
* 0x20428000    0x2044FFFF  163840  SRAM_1
* Last 48 KB of SRAM_1 reserved by HSE Firmware
* Last 176 KB of CODE_FLASH_3 reserved by HSE Firmware
* Last 128 KB of DATA_FLASH reserved by HSE Firmware (not supported in this linker file)
* 
* For configuration APP it will be used BLOCK 0
* 0x00400000	0x00401FFF	8192 	IVT
* 0x00402000	0x0043FFFF	253952 	Secure Boot Configuration App
* 0x00440000	0x00441FFF	8192 	Authentication Tag
* 0x004420C0	0x004D4000	597824 	Application
*/
__STACK_SIZE = 0x00001000;
__HEAP_SIZE  = 0x00001000;

/* Linker script to configure memory regions. */
MEMORY
{
  ITCM       (RWX) : ORIGIN = 0x00000000, LENGTH = 0x10000
  IVT 		  (RX) : ORIGIN = 0x00400000, LENGTH = 0x2000 	/*IVT 0*/
  PFLASH	  (RX) : ORIGIN = 0x00402000, LENGTH = 0x3E000 	/*CONFIG APP*/
  AUTH_TAG    (RX) : ORIGIN = 0x00440000, LENGTH = 0x2000 	/*AUTH TAG*/
  APP_0		  (RX) : ORIGIN = 0x004420C0, LENGTH = 0x91F40  /*APPLICATION*/
  DFLASH      (RX) : ORIGIN = 0x10000000, LENGTH = 0x20000
  DTCM        (RW) : ORIGIN = 0x20000000, LENGTH = 0x20000
  SRAM0_STDBY (RW) : ORIGIN = 0x20400000, LENGTH = 0x8000
  SRAM        (RW) : ORIGIN = 0x20408000, LENGTH = 0x48000
}

/* Linker script to place sections and symbol values. Should be used together
 * with other linker script that defines memory regions FLASH and RAM.
 * It references following symbols, which must be defined in code:
 *   Reset_Handler : Entry of reset handler
 *
 * It defines following symbols, which code can use without definition:
 *   __exidx_start
 *   __exidx_end
 *   __ecc_table_start__
 *   __ecc_table_end__
 *   __etext
 *   __data_start__
 *   __preinit_array_start
 *   __preinit_array_end
 *   __init_array_start
 *   __init_array_end
 *   __fini_array_start
 *   __fini_array_end
 *   __data_end__
 *   __bss_start__
 *   __bss_end__
 *   __end__
 *   end
 *   __HeapLimit
 *   __StackLimit
 *   __StackTop
 *   __stack
 */
ENTRY(Reset_Handler)

/* specify the file format of binary file */
/**/
TARGET(binary)
/*Specify your binary path*/
INPUT (..\S32K344_SecureBoot_AppBL\Debug_FLASH\S32K344_SecureBoot_AppBL.bin) /*MODIFY TO YOUR ADDRESS*/
OUTPUT_FORMAT(default)
/**/

SECTIONS
{

/*Describes sections for Secure Boot APP*/

/*Authentication Tag Section*/
  .int_flash_auth_tag_0 :
  {
      __int_flash_auth_tag_0_start__ = .;
      KEEP(*(._app_auth_tag_0))
      . = ALIGN(0x2000);
      __int_flash_auth_tag_0_end__ = .;
  } > AUTH_TAG
   
  __int_flash_app_0_auth_tag_size = __int_flash_auth_tag_0_end__ - __int_flash_auth_tag_0_start__;
   
/*Application Section*/
  .int_flash_app_0 :
  {
      . = ALIGN(4);
      __int_flash_app_bin_0_start__ = .;
      ..\S32K344_SecureBoot_AppBL\Debug_FLASH\S32K344_SecureBoot_AppBL.bin (.data) /*MODIFY TO YOUR ADDRESS*/
      __int_flash_app_bin_0_end__ = .;
  } > APP_0
  
  __APPBL_START = ORIGIN(APP_0);
  __int_flash_app_0_size = __int_flash_app_bin_0_end__ - __int_flash_app_bin_0_start__;
  __APPBL_SIZE = __int_flash_app_0_size;
    
/*END: Describes sections for Secure Boot APP*/


  .text :
  {
    
    KEEP(*(.vectors))
    *(.text*)

    KEEP(*(.init))
    KEEP(*(.fini))

    /* .ctors */
    *crtbegin.o(.ctors)
    *crtbegin?.o(.ctors)
    *(EXCLUDE_FILE(*crtend?.o *crtend.o) .ctors)
    *(SORT(.ctors.*))
    *(.ctors)

    /* .dtors */
     *crtbegin.o(.dtors)
     *crtbegin?.o(.dtors)
     *(EXCLUDE_FILE(*crtend?.o *crtend.o) .dtors)
     *(SORT(.dtors.*))
     *(.dtors)

    *(.rodata*)
    KEEP(*(.eh_frame*))
  } > PFLASH

  .ARM.extab :
  {
    *(.ARM.extab* .gnu.linkonce.armextab.*)
  } > PFLASH

  __exidx_start = .;
  .ARM.exidx :
  {
    *(.ARM.exidx* .gnu.linkonce.armexidx.*)
  } > PFLASH
  __exidx_end = .;

  .ecc.table :
  {
    . = ALIGN(4);
    __ecc_table_start__ = .;

    QUAD (__data_start__)
    QUAD ((__data_end__ - __data_start__) / 8)

    QUAD (__bss_start__)
    QUAD ((__bss_end__ - __bss_start__) / 8)

    QUAD (__HeapTop)
    QUAD ((__HeapLimit - __HeapTop) / 8)

    QUAD (__StackLimit)
    QUAD ((__StackTop - __StackLimit) / 8)

    QUAD (ORIGIN(ITCM))
    QUAD (LENGTH(ITCM) / 8)

    QUAD (ORIGIN(DTCM))
    QUAD (LENGTH(DTCM) / 8)

    __ecc_table_end__ = .;
  } > PFLASH

  .copy.table :
  {
    . = ALIGN(4);
    __copy_table_start__ = .;

    LONG (__etext)
    LONG (__data_start__)
    LONG ((__data_end__ - __data_start__) / 4)

    /* Add each additional data section here */
    /*
      LONG (__etext2)
      LONG (__data2_start__)
      LONG ((__data2_end__ - __data2_start__) / 4)
    */
    __copy_table_end__ = .;
  } > PFLASH

  .zero.table :
  {
    . = ALIGN(4);
    __zero_table_start__ = .;
    /* Add each additional bss section here */
    /*
      LONG (__bss2_start__)
      LONG ((__bss2_end__ - __bss2_start__) / 4)
    */
    __zero_table_end__ = .;
  } > PFLASH

  __etext = ALIGN(8);

/* Set te boot header in the IVT section*/
  .boot_header :
  {
    KEEP(*(.boot_header))
  } > IVT

  .standby_ram :
  {
    *(.standby_ram)
  } > SRAM0_STDBY

  /* Due ECC initialization sequence __data_start__ and __data_end__ should be aligned on 8 bytes */
  .data : AT (__etext)
  {
    . = ALIGN(8);
    __data_start__ = .;
    *(vtable)
    *(.data)
    *(.data.*)

    . = ALIGN(4);
    /* preinit data */
    PROVIDE_HIDDEN (__preinit_array_start = .);
    KEEP(*(.preinit_array))
    PROVIDE_HIDDEN (__preinit_array_end = .);

    . = ALIGN(4);
    /* init data */
    PROVIDE_HIDDEN (__init_array_start = .);
    KEEP(*(SORT(.init_array.*)))
    KEEP(*(.init_array))
    PROVIDE_HIDDEN (__init_array_end = .);

    . = ALIGN(4);
    /* finit data */
    PROVIDE_HIDDEN (__fini_array_start = .);
    KEEP(*(SORT(.fini_array.*)))
    KEEP(*(.fini_array))
    PROVIDE_HIDDEN (__fini_array_end = .);

    KEEP(*(.jcr*))
    . = ALIGN(8);
    /* All data end */
    __data_end__ = .;

  } > SRAM

  /* Due ECC initialization sequence __bss_start__ and __bss_end__ should be aligned on 8 bytes */
  .bss :
  {
    . = ALIGN(8);
    __bss_start__ = .;
    *(.bss)
    *(.bss.*)
    *(COMMON)
    . = ALIGN(8);
    __bss_end__ = .;
  } > SRAM

  /* Due ECC initialization sequence __HeapTop and __HeapLimit should be aligned on 8 bytes */
  .heap (COPY):
  {
    . = ALIGN(8);
    __HeapTop = .;
    __end__ = .;
    _end = .;
    PROVIDE(end = .);
    . = . + __HEAP_SIZE;
    . = ALIGN(8);
    __HeapLimit = .;
  } > SRAM

  /* Due ECC initialization sequence __StackLimit and __StackTop should be aligned on 8 bytes */
  .stack (ORIGIN(SRAM) + LENGTH(SRAM) - __STACK_SIZE) (COPY) :
  {
    . = ALIGN(8);
    __StackLimit = .;
    . = . + __STACK_SIZE;
    . = ALIGN(8);
    __StackTop = .;
  } > SRAM
  PROVIDE(__stack = __StackTop);

  /* Check if data + heap + stack exceeds RAM limit */
  ASSERT(__StackLimit >= __HeapLimit, "region RAM overflowed with stack")
  CM7_START_ADDRESS = ORIGIN(PFLASH);
  __STDBYRAM_START = ORIGIN(SRAM0_STDBY);
  __STDBYRAM_SIZE = LENGTH(SRAM0_STDBY);
}
