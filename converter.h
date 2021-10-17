#ifndef _CONVERTER
#define _CONVERTER

//#include "definitions.h"

#include <NTL/ZZ.h>
#include <NTL/vec_GF2.h>
#include <NTL/vec_ZZ.h>
#include <NTL/GF2.h>

// Octal domains are as follows:
// 0,1,3,7,5,2,6,4
//-------------------------------------
// "0" -- trivial:	00							(start: 00)
// "1" -- trivial:	10							(start: 01)
// "3" -- trivial:	30							(start: 02)
// "7" -- usual:	70 71 73 77 75 72 76 74		(start: 03)
//					31 33 37 35 32 36 34		(start: 11)
// "5" -- usual:	50 51 53 57 55 52 56 54		(start: 18)
//					11 13 17 15 12 16 14		(start: 26)
// "2" -- trivial:	20							(start: 33)
// "6" -- usual:	60 61 63 67 65 62 66 64		(start: 34)
//					21 23 27 25 22 26 24		(start: 42)
// "4" -- usual:	40 41 43 47 45 42 46 44		(start: 49)
//					01 03 07 05 02 06 04  		(start: 57)
//-------------------------------------
// Total: 4*15 + 4*1 = 64

//// 0000 1000 1100 1110 1111		00 40 41 43 47
//// 1101 1010 1011 1001 0100		45 42 46 44 01
//// 0110 0111 0101 0010 0011		03 07 05 02 06
//// 0001				04

// Domain sizes for octal (b=8) domains for n=3..63
// For a given n, the field to use is (n/3)+1*!!(n%3)
/*const NTL::ZZ DomainSize[] = {
//------------------------------------------------------------------------------------
// 	            Domain Size		            Domain Level	Power of 2	    Max. n
	NTL::ZZ(  (long)1                    ),		// Lv0			1			    3
	NTL::ZZ(  (long)15                   ),		// Lv1			4			    6
	NTL::ZZ(  (long)127                  ),		// Lv2			7			    9
	NTL::ZZ(  (long)1023                 ),		// Lv3			10			    12
	NTL::ZZ(  (long)8191                 ),		// Lv4			13			    15
	NTL::ZZ(  (long)65535                ),		// Lv5			16			    18
	NTL::ZZ(  (long)524287               ),		// Lv6			19			    21
	NTL::ZZ(  (long)4194303              ),		// Lv7			22			    24
	NTL::ZZ(  (long)33554431             ),		// Lv8			25			    27
	NTL::ZZ(  (long)268435455            ),		// Lv9			28			    30
	NTL::ZZ(  (long)2147483647           ),		// Lv10			31			    33
	NTL::ZZ(  (long)17179869183          ),		// Lv11			34			    36
	NTL::ZZ(  (long)137438953471         ),		// Lv12			37			    39
	NTL::ZZ(  (long)1099511627775        ),		// Lv13			40			    42
	NTL::ZZ(  (long)8796093022207        ),		// Lv14			43			    45
	NTL::ZZ(  (long)70368744177663       ),		// Lv15			46			    48
	NTL::ZZ(  (long)562949953421311      ),		// Lv16			49			    51
	NTL::ZZ(  (long)4503599627370495     ),		// Lv17			52			    54
	NTL::ZZ(  (long)36028797018963967    ),		// Lv18			55			    57
	NTL::ZZ(  (long)288230376151711743   ),		// Lv19			58			    60
	NTL::ZZ(  (long)2305843009213693951  )		// Lv20			61			    63
//------------------------------------------------------------------------------------
};*/

// Several NODE_BASE-typed variables are used to store a large
// integral node index (varied from 0 to 2 to the power of n).
// typedef __int64 NODE_BASE;
// typedef NODE_BASE* NODE;

// This function allocates the necessary amount of memory to
// store a single node order
//
/// !!! DEVELOPMENT FROZEN
/// !!! TRANSFER CODES TO .CPP
//NODE AllocateNode(unsigned int TaskSize);
//{
//	return (NODE)malloc	(
//				(TaskSize / sizeof(NODE_BASE)
//				+ 1 * !!(TaskSize%sizeof(NODE_BASE)))
//				* sizeof(NODE_BASE)
//				);
//}

// This function shows whether more than one integer is required
// to store the node order or not. If only one is requires, built-in
// CPP math can be used and NODE may be treated as NODE_TYPE*.
// unsigned int IsOutOfBoundaries(unsigned int TaskSize);

// Domain Detection
//
// These supplementary functions work with node domains
// Recognition of node domains is essential to determine the
// node order.

// This enum is used to separate domain level numbers from
// other integral-type variables and to assign them names
enum DomainType
{
	// Simple Literals
	Domain_Lv0 = 0,
	Domain_Lv1 = 1,
	Domain_Lv3 = 3,
	Domain_Lv7 = 7,
	Domain_Lv5 = 5,
	Domain_Lv2 = 2,
	Domain_Lv6 = 6,
	Domain_Lv4 = 4,
	// Service Values
	Domain_TOPMOST = -1,
	Domain_DOWNMOST = -2
};

static NTL::ZZ* DomainSizeCache = 0;	///< This variable contains a pointer to the Domain Size Cache when it is initialized.

// This function returns the maximum octal domain level applicable
// for the given task size
unsigned int GetMaxDomainDepth(unsigned int TaskSize);

// Set of the highest-level domains is incomplete whenever 3 does not
// divide n. The function returns the domain reduction level: 0 if the
// all 8 of the highest-level domain are present, 1 if only 4 or 2 if
// just 2 of the domains are present.
// Reduction 0: 0,1,3,7,5,2,6,4
// Reduction 1: 0,        2,6,4
// Reduction 2: 0,            4
unsigned int GetTopDomainReductionRate(unsigned int TaskSize);

// Mask <-> Number Transitions
//
// These are baseline functions used to make linear time conversions
// between node order (1 for the first node to be examined, 127 for the 127th)
// That provides for treating all the nodes as a linear pool.

void GetLiteralStringByMask(unsigned int TaskSize, DomainType* lit, NTL::vec_GF2 mask);
void GetLiteralStringByNumber(unsigned int TaskSize, DomainType* e, NTL::ZZ number);
NTL::ZZ GetDomainStartFromLiteralString(DomainType* LiteralString, unsigned int offset, unsigned int ReductionRate);
void SetMaskByLiteralString(unsigned int TaskSize, NTL::vec_GF2* mask, DomainType* LiteralString);
NTL::ZZ GetNumberByLiteralString(unsigned int TaskSize, DomainType* lit);

void InitializeDomainSizeCache(unsigned int ts);
void DeinitializeDomainSizeCache();

#endif
