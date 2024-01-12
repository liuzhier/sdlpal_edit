#include "stdafx.h"
#include "../main/cpalapp.h"
#include <cassert>

#define SWAP16(X) (X)
#define SWAP32(x)  (x)


typedef struct _TreeNode
{
    unsigned short   value;
    unsigned short   leaf;
    unsigned short   level;
    unsigned short   weight;

    struct _TreeNode* parent;
    struct _TreeNode* left;
    struct _TreeNode* right;
} TreeNode;

typedef struct _TreeNodeList
{
    TreeNode* node;
    struct _TreeNodeList* next;
} TreeNodeList;

typedef struct _YJ_1_FILEHEADER
{
    unsigned int   Signature;          // 'YJ_1'
    unsigned int   UncompressedLength; // size before compression
    unsigned int   CompressedLength;   // size after compression
    unsigned short BlockCount;       // number of blocks
    unsigned char Unknown;
    unsigned char HuffmanTreeLength; // length of huffman tree
} YJ_1_FILEHEADER, * PYJ_1_FILEHEADER;

typedef struct _YJ_1_BLOCKHEADER
{
    unsigned short UncompressedLength; // maximum 0x4000
    unsigned short CompressedLength;   // including the header
    unsigned short LZSSRepeatTable[4];
    unsigned char LZSSOffsetCodeLengthTable[4];
    unsigned char LZSSRepeatCodeLengthTable[3];
    unsigned char CodeCountCodeLengthTable[3];
    unsigned char CodeCountTable[2];
} YJ_1_BLOCKHEADER, * PYJ_1_BLOCKHEADER;

static unsigned int
get_bits(
    const void* src,
    unsigned int* bitptr,
    unsigned int count
)
{
    unsigned char* temp = ((unsigned char*)src) + ((*bitptr >> 4) << 1);
    unsigned int bptr = *bitptr & 0xf;
    unsigned short mask;
    *bitptr += count;
    if (count > 16 - bptr)
    {
        count = count + bptr - 16;
        mask = 0xffff >> bptr;
        return (((temp[0] | (temp[1] << 8)) & mask) << count) | ((temp[2] | (temp[3] << 8)) >> (16 - count));
    }
    else
        return (((unsigned short)((temp[0] | (temp[1] << 8)) << bptr)) >> (16 - count));
}

static unsigned short
get_loop(
    const void* src,
    unsigned int* bitptr,
    PYJ_1_BLOCKHEADER header
)
{
    if (get_bits(src, bitptr, 1))
        return header->CodeCountTable[0];
    else
    {
        unsigned int temp = get_bits(src, bitptr, 2);
        if (temp)
            return get_bits(src, bitptr, header->CodeCountCodeLengthTable[temp - 1]);
        else
            return header->CodeCountTable[1];
    }
}

static unsigned short
get_count(
    const void* src,
    unsigned int* bitptr,
    PYJ_1_BLOCKHEADER header
)
{
    unsigned short temp;
    if ((temp = get_bits(src, bitptr, 2)) != 0)
    {
        if (get_bits(src, bitptr, 1))
            return get_bits(src, bitptr, header->LZSSRepeatCodeLengthTable[temp - 1]);
        else
            return SWAP16(header->LZSSRepeatTable[temp]);
    }
    else
        return SWAP16(header->LZSSRepeatTable[0]);
}



INT CPalEdit::is_Use_YJ1_Decompress()
{
	//测试是否应用YJ1解压

	//六个文件用压缩存储
	FILE* fps[] = { 
		UTIL_OpenRequiredFile((LPCSTR)"abc.mkf"), 
		UTIL_OpenRequiredFile("map.mkf"), 
		gpGlobals->f.fpF, 
		gpGlobals->f.fpFBP, 
		gpGlobals->f.fpFIRE, 
		gpGlobals->f.fpMGO };
	uint8_t* data = NULL;
	int data_size = 0;
	int  result = 0;
	for (int i = 0; i < sizeof(fps) / sizeof(FILE*); i++)
	{
		//
		// Find the first non-empty sub-file
		//
		int count = PAL_MKFGetChunkCount(fps[i]), j = 0, size;
		while (j < count && (size = PAL_MKFGetChunkSize(j, fps[i])) < 4) j++;
		if (j >= count)
		{
			//出错了 ，空的文件 
			result = -1;
			break;
		}
		//
		// Read the content and check the compression signature
		// Note that this check is not 100% correct, however in incorrect situations,
		// the sub-file will be over 784MB if uncompressed, which is highly unlikely.
		//
		if (data_size < size) data = (uint8_t*)realloc(data, data_size = size);
		PAL_MKFReadChunk(data, data_size, j, fps[i]);
		if (data[0] == 'Y' && data[1] == 'J' && data[2] == '_' && data[3] == '1')
			result++;
	}

	//
	free(data);
	fclose(fps[1]);
	fclose(fps[0]);
	if (!result)
		return 0;
	else if (result == sizeof(fps) / sizeof(FILE*))
		return 1;

	//出错退出
	exit(9);
}

INT CPalEdit::YJ1_Decompress(
    LPCVOID       Source,
    LPVOID        Destination,
    INT           DestSize
)
{
    PYJ_1_FILEHEADER hdr = (PYJ_1_FILEHEADER)Source;
    unsigned char* src = (unsigned char*)Source;
    unsigned char* dest;
    unsigned int i;
    TreeNode* root, * node;

    if (Source == NULL)
        return -1;
    if (SWAP32(hdr->Signature) != 0x315f4a59)
        return -1;
    if (SWAP32(hdr->UncompressedLength) > (unsigned int)DestSize)
        return -1;

    do
    {
        unsigned short tree_len = ((unsigned short)hdr->HuffmanTreeLength) * 2;
        unsigned int bitptr = 0;
        unsigned char* flag = (unsigned char*)src + 16 + tree_len;

        if ((node = root = (TreeNode*)malloc(sizeof(TreeNode) * (tree_len + 1))) == NULL)
            return -1;
        root[0].leaf = 0;
        root[0].value = 0;
        root[0].left = root + 1;
        root[0].right = root + 2;
        for (i = 1; i <= tree_len; i++)
        {
            root[i].leaf = !get_bits(flag, &bitptr, 1);
            root[i].value = src[15 + i];
            if (root[i].leaf)
                root[i].left = root[i].right = NULL;
            else
            {
                root[i].left = root + (root[i].value << 1) + 1;
                root[i].right = root[i].left + 1;
            }
        }
        src += 16 + tree_len + (((tree_len & 0xf) ? (tree_len >> 4) + 1 : (tree_len >> 4)) << 1);
    } while (0);

    dest = (unsigned char*)Destination;

    for (i = 0; i < SWAP16(hdr->BlockCount); i++)
    {
        unsigned int bitptr;
        PYJ_1_BLOCKHEADER header;

        header = (PYJ_1_BLOCKHEADER)src;
        src += 4;
        if (!SWAP16(header->CompressedLength))
        {
            unsigned short hul = SWAP16(header->UncompressedLength);
            while (hul--)
            {
                *dest++ = *src++;
            }
            continue;
        }
        src += 20;
        bitptr = 0;
        for (;;)
        {
            unsigned short loop;
            if ((loop = get_loop(src, &bitptr, header)) == 0)
                break;

            while (loop--)
            {
                node = root;
                for (; !node->leaf;)
                {
                    if (get_bits(src, &bitptr, 1))
                        node = node->right;
                    else
                        node = node->left;
                }
                *dest++ = node->value;
            }

            if ((loop = get_loop(src, &bitptr, header)) == 0)
                break;

            while (loop--)
            {
                unsigned int pos, count;
                count = get_count(src, &bitptr, header);
                pos = get_bits(src, &bitptr, 2);
                pos = get_bits(src, &bitptr, header->LZSSOffsetCodeLengthTable[pos]);
                while (count--)
                {
                    *dest = *(dest - pos);
                    dest++;
                }
            }
        }
        src = ((unsigned char*)header) + SWAP16(header->CompressedLength);
    }
    free(root);

    return SWAP32(hdr->UncompressedLength);
}

//-----------------------------------------------------------------------------------------------
/*
 * PAL WIN95 compress format (YJ_2) streaming library
 *
 * Author: Lou Yihua <louyihua@21cn.com>
 *
 * Copyright 2007 Lou Yihua
 *
 * This file is part of PAL library.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA

 *
 *《仙剑奇侠传》WIN95版压缩格式(YJ_2)流式处理库
 *
 * 作者： 楼奕华 <louyihua@21cn.com>
 *
 * 版权所有 2007 楼奕华
 *
 * 本文件是《仙剑奇侠传》库的一部分。
 *
 * 这个库属于自由软件，你可以遵照自由软件基金会出版的GNU次通用公共许可证条
 * 款来修改和重新发布这一程序。或者用许可证2.1版，或者（根据你的选择）用任
 * 何较新的版本。发布这一库的目的是希望它有用，但没有任何担保。甚至没有适合
 * 特定目的隐含的担保。更详细的情况请参阅GNU次通用公共许可证。
 *
 * 你应该已经和库一起收到一份GNU次通用公共许可证的拷贝。如果还没有，写信给
 * 自由软件基金会：51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
*/

typedef struct _YJ2_TreeNode
{
	unsigned short      weight;
	unsigned short      value;
	struct _YJ2_TreeNode* parent;
	struct _YJ2_TreeNode* left;
	struct _YJ2_TreeNode* right;
} YJ2_TreeNode;

typedef struct _YJ2_Tree
{
	YJ2_TreeNode* node;
	YJ2_TreeNode** list;
} YJ2_Tree;

static unsigned char yj2_data1[0x100] =
{
	0x3f, 0x0b, 0x17, 0x03, 0x2f, 0x0a, 0x16, 0x00, 0x2e, 0x09, 0x15, 0x02, 0x2d, 0x01, 0x08, 0x00,
	0x3e, 0x07, 0x14, 0x03, 0x2c, 0x06, 0x13, 0x00, 0x2b, 0x05, 0x12, 0x02, 0x2a, 0x01, 0x04, 0x00,
	0x3d, 0x0b, 0x11, 0x03, 0x29, 0x0a, 0x10, 0x00, 0x28, 0x09, 0x0f, 0x02, 0x27, 0x01, 0x08, 0x00,
	0x3c, 0x07, 0x0e, 0x03, 0x26, 0x06, 0x0d, 0x00, 0x25, 0x05, 0x0c, 0x02, 0x24, 0x01, 0x04, 0x00,
	0x3b, 0x0b, 0x17, 0x03, 0x23, 0x0a, 0x16, 0x00, 0x22, 0x09, 0x15, 0x02, 0x21, 0x01, 0x08, 0x00,
	0x3a, 0x07, 0x14, 0x03, 0x20, 0x06, 0x13, 0x00, 0x1f, 0x05, 0x12, 0x02, 0x1e, 0x01, 0x04, 0x00,
	0x39, 0x0b, 0x11, 0x03, 0x1d, 0x0a, 0x10, 0x00, 0x1c, 0x09, 0x0f, 0x02, 0x1b, 0x01, 0x08, 0x00,
	0x38, 0x07, 0x0e, 0x03, 0x1a, 0x06, 0x0d, 0x00, 0x19, 0x05, 0x0c, 0x02, 0x18, 0x01, 0x04, 0x00,
	0x37, 0x0b, 0x17, 0x03, 0x2f, 0x0a, 0x16, 0x00, 0x2e, 0x09, 0x15, 0x02, 0x2d, 0x01, 0x08, 0x00,
	0x36, 0x07, 0x14, 0x03, 0x2c, 0x06, 0x13, 0x00, 0x2b, 0x05, 0x12, 0x02, 0x2a, 0x01, 0x04, 0x00,
	0x35, 0x0b, 0x11, 0x03, 0x29, 0x0a, 0x10, 0x00, 0x28, 0x09, 0x0f, 0x02, 0x27, 0x01, 0x08, 0x00,
	0x34, 0x07, 0x0e, 0x03, 0x26, 0x06, 0x0d, 0x00, 0x25, 0x05, 0x0c, 0x02, 0x24, 0x01, 0x04, 0x00,
	0x33, 0x0b, 0x17, 0x03, 0x23, 0x0a, 0x16, 0x00, 0x22, 0x09, 0x15, 0x02, 0x21, 0x01, 0x08, 0x00,
	0x32, 0x07, 0x14, 0x03, 0x20, 0x06, 0x13, 0x00, 0x1f, 0x05, 0x12, 0x02, 0x1e, 0x01, 0x04, 0x00,
	0x31, 0x0b, 0x11, 0x03, 0x1d, 0x0a, 0x10, 0x00, 0x1c, 0x09, 0x0f, 0x02, 0x1b, 0x01, 0x08, 0x00,
	0x30, 0x07, 0x0e, 0x03, 0x1a, 0x06, 0x0d, 0x00, 0x19, 0x05, 0x0c, 0x02, 0x18, 0x01, 0x04, 0x00
};

static unsigned char yj2_data2[0x10] =
{
	0x08, 0x05, 0x06, 0x04, 0x07, 0x05, 0x06, 0x03, 0x07, 0x05, 0x06, 0x04, 0x07, 0x04, 0x05, 0x03
};

static void yj2_adjust_tree(YJ2_Tree tree, unsigned short value)
{
	YJ2_TreeNode* node = tree.list[value];
	YJ2_TreeNode tmp;
	YJ2_TreeNode* tmp1;
	YJ2_TreeNode* temp;
	while (node->value != 0x280)
	{
		temp = node + 1;
		while (node->weight == temp->weight)
			temp++;
		temp--;
		if (temp != node)
		{
			tmp1 = node->parent;
			node->parent = temp->parent;
			temp->parent = tmp1;
			if (node->value > 0x140)
			{
				node->left->parent = temp;
				node->right->parent = temp;
			}
			else
				tree.list[node->value] = temp;
			if (temp->value > 0x140)
			{
				temp->left->parent = node;
				temp->right->parent = node;
			}
			else
				tree.list[temp->value] = node;
			tmp = *node; *node = *temp; *temp = tmp;
			node = temp;
		}
		node->weight++;
		node = node->parent;
	}
	node->weight++;
}

static int yj2_build_tree(YJ2_Tree* tree)
{
	int i, ptr;
	YJ2_TreeNode** list;
	YJ2_TreeNode* node;
	if ((tree->list = list = (YJ2_TreeNode**)malloc(sizeof(YJ2_TreeNode*) * 321)) == NULL)
		return 0;
	if ((tree->node = node = (YJ2_TreeNode*)malloc(sizeof(YJ2_TreeNode) * 641)) == NULL)
	{
		free(list);
		return 0;
	}
	memset(list, 0, 321 * sizeof(YJ2_TreeNode*));
	memset(node, 0, 641 * sizeof(YJ2_TreeNode));
	for (i = 0; i <= 0x140; i++)
		list[i] = node + i;
	for (i = 0; i <= 0x280; i++)
	{
		node[i].value = i;
		node[i].weight = 1;
	}
	tree->node[0x280].parent = tree->node + 0x280;
	for (i = 0, ptr = 0x141; ptr <= 0x280; i += 2, ptr++)
	{
		node[ptr].left = node + i;
		node[ptr].right = node + i + 1;
		node[i].parent = node[i + 1].parent = node + ptr;
		node[ptr].weight = node[i].weight + node[i + 1].weight;
	}
	return 1;
}

static int yj2_bt(const unsigned char* data, unsigned int pos)
{
	return (data[pos >> 3] & (unsigned char)(1 << (pos & 0x7))) >> (pos & 0x7);
}
#ifndef SDL_SwapLE32
#define SDL_SwapLE32
#endif // !SDL_SwapLE32


INT CPalEdit::YJ2_Decompress(
	LPCVOID       Source,
	LPVOID        Destination,
	INT           DestSize
)
{
	int Length;
	unsigned int len = 0, ptr = 0;
	unsigned char* src = (unsigned char*)Source + 4;
	unsigned char* dest;
	YJ2_Tree tree;
	YJ2_TreeNode* node;

	if (Source == NULL)
		return -1;

	if (!yj2_build_tree(&tree))
		return -1;

	Length = SDL_SwapLE32(*((unsigned int*)Source));
	if (Length > DestSize)
		return -1;
	dest = (unsigned char*)Destination;

	while (1)
	{
		unsigned short val;
		node = tree.node + 0x280;
		while (node->value > 0x140)
		{
			if (yj2_bt(src, ptr))
				node = node->right;
			else
				node = node->left;
			ptr++;
		}
		val = node->value;
		if (tree.node[0x280].weight == 0x8000)
		{
			int i;
			for (i = 0; i < 0x141; i++)
				if (tree.list[i]->weight & 0x1)
					yj2_adjust_tree(tree, i);
			for (i = 0; i <= 0x280; i++)
				tree.node[i].weight >>= 1;
		}
		yj2_adjust_tree(tree, val);
		if (val > 0xff)
		{
			int i;
			unsigned int temp, tmp, pos;
			unsigned char* pre;
			for (i = 0, temp = 0; i < 8; i++, ptr++)
				temp |= (unsigned int)yj2_bt(src, ptr) << i;
			tmp = temp & 0xff;
			for (; i < yj2_data2[tmp & 0xf] + 6; i++, ptr++)
				temp |= (unsigned int)yj2_bt(src, ptr) << i;
			temp >>= yj2_data2[tmp & 0xf];
			pos = (temp & 0x3f) | ((unsigned int)yj2_data1[tmp] << 6);
			if (pos == 0xfff)
				break;
			pre = dest - pos - 1;
			for (i = 0; i < val - 0xfd; i++)
				*dest++ = *pre++;
			len += val - 0xfd;
		}
		else
		{
			*dest++ = (unsigned char)val;
			len++;
		}
	}

	free(tree.list);
	free(tree.node);
	return Length;
}

//以下是压缩命令
//////////////////////////////////////////////////////
//YJ1
//////////////////////////////////////////////////////
#define	PAL_INVALID_DATA		1024
#define	PAL_EMPTY_POINTER		1025
#define	PAL_NOT_ENOUGH_SPACE	1026
#define	PAL_INVALID_FORMAT		1027
#define	PAL_OK					0
#define	PAL_OUT_OF_MEMORY		ENOMEM
#define	PAL_INVALID_PARAMETER	EINVAL
static inline UINT16 get_bit_count(UINT32 word)
{
	UINT16 bits = 0;
	while (word)
	{
		bits++;
		word >>= 1;
	}
	return bits;
}

static inline UINT32 lz_analysize(UINT8* base, UINT16* result, UINT32 block_len, UINT32 freq[])
{
	INT32	head[0x100], prev[0x4000];
	UINT32 ptr, dptr, baseptr;

	memset(head, 0xff, 0x100 * sizeof(INT32));
	memset(prev, 0xff, 0x4000 * sizeof(INT32));
	for (ptr = 0; ptr < block_len - 1; ptr++)
	{
		UINT8 hash = base[ptr] ^ base[ptr + 1];
		if (head[hash] >= 0)
			prev[ptr] = head[hash];
		head[hash] = ptr;
	}

	result[0] = 0; dptr = 1;
	for (baseptr = ptr = 0; ptr < block_len;)
	{
		UINT16 match_len;
		INT32 prv, tmp;

		match_len = 0; tmp = ptr;
		while ((tmp = prev[tmp]) >= 0)
		{
			UINT16 match_len_t = 0;
			INT32 prv_t, cur_t;

			prv_t = tmp; cur_t = ptr;
			while (base[prv_t++] == base[cur_t++] && match_len_t + ptr < block_len)
				match_len_t++;
			if (match_len_t > 1 && match_len < match_len_t)
			{
				match_len = match_len_t;
				prv = tmp;
			}
		}
		if (match_len > 1 && match_len < 5)
		{
			UINT16 bit_count = 5 + get_bit_count(match_len) + get_bit_count(ptr - prv);
			if (bit_count > (match_len << 3))
				match_len = 1;
		}
		if (match_len > 1)
		{
			if (result[baseptr] > 0x8000 && result[baseptr] < 0xffff)
				result[baseptr]++;
			else
			{
				baseptr = dptr++;
				result[baseptr] = 0x8001;
			}
			result[dptr++] = match_len;
			result[dptr++] = ptr - prv;
			ptr += match_len;
		}
		else
		{
			if (!result[baseptr])
				result[baseptr] = 0x1;
			else if (result[baseptr] < 0x7fff)
				result[baseptr]++;
			else
			{
				baseptr = dptr++;
				result[baseptr] = 0x1;
			}
			freq[base[ptr++]]++;
		}
	}
	return dptr << 1;
}

static  UINT32 cb_analysize(PYJ_1_BLOCKHEADER header, UINT16 block[], UINT32 cb_len)
{
	INT32 i, j, min, max, total, total_min, total_bits, count_len[15], total_len[15];
	UINT32 ptr;

	do
	{
		UINT32 count[0x100];
		UINT8 max1, max2;
		memset(count, 0, 0x100 * sizeof(INT32));
		memset(count_len, 0, 15 * sizeof(INT32));
		memset(total_len, 0, 15 * sizeof(INT32));
		max1 = max2 = 0;
		for (ptr = 0; ptr < (cb_len >> 1); ptr++)
		{
			UINT16 temp = block[ptr] & 0x7fff;
			if (temp < 0x100)
			{
				count[temp]++;
				if (count[max1] < count[temp])
				{
					max2 = max1;
					max1 = (UINT8)temp;
				}
				else if (count[max2] < count[temp] && temp != max1)
					max2 = (UINT8)temp;
			}
			count_len[get_bit_count(temp)]++;
			if (block[ptr] & 0x8000)
				ptr += temp << 1;
		}
		header->CodeCountTable[0] = max1;
		if (max2)
			header->CodeCountTable[1] = max2;
		else
			header->CodeCountTable[1] = max1;
		total_bits = count[max1] + count[max2] * 3;
		count_len[get_bit_count(max1)] -= count[max1];
		count_len[get_bit_count(max2)] -= count[max2];
		for (max = 14; !count_len[max] && max > 0; max--);
		for (min = 1; !count_len[min] && min < 15; min++);
		if (max < min)
		{
			header->CodeCountCodeLengthTable[0] =
				header->CodeCountCodeLengthTable[1] =
				header->CodeCountCodeLengthTable[2] = 0;
			break;
		}
		for (i = min; i <= max; i++)
			total_len[i] = total_len[i - 1] + count_len[i];
		header->CodeCountCodeLengthTable[0] = min;
		header->CodeCountCodeLengthTable[1] =
			header->CodeCountCodeLengthTable[2] = max;
		total_min = total_len[min] * min + (total_len[max] - total_len[min]) * max;
		for (i = min; i < max - 1; i++)
			if (count_len[i])
			{
				for (j = i + 1; j < max; j++)
					if (count_len[j])
					{
						total = total_len[i] * i + (total_len[j] - total_len[i]) * j + (total_len[max] - total_len[j]) * max;
						if (total < total_min)
						{
							total_min = total;
							header->CodeCountCodeLengthTable[0] = i;
							header->CodeCountCodeLengthTable[1] = j;
						}
					}
			}
		total_bits += total_min + total_len[max] * 3;
	} while (0);

	do
	{
		UINT32 count[0x4000], tmp, maxs[4];
		memset(maxs, 0, 4 * sizeof(INT32));
		memset(count, 0, 0x4000 * sizeof(INT32));
		memset(count_len, 0, 15 * sizeof(INT32));
		memset(total_len, 0, 15 * sizeof(INT32));
		for (ptr = 0; ptr < (cb_len >> 1);)
		{
			UINT16 temp = block[ptr] & 0x7fff;
			if (block[ptr++] & 0x8000)
				for (i = 0; i < temp; i++)
				{
					tmp = block[ptr++]; ptr++; count[tmp]++;
					count_len[get_bit_count(tmp)]++;
					if (count[maxs[0]] < count[tmp])
					{
						for (j = 1; j < 4; j++)
							if (tmp == maxs[j])
							{
								maxs[j] = maxs[0];
								maxs[0] = tmp;
								break;
							}
						if (j == 4)
						{
							maxs[3] = maxs[2]; maxs[2] = maxs[1];
							maxs[1] = maxs[0]; maxs[0] = tmp;
						}
					}
					else if (count[maxs[1]] < count[tmp] && tmp != maxs[0])
					{
						for (j = 2; j < 4; j++)
							if (tmp == maxs[j])
							{
								maxs[j] = maxs[1]; maxs[1] = tmp;
								break;
							}
						if (j == 4)
						{
							maxs[3] = maxs[2]; maxs[2] = maxs[1]; maxs[1] = tmp;
						}
					}
					else if (count[maxs[2]] < count[tmp] && tmp != maxs[0] && tmp != maxs[1])
					{
						maxs[3] = maxs[2]; maxs[2] = tmp;
					}
					else if (count[maxs[3]] < count[tmp] && tmp != maxs[0] && tmp != maxs[1] && tmp != maxs[2])
						maxs[3] = tmp;
				}
		}
		total_bits += (count[maxs[0]] << 1) + (count[maxs[1]] + count[maxs[2]] + count[maxs[3]]) * 3;
		do
		{
			INT32 lastmax = maxs[0];
			for (i = 0; i < 4; i++)
			{
				if (maxs[i])
				{
					count_len[get_bit_count(maxs[i])] -= count[maxs[i]];
					header->LZSSRepeatTable[i] = maxs[i];
					lastmax = maxs[i];
				}
				else
					header->LZSSRepeatTable[i] = lastmax;
			}
		} while (0);
		for (max = 14; !count_len[max] && max > 0; max--);
		for (min = 1; !count_len[min] && min < 15; min++);
		if (max < min)
		{
			header->LZSSRepeatCodeLengthTable[0] =
				header->LZSSRepeatCodeLengthTable[1] =
				header->LZSSRepeatCodeLengthTable[2] = 0;
			break;
		}
		for (i = min; i <= max; i++)
			total_len[i] = total_len[i - 1] + count_len[i];
		header->LZSSRepeatCodeLengthTable[0] = min;
		header->LZSSRepeatCodeLengthTable[1] =
			header->LZSSRepeatCodeLengthTable[2] = max;
		total_min = total_len[min] * min + (total_len[max] - total_len[min]) * max;
		for (i = min; i < max - 1; i++)
			if (count_len[i])
			{
				for (j = i + 1; j < max; j++)
					if (count_len[j])
					{
						total = total_len[i] * i + (total_len[j] - total_len[i]) * j + (total_len[max] - total_len[j]) * max;
						if (total < total_min)
						{
							total_min = total;
							header->LZSSRepeatCodeLengthTable[0] = i;
							header->LZSSRepeatCodeLengthTable[1] = j;
						}
					}
			}
		total_bits += total_min + total_len[max] * 3;
	} while (0);

	do
	{
		INT32 k, tmp;
		memset(count_len, 0, 15 * sizeof(INT32));
		memset(total_len, 0, 15 * sizeof(INT32));
		for (ptr = 0; ptr < (cb_len >> 1);)
		{
			UINT16 temp = block[ptr] & 0x7fff;
			if (block[ptr++] & 0x8000)
				for (i = 0; i < temp; i++)
				{
					ptr++; tmp = block[ptr++];
					count_len[get_bit_count(tmp)]++;
				}
		}
		for (max = 14; !count_len[max] && max > 0; max--);
		for (min = 1; !count_len[min] && min < 15; min++);
		if (max < min)
		{
			header->LZSSOffsetCodeLengthTable[0] =
				header->LZSSOffsetCodeLengthTable[1] =
				header->LZSSOffsetCodeLengthTable[2] =
				header->LZSSOffsetCodeLengthTable[3] = 0;
			break;
		}
		for (i = min; i <= max; i++)
			total_len[i] = total_len[i - 1] + count_len[i];
		header->LZSSOffsetCodeLengthTable[0] = min;
		header->LZSSOffsetCodeLengthTable[2] =
			header->LZSSOffsetCodeLengthTable[3] = max;
		for (i = min + 1; i < max; i++)
			if (count_len[i])
			{
				header->LZSSOffsetCodeLengthTable[1] = i;
				break;
			}
		if (i < max)
			total_min = total_len[min] * min + (total_len[i] - total_len[min]) * i + (total_len[max] - total_len[i]) * max;
		else
		{
			header->LZSSOffsetCodeLengthTable[1] = min;
			total_min = total_len[min] * min + (total_len[max] - total_len[min]) * max;
		}
		for (i = min; i < max - 2; i++)
			if (count_len[i])
			{
				for (j = i + 1; j < max - 1; j++)
					if (count_len[j])
					{
						for (k = j + 1; k < max; k++)
							if (count_len[k])
							{
								total = total_len[i] * i + (total_len[j] - total_len[i]) * j +
									(total_len[k] - total_len[j]) * k + (total_len[max] - total_len[k]) * max;
								if (total < total_min)
								{
									total_min = total;
									header->LZSSOffsetCodeLengthTable[0] = i;
									header->LZSSOffsetCodeLengthTable[1] = j;
									header->LZSSOffsetCodeLengthTable[2] = k;
								}
							}
					}
			}
		total_bits += total_min + total_len[max] * 2;
	} while (0);

	return total_bits;
}

static  bool list_insert(TreeNodeList*& head, TreeNode* node)
{
	TreeNodeList* list;
	TreeNodeList* temp;
	if (!head)
	{
		if ((head = new TreeNodeList) == NULL)
			return false;
		head->next = NULL;
		head->node = node;
		return true;
	}

	for (list = head; list->next; list = list->next)
		if (list->node->weight <= node->weight && list->next->node->weight > node->weight)
		{
			if ((temp = new TreeNodeList) == NULL)
				return false;
			temp->next = list->next;
			list->next = temp;
			temp->node = node;
			break;
		}
	if (!list->next)
	{
		if ((list->next = new TreeNodeList) == NULL)
			return false;
		list->next->next = NULL;
		list->next->node = node;
	}
	return true;
}

static  void list_delete(TreeNodeList*& head, TreeNode* node)
{
	TreeNodeList* list;
	TreeNodeList* temp;
	if (!head || !node)
		return;

	if (head->node == node)
	{
		temp = head;
		head = head->next;
		delete temp;
		return;
	}

	for (list = head; list->next; list = list->next)
	{
		if (list->next->node == node)
		{
			temp = list->next;
			list->next = temp->next;
			list = NULL;
			delete temp;
			break;
		}
	}
}

static  TreeNode* build_tree(UINT32 freq[])
{
	TreeNode* root;
	TreeNode* node;
	TreeNodeList* head = NULL;
	UINT32 i;

	for (i = 0; i < 0x100; i++)
		if (freq[i])
		{
			if ((node = new TreeNode) == NULL)
			{
				for (TreeNodeList* temp = head; temp;)
				{
					TreeNodeList* tmp = temp->next;
					delete temp->node;
					delete temp;
					temp = tmp;
				}
				return NULL;
			}
			node->leaf = true;
			node->value = (UINT8)i;
			node->weight = freq[i];
			node->left = node->right = NULL;
			if (!list_insert(head, node))
			{
				delete node;
				return NULL;
			}
		}

	if (!head)
		return NULL;

	if (!head->next)
	{
		if ((root = new TreeNode) == NULL)
		{
			delete head->node;
			delete head;
			return NULL;
		}
		root->left = head->node;
		if ((root->right = new TreeNode) == NULL)
		{
			delete head->node;
			delete head;
			delete root;
			return NULL;
		}
		root->right->leaf = true;
		root->right->left = root->right->right = NULL;
		root->leaf = false;
		root->value = 0;
		root->right->value = ~head->node->value;
		root->left->parent = root->right->parent = root;
		root->parent = NULL;
		delete head;
		return root;
	}

	for (; head->next;)
	{
		if ((node = new TreeNode) == NULL)
		{
			for (TreeNodeList* temp = head; temp;)
			{
				TreeNodeList* tmp = temp->next;
				delete temp->node;
				delete temp;
				temp = tmp;
			}
			return NULL;
		}
		node->left = head->node;
		node->right = head->next->node;
		node->weight = node->left->weight + node->right->weight;
		node->leaf = false;
		node->value = 0;
		node->left->parent = node->right->parent = node;
		list_delete(head, node->left);
		list_delete(head, node->right);
		list_insert(head, node);
	}
	root = head->node;
	root->parent = NULL;
	delete head;

	return root;
}

static  void traverse_tree(TreeNode* root, UINT32 level, UINT32& nodes)
{
	if (!level)
		nodes = 0;
	else
		nodes++;
	root->level = level;

	if (root->leaf)
		return;

	traverse_tree(root->left, level + 1, nodes);
	traverse_tree(root->right, level + 1, nodes);
}

static  UINT32 traverse_tree(TreeNode* root, UINT32 level, UINT32* freq)
{
	if (root->leaf)
		return level * freq[root->value];

	return traverse_tree(root->left, level + 1, freq) + traverse_tree(root->right, level + 1, freq);
}

static  void set_bit(void* dest, UINT32& bitptr, bool bit)
{
	UINT16* temp = ((UINT16*)dest) + (bitptr >> 4);
	UINT32 bptr = 15 - (bitptr & 0xf);
	UINT16 mask = 1 << bptr;
	if (bit)
		*temp |= mask;
	else
		*temp &= ~mask;
	bitptr++;
}

static void set_bits(void* dest, UINT32& bitptr, UINT32 data, UINT32 count)
{
	UINT16* temp = ((UINT16*)dest) + (bitptr >> 4), tmp;
	UINT32 bptr = bitptr & 0xf, cnt;
	UINT16 mask;
	bitptr += count;
	if (count > 16 - bptr)
	{
		cnt = count + bptr - 16;
		mask = 0xffff << (16 - bptr);
		*temp = (*temp & mask) | ((UINT16)(data >> cnt) & (UINT16)(0xffff >> bptr));
		temp[1] = (temp[1] & (UINT16)(0xffff >> cnt)) | (UINT16)(data << (16 - cnt));
	}
	else
	{
		cnt = 16 - count - bptr;
		tmp = (data & (0xffff >> (16 - count))) << cnt;
		*temp = (*temp & (UINT16)(~((0xffff >> bptr) & (0xffff << cnt)))) | tmp;
	}
}

static void set_loop(void* dest, UINT32& ptr, UINT32 count, PYJ_1_BLOCKHEADER header)
{
	if (count == header->CodeCountTable[0])
		set_bit(dest, ptr, 1);
	else
	{
		set_bit(dest, ptr, 0);
		if (count == header->CodeCountTable[1])
			set_bits(dest, ptr, 0, 2);
		else
		{
			UINT16 cnt = get_bit_count(count);
			UINT32 j;
			for (j = 0; j < 3; j++)
				if (cnt <= header->CodeCountCodeLengthTable[j])
				{
					set_bits(dest, ptr, j + 1, 2);
					set_bits(dest, ptr, count, header->CodeCountCodeLengthTable[j]);
					return;
				}
		}
	}
}

static void set_count(void* dest, UINT32& ptr, UINT32 match_len, PYJ_1_BLOCKHEADER header)
{
	UINT32 k;
	UINT16 cnt;
	for (k = 0; k < 4; k++)
		if (match_len == header->LZSSRepeatTable[k])
		{
			set_bits(dest, ptr, k, 2);
			if (k > 0)
				set_bit(dest, ptr, 0);
			return;
		}
	cnt = get_bit_count(match_len);
	for (k = 0; k < 3; k++)
		if (cnt <= header->LZSSRepeatCodeLengthTable[k])
		{
			set_bits(dest, ptr, k + 1, 2);
			set_bit(dest, ptr, 1);
			set_bits(dest, ptr, match_len, header->LZSSRepeatCodeLengthTable[k]);
			return;
		}
}

INT CPalEdit::PAL_DeCompress(LPCVOID Source, LPVOID Destination, INT DestSize)
{
	if (Use_YJ1_Decompress)
		return YJ1_Decompress(Source, Destination, DestSize);
	else
		return YJ2_Decompress(Source, Destination, DestSize);
}

static void TreeNodeDelete(TreeNode* p)
{
	if (p->left)TreeNodeDelete(p->left);
	if (p->right)TreeNodeDelete(p->right);
	delete p;
}


////
int  CPalEdit::EncodeYJ1(const void* Source, UINT32 SourceLength, void*& Destination, UINT32& Length)
{
	YJ_1_FILEHEADER hdr;
	PYJ_1_BLOCKHEADER header;
	UINT16 code[0x100][0x10];
	UINT32 freq[0x100];
	UINT32* cb_len;
	UINT32* lz_len;
	UINT32** bfreq;
	UINT32 tree_nodes;
	UINT8** block;
	UINT8* src = (UINT8*)Source;
	UINT8* dest;
	INT32 srclen = SourceLength;
	UINT32 length;
	void* pNewData;
	TreeNode* root;
	TreeNode* leaf[0x100];

	if (Source == NULL || SourceLength == 0)
		return PAL_EMPTY_POINTER;

	strncpy((char*)&hdr.Signature, "YJ_1", 4);
	hdr.UncompressedLength = srclen;
	hdr.CompressedLength = 0;
	hdr.BlockCount = (srclen & 0x3fff) ? (srclen >> 14) + 1 : (srclen >> 14);
	hdr.Unknown = 0xff;
	hdr.HuffmanTreeLength = 0;

	memset(freq, 0, 0x100 * sizeof(INT32));
	memset(code, 0, 0x100 * 0x10 * sizeof(INT16));
	memset(leaf, 0, 0x100 * sizeof(TreeNode*));
	if ((bfreq = new UINT32 * [hdr.BlockCount]) == NULL)
		return PAL_OUT_OF_MEMORY;
	if ((block = new UINT8 * [hdr.BlockCount]) == NULL)
	{
		delete[] bfreq;
		return PAL_OUT_OF_MEMORY;
	}
	if ((cb_len = new UINT32[hdr.BlockCount]) == NULL)
	{
		delete[] block;
		delete[] bfreq;
		return PAL_OUT_OF_MEMORY;
	}
	if ((lz_len = new UINT32[hdr.BlockCount]) == NULL)
	{
		delete[] cb_len;
		delete[] block;
		delete[] bfreq;
		return PAL_OUT_OF_MEMORY;
	}
	if ((header = new YJ_1_BLOCKHEADER[hdr.BlockCount]) == NULL)
	{
		delete[] lz_len;
		delete[] cb_len;
		delete[] block;
		delete[] bfreq;
		return PAL_OUT_OF_MEMORY;
	}

	for (INT32 i = 0; i < hdr.BlockCount; i++)
	{
		UINT32	baseptr;
		INT32	j;
		UINT8* base;

		baseptr = i << 14; base = (UINT8*)src + baseptr;
		header[i].UncompressedLength = (srclen - baseptr < 0x4000) ? srclen - baseptr : 0x4000;
		header[i].CompressedLength = sizeof(YJ_1_BLOCKHEADER);
		if ((bfreq[i] = new UINT32[0x100]) == NULL)
		{
			for (j = 0; j < i; j++)
			{
				delete[] block[j];
				delete[] bfreq[j];
			}
			delete[] header;
			delete[] lz_len;
			delete[] cb_len;
			delete[] block;
			delete[] bfreq;
			return PAL_OUT_OF_MEMORY;
		}
		if ((block[i] = new UINT8[0xa000]) == NULL)
		{
			for (j = 0; j < i; j++)
			{
				delete[] block[j];
				delete[] bfreq[j];
			}
			delete[] bfreq[i];
			delete[] header;
			delete[] lz_len;
			delete[] cb_len;
			delete[] block;
			delete[] bfreq;
			return PAL_OUT_OF_MEMORY;
		}
		memset(bfreq[i], 0, 0x100 * sizeof(INT32));
		cb_len[i] = lz_analysize(base, (UINT16*)block[i], header[i].UncompressedLength, bfreq[i]);
		lz_len[i] = cb_analysize(header + i, (UINT16*)block[i], cb_len[i]);
		for (j = 0; j < 0x100; j++)
			freq[j] += bfreq[i][j];

		base = block[i];
		if ((block[i] = new UINT8[cb_len[i]]) == NULL)
		{
			for (j = 0; j < i; j++)
			{
				delete[] block[j];
				delete[] bfreq[j];
			}
			delete[] bfreq[i];
			delete[] base;
			delete[] header;
			delete[] lz_len;
			delete[] cb_len;
			delete[] block;
			delete[] bfreq;
			return PAL_OUT_OF_MEMORY;
		}
		memcpy(block[i], base, cb_len[i]);
		delete[] base;
	}

	if ((root = build_tree(freq)) == NULL)
	{
		for (INT32 i = 0; i < hdr.BlockCount; i++)
		{
			delete[] block[i];
			delete[] bfreq[i];
		}
		delete[] header;
		delete[] lz_len;
		delete[] cb_len;
		delete[] block;
		delete[] bfreq;
		return PAL_OUT_OF_MEMORY;
	}
	traverse_tree(root, 0, tree_nodes);
	hdr.HuffmanTreeLength = tree_nodes >> 1;
	hdr.CompressedLength = sizeof(YJ_1_FILEHEADER) + tree_nodes;
	if (tree_nodes & 0xf)
	{
		UINT16 _tmp = (tree_nodes >> 4) + 1;
		hdr.CompressedLength += _tmp << 1;
	}
	else
		hdr.CompressedLength += tree_nodes >> 3;
	for (INT32 i = 0; i < hdr.BlockCount; i++)
	{
		UINT32 len = lz_len[i] + traverse_tree(root, 0, bfreq[i]);
		len += header[i].CodeCountCodeLengthTable[0] + 3;
		if (len & 0xf)
			len = (len >> 4) + 1;
		else
			len >>= 4;
		header[i].CompressedLength += len << 1;
		hdr.CompressedLength += header[i].CompressedLength;
	}

	length = hdr.CompressedLength;
	if ((pNewData = dest = (UINT8*)malloc(hdr.CompressedLength)) == NULL)
	{
		for (INT32 i = 0; i < hdr.BlockCount; i++)
		{
			delete[] block[i];
			delete[] bfreq[i];
		}
		delete[] header;
		delete[] lz_len;
		delete[] cb_len;
		delete[] block;
		delete[] bfreq;
		return PAL_OUT_OF_MEMORY;
	}
	memcpy(dest, &hdr, sizeof(YJ_1_FILEHEADER));
	dest += sizeof(YJ_1_FILEHEADER);
	do
	{
		UINT32 head = 0, tail = 0, ptr = 0, i;
		UINT8* dst = dest + tree_nodes;
		TreeNode* queue[0x200];
		TreeNode* node;

#define	put_in(v)	\
	if (tail < 0x1ff)	\
		queue[tail++] = (v);	\
	else	\
		queue[tail = 0] = (v)
#define	get_out(v)	\
	if (head < 0x1ff)	\
		(v) = queue[head++];	\
	else	\
		(v) = queue[head = 0]

		put_in(root->left);
		put_in(root->right);
		for (i = 0; i < tree_nodes; i++)
		{
			get_out(node);
			if (node->leaf)
			{
				leaf[node->value] = node;
				*dest++ = node->value;
			}
			else
			{
				*dest++ = tail >> 1;
				put_in(node->left);
				put_in(node->right);
			}
			set_bit(dst, ptr, !node->leaf);
		}

#undef	get_out
#undef	put_in

		if (ptr & 0xf)
		{
			i = ((ptr >> 4) + 1) << 4;
			for (; ptr < i;)
				set_bit(dst, ptr, 0);
		}
		dest += ptr >> 3;

		for (i = 0; i < 0x100; i++)
			if (leaf[i])
			{
				UINT32 k = 0;
				UINT32 hcode[0x8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
				node = leaf[i];
				while (node->parent)
				{
					hcode[k >> 5] <<= 1;
					if (node == node->parent->right)
						hcode[k >> 5] |= 1;
					k++; node = node->parent;
				}
				for (k = 0; k < leaf[i]->level; k++)
				{
					code[i][k >> 4] <<= 1;
					code[i][k >> 4] |= hcode[k >> 5] & 0x1;
					hcode[k >> 5] >>= 1;
				}
			}

	} while (0);

	for (UINT32 i = 0; i < hdr.BlockCount; i++)
	{
		UINT16* bptr = (UINT16*)block[i], count, match_len, pos;
		UINT8* base = (UINT8*)src + (i << 14);
		UINT8* dst = dest;
		UINT32 sptr = 0, ptr = 0, j;
		TreeNode* node;
		memcpy(dest, header + i, sizeof(YJ_1_BLOCKHEADER));
		dest += sizeof(YJ_1_BLOCKHEADER);
		while (sptr < header[i].UncompressedLength)
		{
			set_loop(dest, ptr, *bptr & 0x7fff, header + i);
			count = *bptr++;
			if (count & 0x8000)
			{
				count &= 0x7fff;
				for (j = 0; j < count; j++)
				{
					match_len = *bptr++;
					pos = *bptr++;
					sptr += match_len;
					set_count(dest, ptr, match_len, header + i);
					do
					{
						UINT32 k;
						UINT16 cnt = get_bit_count(pos);
						for (k = 0; k < 4; k++)
							if (cnt <= header[i].LZSSOffsetCodeLengthTable[k])
							{
								set_bits(dest, ptr, k, 2);
								set_bits(dest, ptr, pos, header[i].LZSSOffsetCodeLengthTable[k]);
								break;
							}
					} while (0);
				}
			}
			else
			{
				UINT8 val;
				UINT32 maxl;
				for (j = 0; j < count; j++)
				{
					val = base[sptr++];
					node = leaf[val];
					maxl = node->level >> 4;
					if (node->level & 0xf)
						set_bits(dest, ptr, code[val][maxl], node->level & 0xf);
					while (maxl--)
						set_bits(dest, ptr, code[val][maxl], 16);
				}
			}
		}
		set_loop(dest, ptr, 0, header + i);
		if (ptr & 0xf)
			set_bits(dest, ptr, 0, 16 - (ptr & 0xf));
		ptr >>= 3;
		dest += ptr;
	}

	for (INT32 i = 0; i < hdr.BlockCount; i++)
	{
		delete[] block[i]; block[i] = 0;
		delete[] bfreq[i]; bfreq[i] = 0;
	}
	delete[] header; header = 0;
	delete[] lz_len; lz_len = 0;
	delete[] cb_len; cb_len = 0;
	delete[] block; block = 0;
	delete[] bfreq; bfreq = 0;
	//root链表未删除，删除
	TreeNodeDelete(root);
	Destination = pNewData;
	Length = length;
	return PAL_OK;
}



/////////////////////////////////////////////////////////////////////
//YJ2
/////////////////////////////////////////////////////////////////////



typedef struct _Tree
{
	TreeNode* node;
	TreeNode** list;
} Tree;

static UINT8 data1[0x100] =
{
0x3f,0x0b,0x17,0x03,0x2f,0x0a,0x16,0x00,0x2e,0x09,0x15,0x02,0x2d,0x01,0x08,0x00,
0x3e,0x07,0x14,0x03,0x2c,0x06,0x13,0x00,0x2b,0x05,0x12,0x02,0x2a,0x01,0x04,0x00,
0x3d,0x0b,0x11,0x03,0x29,0x0a,0x10,0x00,0x28,0x09,0x0f,0x02,0x27,0x01,0x08,0x00,
0x3c,0x07,0x0e,0x03,0x26,0x06,0x0d,0x00,0x25,0x05,0x0c,0x02,0x24,0x01,0x04,0x00,
0x3b,0x0b,0x17,0x03,0x23,0x0a,0x16,0x00,0x22,0x09,0x15,0x02,0x21,0x01,0x08,0x00,
0x3a,0x07,0x14,0x03,0x20,0x06,0x13,0x00,0x1f,0x05,0x12,0x02,0x1e,0x01,0x04,0x00,
0x39,0x0b,0x11,0x03,0x1d,0x0a,0x10,0x00,0x1c,0x09,0x0f,0x02,0x1b,0x01,0x08,0x00,
0x38,0x07,0x0e,0x03,0x1a,0x06,0x0d,0x00,0x19,0x05,0x0c,0x02,0x18,0x01,0x04,0x00,
0x37,0x0b,0x17,0x03,0x2f,0x0a,0x16,0x00,0x2e,0x09,0x15,0x02,0x2d,0x01,0x08,0x00,
0x36,0x07,0x14,0x03,0x2c,0x06,0x13,0x00,0x2b,0x05,0x12,0x02,0x2a,0x01,0x04,0x00,
0x35,0x0b,0x11,0x03,0x29,0x0a,0x10,0x00,0x28,0x09,0x0f,0x02,0x27,0x01,0x08,0x00,
0x34,0x07,0x0e,0x03,0x26,0x06,0x0d,0x00,0x25,0x05,0x0c,0x02,0x24,0x01,0x04,0x00,
0x33,0x0b,0x17,0x03,0x23,0x0a,0x16,0x00,0x22,0x09,0x15,0x02,0x21,0x01,0x08,0x00,
0x32,0x07,0x14,0x03,0x20,0x06,0x13,0x00,0x1f,0x05,0x12,0x02,0x1e,0x01,0x04,0x00,
0x31,0x0b,0x11,0x03,0x1d,0x0a,0x10,0x00,0x1c,0x09,0x0f,0x02,0x1b,0x01,0x08,0x00,
0x30,0x07,0x0e,0x03,0x1a,0x06,0x0d,0x00,0x19,0x05,0x0c,0x02,0x18,0x01,0x04,0x00
};
static UINT8 data2[0x10] =
{
0x08,0x05,0x06,0x04,0x07,0x05,0x06,0x03,0x07,0x05,0x06,0x04,0x07,0x04,0x05,0x03
};
static UINT8 data3[0x40] =
{
0x07,0x0d,0x0b,0x03,0x1e,0x19,0x15,0x11,0x0e,0x09,0x05,0x01,0x3a,0x36,0x32,0x2a,
0x26,0x22,0x1a,0x16,0x12,0x0a,0x06,0x02,0x7c,0x78,0x74,0x6c,0x68,0x64,0x5c,0x58,
0x54,0x4c,0x48,0x44,0x3c,0x38,0x34,0x2c,0x28,0x24,0x1c,0x18,0x14,0x0c,0x08,0x04,
0xf0,0xe0,0xd0,0xc0,0xb0,0xa0,0x90,0x80,0x70,0x60,0x50,0x40,0x30,0x20,0x10,0x00
};
static UINT8 data4[0x40] =
{
0x03,0x04,0x04,0x04,0x05,0x05,0x05,0x05,0x05,0x05,0x05,0x05,0x06,0x06,0x06,0x06,
0x06,0x06,0x06,0x06,0x06,0x06,0x06,0x06,0x07,0x07,0x07,0x07,0x07,0x07,0x07,0x07,
0x07,0x07,0x07,0x07,0x07,0x07,0x07,0x07,0x07,0x07,0x07,0x07,0x07,0x07,0x07,0x07,
0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08
};

static void adjust_tree(Tree tree, UINT16 value)
{
	TreeNode* node = tree.list[value];
	TreeNode tmp;
	TreeNode* tmp1;
	TreeNode* temp;
	while (node->value != 0x280)
	{
		temp = node + 1;
		while (node->weight == temp->weight)
			temp++;
		temp--;
		if (temp != node)
		{
			tmp1 = node->parent;
			node->parent = temp->parent;
			temp->parent = tmp1;
			if (node->value > 0x140)
			{
				node->left->parent = temp;
				node->right->parent = temp;
			}
			else
				tree.list[node->value] = temp;
			if (temp->value > 0x140)
			{
				temp->left->parent = node;
				temp->right->parent = node;
			}
			else
				tree.list[temp->value] = node;
			tmp = *node; *node = *temp; *temp = tmp;
			node = temp;
		}
		node->weight++;
		node = node->parent;
	}
	node->weight++;
}

static bool build_tree(Tree& tree)
{
	INT32 i, ptr;
	TreeNode** list;
	TreeNode* node;
	if ((tree.list = list = new TreeNode * [321]) == NULL)
		return false;
	if ((tree.node = node = new TreeNode[641]) == NULL)
	{
		delete[] list;
		return false;
	}
	memset(list, 0, 321 * sizeof(TreeNode*));
	memset(node, 0, 641 * sizeof(TreeNode));
	for (i = 0; i <= 0x140; i++)
		list[i] = node + i;
	for (i = 0; i <= 0x280; i++)
	{
		node[i].value = i;
		node[i].weight = 1;
	}
	tree.node[0x280].parent = tree.node + 0x280;
	for (i = 0, ptr = 0x141; ptr <= 0x280; i += 2, ptr++)
	{
		node[ptr].left = node + i;
		node[ptr].right = node + i + 1;
		node[i].parent = node[i + 1].parent = node + ptr;
		node[ptr].weight = node[i].weight + node[i + 1].weight;
	}
	return true;
}

#pragma pack(1)
typedef struct _BitField
{
	UINT8	b0 : 1;
	UINT8	b1 : 1;
	UINT8	b2 : 1;
	UINT8	b3 : 1;
	UINT8	b4 : 1;
	UINT8	b5 : 1;
	UINT8	b6 : 1;
	UINT8	b7 : 1;
} BitField;
#pragma pack()

static inline bool bt(const void* data, UINT32 pos)
{
	BitField* bit = (BitField*)((UINT8*)data + (pos >> 3));
	switch (pos & 0x7)
	{
	case 0:	return bit->b0;
	case 1:	return bit->b1;
	case 2:	return bit->b2;
	case 3:	return bit->b3;
	case 4:	return bit->b4;
	case 5:	return bit->b5;
	case 6:	return bit->b6;
	case 7:	return bit->b7;
	}
	return 0;
}

static inline void bit(void* data, UINT32 pos, bool set)
{
	BitField* bit = (BitField*)((UINT8*)data + (pos >> 3));
	switch (pos & 0x7)
	{
	case 0:
		bit->b0 = set;
		break;
	case 1:
		bit->b1 = set;
		break;
	case 2:
		bit->b2 = set;
		break;
	case 3:
		bit->b3 = set;
		break;
	case 4:
		bit->b4 = set;
		break;
	case 5:
		bit->b5 = set;
		break;
	case 6:
		bit->b6 = set;
		break;
	case 7:
		bit->b7 = set;
		break;
	}
}



int CPalEdit::EncodeYJ2(const void* Source, UINT32 SourceLength, void*& Destination, UINT32& Length, bool bCompatible)
{
	UINT32 len = 0, ptr, pos, top, srclen = SourceLength;
	INT32 head[0x100]{ 0 }, _prev[0x2000]{0};
	INT32* prev;
	INT32* pre;
	unsigned long dptr = 0;
	UINT8* src = (UINT8*)Source;
	UINT8* dest;
	bool code[0x140]{false};
	Tree tree;
	TreeNode* node;
	void* pNewData;

	if (Source == NULL)
		return PAL_EMPTY_POINTER;

	if (!build_tree(tree))
		return PAL_OUT_OF_MEMORY;

	if ((pNewData = malloc(SourceLength + 260)) == NULL)
	{
		delete[] tree.list;
		delete[] tree.node;
		return PAL_OUT_OF_MEMORY;
	}
	dest = (UINT8*)pNewData + 4;

	pre = prev = _prev;
	memset(head, 0xff, 0x100 * sizeof(INT32));
	memset(prev, 0xff, 0x2000 * sizeof(INT32));
	prev += 0x1000;
	for (ptr = 0; ptr + 2 < srclen && ptr < 0x1000; ptr++)
	{
		UINT8 hash = src[ptr] ^ src[ptr + 1] ^ src[ptr + 2];
		if (head[hash] >= 0)
			prev[ptr] = head[hash];
		head[hash] = ptr;
	}
	ptr = 0; top = 0x1000;
	while (ptr < srclen)
	{
		UINT16 val = src[ptr];
		INT32 temp = (INT32)ptr, prv, cur;
		UINT32 match_len_t, table, match = 0, match_len = 1;
		if (ptr >= top)
		{
			memmove(_prev, _prev + 0x1000, 0x1000 * sizeof(INT32));
			prev -= 0x1000; top += 0x1000;
			for (ptr = top - 0x1000; ptr + 2 < srclen && ptr < top; ptr++)
			{
				UINT8 hash = src[ptr] ^ src[ptr + 1] ^ src[ptr + 2];
				if (head[hash] >= 0 && ptr - head[hash] < 0x1000)
					prev[ptr] = head[hash];
				head[hash] = ptr;
			}
			ptr = temp;
		}
		while ((prv = prev[temp]) >= 0 && ptr - prev[temp] < 0x1000)
		{
			cur = ptr; match_len_t = 0;
			while (match_len_t < 67 && ptr + match_len_t < srclen && src[prv++] == src[cur++])
				match_len_t++;
			if (match_len_t >= 3 && match_len < match_len_t)
			{
				match_len = match_len_t;
				match = ptr - prev[temp] - 1;
				if (match_len == 67 || ptr + match_len >= srclen)
					break;
			}
			temp = prev[temp];
		}
		if (match_len >= 3)
			val = match_len + 0xfd;
		else
			match_len = 1;
		node = tree.list[val]; pos = 0;
		while (node->value != 0x280)
		{
			assert(pos < 320);
			if (node->parent->left == node)
				code[pos++] = false;
			else
				code[pos++] = true;
			node = node->parent;
		}
		while (pos > 0)
			bit(dest, dptr++, code[--pos]);
		if (tree.node[0x280].weight == 0x8000)
		{
			INT32 i;
			for (i = 0; i < 0x141; i++)
				if (tree.list[i]->weight & 0x1)
					adjust_tree(tree, i);
			for (i = 0; i <= 0x280; i++)
				tree.node[i].weight >>= 1;
		}
		adjust_tree(tree, val);
		if (val > 0xff)
		{
			INT32 shift = 0;
			UINT32 tmp = (match >> 6) & 0x3f;
			table = ((match << data4[tmp]) & 0xff) | data3[tmp];
			tmp = table | (((match & 0x3f) >> (8 - data2[table & 0xf])) << 8);
			while (shift < data2[table & 0xf] + 6)
			{
				bit(dest, dptr++, tmp & 0x1);
				shift++; tmp >>= 1;
			}
		}
		ptr += match_len;
	}
	node = tree.list[0x140]; pos = 0;
	while (node->value != 0x280)
	{
		if (node->parent->left == node)
			code[pos++] = false;
		else
			code[pos++] = true;
		node = node->parent;
	}
	top = pos;
	while (pos > 0)
		bit(dest, dptr++, code[--pos]);
	for (INT32 i = 0; i < 8; i++)
		bit(dest, dptr++, 0);
	for (INT32 i = 0; i < 6; i++)
		bit(dest, dptr++, 1);
	len = dptr >> 3;
	if (dptr & 0x7)
	{
		for (UINT32 i = dptr & 0x7; i < 8; i++)
			bit(dest + len, i, 0);
		len++;
	}
	if (bCompatible)
	{
		UINT32 temp = ((dptr - top - 14) >> 3) + 4;
		for (; len < temp && len + 4 < Length; len++)
			dest[len] = 0;
	}
	len += 4;

	if ((Destination = realloc(pNewData, len)) == NULL)
	{
		free(pNewData);
		delete[] tree.list;
		delete[] tree.node;
		return PAL_OUT_OF_MEMORY;
	}
	*((UINT32*)Destination) = srclen;
	Length = len;

	delete[] tree.list;
	delete[] tree.node;

	return PAL_OK;
}

int CPalEdit::EnCompress(const void* Source, UINT32 SourceLength, void*& Destination, UINT32& Length, bool bCopatible)
{
	if (Use_YJ1_Decompress)
		return EncodeYJ1( Source, SourceLength,  Destination,  Length);
	else
		return EncodeYJ2(Source, SourceLength, Destination, Length, bCopatible);
}
