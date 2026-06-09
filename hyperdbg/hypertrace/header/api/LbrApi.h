/**
 * @file LbrApi.h
 * @author Hari Mishal (harimishal6@gmail.com)
 * @brief Header for LBR tracing routines for HyperTrace module (Intel Last Branch Record)
 * @details
 * @version 0.18
 * @date 2025-12-02
 *
 * @copyright This project is released under the GNU Public License v3.
 */
#pragma once

//////////////////////////////////////////////////
//			   	   Functions    		    	//
//////////////////////////////////////////////////

VOID
HyperTraceLbrExamplePerformTrace();

BOOLEAN
HyperTraceLbrDisable(HYPERTRACE_LBR_OPERATION_PACKETS * HyperTraceOperationRequest);
