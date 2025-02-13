/**
 * @file Pci.c
 * @author Björn Ruytenberg (bjorn@bjornweb.nl)
 * @brief Headers related to interacting with PCI(e) fabric
 * @details
 * @version 0.10.3
 * @date 2024-11-11
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once
#include "pch.h"

//////////////////////////////////////////////////
//				   Definition					//
//////////////////////////////////////////////////

#define CFGADR 0xCF8
#define CFGDAT 0xCFC

//////////////////////////////////////////////////
//				   Functions					//
//////////////////////////////////////////////////

QWORD
PciReadCam(WORD Bus, WORD Device, WORD Function, BYTE Offset, UINT8 Width);
BOOLEAN
PciWriteCam(WORD Bus, WORD Device, WORD Function, BYTE Offset, UINT8 Width, QWORD Value);
