/// @file Converter.cpp
/// @brief This file provides the necessary tools for the Packing Tree Linearization.
///
/// Functions implemented here translate binary packing vectors into their ordinal
/// numbers during the depth-first tree traversal and vice versa. The imtermediate
/// representation named the literal string is used.

#include <cmath>

#include "converter.h"

/// @defgroup domainsizecache Base Subtree Offsets
/// @{

#define Start0     (DomainSizeCache[(offset-1)*12+0]) 	///< Root of the base subtree '0'
#define Start1     (DomainSizeCache[(offset-1)*12+1]) 	///< Root of the base subtree '1'
#define Start3     (DomainSizeCache[(offset-1)*12+3]) 	///< Root of the base subtree '3'
#define Start7     (DomainSizeCache[(offset-1)*12+7]) 	///< Root and body of the base subtree '7'
#define Start7_Sub (DomainSizeCache[(offset-1)*12+8]) 	///< Body of the base subtree '3'
#define Start5     (DomainSizeCache[(offset-1)*12+5]) 	///< Root and body of the base subtree '5'
#define Start5_Sub (DomainSizeCache[(offset-1)*12+9]) 	///< Body of the base subtree '1'
#define Start2     (DomainSizeCache[(offset-1)*12+2]) 	///< Root of the base subtree '2'
#define Start6     (DomainSizeCache[(offset-1)*12+6]) 	///< Root and body of the base subtree '6'
#define Start6_Sub (DomainSizeCache[(offset-1)*12+10])	///< Body of the base subtree '2'
#define Start4     (DomainSizeCache[(offset-1)*12+4]) 	///< Root and body of the base subtree '4'
#define Start4_Sub (DomainSizeCache[(offset-1)*12+11])	///< Body of the base subtree '0'

/// @}

/// Gets the number of collapses necessary to get the final simple tree.
///
/// Knapsack Packing Tree structure only depends on the Task Size. Moreover
/// if some identical subtrees are collapsed into nodes, the resulting structure
/// is a Knapsack Packing Tree for smaller Task Size. This module deals with
/// subtrees of size 8 (that is, Knapsack Packing Trees for Task Size 3) -- the
/// base subtrees. If all of these are collapsed into a single node, the
/// resulting structure is the Knapsack Packing Tree for (Task Size - 3).
/// This operation may be repeated until the tree is simpler or equal to
/// the base subtree. This function gets the maximum number of collapses
/// that make the tree simple.
///
/// @param	TaskSize	Task Size for the corresponding Knapsack Problem;
///
/// @returns			The number of collapses to make the tree simple;
unsigned int GetMaxDomainDepth(unsigned int TaskSize)
{
	// n = 1 --> Max.Lv. = 0
	// n = 2 --> Max.Lv. = 0
	// n = 3 --> Max.Lv. = 0
	// n = 4 --> Max.Lv. = 1
	// n = 5 --> Max.Lv. = 1
	// n = 6 --> Max.Lv. = 1
	// n = 7 --> Max.Lv. = 2
	// n = 8 --> Max.Lv. = 2
	// n = 9 --> Max.Lv. = 2
	// ...
	return (unsigned int)((TaskSize-1)/3);
}

/// Shows the equivalent task size for the final simple tree.
///
/// After a number of collapses (see GetMaxDomainDepth()) the Knapsack
/// Packing Tree becomes the final simple tree. Depending on the Task
/// Size, there are exactly three types of such trees, corresponding to
/// Task Sizes 3, 2 and 1. Reduction rate is the number of binary variables
/// missing from the final tree, but present in the base subtree. In other
/// words, if the reduction rate is 0, then the final tree is isomorphic to
/// the base subtree with 8 nodes. On reduction level 1 the final tree only
/// contains 4 nodes, and on level 2 only 2 nodes.
///
/// @param	TaskSize	Task Size for the corresponding Knapsack Problem;
///
/// @returns			The reduction rate of the final tree
unsigned int GetTopDomainReductionRate(unsigned int TaskSize)
{
	// n = 1 --> Reduced (2)
	// n = 2 --> Reduced (4)
	// n = 3 --> Full (8)
	// n = 4 --> Reduced
	// n = 5 --> Reduced
	// n = 6 --> Full
	// n = 7 --> Reduced
	// n = 8 --> Reduced
	// n = 9 --> Full
	// ...

	return 3 * (!!(TaskSize % 3)) - (TaskSize % 3);
}

/// Shows how many nodes of the initial tree is contained in each node of the collapsed tree
///
/// After each collapse, all the tree nodes become multinodes that hold all the nodes of
/// the collapsed subtree. This function gets the number of initial tree nodes
/// corresponding to a single node in the tree after a given number of collapses.
///
/// @param	lv	Level: number of collapse operations on the tree;
///
/// @returns		The number of initial nodes (knapsack packings) in each multinode of the tree;
NTL::ZZ GetDomainSize(unsigned int lv)
{
	return NTL::ZZ((unsigned long long)(pow(2, 3 * lv + 1) - 1));
}

/// Gets the literal string showing the packing vector position in the collapsed trees
///
/// Base subtrees (also called blocks or domains) are isomorphic, but differ in the position
/// in the initial tree. So, there are 8 types of the base trees, given by the correponding
/// invariant binary components: 1, 3, 7, 5, 2, 6, 4. At every level of collapse, the given
/// packing belogs to one of the base subtrees. Literal string is an array, made of numbers
/// of the subtrees the packing belongs to on each collapse level, most significant first.
/// This function uses the knowledge about domain sizes and the search sequence to convert
/// the ordinal number of the packing to the corresponding literal tree.
///
/// @pre	Domain Size Cache must be initialized with enough level of collapse. See InitializeDomainSizeCache().
///
/// @param	TaskSize	Task Size for the corresponding Knapsack Problem;
/// @param[out]	e		Memory buffer for the literal string;
/// @param	number		Ordinal number of the packing vector to convert;
 void GetLiteralStringByNumber(unsigned int TaskSize, DomainType* e, NTL::ZZ number)
{
	unsigned int offset = (unsigned int)(TaskSize / 3) + 1 + !!GetTopDomainReductionRate(TaskSize);
	//unsigned long long DomainSizeForCurrentOffset = 0;

	e[0] = Domain_DOWNMOST;
	e[offset] = Domain_TOPMOST;
	e++;

	while (offset>1)
	{
		//unsigned long long DomainSizeForCurrentOffset = (unsigned long long)pow(2, --offset * 3 - 2) - 1;
		--offset;
		NTL::ZZ cursor = NTL::ZZ(0);

		if (number < Start1)	{ number -= Start0;			e[offset - 1] = Domain_Lv0; continue; }
		if (number < Start3)	{ number -= Start1;			e[offset - 1] = Domain_Lv1; continue; }
		if (number < Start7)	{ number -= Start3;			e[offset - 1] = Domain_Lv3; continue; }
		if (number < Start7_Sub){ number -= Start7;			e[offset - 1] = Domain_Lv7; continue; }
		if (number < Start5)	{ number -= Start7_Sub - 1;	e[offset - 1] = Domain_Lv3; continue; }
		if (number < Start5_Sub){ number -= Start5;			e[offset - 1] = Domain_Lv5; continue; }
		if (number < Start2)	{ number -= Start5_Sub - 1;	e[offset - 1] = Domain_Lv1; continue; }
		if (number < Start6)	{ number -= Start2;			e[offset - 1] = Domain_Lv2; continue; }
		if (number < Start6_Sub){ number -= Start6;			e[offset - 1] = Domain_Lv6; continue; }
		if (number < Start4)	{ number -= Start6_Sub - 1;	e[offset - 1] = Domain_Lv2; continue; }
		if (number < Start4_Sub){ number -= Start4;			e[offset - 1] = Domain_Lv4; continue; }
								{ number -= Start4_Sub - 1;	e[offset - 1] = Domain_Lv0; }
	}
	--e;
	return;
}

//// Get the binary representation of the packing given by its literal string.
////
//// Each of the base subtrees corresponds to a set of packings with several coordinates fixed.
//// The more collapse levels are examined, the more coordinates are known. Once all the collapse
 /// leveles are inspected, the full binary vector is known. As the fixed binary sequences are
 /// known in advance, it is sufficient to know the literal string in order to restore the
 /// binary vector representation.
 ///
 /// @pre	Domain Size Cache must be initialized with enough level of collapse. See InitializeDomainSizeCache().
 ///
 /// @param	 TaskSize	Task Size for the corresponding Knapsack Problem;
 /// @param[out] mask		The memory buffer for the restored binary vector;
 /// @param	 LiteralString	The literal string of the packing in question;
void SetMaskByLiteralString(unsigned int TaskSize, NTL::vec_GF2* mask, DomainType* LiteralString)
{
	unsigned int caret = 2;
	unsigned int cursor = (unsigned int)(TaskSize/3) + 1*!!(TaskSize%3);

	if (GetTopDomainReductionRate(TaskSize) == 2)
	{
		caret = 0;
		switch (LiteralString[cursor])
		{
		case Domain_Lv0: (*mask)[caret] = NTL::GF2(0); break;
		case Domain_Lv4: (*mask)[caret] = NTL::GF2(1); break;
		default: throw; break;
		}
		caret += 3;
		cursor--;
	}

	if (GetTopDomainReductionRate(TaskSize) == 1)
	{
		caret = 1;
		switch (LiteralString[cursor])
		{
		case Domain_Lv0: (*mask)[caret - 1] = NTL::GF2(0); (*mask)[caret - 0] = NTL::GF2(0); break;
		case Domain_Lv2: (*mask)[caret - 1] = NTL::GF2(1); (*mask)[caret - 0] = NTL::GF2(0); break;
		case Domain_Lv6: (*mask)[caret - 1] = NTL::GF2(1); (*mask)[caret - 0] = NTL::GF2(1); break;
		case Domain_Lv4: (*mask)[caret - 1] = NTL::GF2(0); (*mask)[caret - 0] = NTL::GF2(1); break;
		default: throw; break;
		}
		caret += 3;
		cursor--;
	}

	while (caret < TaskSize)
	{
		switch (LiteralString[cursor])
		{
		case Domain_Lv0: (*mask)[caret - 2] = NTL::GF2(0); (*mask)[caret - 1] = NTL::GF2(0); (*mask)[caret - 0] = NTL::GF2(0); break;
		case Domain_Lv1: (*mask)[caret - 2] = NTL::GF2(1); (*mask)[caret - 1] = NTL::GF2(0); (*mask)[caret - 0] = NTL::GF2(0); break;
		case Domain_Lv3: (*mask)[caret - 2] = NTL::GF2(1); (*mask)[caret - 1] = NTL::GF2(1); (*mask)[caret - 0] = NTL::GF2(0); break;
		case Domain_Lv5: (*mask)[caret - 2] = NTL::GF2(1); (*mask)[caret - 1] = NTL::GF2(0); (*mask)[caret - 0] = NTL::GF2(1); break;
		case Domain_Lv7: (*mask)[caret - 2] = NTL::GF2(1); (*mask)[caret - 1] = NTL::GF2(1); (*mask)[caret - 0] = NTL::GF2(1); break;
		case Domain_Lv2: (*mask)[caret - 2] = NTL::GF2(0); (*mask)[caret - 1] = NTL::GF2(1); (*mask)[caret - 0] = NTL::GF2(0); break;
		case Domain_Lv6: (*mask)[caret - 2] = NTL::GF2(0); (*mask)[caret - 1] = NTL::GF2(1); (*mask)[caret - 0] = NTL::GF2(1); break;
		case Domain_Lv4: (*mask)[caret - 2] = NTL::GF2(0); (*mask)[caret - 1] = NTL::GF2(0); (*mask)[caret - 0] = NTL::GF2(1); break;
		default: throw; break;
		}
		caret += 3;
		cursor--;
	}
}

/// Executes preliminary calculations, necessary to work with the base subtrees, up to the given collapse level.
///
/// As the search sequence is known (depth-first traversal) and the number of
/// packings in each of the nodes of collapsed trees is known (see GetDomainSize())
/// it is possible to calculate the number of the first node in each of the subtrees.
/// The more collapses took place, the more data is needed. All the data goes to the
/// DomainSizeCache static variable, so no return value is present.
///
/// @param	ts	Task Size for the corresponding Knapsack Problem;
void InitializeDomainSizeCache(unsigned int ts)
{
	if (ts < 3) throw "Unable to use linearization algorithm for n values under 3";

	int depth = GetMaxDomainDepth(ts);

	// Reducers prevent domain start number increment for reduced domains. For example, a domain with 1st level of
	// reduction does not have 1, 3,7 and 5 domains, so these domain point to the same point as the underlying domain 0.
	//

	DomainSizeCache = (NTL::ZZ*)malloc((unsigned long)((depth + 1) * 12 * sizeof(NTL::ZZ) + 6 + 3));
	if (!DomainSizeCache) throw;

	for(unsigned long ii=0;ii<(unsigned long)((depth + 1) * 12 * sizeof(NTL::ZZ) + 6 + 3);ii++)
	    ((unsigned char*)DomainSizeCache)[ii]=(unsigned char)0;

	for (int i = 0; i <= depth; i++)
	{
		int reducer1 = (GetTopDomainReductionRate(ts) >= 1) ? (!(depth==i)) : (1);
		int reducer2 = (GetTopDomainReductionRate(ts) >= 2) ? (!(depth==i)) : (1);

		DomainSizeCache[0 + 12 * i] =		0;
		DomainSizeCache[1 + 12 * i] =		DomainSizeCache[0 + 12 * i] + (1)											* reducer1 + 1 * !reducer1;	//Artificial wall
		DomainSizeCache[3 + 12 * i] = 		DomainSizeCache[1 + 12 * i] + (1)													* reducer1;

		DomainSizeCache[7 + 12 * i] =		DomainSizeCache[3 + 12 * i] + (1)											* reducer1;
			DomainSizeCache[8 + 12 * i] =	DomainSizeCache[7 + 12 * i] + (1+(GetDomainSize(i)-1) / 2)					* reducer1;

		DomainSizeCache[5 + 12 * i] =		DomainSizeCache[7 + 12 * i] + (GetDomainSize(i))							* reducer1;
			DomainSizeCache[9 + 12 * i] =	DomainSizeCache[5 + 12 * i] + (1+(GetDomainSize(i)-1) / 2)					* reducer1;

		DomainSizeCache[2 + 12 * i] = DomainSizeCache[5 + 12 * i] + (GetDomainSize(i))									* reducer1 * reducer2;	//Artificial wall

		DomainSizeCache[6 + 12 * i] = DomainSizeCache[2 + 12 * i] + (1)													* reducer2;
			DomainSizeCache[10 + 12 * i] =	DomainSizeCache[6 + 12 * i] + (1+(GetDomainSize(i)-1) / 2)					* reducer2;

		DomainSizeCache[4 + 12 * i] = DomainSizeCache[6 + 12 * i] + (GetDomainSize(i))									* reducer2;
			DomainSizeCache[11 + 12 * i] =	DomainSizeCache[4 + 12 * i] + (1+(GetDomainSize(i)-1) / 2);
	}
	return;
}

/// This function frees up the resources taken by InitializeDomainSizeCache().
void DeinitializeDomainSizeCache()
{
	if (DomainSizeCache) free(DomainSizeCache);
	return;
}

/// This function builds the literal string for a packing based on its binary vector.
///
/// As the fixed binary coordinates for each of the base subtrees is known, the binary
/// vector may be used to reconstruct the corresponding literal string.
///
/// @param	TaskSize	Task Size for the corresponding Knapsack Problem;
/// @param[out]	lit		Memory buffer to store the literal string;
/// @param	mask		The binary vector of the packing in question;
void GetLiteralStringByMask(unsigned int TaskSize, DomainType* lit, NTL::vec_GF2 mask)
{
	unsigned int LiteralStringSize = (unsigned int)(TaskSize / 3) + 1 * !!(TaskSize % 3) + 2;
	lit++;

	unsigned int k = LiteralStringSize * 3 - 6 - 1 - GetTopDomainReductionRate(TaskSize);
	while (k>=2){
/*		unsigned int l1 = (k - 0 > TaskSize - 1) ? (0) : (mask[k - 0]);
		unsigned int l2 = (k - 1 > TaskSize - 1) ? (0) : (mask[k - 1]);
		unsigned int l3 = (k - 2 > TaskSize - 1) ? (0) : (mask[k - 2]);
		lit[LiteralStringSize - 3 - (unsigned int)(k / 3)] = (DomainType)(l1 * 4 + l2 * 2 + l3 * 1);
		k -= 3;*/
		unsigned int l1 = (NTL::conv<int>(mask[k - 0]));
		unsigned int l2 = (NTL::conv<int>(mask[k - 1]));
		unsigned int l3 = (NTL::conv<int>(mask[k - 2]));
		lit[LiteralStringSize - 3 - (unsigned int)(k / 3)] = (DomainType)(l1 * 4 + l2 * 2 + l3 * 1);
		if (k == 2) break; else  k -= 3;
	};
	if (k == 1)
	{
		unsigned int l1 = (NTL::conv<int>(mask[k - 0]));
		unsigned int l2 = (NTL::conv<int>(mask[k - 1]));
		lit[LiteralStringSize - 3 - (unsigned int)(k / 3)] = (DomainType)(l1 * 4 + l2 * 2);
	}
	if (k == 0)
	{
		unsigned int l1 = (NTL::conv<int>(mask[k - 0]));
		lit[LiteralStringSize - 3 - (unsigned int)(k / 3)] = (DomainType)(l1 * 4);
	}
	lit[-1] = Domain_DOWNMOST;
	lit[LiteralStringSize - 2] = Domain_TOPMOST;
	lit--;
	return;
    }

/// This function gets the ordinal number of the packing based on its literal string
///
/// This function reverses the GetLiteralStringByNumber().
///
/// @param	TaskSize	Task Size for the corresponding Knapsack Problem;
/// @param	lit		The literral string of the packing in question;
///
/// @returns			Ordinal number of the packing in question;
NTL::ZZ GetNumberByLiteralString(unsigned int TaskSize, DomainType* lit)
    {
    NTL::ZZ num; num=0;
    for (unsigned int y = 1; lit[y] != Domain_TOPMOST; y++)
    	 num += GetDomainStartFromLiteralString(lit, y, !!(lit[y+1]==Domain_TOPMOST)*GetTopDomainReductionRate(TaskSize));
    return(num);
    }

/// This function gets the distance to the given node from the start of the base subtree of the
/// given level.
///
/// @param	LiteralString	Literal string for the packing in queation;
/// @param	offset		The collapse level to work in;
/// @param	ReductionRate	The reduction level for the initail tree (see GetTopDomainReductionLevel());
///
/// @returns			Distance between the given packing and the root of its subtree at given collapse level;
NTL::ZZ GetDomainStartFromLiteralString(DomainType* LiteralString, unsigned int offset, unsigned int ReductionRate)
{
	if ((LiteralString[offset] == Domain_TOPMOST) || (LiteralString[offset] == Domain_DOWNMOST)) { throw; return NTL::ZZ(0); }
	NTL::ZZ DomainSizeForCurrentOffset; NTL::power(DomainSizeForCurrentOffset, 2, offset * 3 - 2);
    DomainSizeForCurrentOffset--;
	//	unsigned long long DomainSizeForIncreasedOffset = DomainSizeForCurrentOffset * 8 + 7;
	NTL::ZZ ret; ret = 0;

	DomainType curr = LiteralString[offset + 0];
	DomainType next = LiteralString[offset - 1];

	unsigned int trivial = 1;

	for (unsigned int d = offset - 1; d > 0; d--) trivial *= (LiteralString[d] == Domain_Lv0);
	if (next == Domain_DOWNMOST) trivial = 1;
	switch (ReductionRate)
	{
	case 0:
		if (trivial)
			switch (curr)
			{
				case Domain_Lv0: ret = 0;									break;
				case Domain_Lv1: ret = 1;									break;
				case Domain_Lv3: ret = 2;									break;
				case Domain_Lv7: ret = 3;									break;
				case Domain_Lv5: ret = 3 + DomainSizeForCurrentOffset;		break;
				case Domain_Lv2: ret = 3 + 2 * DomainSizeForCurrentOffset;	break;
				case Domain_Lv6: ret = 4 + 2 * DomainSizeForCurrentOffset;	break;
				case Domain_Lv4: ret = 4 + 3 * DomainSizeForCurrentOffset;	break;
				default: throw; break;
			}
		else
			switch (curr)
			{
				// First subdomain
				case Domain_Lv7: ret = 3;									break;
				case Domain_Lv5: ret = 3 + DomainSizeForCurrentOffset;		break;
				case Domain_Lv6: ret = 4 + 2 * DomainSizeForCurrentOffset;	break;
				case Domain_Lv4: ret = 4 + 3 * DomainSizeForCurrentOffset;	break;
				// Second subdomain
				case Domain_Lv0: ret = 4 + 3 * DomainSizeForCurrentOffset - 1 + DomainSizeForCurrentOffset / 2 + 1;	break;
				case Domain_Lv1: ret = 3 + 1 * DomainSizeForCurrentOffset - 1 + DomainSizeForCurrentOffset / 2 + 1;	break;
				case Domain_Lv3: ret = 3 - 1 + DomainSizeForCurrentOffset / 2 + 1;	break;
				case Domain_Lv2: ret = 4 + 2 * DomainSizeForCurrentOffset - 1 + DomainSizeForCurrentOffset / 2 + 1;	break;
				default: throw; break;
			}
		break;
	case 1:
		if (trivial)
			switch (curr)
		{
			case Domain_Lv0: ret = 0;									break;
			case Domain_Lv2: ret = 1;									break;
			case Domain_Lv6: ret = 2;									break;
			case Domain_Lv4: ret = 2 + 1 * DomainSizeForCurrentOffset;	break;
			default: throw; break;
		}
		else
			switch (curr)
		{
			// First subdomain
			case Domain_Lv6: ret = 2;									break;
			case Domain_Lv4: ret = 2 + DomainSizeForCurrentOffset;		break;
			// Second subdomain
			case Domain_Lv0: ret = 2 + 1 * DomainSizeForCurrentOffset - 1 + DomainSizeForCurrentOffset / 2 + 1;	break;
			case Domain_Lv2: ret = 2 - 1 + DomainSizeForCurrentOffset / 2 + 1;									break;
			default: throw; break;
		}
		break;
	case 2:
		if (trivial)
			switch (curr)
		{
			case Domain_Lv0: ret = 0;	break;
			case Domain_Lv4: ret = 1;	break;
			default: throw; break;
		}
		else
			switch (curr)
		{
			// First subdomain
			case Domain_Lv4: ret = 1;	break;
			// Second subdomain
			case Domain_Lv0: ret = 1 - 1 + DomainSizeForCurrentOffset / 2 + 1;	break;
			default: throw; break;
		}
		break;
	default: throw; break;
	}
	return(ret);
}
