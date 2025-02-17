#include <lib/k2dis.h>

char* arm_32a_s(UINT8 s)
{
	if (s)
	{
		return "s";
	}
	else
	{
		return "\0";
	}
}

char* arm_32a_c4(UINT8 c)
{
	if (c == 14)
	{
		return "\0";
	}
	else if (c == 0)
	{
		return "eq";
	}
	else if (c == 1)
	{
		return "ne";
	}
	else if (c == 2)
	{
		return "cs";
	}
	else if (c == 3)
	{
		return "lo";
	}
	else if (c == 4)
	{
		return "mi";
	}
	else if (c == 5)
	{
		return "pl";
	}
	else if (c == 6)
	{
		return "vs";
	}
	else if (c == 7)
	{
		return "vc";
	}
	else if (c == 8)
	{
		return "hi";
	}
	else if (c == 9)
	{
		return "ls";
	}
	else if (c == 10)
	{
		return "ge";
	}
	else if (c == 11)
	{
		return "lt";
	}
	else if (c == 12)
	{
		return "gt";
	}
	else if (c == 13)
	{
		return "le";
	}
	K2_ASSERT(0);
	return "??";
}

char* arm_32a_sh(UINT8 sh)
{
	if (sh == 0)
	{
		return "lsl";
	}
	else if (sh == 32)
	{
		return "lsr";
	}
	else if (sh == 64)
	{
		return "asr";
	}
	else if (sh == 96)
	{
		return "ror";
	}
	K2_ASSERT(0);
	return "???";
}

void
K2DIS_a32(
	char *          apOutput,
	UINT_PTR        aMaxLen,
	UINT8 const *   apTextSegBase,
	UINT32 *        apSegOffset,
	UINT32 *        apRetDataAddr
)
{
	UINT8	i;
	UINT32	v;

	v = K2ASC_PrintfLen(apOutput, aMaxLen, "%04X%04X    ", ((((UINT32)apTextSegBase) + *apSegOffset) >> 16) & 65535, (((UINT32)apTextSegBase) + *apSegOffset) & 65535); //address
	
	aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "%02X%02X%02X%02X    ", apTextSegBase[*apSegOffset + 3], apTextSegBase[*apSegOffset + 2], apTextSegBase[*apSegOffset + 1], apTextSegBase[*apSegOffset]); //machine code

	if ((apTextSegBase[*apSegOffset + 3] & 14) == 0 && (apTextSegBase[*apSegOffset + 2] & 48) == 0 && (apTextSegBase[*apSegOffset] & 240) == 176)
	{
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "strh%s ", arm_32a_c4(apTextSegBase[*apSegOffset + 3] >> 4));
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", (apTextSegBase[*apSegOffset + 1] >> 4) & 15);
		if ((apTextSegBase[*apSegOffset + 3] & 1) && (apTextSegBase[*apSegOffset + 2] & 192) == 64)
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(r%u, ", apTextSegBase[*apSegOffset + 2] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "%i) ", -1 * ((apTextSegBase[*apSegOffset] & 15) + ((apTextSegBase[*apSegOffset + 1] & 15) << 4)));
		}
		else if ((apTextSegBase[*apSegOffset + 3] & 1) && (apTextSegBase[*apSegOffset + 2] & 192) == 192)
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(r%u, ", apTextSegBase[*apSegOffset + 2] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "%i) ", ((apTextSegBase[*apSegOffset] & 15) + ((apTextSegBase[*apSegOffset + 1] & 15) << 4)));
		}
		else if ((apTextSegBase[*apSegOffset + 2] & 192) == 64)
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(r%u), ", apTextSegBase[*apSegOffset + 2] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "%i ", -1 * ((apTextSegBase[*apSegOffset] & 15) + ((apTextSegBase[*apSegOffset + 1] & 15) << 4)));
		}
		else if ((apTextSegBase[*apSegOffset + 2] & 192) == 192)
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(r%u), ", apTextSegBase[*apSegOffset + 2] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "%i ", ((apTextSegBase[*apSegOffset] & 15) + ((apTextSegBase[*apSegOffset + 1] & 15) << 4)));
		}
		else if ((apTextSegBase[*apSegOffset + 3] & 1) && (apTextSegBase[*apSegOffset + 2] & 192) == 128)
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(r%u, ", apTextSegBase[*apSegOffset + 2] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u) ", apTextSegBase[*apSegOffset] & 15);
		}
		else if ((apTextSegBase[*apSegOffset + 2] & 192) == 128)
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(r%u), ", apTextSegBase[*apSegOffset + 2] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u ", apTextSegBase[*apSegOffset] & 15);
		}
		*apSegOffset += 4;
	}
	else if ((apTextSegBase[*apSegOffset + 3] & 15) == 0 && (apTextSegBase[*apSegOffset + 2] & 48) == 32 && (apTextSegBase[*apSegOffset] & 240) == 176)
	{
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "strht%s ", arm_32a_c4(apTextSegBase[*apSegOffset + 3] >> 4));
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", (apTextSegBase[*apSegOffset + 1] >> 4) & 15);
		if ((apTextSegBase[*apSegOffset + 2] & 192) == 64)
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(r%u), ", apTextSegBase[*apSegOffset + 2] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "%i ", -1 * ((apTextSegBase[*apSegOffset] & 15) + ((apTextSegBase[*apSegOffset + 1] & 15) << 4)));
		}
		else if ((apTextSegBase[*apSegOffset + 2] & 192) == 192)
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(r%u), ", apTextSegBase[*apSegOffset + 2] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "%i ", ((apTextSegBase[*apSegOffset] & 15) + ((apTextSegBase[*apSegOffset + 1] & 15) << 4)));
		}
		else if ((apTextSegBase[*apSegOffset + 2] & 192) == 128)
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(r%u), ", apTextSegBase[*apSegOffset + 2] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u ", apTextSegBase[*apSegOffset] & 15);
		}
		*apSegOffset += 4;
	}
	else if ((apTextSegBase[*apSegOffset + 3] & 15) == 1 && (apTextSegBase[*apSegOffset + 2] & 48) == 32 && (apTextSegBase[*apSegOffset] & 240) == 176)
	{
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "strhw%s ", arm_32a_c4(apTextSegBase[*apSegOffset + 3] >> 4));
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", (apTextSegBase[*apSegOffset + 1] >> 4) & 15);
		if ((apTextSegBase[*apSegOffset + 2] & 192) == 64)
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(r%u, ", apTextSegBase[*apSegOffset + 2] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "%i) ", -1 * ((apTextSegBase[*apSegOffset] & 15) + ((apTextSegBase[*apSegOffset + 1] & 15) << 4)));
		}
		else if ((apTextSegBase[*apSegOffset + 2] & 192) == 192)
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(r%u, ", apTextSegBase[*apSegOffset + 2] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "%i) ", ((apTextSegBase[*apSegOffset] & 15) + ((apTextSegBase[*apSegOffset + 1] & 15) << 4)));
		}
		else if ((apTextSegBase[*apSegOffset + 2] & 192) == 128)
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(r%u, ", apTextSegBase[*apSegOffset + 2] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u) ", apTextSegBase[*apSegOffset] & 15);
		}
		*apSegOffset += 4;
	}
	else if ((apTextSegBase[*apSegOffset + 3] & 14) == 0 && (apTextSegBase[*apSegOffset + 2] & 48) == 16 && (apTextSegBase[*apSegOffset] & 240) == 176)
	{
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "ldrh%s ", arm_32a_c4(apTextSegBase[*apSegOffset + 3] >> 4));
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", (apTextSegBase[*apSegOffset + 1] >> 4) & 15);
		if ((apTextSegBase[*apSegOffset + 3] & 1) && (apTextSegBase[*apSegOffset + 2] & 192) == 64)
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(r%u, ", apTextSegBase[*apSegOffset + 2] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "%i) ", -1 * ((apTextSegBase[*apSegOffset] & 15) + ((apTextSegBase[*apSegOffset + 1] & 15) << 4)));
		}
		else if ((apTextSegBase[*apSegOffset + 3] & 1) && (apTextSegBase[*apSegOffset + 2] & 192) == 192)
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(r%u, ", apTextSegBase[*apSegOffset + 2] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "%i) ", ((apTextSegBase[*apSegOffset] & 15) + ((apTextSegBase[*apSegOffset + 1] & 15) << 4)));
		}
		else if ((apTextSegBase[*apSegOffset + 2] & 192) == 64)
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(r%u), ", apTextSegBase[*apSegOffset + 2] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "%i ", -1 * ((apTextSegBase[*apSegOffset] & 15) + ((apTextSegBase[*apSegOffset + 1] & 15) << 4)));
		}
		else if ((apTextSegBase[*apSegOffset + 2] & 192) == 192)
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(r%u), ", apTextSegBase[*apSegOffset + 2] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "%i ", ((apTextSegBase[*apSegOffset] & 15) + ((apTextSegBase[*apSegOffset + 1] & 15) << 4)));
		}
		else if ((apTextSegBase[*apSegOffset + 3] & 1) && (apTextSegBase[*apSegOffset + 2] & 192) == 128)
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(r%u, ", apTextSegBase[*apSegOffset + 2] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u) ", apTextSegBase[*apSegOffset] & 15);
		}
		else if ((apTextSegBase[*apSegOffset + 2] & 192) == 128)
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(r%u), ", apTextSegBase[*apSegOffset + 2] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u ", apTextSegBase[*apSegOffset] & 15);
		}
		*apSegOffset += 4;
	}
	else if ((apTextSegBase[*apSegOffset + 3] & 15) == 0 && (apTextSegBase[*apSegOffset + 2] & 48) == 48 && (apTextSegBase[*apSegOffset] & 240) == 176)
	{
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "ldrht%s ", arm_32a_c4(apTextSegBase[*apSegOffset + 3] >> 4));
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", (apTextSegBase[*apSegOffset + 1] >> 4) & 15);
		if ((apTextSegBase[*apSegOffset + 2] & 192) == 64)
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(r%u), ", apTextSegBase[*apSegOffset + 2] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "%i ", -1 * ((apTextSegBase[*apSegOffset] & 15) + ((apTextSegBase[*apSegOffset + 1] & 15) << 4)));
		}
		else if ((apTextSegBase[*apSegOffset + 2] & 192) == 192)
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(r%u), ", apTextSegBase[*apSegOffset + 2] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "%i ", ((apTextSegBase[*apSegOffset] & 15) + ((apTextSegBase[*apSegOffset + 1] & 15) << 4)));
		}
		else if ((apTextSegBase[*apSegOffset + 2] & 192) == 128)
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(r%u), ", apTextSegBase[*apSegOffset + 2] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u ", apTextSegBase[*apSegOffset] & 15);
		}
		*apSegOffset += 4;
	}
	else if ((apTextSegBase[*apSegOffset + 3] & 15) == 1 && (apTextSegBase[*apSegOffset + 2] & 48) == 48 && (apTextSegBase[*apSegOffset] & 240) == 176)
	{
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "ldrhw%s ", arm_32a_c4(apTextSegBase[*apSegOffset + 3] >> 4));
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", (apTextSegBase[*apSegOffset + 1] >> 4) & 15);
		if ((apTextSegBase[*apSegOffset + 2] & 192) == 64)
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(r%u, ", apTextSegBase[*apSegOffset + 2] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "%i) ", -1 * ((apTextSegBase[*apSegOffset] & 15) + ((apTextSegBase[*apSegOffset + 1] & 15) << 4)));
		}
		else if ((apTextSegBase[*apSegOffset + 2] & 192) == 192)
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(r%u, ", apTextSegBase[*apSegOffset + 2] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "%i) ", ((apTextSegBase[*apSegOffset] & 15) + ((apTextSegBase[*apSegOffset + 1] & 15) << 4)));
		}
		else if ((apTextSegBase[*apSegOffset + 2] & 192) == 128)
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(r%u, ", apTextSegBase[*apSegOffset + 2] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u) ", apTextSegBase[*apSegOffset] & 15);
		}
		*apSegOffset += 4;
	}
	else if ((apTextSegBase[*apSegOffset + 3] & 14) == 0 && (apTextSegBase[*apSegOffset + 2] & 48) == 0 && (apTextSegBase[*apSegOffset] & 240) == 208)
	{
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "ldrd%s ", arm_32a_c4(apTextSegBase[*apSegOffset + 3] >> 4));
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", (apTextSegBase[*apSegOffset + 1] >> 4) & 15);
		if ((apTextSegBase[*apSegOffset + 3] & 1) && (apTextSegBase[*apSegOffset + 2] & 192) == 64)
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(r%u, ", apTextSegBase[*apSegOffset + 2] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "%i) ", -1 * ((apTextSegBase[*apSegOffset] & 15) + ((apTextSegBase[*apSegOffset + 1] & 15) << 4)));
		}
		else if ((apTextSegBase[*apSegOffset + 3] & 1) && (apTextSegBase[*apSegOffset + 2] & 192) == 192)
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(r%u, ", apTextSegBase[*apSegOffset + 2] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "%i) ", ((apTextSegBase[*apSegOffset] & 15) + ((apTextSegBase[*apSegOffset + 1] & 15) << 4)));
		}
		else if ((apTextSegBase[*apSegOffset + 2] & 192) == 64)
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(r%u), ", apTextSegBase[*apSegOffset + 2] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "%i ", -1 * ((apTextSegBase[*apSegOffset] & 15) + ((apTextSegBase[*apSegOffset + 1] & 15) << 4)));
		}
		else if ((apTextSegBase[*apSegOffset + 2] & 192) == 192)
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(r%u), ", apTextSegBase[*apSegOffset + 2] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "%i ", ((apTextSegBase[*apSegOffset] & 15) + ((apTextSegBase[*apSegOffset + 1] & 15) << 4)));
		}
		else if ((apTextSegBase[*apSegOffset + 3] & 1) && (apTextSegBase[*apSegOffset + 2] & 192) == 128)
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(r%u, ", apTextSegBase[*apSegOffset + 2] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u) ", apTextSegBase[*apSegOffset] & 15);
		}
		else if ((apTextSegBase[*apSegOffset + 2] & 192) == 128)
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(r%u), ", apTextSegBase[*apSegOffset + 2] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u ", apTextSegBase[*apSegOffset] & 15);
		}
		*apSegOffset += 4;
	}
	else if ((apTextSegBase[*apSegOffset + 3] & 15) == 0 && (apTextSegBase[*apSegOffset + 2] & 48) == 32 && (apTextSegBase[*apSegOffset] & 240) == 208)
	{
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "ldrdt%s ", arm_32a_c4(apTextSegBase[*apSegOffset + 3] >> 4));
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", (apTextSegBase[*apSegOffset + 1] >> 4) & 15);
		if ((apTextSegBase[*apSegOffset + 2] & 192) == 64)
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(r%u), ", apTextSegBase[*apSegOffset + 2] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "%i ", -1 * ((apTextSegBase[*apSegOffset] & 15) + ((apTextSegBase[*apSegOffset + 1] & 15) << 4)));
		}
		else if ((apTextSegBase[*apSegOffset + 2] & 192) == 192)
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(r%u), ", apTextSegBase[*apSegOffset + 2] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "%i ", ((apTextSegBase[*apSegOffset] & 15) + ((apTextSegBase[*apSegOffset + 1] & 15) << 4)));
		}
		else if ((apTextSegBase[*apSegOffset + 2] & 192) == 128)
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(r%u), ", apTextSegBase[*apSegOffset + 2] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u ", apTextSegBase[*apSegOffset] & 15);
		}
		*apSegOffset += 4;
	}
	else if ((apTextSegBase[*apSegOffset + 3] & 15) == 1 && (apTextSegBase[*apSegOffset + 2] & 48) == 32 && (apTextSegBase[*apSegOffset] & 240) == 208)
	{
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "ldrdw%s ", arm_32a_c4(apTextSegBase[*apSegOffset + 3] >> 4));
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", (apTextSegBase[*apSegOffset + 1] >> 4) & 15);
		if ((apTextSegBase[*apSegOffset + 2] & 192) == 64)
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(r%u, ", apTextSegBase[*apSegOffset + 2] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "%i) ", -1 * ((apTextSegBase[*apSegOffset] & 15) + ((apTextSegBase[*apSegOffset + 1] & 15) << 4)));
		}
		else if ((apTextSegBase[*apSegOffset + 2] & 192) == 192)
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(r%u, ", apTextSegBase[*apSegOffset + 2] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "%i) ", ((apTextSegBase[*apSegOffset] & 15) + ((apTextSegBase[*apSegOffset + 1] & 15) << 4)));
		}
		else if ((apTextSegBase[*apSegOffset + 2] & 192) == 128)
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(r%u, ", apTextSegBase[*apSegOffset + 2] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u) ", apTextSegBase[*apSegOffset] & 15);
		}
		*apSegOffset += 4;
	}
	else if ((apTextSegBase[*apSegOffset + 3] & 14) == 0 && (apTextSegBase[*apSegOffset + 2] & 48) == 16 && (apTextSegBase[*apSegOffset] & 240) == 208)
	{
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "ldrsb%s ", arm_32a_c4(apTextSegBase[*apSegOffset + 3] >> 4));
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", (apTextSegBase[*apSegOffset + 1] >> 4) & 15);
		if ((apTextSegBase[*apSegOffset + 3] & 1) && (apTextSegBase[*apSegOffset + 2] & 192) == 64)
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(r%u, ", apTextSegBase[*apSegOffset + 2] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "%i) ", -1 * ((apTextSegBase[*apSegOffset] & 15) + ((apTextSegBase[*apSegOffset + 1] & 15) << 4)));
		}
		else if ((apTextSegBase[*apSegOffset + 3] & 1) && (apTextSegBase[*apSegOffset + 2] & 192) == 192)
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(r%u, ", apTextSegBase[*apSegOffset + 2] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "%i) ", ((apTextSegBase[*apSegOffset] & 15) + ((apTextSegBase[*apSegOffset + 1] & 15) << 4)));
		}
		else if ((apTextSegBase[*apSegOffset + 2] & 192) == 64)
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(r%u), ", apTextSegBase[*apSegOffset + 2] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "%i ", -1 * ((apTextSegBase[*apSegOffset] & 15) + ((apTextSegBase[*apSegOffset + 1] & 15) << 4)));
		}
		else if ((apTextSegBase[*apSegOffset + 2] & 192) == 192)
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(r%u), ", apTextSegBase[*apSegOffset + 2] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "%i ", ((apTextSegBase[*apSegOffset] & 15) + ((apTextSegBase[*apSegOffset + 1] & 15) << 4)));
		}
		else if ((apTextSegBase[*apSegOffset + 3] & 1) && (apTextSegBase[*apSegOffset + 2] & 192) == 128)
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(r%u, ", apTextSegBase[*apSegOffset + 2] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u) ", apTextSegBase[*apSegOffset] & 15);
		}
		else if ((apTextSegBase[*apSegOffset + 2] & 192) == 128)
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(r%u), ", apTextSegBase[*apSegOffset + 2] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u ", apTextSegBase[*apSegOffset] & 15);
		}
		*apSegOffset += 4;
	}
	else if ((apTextSegBase[*apSegOffset + 3] & 15) == 0 && (apTextSegBase[*apSegOffset + 2] & 48) == 48 && (apTextSegBase[*apSegOffset] & 240) == 208)
	{
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "ldrsbt%s ", arm_32a_c4(apTextSegBase[*apSegOffset + 3] >> 4));
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", (apTextSegBase[*apSegOffset + 1] >> 4) & 15);
		if ((apTextSegBase[*apSegOffset + 2] & 192) == 64)
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(r%u), ", apTextSegBase[*apSegOffset + 2] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "%i ", -1 * ((apTextSegBase[*apSegOffset] & 15) + ((apTextSegBase[*apSegOffset + 1] & 15) << 4)));
		}
		else if ((apTextSegBase[*apSegOffset + 2] & 192) == 192)
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(r%u), ", apTextSegBase[*apSegOffset + 2] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "%i ", ((apTextSegBase[*apSegOffset] & 15) + ((apTextSegBase[*apSegOffset + 1] & 15) << 4)));
		}
		else if ((apTextSegBase[*apSegOffset + 2] & 192) == 128)
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(r%u), ", apTextSegBase[*apSegOffset + 2] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u ", apTextSegBase[*apSegOffset] & 15);
		}
		*apSegOffset += 4;
	}
	else if ((apTextSegBase[*apSegOffset + 3] & 15) == 1 && (apTextSegBase[*apSegOffset + 2] & 48) == 48 && (apTextSegBase[*apSegOffset] & 240) == 208)
	{
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "ldrsbw%s ", arm_32a_c4(apTextSegBase[*apSegOffset + 3] >> 4));
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", (apTextSegBase[*apSegOffset + 1] >> 4) & 15);
		if ((apTextSegBase[*apSegOffset + 2] & 192) == 64)
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(r%u, ", apTextSegBase[*apSegOffset + 2] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "%i) ", -1 * ((apTextSegBase[*apSegOffset] & 15) + ((apTextSegBase[*apSegOffset + 1] & 15) << 4)));
		}
		else if ((apTextSegBase[*apSegOffset + 2] & 192) == 192)
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(r%u, ", apTextSegBase[*apSegOffset + 2] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "%i) ", ((apTextSegBase[*apSegOffset] & 15) + ((apTextSegBase[*apSegOffset + 1] & 15) << 4)));
		}
		else if ((apTextSegBase[*apSegOffset + 2] & 192) == 128)
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(r%u, ", apTextSegBase[*apSegOffset + 2] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u) ", apTextSegBase[*apSegOffset] & 15);
		}
		*apSegOffset += 4;
	}
	else if ((apTextSegBase[*apSegOffset + 3] & 14) == 0 && (apTextSegBase[*apSegOffset + 2] & 48) == 0 && (apTextSegBase[*apSegOffset] & 240) == 240)
	{
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "strd%s ", arm_32a_c4(apTextSegBase[*apSegOffset + 3] >> 4));
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", (apTextSegBase[*apSegOffset + 1] >> 4) & 15);
		if ((apTextSegBase[*apSegOffset + 3] & 1) && (apTextSegBase[*apSegOffset + 2] & 192) == 64)
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(r%u, ", apTextSegBase[*apSegOffset + 2] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "%i) ", -1 * ((apTextSegBase[*apSegOffset] & 15) + ((apTextSegBase[*apSegOffset + 1] & 15) << 4)));
		}
		else if ((apTextSegBase[*apSegOffset + 3] & 1) && (apTextSegBase[*apSegOffset + 2] & 192) == 192)
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(r%u, ", apTextSegBase[*apSegOffset + 2] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "%i) ", ((apTextSegBase[*apSegOffset] & 15) + ((apTextSegBase[*apSegOffset + 1] & 15) << 4)));
		}
		else if ((apTextSegBase[*apSegOffset + 2] & 192) == 64)
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(r%u), ", apTextSegBase[*apSegOffset + 2] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "%i ", -1 * ((apTextSegBase[*apSegOffset] & 15) + ((apTextSegBase[*apSegOffset + 1] & 15) << 4)));
		}
		else if ((apTextSegBase[*apSegOffset + 2] & 192) == 192)
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(r%u), ", apTextSegBase[*apSegOffset + 2] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "%i ", ((apTextSegBase[*apSegOffset] & 15) + ((apTextSegBase[*apSegOffset + 1] & 15) << 4)));
		}
		else if ((apTextSegBase[*apSegOffset + 3] & 1) && (apTextSegBase[*apSegOffset + 2] & 192) == 128)
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(r%u, ", apTextSegBase[*apSegOffset + 2] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u) ", apTextSegBase[*apSegOffset] & 15);
		}
		else if ((apTextSegBase[*apSegOffset + 2] & 192) == 128)
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(r%u), ", apTextSegBase[*apSegOffset + 2] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u ", apTextSegBase[*apSegOffset] & 15);
		}
		*apSegOffset += 4;
	}
	else if ((apTextSegBase[*apSegOffset + 3] & 15) == 0 && (apTextSegBase[*apSegOffset + 2] & 48) == 32 && (apTextSegBase[*apSegOffset] & 240) == 240)
	{
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "strdt%s ", arm_32a_c4(apTextSegBase[*apSegOffset + 3] >> 4));
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", (apTextSegBase[*apSegOffset + 1] >> 4) & 15);
		if ((apTextSegBase[*apSegOffset + 2] & 192) == 64)
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(r%u), ", apTextSegBase[*apSegOffset + 2] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "%i ", -1 * ((apTextSegBase[*apSegOffset] & 15) + ((apTextSegBase[*apSegOffset + 1] & 15) << 4)));
		}
		else if ((apTextSegBase[*apSegOffset + 2] & 192) == 192)
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(r%u), ", apTextSegBase[*apSegOffset + 2] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "%i ", ((apTextSegBase[*apSegOffset] & 15) + ((apTextSegBase[*apSegOffset + 1] & 15) << 4)));
		}
		else if ((apTextSegBase[*apSegOffset + 2] & 192) == 128)
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(r%u), ", apTextSegBase[*apSegOffset + 2] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u ", apTextSegBase[*apSegOffset] & 15);
		}
		*apSegOffset += 4;
	}
	else if ((apTextSegBase[*apSegOffset + 3] & 15) == 1 && (apTextSegBase[*apSegOffset + 2] & 48) == 32 && (apTextSegBase[*apSegOffset] & 240) == 240)
	{
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "strdw%s ", arm_32a_c4(apTextSegBase[*apSegOffset + 3] >> 4));
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", (apTextSegBase[*apSegOffset + 1] >> 4) & 15);
		if ((apTextSegBase[*apSegOffset + 2] & 192) == 64)
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(r%u, ", apTextSegBase[*apSegOffset + 2] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "%i) ", -1 * ((apTextSegBase[*apSegOffset] & 15) + ((apTextSegBase[*apSegOffset + 1] & 15) << 4)));
		}
		else if ((apTextSegBase[*apSegOffset + 2] & 192) == 192)
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(r%u, ", apTextSegBase[*apSegOffset + 2] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "%i) ", ((apTextSegBase[*apSegOffset] & 15) + ((apTextSegBase[*apSegOffset + 1] & 15) << 4)));
		}
		else if ((apTextSegBase[*apSegOffset + 2] & 192) == 128)
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(r%u, ", apTextSegBase[*apSegOffset + 2] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u) ", apTextSegBase[*apSegOffset] & 15);
		}
		*apSegOffset += 4;
	}
	else if ((apTextSegBase[*apSegOffset + 3] & 14) == 0 && (apTextSegBase[*apSegOffset + 2] & 48) == 16 && (apTextSegBase[*apSegOffset] & 240) == 240)
	{
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "ldrsh%s ", arm_32a_c4(apTextSegBase[*apSegOffset + 3] >> 4));
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", (apTextSegBase[*apSegOffset + 1] >> 4) & 15);
		if ((apTextSegBase[*apSegOffset + 3] & 1) && (apTextSegBase[*apSegOffset + 2] & 192) == 64)
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(r%u, ", apTextSegBase[*apSegOffset + 2] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "%i) ", -1 * ((apTextSegBase[*apSegOffset] & 15) + ((apTextSegBase[*apSegOffset + 1] & 15) << 4)));
		}
		else if ((apTextSegBase[*apSegOffset + 3] & 1) && (apTextSegBase[*apSegOffset + 2] & 192) == 192)
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(r%u, ", apTextSegBase[*apSegOffset + 2] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "%i) ", ((apTextSegBase[*apSegOffset] & 15) + ((apTextSegBase[*apSegOffset + 1] & 15) << 4)));
		}
		else if ((apTextSegBase[*apSegOffset + 2] & 192) == 64)
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(r%u), ", apTextSegBase[*apSegOffset + 2] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "%i ", -1 * ((apTextSegBase[*apSegOffset] & 15) + ((apTextSegBase[*apSegOffset + 1] & 15) << 4)));
		}
		else if ((apTextSegBase[*apSegOffset + 2] & 192) == 192)
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(r%u), ", apTextSegBase[*apSegOffset + 2] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "%i ", ((apTextSegBase[*apSegOffset] & 15) + ((apTextSegBase[*apSegOffset + 1] & 15) << 4)));
		}
		else if ((apTextSegBase[*apSegOffset + 3] & 1) && (apTextSegBase[*apSegOffset + 2] & 192) == 128)
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(r%u, ", apTextSegBase[*apSegOffset + 2] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u) ", apTextSegBase[*apSegOffset] & 15);
		}
		else if ((apTextSegBase[*apSegOffset + 2] & 192) == 128)
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(r%u), ", apTextSegBase[*apSegOffset + 2] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u ", apTextSegBase[*apSegOffset] & 15);
		}
		*apSegOffset += 4;
	}
	else if ((apTextSegBase[*apSegOffset + 3] & 15) == 0 && (apTextSegBase[*apSegOffset + 2] & 48) == 48 && (apTextSegBase[*apSegOffset] & 240) == 240)
	{
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "ldrsht%s ", arm_32a_c4(apTextSegBase[*apSegOffset + 3] >> 4));
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", (apTextSegBase[*apSegOffset + 1] >> 4) & 15);
		if ((apTextSegBase[*apSegOffset + 2] & 192) == 64)
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(r%u), ", apTextSegBase[*apSegOffset + 2] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "%i ", -1 * ((apTextSegBase[*apSegOffset] & 15) + ((apTextSegBase[*apSegOffset + 1] & 15) << 4)));
		}
		else if ((apTextSegBase[*apSegOffset + 2] & 192) == 192)
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(r%u), ", apTextSegBase[*apSegOffset + 2] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "%i ", ((apTextSegBase[*apSegOffset] & 15) + ((apTextSegBase[*apSegOffset + 1] & 15) << 4)));
		}
		else if ((apTextSegBase[*apSegOffset + 2] & 192) == 128)
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(r%u), ", apTextSegBase[*apSegOffset + 2] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u ", apTextSegBase[*apSegOffset] & 15);
		}
		*apSegOffset += 4;
	}
	else if ((apTextSegBase[*apSegOffset + 3] & 15) == 1 && (apTextSegBase[*apSegOffset + 2] & 48) == 48 && (apTextSegBase[*apSegOffset] & 240) == 240)
	{
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "ldrshw%s ", arm_32a_c4(apTextSegBase[*apSegOffset + 3] >> 4));
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", (apTextSegBase[*apSegOffset + 1] >> 4) & 15);
		if ((apTextSegBase[*apSegOffset + 2] & 192) == 64)
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(r%u, ", apTextSegBase[*apSegOffset + 2] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "%i) ", -1 * ((apTextSegBase[*apSegOffset] & 15) + ((apTextSegBase[*apSegOffset + 1] & 15) << 4)));
		}
		else if ((apTextSegBase[*apSegOffset + 2] & 192) == 192)
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(r%u, ", apTextSegBase[*apSegOffset + 2] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "%i) ", ((apTextSegBase[*apSegOffset] & 15) + ((apTextSegBase[*apSegOffset + 1] & 15) << 4)));
		}
		else if ((apTextSegBase[*apSegOffset + 2] & 192) == 128)
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(r%u, ", apTextSegBase[*apSegOffset + 2] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u) ", apTextSegBase[*apSegOffset] & 15);
		}
		*apSegOffset += 4;
	}
	else if ((apTextSegBase[*apSegOffset + 3] & 12) == 4 && (apTextSegBase[*apSegOffset + 2] & 112) == 0)
	{
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "str%s ", arm_32a_c4(apTextSegBase[*apSegOffset + 3] >> 4));
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", (apTextSegBase[*apSegOffset + 1] >> 4) & 15);
		if ((apTextSegBase[*apSegOffset + 3] & 3) == 1 && (apTextSegBase[*apSegOffset + 2] & 128))
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(r%u, ", apTextSegBase[*apSegOffset + 2] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "%i) ", apTextSegBase[*apSegOffset] + ((apTextSegBase[*apSegOffset + 1] & 15) << 8));
		}
		else if ((apTextSegBase[*apSegOffset + 3] & 3) == 1)
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(r%u, ", apTextSegBase[*apSegOffset + 2] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "%i) ", -1 * (apTextSegBase[*apSegOffset] + ((apTextSegBase[*apSegOffset + 1] & 15) << 8)));
		}
		else if ((apTextSegBase[*apSegOffset + 3] & 3) == 0 && (apTextSegBase[*apSegOffset + 2] & 128))
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(r%u), ", apTextSegBase[*apSegOffset + 2] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "%i ", apTextSegBase[*apSegOffset] + ((apTextSegBase[*apSegOffset + 1] & 15) << 8));
		}
		else if ((apTextSegBase[*apSegOffset + 3] & 3) == 0)
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(r%u), ", apTextSegBase[*apSegOffset + 2] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "%i ", -1 * (apTextSegBase[*apSegOffset] + ((apTextSegBase[*apSegOffset + 1] & 15) << 8)));
		}
		else if ((apTextSegBase[*apSegOffset + 3] & 3) == 3 && (apTextSegBase[*apSegOffset + 2] & 128) && ((apTextSegBase[*apSegOffset] & 96) || ((apTextSegBase[*apSegOffset] >> 7) + ((apTextSegBase[*apSegOffset + 1] & 15) << 1))))
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(r%u, ", apTextSegBase[*apSegOffset + 2] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u), ", apTextSegBase[*apSegOffset] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(%s, ", arm_32a_sh(apTextSegBase[*apSegOffset] & 96));
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "%u) ", (apTextSegBase[*apSegOffset] >> 7) + ((apTextSegBase[*apSegOffset + 1] & 15) << 1));
		}
		else if ((apTextSegBase[*apSegOffset + 3] & 3) == 3 && (apTextSegBase[*apSegOffset + 2] & 128))
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(r%u, ", apTextSegBase[*apSegOffset + 2] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u) ", apTextSegBase[*apSegOffset] & 15);
		}
		else if ((apTextSegBase[*apSegOffset + 3] & 3) == 2 && (apTextSegBase[*apSegOffset + 2] & 128) && ((apTextSegBase[*apSegOffset] & 96) || ((apTextSegBase[*apSegOffset] >> 7) + ((apTextSegBase[*apSegOffset + 1] & 15) << 1))))
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(r%u), ", apTextSegBase[*apSegOffset + 2] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", apTextSegBase[*apSegOffset] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(%s, ", arm_32a_sh(apTextSegBase[*apSegOffset] & 96));
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "%u) ", (apTextSegBase[*apSegOffset] >> 7) + ((apTextSegBase[*apSegOffset + 1] & 15) << 1));
		}
		else if ((apTextSegBase[*apSegOffset + 3] & 3) == 2 && (apTextSegBase[*apSegOffset + 2] & 128))
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(r%u), ", apTextSegBase[*apSegOffset + 2] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u ", apTextSegBase[*apSegOffset] & 15);
		}
		*apSegOffset += 4;
	}
	else if ((apTextSegBase[*apSegOffset + 3] & 13) == 4 && (apTextSegBase[*apSegOffset + 2] & 112) == 32)
	{
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "strt%s ", arm_32a_c4(apTextSegBase[*apSegOffset + 3] >> 4));
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", (apTextSegBase[*apSegOffset + 1] >> 4) & 15);
		if ((apTextSegBase[*apSegOffset + 3] & 2) == 0 && (apTextSegBase[*apSegOffset + 2] & 128))
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(r%u), ", apTextSegBase[*apSegOffset + 2] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "%i ", apTextSegBase[*apSegOffset] + ((apTextSegBase[*apSegOffset + 1] & 15) << 8));
		}
		else if ((apTextSegBase[*apSegOffset + 3] & 2) == 0)
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(r%u), ", apTextSegBase[*apSegOffset + 2] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "%i ", -1 * (apTextSegBase[*apSegOffset] + ((apTextSegBase[*apSegOffset + 1] & 15) << 8)));
		}
		else if ((apTextSegBase[*apSegOffset + 3] & 2) == 2 && (apTextSegBase[*apSegOffset + 2] & 128) && ((apTextSegBase[*apSegOffset] & 96) || ((apTextSegBase[*apSegOffset] >> 7) + ((apTextSegBase[*apSegOffset + 1] & 15) << 1))))
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(r%u), ", apTextSegBase[*apSegOffset + 2] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", apTextSegBase[*apSegOffset] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(%s, ", arm_32a_sh(apTextSegBase[*apSegOffset] & 96));
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "%u) ", (apTextSegBase[*apSegOffset] >> 7) + ((apTextSegBase[*apSegOffset + 1] & 15) << 1));
		}
		else if ((apTextSegBase[*apSegOffset + 3] & 2) == 2 && (apTextSegBase[*apSegOffset + 2] & 128))
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(r%u), ", apTextSegBase[*apSegOffset + 2] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u ", apTextSegBase[*apSegOffset] & 15);
		}
		*apSegOffset += 4;
	}
	else if ((apTextSegBase[*apSegOffset + 3] & 13) == 5 && (apTextSegBase[*apSegOffset + 2] & 112) == 32)
	{
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "strw%s ", arm_32a_c4(apTextSegBase[*apSegOffset + 3] >> 4));
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", (apTextSegBase[*apSegOffset + 1] >> 4) & 15);
		if ((apTextSegBase[*apSegOffset + 3] & 2) == 0 && (apTextSegBase[*apSegOffset + 2] & 128))
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(r%u, ", apTextSegBase[*apSegOffset + 2] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "%i) ", apTextSegBase[*apSegOffset] + ((apTextSegBase[*apSegOffset + 1] & 15) << 8));
		}
		else if ((apTextSegBase[*apSegOffset + 3] & 2) == 0)
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(r%u, ", apTextSegBase[*apSegOffset + 2] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "%i) ", -1 * (apTextSegBase[*apSegOffset] + ((apTextSegBase[*apSegOffset + 1] & 15) << 8)));
		}
		else if ((apTextSegBase[*apSegOffset + 3] & 2) == 2 && (apTextSegBase[*apSegOffset + 2] & 128) && ((apTextSegBase[*apSegOffset] & 96) || ((apTextSegBase[*apSegOffset] >> 7) + ((apTextSegBase[*apSegOffset + 1] & 15) << 1))))
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(r%u, ", apTextSegBase[*apSegOffset + 2] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u), ", apTextSegBase[*apSegOffset] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(%s, ", arm_32a_sh(apTextSegBase[*apSegOffset] & 96));
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "%u) ", (apTextSegBase[*apSegOffset] >> 7) + ((apTextSegBase[*apSegOffset + 1] & 15) << 1));
		}
		else if ((apTextSegBase[*apSegOffset + 3] & 2) == 2 && (apTextSegBase[*apSegOffset + 2] & 128))
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(r%u, ", apTextSegBase[*apSegOffset + 2] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u) ", apTextSegBase[*apSegOffset] & 15);
		}
		*apSegOffset += 4;
	}
	else if ((apTextSegBase[*apSegOffset + 3] & 12) == 4 && (apTextSegBase[*apSegOffset + 2] & 112) == 16)
	{
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "ldr%s ", arm_32a_c4(apTextSegBase[*apSegOffset + 3] >> 4));
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", (apTextSegBase[*apSegOffset + 1] >> 4) & 15);
		if ((apTextSegBase[*apSegOffset + 3] & 3) == 1 && (apTextSegBase[*apSegOffset + 2] & 128))
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(r%u, ", apTextSegBase[*apSegOffset + 2] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "%i) ", apTextSegBase[*apSegOffset] + ((apTextSegBase[*apSegOffset + 1] & 15) << 8));
		}
		else if ((apTextSegBase[*apSegOffset + 3] & 3) == 1)
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(r%u, ", apTextSegBase[*apSegOffset + 2] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "%i) ", -1 * (apTextSegBase[*apSegOffset] + ((apTextSegBase[*apSegOffset + 1] & 15) << 8)));
		}
		else if ((apTextSegBase[*apSegOffset + 3] & 3) == 0 && (apTextSegBase[*apSegOffset + 2] & 128))
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(r%u), ", apTextSegBase[*apSegOffset + 2] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "%i ", apTextSegBase[*apSegOffset] + ((apTextSegBase[*apSegOffset + 1] & 15) << 8));
		}
		else if ((apTextSegBase[*apSegOffset + 3] & 3) == 0)
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(r%u), ", apTextSegBase[*apSegOffset + 2] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "%i ", -1 * (apTextSegBase[*apSegOffset] + ((apTextSegBase[*apSegOffset + 1] & 15) << 8)));
		}
		else if ((apTextSegBase[*apSegOffset + 3] & 3) == 3 && (apTextSegBase[*apSegOffset + 2] & 128) && ((apTextSegBase[*apSegOffset] & 96) || ((apTextSegBase[*apSegOffset] >> 7) + ((apTextSegBase[*apSegOffset + 1] & 15) << 1))))
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(r%u, ", apTextSegBase[*apSegOffset + 2] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u), ", apTextSegBase[*apSegOffset] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(%s, ", arm_32a_sh(apTextSegBase[*apSegOffset] & 96));
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "%u) ", (apTextSegBase[*apSegOffset] >> 7) + ((apTextSegBase[*apSegOffset + 1] & 15) << 1));
		}
		else if ((apTextSegBase[*apSegOffset + 3] & 3) == 3 && (apTextSegBase[*apSegOffset + 2] & 128))
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(r%u, ", apTextSegBase[*apSegOffset + 2] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u) ", apTextSegBase[*apSegOffset] & 15);
		}
		else if ((apTextSegBase[*apSegOffset + 3] & 3) == 2 && (apTextSegBase[*apSegOffset + 2] & 128) && ((apTextSegBase[*apSegOffset] & 96) || ((apTextSegBase[*apSegOffset] >> 7) + ((apTextSegBase[*apSegOffset + 1] & 15) << 1))))
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(r%u), ", apTextSegBase[*apSegOffset + 2] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", apTextSegBase[*apSegOffset] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(%s, ", arm_32a_sh(apTextSegBase[*apSegOffset] & 96));
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "%u) ", (apTextSegBase[*apSegOffset] >> 7) + ((apTextSegBase[*apSegOffset + 1] & 15) << 1));
		}
		else if ((apTextSegBase[*apSegOffset + 3] & 3) == 2 && (apTextSegBase[*apSegOffset + 2] & 128))
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(r%u), ", apTextSegBase[*apSegOffset + 2] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u ", apTextSegBase[*apSegOffset] & 15);
		}
		*apSegOffset += 4;
	}
	else if ((apTextSegBase[*apSegOffset + 3] & 13) == 4 && (apTextSegBase[*apSegOffset + 2] & 112) == 48)
	{
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "ldrt%s ", arm_32a_c4(apTextSegBase[*apSegOffset + 3] >> 4));
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", (apTextSegBase[*apSegOffset + 1] >> 4) & 15);
		if ((apTextSegBase[*apSegOffset + 3] & 2) == 0 && (apTextSegBase[*apSegOffset + 2] & 128))
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(r%u), ", apTextSegBase[*apSegOffset + 2] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "%i ", apTextSegBase[*apSegOffset] + ((apTextSegBase[*apSegOffset + 1] & 15) << 8));
		}
		else if ((apTextSegBase[*apSegOffset + 3] & 2) == 0)
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(r%u), ", apTextSegBase[*apSegOffset + 2] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "%i ", -1 * (apTextSegBase[*apSegOffset] + ((apTextSegBase[*apSegOffset + 1] & 15) << 8)));
		}
		else if ((apTextSegBase[*apSegOffset + 3] & 2) == 2 && (apTextSegBase[*apSegOffset + 2] & 128) && ((apTextSegBase[*apSegOffset] & 96) || ((apTextSegBase[*apSegOffset] >> 7) + ((apTextSegBase[*apSegOffset + 1] & 15) << 1))))
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(r%u), ", apTextSegBase[*apSegOffset + 2] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", apTextSegBase[*apSegOffset] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(%s, ", arm_32a_sh(apTextSegBase[*apSegOffset] & 96));
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "%u) ", (apTextSegBase[*apSegOffset] >> 7) + ((apTextSegBase[*apSegOffset + 1] & 15) << 1));
		}
		else if ((apTextSegBase[*apSegOffset + 3] & 2) == 2 && (apTextSegBase[*apSegOffset + 2] & 128))
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(r%u), ", apTextSegBase[*apSegOffset + 2] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u ", apTextSegBase[*apSegOffset] & 15);
		}
		*apSegOffset += 4;
	}
	else if ((apTextSegBase[*apSegOffset + 3] & 13) == 5 && (apTextSegBase[*apSegOffset + 2] & 112) == 48)
	{
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "ldrw%s ", arm_32a_c4(apTextSegBase[*apSegOffset + 3] >> 4));
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", (apTextSegBase[*apSegOffset + 1] >> 4) & 15);
		if ((apTextSegBase[*apSegOffset + 3] & 2) == 0 && (apTextSegBase[*apSegOffset + 2] & 128))
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(r%u, ", apTextSegBase[*apSegOffset + 2] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "%i) ", apTextSegBase[*apSegOffset] + ((apTextSegBase[*apSegOffset + 1] & 15) << 8));
		}
		else if ((apTextSegBase[*apSegOffset + 3] & 2) == 0)
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(r%u, ", apTextSegBase[*apSegOffset + 2] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "%i) ", -1 * (apTextSegBase[*apSegOffset] + ((apTextSegBase[*apSegOffset + 1] & 15) << 8)));
		}
		else if ((apTextSegBase[*apSegOffset + 3] & 2) == 2 && (apTextSegBase[*apSegOffset + 2] & 128) && ((apTextSegBase[*apSegOffset] & 96) || ((apTextSegBase[*apSegOffset] >> 7) + ((apTextSegBase[*apSegOffset + 1] & 15) << 1))))
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(r%u, ", apTextSegBase[*apSegOffset + 2] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u), ", apTextSegBase[*apSegOffset] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(%s, ", arm_32a_sh(apTextSegBase[*apSegOffset] & 96));
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "%u) ", (apTextSegBase[*apSegOffset] >> 7) + ((apTextSegBase[*apSegOffset + 1] & 15) << 1));
		}
		else if ((apTextSegBase[*apSegOffset + 3] & 2) == 2 && (apTextSegBase[*apSegOffset + 2] & 128))
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(r%u, ", apTextSegBase[*apSegOffset + 2] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u) ", apTextSegBase[*apSegOffset] & 15);
		}
		*apSegOffset += 4;
	}
	else if ((apTextSegBase[*apSegOffset + 3] & 12) == 4 && (apTextSegBase[*apSegOffset + 2] & 112) == 64)
	{
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "strb%s ", arm_32a_c4(apTextSegBase[*apSegOffset + 3] >> 4));
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", (apTextSegBase[*apSegOffset + 1] >> 4) & 15);
		if ((apTextSegBase[*apSegOffset + 3] & 3) == 1 && (apTextSegBase[*apSegOffset + 2] & 128))
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(r%u, ", apTextSegBase[*apSegOffset + 2] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "%i) ", apTextSegBase[*apSegOffset] + ((apTextSegBase[*apSegOffset + 1] & 15) << 8));
		}
		else if ((apTextSegBase[*apSegOffset + 3] & 3) == 1)
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(r%u, ", apTextSegBase[*apSegOffset + 2] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "%i) ", -1 * (apTextSegBase[*apSegOffset] + ((apTextSegBase[*apSegOffset + 1] & 15) << 8)));
		}
		else if ((apTextSegBase[*apSegOffset + 3] & 3) == 0 && (apTextSegBase[*apSegOffset + 2] & 128))
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(r%u), ", apTextSegBase[*apSegOffset + 2] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "%i ", apTextSegBase[*apSegOffset] + ((apTextSegBase[*apSegOffset + 1] & 15) << 8));
		}
		else if ((apTextSegBase[*apSegOffset + 3] & 3) == 0)
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(r%u), ", apTextSegBase[*apSegOffset + 2] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "%i ", -1 * (apTextSegBase[*apSegOffset] + ((apTextSegBase[*apSegOffset + 1] & 15) << 8)));
		}
		else if ((apTextSegBase[*apSegOffset + 3] & 3) == 3 && (apTextSegBase[*apSegOffset + 2] & 128) && ((apTextSegBase[*apSegOffset] & 96) || ((apTextSegBase[*apSegOffset] >> 7) + ((apTextSegBase[*apSegOffset + 1] & 15) << 1))))
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(r%u, ", apTextSegBase[*apSegOffset + 2] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u), ", apTextSegBase[*apSegOffset] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(%s, ", arm_32a_sh(apTextSegBase[*apSegOffset] & 96));
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "%u) ", (apTextSegBase[*apSegOffset] >> 7) + ((apTextSegBase[*apSegOffset + 1] & 15) << 1));
		}
		else if ((apTextSegBase[*apSegOffset + 3] & 3) == 3 && (apTextSegBase[*apSegOffset + 2] & 128))
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(r%u, ", apTextSegBase[*apSegOffset + 2] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u) ", apTextSegBase[*apSegOffset] & 15);
		}
		else if ((apTextSegBase[*apSegOffset + 3] & 3) == 2 && (apTextSegBase[*apSegOffset + 2] & 128) && ((apTextSegBase[*apSegOffset] & 96) || ((apTextSegBase[*apSegOffset] >> 7) + ((apTextSegBase[*apSegOffset + 1] & 15) << 1))))
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(r%u), ", apTextSegBase[*apSegOffset + 2] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", apTextSegBase[*apSegOffset] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(%s, ", arm_32a_sh(apTextSegBase[*apSegOffset] & 96));
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "%u) ", (apTextSegBase[*apSegOffset] >> 7) + ((apTextSegBase[*apSegOffset + 1] & 15) << 1));
		}
		else if ((apTextSegBase[*apSegOffset + 3] & 3) == 2 && (apTextSegBase[*apSegOffset + 2] & 128))
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(r%u), ", apTextSegBase[*apSegOffset + 2] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u ", apTextSegBase[*apSegOffset] & 15);
		}
		*apSegOffset += 4;
	}
	else if ((apTextSegBase[*apSegOffset + 3] & 13) == 4 && (apTextSegBase[*apSegOffset + 2] & 112) == 96)
	{
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "strbt%s ", arm_32a_c4(apTextSegBase[*apSegOffset + 3] >> 4));
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", (apTextSegBase[*apSegOffset + 1] >> 4) & 15);
		if ((apTextSegBase[*apSegOffset + 3] & 2) == 0 && (apTextSegBase[*apSegOffset + 2] & 128))
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(r%u), ", apTextSegBase[*apSegOffset + 2] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "%i ", apTextSegBase[*apSegOffset] + ((apTextSegBase[*apSegOffset + 1] & 15) << 8));
		}
		else if ((apTextSegBase[*apSegOffset + 3] & 2) == 0)
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(r%u), ", apTextSegBase[*apSegOffset + 2] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "%i ", -1 * (apTextSegBase[*apSegOffset] + ((apTextSegBase[*apSegOffset + 1] & 15) << 8)));
		}
		else if ((apTextSegBase[*apSegOffset + 3] & 2) == 2 && (apTextSegBase[*apSegOffset + 2] & 128) && ((apTextSegBase[*apSegOffset] & 96) || ((apTextSegBase[*apSegOffset] >> 7) + ((apTextSegBase[*apSegOffset + 1] & 15) << 1))))
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(r%u), ", apTextSegBase[*apSegOffset + 2] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", apTextSegBase[*apSegOffset] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(%s, ", arm_32a_sh(apTextSegBase[*apSegOffset] & 96));
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "%u) ", (apTextSegBase[*apSegOffset] >> 7) + ((apTextSegBase[*apSegOffset + 1] & 15) << 1));
		}
		else if ((apTextSegBase[*apSegOffset + 3] & 2) == 2 && (apTextSegBase[*apSegOffset + 2] & 128))
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(r%u), ", apTextSegBase[*apSegOffset + 2] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u ", apTextSegBase[*apSegOffset] & 15);
		}
		*apSegOffset += 4;
	}
	else if ((apTextSegBase[*apSegOffset + 3] & 13) == 5 && (apTextSegBase[*apSegOffset + 2] & 112) == 96)
	{
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "strbw%s ", arm_32a_c4(apTextSegBase[*apSegOffset + 3] >> 4));
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", (apTextSegBase[*apSegOffset + 1] >> 4) & 15);
		if ((apTextSegBase[*apSegOffset + 3] & 2) == 0 && (apTextSegBase[*apSegOffset + 2] & 128))
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(r%u, ", apTextSegBase[*apSegOffset + 2] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "%i) ", apTextSegBase[*apSegOffset] + ((apTextSegBase[*apSegOffset + 1] & 15) << 8));
		}
		else if ((apTextSegBase[*apSegOffset + 3] & 2) == 0)
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(r%u, ", apTextSegBase[*apSegOffset + 2] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "%i) ", -1 * (apTextSegBase[*apSegOffset] + ((apTextSegBase[*apSegOffset + 1] & 15) << 8)));
		}
		else if ((apTextSegBase[*apSegOffset + 3] & 2) == 2 && (apTextSegBase[*apSegOffset + 2] & 128) && ((apTextSegBase[*apSegOffset] & 96) || ((apTextSegBase[*apSegOffset] >> 7) + ((apTextSegBase[*apSegOffset + 1] & 15) << 1))))
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(r%u, ", apTextSegBase[*apSegOffset + 2] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u), ", apTextSegBase[*apSegOffset] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(%s, ", arm_32a_sh(apTextSegBase[*apSegOffset] & 96));
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "%u) ", (apTextSegBase[*apSegOffset] >> 7) + ((apTextSegBase[*apSegOffset + 1] & 15) << 1));
		}
		else if ((apTextSegBase[*apSegOffset + 3] & 2) == 2 && (apTextSegBase[*apSegOffset + 2] & 128))
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(r%u, ", apTextSegBase[*apSegOffset + 2] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u) ", apTextSegBase[*apSegOffset] & 15);
		}
		*apSegOffset += 4;
	}
	else if ((apTextSegBase[*apSegOffset + 3] & 12) == 4 && (apTextSegBase[*apSegOffset + 2] & 112) == 80)
	{
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "ldrb%s ", arm_32a_c4(apTextSegBase[*apSegOffset + 3] >> 4));
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", (apTextSegBase[*apSegOffset + 1] >> 4) & 15);
		if ((apTextSegBase[*apSegOffset + 3] & 3) == 1 && (apTextSegBase[*apSegOffset + 2] & 128))
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(r%u, ", apTextSegBase[*apSegOffset + 2] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "%i) ", apTextSegBase[*apSegOffset] + ((apTextSegBase[*apSegOffset + 1] & 15) << 8));
		}
		else if ((apTextSegBase[*apSegOffset + 3] & 3) == 1)
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(r%u, ", apTextSegBase[*apSegOffset + 2] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "%i) ", -1 * (apTextSegBase[*apSegOffset] + ((apTextSegBase[*apSegOffset + 1] & 15) << 8)));
		}
		else if ((apTextSegBase[*apSegOffset + 3] & 3) == 0 && (apTextSegBase[*apSegOffset + 2] & 128))
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(r%u), ", apTextSegBase[*apSegOffset + 2] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "%i ", apTextSegBase[*apSegOffset] + ((apTextSegBase[*apSegOffset + 1] & 15) << 8));
		}
		else if ((apTextSegBase[*apSegOffset + 3] & 3) == 0)
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(r%u), ", apTextSegBase[*apSegOffset + 2] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "%i ", -1 * (apTextSegBase[*apSegOffset] + ((apTextSegBase[*apSegOffset + 1] & 15) << 8)));
		}
		else if ((apTextSegBase[*apSegOffset + 3] & 3) == 3 && (apTextSegBase[*apSegOffset + 2] & 128) && ((apTextSegBase[*apSegOffset] & 96) || ((apTextSegBase[*apSegOffset] >> 7) + ((apTextSegBase[*apSegOffset + 1] & 15) << 1))))
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(r%u, ", apTextSegBase[*apSegOffset + 2] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u), ", apTextSegBase[*apSegOffset] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(%s, ", arm_32a_sh(apTextSegBase[*apSegOffset] & 96));
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "%u) ", (apTextSegBase[*apSegOffset] >> 7) + ((apTextSegBase[*apSegOffset + 1] & 15) << 1));
		}
		else if ((apTextSegBase[*apSegOffset + 3] & 3) == 3 && (apTextSegBase[*apSegOffset + 2] & 128))
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(r%u, ", apTextSegBase[*apSegOffset + 2] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u) ", apTextSegBase[*apSegOffset] & 15);
		}
		else if ((apTextSegBase[*apSegOffset + 3] & 3) == 2 && (apTextSegBase[*apSegOffset + 2] & 128) && ((apTextSegBase[*apSegOffset] & 96) || ((apTextSegBase[*apSegOffset] >> 7) + ((apTextSegBase[*apSegOffset + 1] & 15) << 1))))
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(r%u), ", apTextSegBase[*apSegOffset + 2] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", apTextSegBase[*apSegOffset] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(%s, ", arm_32a_sh(apTextSegBase[*apSegOffset] & 96));
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "%u) ", (apTextSegBase[*apSegOffset] >> 7) + ((apTextSegBase[*apSegOffset + 1] & 15) << 1));
		}
		else if ((apTextSegBase[*apSegOffset + 3] & 3) == 2 && (apTextSegBase[*apSegOffset + 2] & 128))
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(r%u), ", apTextSegBase[*apSegOffset + 2] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u ", apTextSegBase[*apSegOffset] & 15);
		}
		*apSegOffset += 4;
	}
	else if ((apTextSegBase[*apSegOffset + 3] & 13) == 4 && (apTextSegBase[*apSegOffset + 2] & 112) == 112)
	{
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "ldrbt%s ", arm_32a_c4(apTextSegBase[*apSegOffset + 3] >> 4));
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", (apTextSegBase[*apSegOffset + 1] >> 4) & 15);
		if ((apTextSegBase[*apSegOffset + 3] & 2) == 0 && (apTextSegBase[*apSegOffset + 2] & 128))
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(r%u), ", apTextSegBase[*apSegOffset + 2] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "%i ", apTextSegBase[*apSegOffset] + ((apTextSegBase[*apSegOffset + 1] & 15) << 8));
		}
		else if ((apTextSegBase[*apSegOffset + 3] & 2) == 0)
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(r%u), ", apTextSegBase[*apSegOffset + 2] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "%i ", -1 * (apTextSegBase[*apSegOffset] + ((apTextSegBase[*apSegOffset + 1] & 15) << 8)));
		}
		else if ((apTextSegBase[*apSegOffset + 3] & 2) == 2 && (apTextSegBase[*apSegOffset + 2] & 128) && ((apTextSegBase[*apSegOffset] & 96) || ((apTextSegBase[*apSegOffset] >> 7) + ((apTextSegBase[*apSegOffset + 1] & 15) << 1))))
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(r%u), ", apTextSegBase[*apSegOffset + 2] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", apTextSegBase[*apSegOffset] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(%s, ", arm_32a_sh(apTextSegBase[*apSegOffset] & 96));
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "%u) ", (apTextSegBase[*apSegOffset] >> 7) + ((apTextSegBase[*apSegOffset + 1] & 15) << 1));
		}
		else if ((apTextSegBase[*apSegOffset + 3] & 2) == 2 && (apTextSegBase[*apSegOffset + 2] & 128))
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(r%u), ", apTextSegBase[*apSegOffset + 2] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u ", apTextSegBase[*apSegOffset] & 15);
		}
		*apSegOffset += 4;
	}
	else if ((apTextSegBase[*apSegOffset + 3] & 13) == 5 && (apTextSegBase[*apSegOffset + 2] & 112) == 112)
	{
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "ldrbw%s ", arm_32a_c4(apTextSegBase[*apSegOffset + 3] >> 4));
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", (apTextSegBase[*apSegOffset + 1] >> 4) & 15);
		if ((apTextSegBase[*apSegOffset + 3] & 2) == 0 && (apTextSegBase[*apSegOffset + 2] & 128))
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(r%u, ", apTextSegBase[*apSegOffset + 2] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "%i) ", apTextSegBase[*apSegOffset] + ((apTextSegBase[*apSegOffset + 1] & 15) << 8));
		}
		else if ((apTextSegBase[*apSegOffset + 3] & 2) == 0)
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(r%u, ", apTextSegBase[*apSegOffset + 2] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "%i) ", -1 * (apTextSegBase[*apSegOffset] + ((apTextSegBase[*apSegOffset + 1] & 15) << 8)));
		}
		else if ((apTextSegBase[*apSegOffset + 3] & 2) == 2 && (apTextSegBase[*apSegOffset + 2] & 128) && ((apTextSegBase[*apSegOffset] & 96) || ((apTextSegBase[*apSegOffset] >> 7) + ((apTextSegBase[*apSegOffset + 1] & 15) << 1))))
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(r%u, ", apTextSegBase[*apSegOffset + 2] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u), ", apTextSegBase[*apSegOffset] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(%s, ", arm_32a_sh(apTextSegBase[*apSegOffset] & 96));
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "%u) ", (apTextSegBase[*apSegOffset] >> 7) + ((apTextSegBase[*apSegOffset + 1] & 15) << 1));
		}
		else if ((apTextSegBase[*apSegOffset + 3] & 2) == 2 && (apTextSegBase[*apSegOffset + 2] & 128))
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(r%u, ", apTextSegBase[*apSegOffset + 2] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u) ", apTextSegBase[*apSegOffset] & 15);
		}
		*apSegOffset += 4;
	}
	else if ((apTextSegBase[*apSegOffset + 3] & 15) == 8 && (apTextSegBase[*apSegOffset + 2] & 240) == 0)
	{
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "stmda%s ", arm_32a_c4(apTextSegBase[*apSegOffset + 3] >> 4));
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(r%u)", apTextSegBase[*apSegOffset + 2] & 15);
		for(i = 0; i < 16; i++)
		{
			if (i < 8)
			{
				if (apTextSegBase[*apSegOffset] & (1 << i))
				{
					aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, ", r%u", i);
				}
			}
			else
			{
				if (apTextSegBase[*apSegOffset + 1] & (1 << (i - 8)))
				{
					aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, ", r%u", i);
				}
			}
		}
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, " ");
		*apSegOffset += 4;
	}
	else if ((apTextSegBase[*apSegOffset + 3] & 15) == 8 && (apTextSegBase[*apSegOffset + 2] & 240) == 32)
	{
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "stmdaw%s ", arm_32a_c4(apTextSegBase[*apSegOffset + 3] >> 4));
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(r%u)", apTextSegBase[*apSegOffset + 2] & 15);
		for(i = 0; i < 16; i++)
		{
			if (i < 8)
			{
				if (apTextSegBase[*apSegOffset] & (1 << i))
				{
					aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, ", r%u", i);
				}
			}
			else
			{
				if (apTextSegBase[*apSegOffset + 1] & (1 << (i - 8)))
				{
					aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, ", r%u", i);
				}
			}
		}
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, " ");
		*apSegOffset += 4;
	}
	else if ((apTextSegBase[*apSegOffset + 3] & 15) == 8 && (apTextSegBase[*apSegOffset + 2] & 240) == 16)
	{
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "ldmda%s ", arm_32a_c4(apTextSegBase[*apSegOffset + 3] >> 4));
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(r%u)", apTextSegBase[*apSegOffset + 2] & 15);
		for(i = 0; i < 16; i++)
		{
			if (i < 8)
			{
				if (apTextSegBase[*apSegOffset] & (1 << i))
				{
					aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, ", r%u", i);
				}
			}
			else
			{
				if (apTextSegBase[*apSegOffset + 1] & (1 << (i - 8)))
				{
					aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, ", r%u", i);
				}
			}
		}
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, " ");
		*apSegOffset += 4;
	}
	else if ((apTextSegBase[*apSegOffset + 3] & 15) == 8 && (apTextSegBase[*apSegOffset + 2] & 240) == 48)
	{
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "ldmdaw%s ", arm_32a_c4(apTextSegBase[*apSegOffset + 3] >> 4));
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(r%u)", apTextSegBase[*apSegOffset + 2] & 15);
		for(i = 0; i < 16; i++)
		{
			if (i < 8)
			{
				if (apTextSegBase[*apSegOffset] & (1 << i))
				{
					aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, ", r%u", i);
				}
			}
			else
			{
				if (apTextSegBase[*apSegOffset + 1] & (1 << (i - 8)))
				{
					aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, ", r%u", i);
				}
			}
		}
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, " ");
		*apSegOffset += 4;
	}
	else if ((apTextSegBase[*apSegOffset + 3] & 15) == 8 && (apTextSegBase[*apSegOffset + 2] & 240) == 128)
	{
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "stmia%s ", arm_32a_c4(apTextSegBase[*apSegOffset + 3] >> 4));
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(r%u)", apTextSegBase[*apSegOffset + 2] & 15);
		for(i = 0; i < 16; i++)
		{
			if (i < 8)
			{
				if (apTextSegBase[*apSegOffset] & (1 << i))
				{
					aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, ", r%u", i);
				}
			}
			else
			{
				if (apTextSegBase[*apSegOffset + 1] & (1 << (i - 8)))
				{
					aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, ", r%u", i);
				}
			}
		}
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, " ");
		*apSegOffset += 4;
	}
	else if ((apTextSegBase[*apSegOffset + 3] & 15) == 8 && (apTextSegBase[*apSegOffset + 2] & 240) == 160)
	{
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "stmiaw%s ", arm_32a_c4(apTextSegBase[*apSegOffset + 3] >> 4));
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(r%u)", apTextSegBase[*apSegOffset + 2] & 15);
		for(i = 0; i < 16; i++)
		{
			if (i < 8)
			{
				if (apTextSegBase[*apSegOffset] & (1 << i))
				{
					aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, ", r%u", i);
				}
			}
			else
			{
				if (apTextSegBase[*apSegOffset + 1] & (1 << (i - 8)))
				{
					aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, ", r%u", i);
				}
			}
		}
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, " ");
		*apSegOffset += 4;
	}
	else if ((apTextSegBase[*apSegOffset + 3] & 15) == 8 && apTextSegBase[*apSegOffset + 2] == 157)
	{
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "pop%s ", arm_32a_c4(apTextSegBase[*apSegOffset + 3] >> 4));
		UINT8 a = 0;
		for(i = 0; i < 16; i++)
		{
			if (i < 8)
			{
				if (apTextSegBase[*apSegOffset] & (1 << i))
				{
					if (a)
					{
						aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, ", ");
					}
					aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u", i);
					a = 1;
				}
				if (i == 7)
				{
					a = 0;
				}
			}
			else
			{
				if (apTextSegBase[*apSegOffset + 1] & (1 << (i - 8)))
				{
					if (a)
					{
						aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, ", ");
					}
					aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, ", r%u", i);
					a = 1;
				}
			}
		}
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, " ");
		*apSegOffset += 4;
	}
	else if ((apTextSegBase[*apSegOffset + 3] & 15) == 8 && (apTextSegBase[*apSegOffset + 2] & 240) == 144)
	{
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "ldmia%s ", arm_32a_c4(apTextSegBase[*apSegOffset + 3] >> 4));
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(r%u)", apTextSegBase[*apSegOffset + 2] & 15);
		for(i = 0; i < 16; i++)
		{
			if (i < 8)
			{
				if (apTextSegBase[*apSegOffset] & (1 << i))
				{
					aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, ", r%u", i);
				}
			}
			else
			{
				if (apTextSegBase[*apSegOffset + 1] & (1 << (i - 8)))
				{
					aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, ", r%u", i);
				}
			}
		}
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, " ");
		*apSegOffset += 4;
	}
	else if ((apTextSegBase[*apSegOffset + 3] & 15) == 8 && (apTextSegBase[*apSegOffset + 2] & 240) == 176)
	{
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "ldmiaw%s ", arm_32a_c4(apTextSegBase[*apSegOffset + 3] >> 4));
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(r%u)", apTextSegBase[*apSegOffset + 2] & 15);
		for(i = 0; i < 16; i++)
		{
			if (i < 8)
			{
				if (apTextSegBase[*apSegOffset] & (1 << i))
				{
					aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, ", r%u", i);
				}
			}
			else
			{
				if (apTextSegBase[*apSegOffset + 1] & (1 << (i - 8)))
				{
					aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, ", r%u", i);
				}
			}
		}
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, " ");
		*apSegOffset += 4;
	}
	else if ((apTextSegBase[*apSegOffset + 3] & 15) == 9 && apTextSegBase[*apSegOffset + 2] == 45)
	{
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "push%s ", arm_32a_c4(apTextSegBase[*apSegOffset + 3] >> 4));
		UINT8 a = 0;
		for(i = 0; i < 16; i++)
		{
			if (i < 8)
			{
				if (apTextSegBase[*apSegOffset] & (1 << i))
				{
					if (a)
					{
						aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, ", ");
					}
					aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u", i);
					a = 1;
				}
				if (i == 7)
				{
					a = 0;
				}
			}
			else
			{
				if (apTextSegBase[*apSegOffset + 1] & (1 << (i - 8)))
				{
					if (a)
					{
						aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, ", ");
					}
					aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, ", r%u", i);
					a = 1;
				}
			}
		}
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, " ");
		*apSegOffset += 4;
	}
	else if ((apTextSegBase[*apSegOffset + 3] & 15) == 9 && (apTextSegBase[*apSegOffset + 2] & 240) == 0)
	{
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "stmdb%s ", arm_32a_c4(apTextSegBase[*apSegOffset + 3] >> 4));
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(r%u)", apTextSegBase[*apSegOffset + 2] & 15);
		for(i = 0; i < 16; i++)
		{
			if (i < 8)
			{
				if (apTextSegBase[*apSegOffset] & (1 << i))
				{
					aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, ", r%u", i);
				}
			}
			else
			{
				if (apTextSegBase[*apSegOffset + 1] & (1 << (i - 8)))
				{
					aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, ", r%u", i);
				}
			}
		}
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, " ");
		*apSegOffset += 4;
	}
	else if ((apTextSegBase[*apSegOffset + 3] & 15) == 9 && (apTextSegBase[*apSegOffset + 2] & 240) == 32)
	{
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "stmdbw%s ", arm_32a_c4(apTextSegBase[*apSegOffset + 3] >> 4));
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(r%u)", apTextSegBase[*apSegOffset + 2] & 15);
		for(i = 0; i < 16; i++)
		{
			if (i < 8)
			{
				if (apTextSegBase[*apSegOffset] & (1 << i))
				{
					aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, ", r%u", i);
				}
			}
			else
			{
				if (apTextSegBase[*apSegOffset + 1] & (1 << (i - 8)))
				{
					aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, ", r%u", i);
				}
			}
		}
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, " ");
		*apSegOffset += 4;
	}
	else if ((apTextSegBase[*apSegOffset + 3] & 15) == 9 && (apTextSegBase[*apSegOffset + 2] & 240) == 16)
	{
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "ldmdb%s ", arm_32a_c4(apTextSegBase[*apSegOffset + 3] >> 4));
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(r%u)", apTextSegBase[*apSegOffset + 2] & 15);
		for(i = 0; i < 16; i++)
		{
			if (i < 8)
			{
				if (apTextSegBase[*apSegOffset] & (1 << i))
				{
					aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, ", r%u", i);
				}
			}
			else
			{
				if (apTextSegBase[*apSegOffset + 1] & (1 << (i - 8)))
				{
					aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, ", r%u", i);
				}
			}
		}
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, " ");
		*apSegOffset += 4;
	}
	else if ((apTextSegBase[*apSegOffset + 3] & 15) == 9 && (apTextSegBase[*apSegOffset + 2] & 240) == 48)
	{
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "ldmdbw%s ", arm_32a_c4(apTextSegBase[*apSegOffset + 3] >> 4));
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(r%u)", apTextSegBase[*apSegOffset + 2] & 15);
		for(i = 0; i < 16; i++)
		{
			if (i < 8)
			{
				if (apTextSegBase[*apSegOffset] & (1 << i))
				{
					aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, ", r%u", i);
				}
			}
			else
			{
				if (apTextSegBase[*apSegOffset + 1] & (1 << (i - 8)))
				{
					aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, ", r%u", i);
				}
			}
		}
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, " ");
		*apSegOffset += 4;
	}
	else if ((apTextSegBase[*apSegOffset + 3] & 15) == 9 && (apTextSegBase[*apSegOffset + 2] & 240) == 128)
	{
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "stmib%s ", arm_32a_c4(apTextSegBase[*apSegOffset + 3] >> 4));
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(r%u)", apTextSegBase[*apSegOffset + 2] & 15);
		for(i = 0; i < 16; i++)
		{
			if (i < 8)
			{
				if (apTextSegBase[*apSegOffset] & (1 << i))
				{
					aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, ", r%u", i);
				}
			}
			else
			{
				if (apTextSegBase[*apSegOffset + 1] & (1 << (i - 8)))
				{
					aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, ", r%u", i);
				}
			}
		}
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, " ");
		*apSegOffset += 4;
	}
	else if ((apTextSegBase[*apSegOffset + 3] & 15) == 9 && (apTextSegBase[*apSegOffset + 2] & 240) == 160)
	{
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "stmibw%s ", arm_32a_c4(apTextSegBase[*apSegOffset + 3] >> 4));
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(r%u)", apTextSegBase[*apSegOffset + 2] & 15);
		for(i = 0; i < 16; i++)
		{
			if (i < 8)
			{
				if (apTextSegBase[*apSegOffset] & (1 << i))
				{
					aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, ", r%u", i);
				}
			}
			else
			{
				if (apTextSegBase[*apSegOffset + 1] & (1 << (i - 8)))
				{
					aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, ", r%u", i);
				}
			}
		}
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, " ");
		*apSegOffset += 4;
	}
	else if ((apTextSegBase[*apSegOffset + 3] & 15) == 9 && (apTextSegBase[*apSegOffset + 2] & 240) == 144)
	{
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "ldmib%s ", arm_32a_c4(apTextSegBase[*apSegOffset + 3] >> 4));
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(r%u)", apTextSegBase[*apSegOffset + 2] & 15);
		for(i = 0; i < 16; i++)
		{
			if (i < 8)
			{
				if (apTextSegBase[*apSegOffset] & (1 << i))
				{
					aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, ", r%u", i);
				}
			}
			else
			{
				if (apTextSegBase[*apSegOffset + 1] & (1 << (i - 8)))
				{
					aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, ", r%u", i);
				}
			}
		}
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, " ");
		*apSegOffset += 4;
	}
	else if ((apTextSegBase[*apSegOffset + 3] & 15) == 9 && (apTextSegBase[*apSegOffset + 2] & 240) == 176)
	{
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "ldmibw%s ", arm_32a_c4(apTextSegBase[*apSegOffset + 3] >> 4));
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(r%u)", apTextSegBase[*apSegOffset + 2] & 15);
		for(i = 0; i < 16; i++)
		{
			if (i < 8)
			{
				if (apTextSegBase[*apSegOffset] & (1 << i))
				{
					aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, ", r%u", i);
				}
			}
			else
			{
				if (apTextSegBase[*apSegOffset + 1] & (1 << (i - 8)))
				{
					aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, ", r%u", i);
				}
			}
		}
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, " ");
		*apSegOffset += 4;
	}
	else if ((apTextSegBase[*apSegOffset + 3] & 15) == 10)
	{ //todo
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "b%s ", arm_32a_c4(apTextSegBase[*apSegOffset + 3] >> 4));
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "%u ", (UINT32)(apTextSegBase[*apSegOffset] + (apSegOffset[*apSegOffset + 1] << 8) + (apTextSegBase[*apSegOffset + 2] << 16)));
		*apSegOffset += 4;
		*apRetDataAddr = ((apTextSegBase[*apSegOffset] + (apSegOffset[*apSegOffset + 1] << 8) + (apTextSegBase[*apSegOffset + 2] << 16)) * 4) + *apSegOffset + 8;
	}
	else if ((apTextSegBase[*apSegOffset + 3] & 15) == 11)
	{ //todo
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "bl%s ", arm_32a_c4(apTextSegBase[*apSegOffset + 3] >> 4));
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "%u ", (UINT32)(apTextSegBase[*apSegOffset] + (apSegOffset[*apSegOffset + 1] << 8) + (apTextSegBase[*apSegOffset + 2] << 16)));
		*apSegOffset += 4;
		*apRetDataAddr = ((apTextSegBase[*apSegOffset] + (apSegOffset[*apSegOffset + 1] << 8) + (apTextSegBase[*apSegOffset + 2] << 16)) * 4) + *apSegOffset + 8;
	}
	else if (apTextSegBase[*apSegOffset + 3] == 250)
	{ //todo
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "blx%s ", arm_32a_c4(apTextSegBase[*apSegOffset + 3] >> 4));
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "%u ", (UINT32)(apTextSegBase[*apSegOffset] + (apSegOffset[*apSegOffset + 1] << 8) + (apTextSegBase[*apSegOffset + 2] << 16)));
		*apSegOffset += 4;
		*apRetDataAddr = ((apTextSegBase[*apSegOffset] + (apSegOffset[*apSegOffset + 1] << 8) + (apTextSegBase[*apSegOffset + 2] << 16)) * 4) + *apSegOffset + 8;
	}
	else if ((apTextSegBase[*apSegOffset + 3] & 13) == 0 && (apTextSegBase[*apSegOffset + 2] & 224) == 0)
	{
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "and%s%s ", arm_32a_s(apTextSegBase[*apSegOffset + 2] & 16), arm_32a_c4(apTextSegBase[*apSegOffset + 3] >> 4));
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", apTextSegBase[*apSegOffset + 1] >> 4);
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", apTextSegBase[*apSegOffset + 2] & 15);

		if (apTextSegBase[*apSegOffset + 3] & 2)
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "%u ", apTextSegBase[*apSegOffset] + ((apTextSegBase[*apSegOffset + 1] & 15) << 8));
		}
		else if (apTextSegBase[*apSegOffset] & 16)
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", apTextSegBase[*apSegOffset] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(%s, ", arm_32a_sh(apTextSegBase[*apSegOffset] & 96));
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u) ", apTextSegBase[*apSegOffset + 1] & 15);
		}
		else if ((apTextSegBase[*apSegOffset] & 96) || ((apTextSegBase[*apSegOffset] >> 7) + ((apTextSegBase[*apSegOffset + 1] & 15) << 1)))
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", apTextSegBase[*apSegOffset] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(%s, ", arm_32a_sh(apTextSegBase[*apSegOffset] & 96));
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "%u) ", (apTextSegBase[*apSegOffset] >> 7) + ((apTextSegBase[*apSegOffset + 1] & 15) << 1));
		}
		else
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u ", apTextSegBase[*apSegOffset] & 15);
		}

		*apSegOffset += 4;
	}
	else if ((apTextSegBase[*apSegOffset + 3] & 13) == 0 && (apTextSegBase[*apSegOffset + 2] & 224) == 32)
	{
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "eor%s%s ", arm_32a_s(apTextSegBase[*apSegOffset + 2] & 16), arm_32a_c4(apTextSegBase[*apSegOffset + 3] >> 4));
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", apTextSegBase[*apSegOffset + 1] >> 4);
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", apTextSegBase[*apSegOffset + 2] & 15);

		if (apTextSegBase[*apSegOffset + 3] & 2)
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "%u ", apTextSegBase[*apSegOffset] + ((apTextSegBase[*apSegOffset + 1] & 15) << 8));
		}
		else if (apTextSegBase[*apSegOffset] & 16)
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", apTextSegBase[*apSegOffset] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(%s, ", arm_32a_sh(apTextSegBase[*apSegOffset] & 96));
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u) ", apTextSegBase[*apSegOffset + 1] & 15);
		}
		else if ((apTextSegBase[*apSegOffset] & 96) || ((apTextSegBase[*apSegOffset] >> 7) + ((apTextSegBase[*apSegOffset + 1] & 15) << 1)))
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", apTextSegBase[*apSegOffset] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(%s, ", arm_32a_sh(apTextSegBase[*apSegOffset] & 96));
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "%u) ", (apTextSegBase[*apSegOffset] >> 7) + ((apTextSegBase[*apSegOffset + 1] & 15) << 1));
		}
		else
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u ", apTextSegBase[*apSegOffset] & 15);
		}

		*apSegOffset += 4;
	}
	else if ((apTextSegBase[*apSegOffset + 3] & 13) == 0 && (apTextSegBase[*apSegOffset + 2] & 224) == 64)
	{
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "sub%s%s ", arm_32a_s(apTextSegBase[*apSegOffset + 2] & 16), arm_32a_c4(apTextSegBase[*apSegOffset + 3] >> 4));
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", apTextSegBase[*apSegOffset + 1] >> 4);
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", apTextSegBase[*apSegOffset + 2] & 15);

		if (apTextSegBase[*apSegOffset + 3] & 2)
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "%u ", apTextSegBase[*apSegOffset] + ((apTextSegBase[*apSegOffset + 1] & 15) << 8));
		}
		else if (apTextSegBase[*apSegOffset] & 16)
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", apTextSegBase[*apSegOffset] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(%s, ", arm_32a_sh(apTextSegBase[*apSegOffset] & 96));
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u) ", apTextSegBase[*apSegOffset + 1] & 15);
		}
		else if ((apTextSegBase[*apSegOffset] & 96) || ((apTextSegBase[*apSegOffset] >> 7) + ((apTextSegBase[*apSegOffset + 1] & 15) << 1)))
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", apTextSegBase[*apSegOffset] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(%s, ", arm_32a_sh(apTextSegBase[*apSegOffset] & 96));
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "%u) ", (apTextSegBase[*apSegOffset] >> 7) + ((apTextSegBase[*apSegOffset + 1] & 15) << 1));
		}
		else
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u ", apTextSegBase[*apSegOffset] & 15);
		}

		*apSegOffset += 4;
	}
	else if ((apTextSegBase[*apSegOffset + 3] & 13) == 0 && (apTextSegBase[*apSegOffset + 2] & 224) == 96)
	{
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "rsb%s%s ", arm_32a_s(apTextSegBase[*apSegOffset + 2] & 16), arm_32a_c4(apTextSegBase[*apSegOffset + 3] >> 4));
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", apTextSegBase[*apSegOffset + 1] >> 4);
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", apTextSegBase[*apSegOffset + 2] & 15);

		if (apTextSegBase[*apSegOffset + 3] & 2)
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "%u ", apTextSegBase[*apSegOffset] + ((apTextSegBase[*apSegOffset + 1] & 15) << 8));
		}
		else if (apTextSegBase[*apSegOffset] & 16)
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", apTextSegBase[*apSegOffset] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(%s, ", arm_32a_sh(apTextSegBase[*apSegOffset] & 96));
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u) ", apTextSegBase[*apSegOffset + 1] & 15);
		}
		else if ((apTextSegBase[*apSegOffset] & 96) || ((apTextSegBase[*apSegOffset] >> 7) + ((apTextSegBase[*apSegOffset + 1] & 15) << 1)))
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", apTextSegBase[*apSegOffset] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(%s, ", arm_32a_sh(apTextSegBase[*apSegOffset] & 96));
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "%u) ", (apTextSegBase[*apSegOffset] >> 7) + ((apTextSegBase[*apSegOffset + 1] & 15) << 1));
		}
		else
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u ", apTextSegBase[*apSegOffset] & 15);
		}

		*apSegOffset += 4;
	}
	else if ((apTextSegBase[*apSegOffset + 3] & 13) == 0 && (apTextSegBase[*apSegOffset + 2] & 224) == 128)
	{
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "add%s%s ", arm_32a_s(apTextSegBase[*apSegOffset + 2] & 16), arm_32a_c4(apTextSegBase[*apSegOffset + 3] >> 4));
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", apTextSegBase[*apSegOffset + 1] >> 4);
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", apTextSegBase[*apSegOffset + 2] & 15);

		if (apTextSegBase[*apSegOffset + 3] & 2)
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "%u ", apTextSegBase[*apSegOffset] + ((apTextSegBase[*apSegOffset + 1] & 15) << 8));
		}
		else if (apTextSegBase[*apSegOffset] & 16)
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", apTextSegBase[*apSegOffset] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(%s, ", arm_32a_sh(apTextSegBase[*apSegOffset] & 96));
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u) ", apTextSegBase[*apSegOffset + 1] & 15);
		}
		else if ((apTextSegBase[*apSegOffset] & 96) || ((apTextSegBase[*apSegOffset] >> 7) + ((apTextSegBase[*apSegOffset + 1] & 15) << 1)))
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", apTextSegBase[*apSegOffset] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(%s, ", arm_32a_sh(apTextSegBase[*apSegOffset] & 96));
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "%u) ", (apTextSegBase[*apSegOffset] >> 7) + ((apTextSegBase[*apSegOffset + 1] & 15) << 1));
		}
		else
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u ", apTextSegBase[*apSegOffset] & 15);
		}

		*apSegOffset += 4;
	}
	else if ((apTextSegBase[*apSegOffset + 3] & 13) == 0 && (apTextSegBase[*apSegOffset + 2] & 224) == 160)
	{
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "adc%s%s ", arm_32a_s(apTextSegBase[*apSegOffset + 2] & 16), arm_32a_c4(apTextSegBase[*apSegOffset + 3] >> 4));
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", apTextSegBase[*apSegOffset + 1] >> 4);
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", apTextSegBase[*apSegOffset + 2] & 15);

		if (apTextSegBase[*apSegOffset + 3] & 2)
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "%u ", apTextSegBase[*apSegOffset] + ((apTextSegBase[*apSegOffset + 1] & 15) << 8));
		}
		else if (apTextSegBase[*apSegOffset] & 16)
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", apTextSegBase[*apSegOffset] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(%s, ", arm_32a_sh(apTextSegBase[*apSegOffset] & 96));
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u) ", apTextSegBase[*apSegOffset + 1] & 15);
		}
		else if ((apTextSegBase[*apSegOffset] & 96) || ((apTextSegBase[*apSegOffset] >> 7) + ((apTextSegBase[*apSegOffset + 1] & 15) << 1)))
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", apTextSegBase[*apSegOffset] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(%s, ", arm_32a_sh(apTextSegBase[*apSegOffset] & 96));
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "%u) ", (apTextSegBase[*apSegOffset] >> 7) + ((apTextSegBase[*apSegOffset + 1] & 15) << 1));
		}
		else
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u ", apTextSegBase[*apSegOffset] & 15);
		}

		*apSegOffset += 4;
	}
	else if ((apTextSegBase[*apSegOffset + 3] & 13) == 0 && (apTextSegBase[*apSegOffset + 2] & 224) == 192)
	{
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "sbc%s%s ", arm_32a_s(apTextSegBase[*apSegOffset + 2] & 16), arm_32a_c4(apTextSegBase[*apSegOffset + 3] >> 4));
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", apTextSegBase[*apSegOffset + 1] >> 4);
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", apTextSegBase[*apSegOffset + 2] & 15);

		if (apTextSegBase[*apSegOffset + 3] & 2)
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "%u ", apTextSegBase[*apSegOffset] + ((apTextSegBase[*apSegOffset + 1] & 15) << 8));
		}
		else if (apTextSegBase[*apSegOffset] & 16)
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", apTextSegBase[*apSegOffset] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(%s, ", arm_32a_sh(apTextSegBase[*apSegOffset] & 96));
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u) ", apTextSegBase[*apSegOffset + 1] & 15);
		}
		else if ((apTextSegBase[*apSegOffset] & 96) || ((apTextSegBase[*apSegOffset] >> 7) + ((apTextSegBase[*apSegOffset + 1] & 15) << 1)))
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", apTextSegBase[*apSegOffset] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(%s, ", arm_32a_sh(apTextSegBase[*apSegOffset] & 96));
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "%u) ", (apTextSegBase[*apSegOffset] >> 7) + ((apTextSegBase[*apSegOffset + 1] & 15) << 1));
		}
		else
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u ", apTextSegBase[*apSegOffset] & 15);
		}

		*apSegOffset += 4;
	}
	else if ((apTextSegBase[*apSegOffset + 3] & 13) == 0 && (apTextSegBase[*apSegOffset + 2] & 224) == 224)
	{
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "rsc%s%s ", arm_32a_s(apTextSegBase[*apSegOffset + 2] & 16), arm_32a_c4(apTextSegBase[*apSegOffset + 3] >> 4));
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", apTextSegBase[*apSegOffset + 1] >> 4);
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", apTextSegBase[*apSegOffset + 2] & 15);

		if (apTextSegBase[*apSegOffset + 3] & 2)
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "%u ", apTextSegBase[*apSegOffset] + ((apTextSegBase[*apSegOffset + 1] & 15) << 8));
		}
		else if (apTextSegBase[*apSegOffset] & 16)
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", apTextSegBase[*apSegOffset] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(%s, ", arm_32a_sh(apTextSegBase[*apSegOffset] & 96));
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u) ", apTextSegBase[*apSegOffset + 1] & 15);
		}
		else if ((apTextSegBase[*apSegOffset] & 96) || ((apTextSegBase[*apSegOffset] >> 7) + ((apTextSegBase[*apSegOffset + 1] & 15) << 1)))
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", apTextSegBase[*apSegOffset] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(%s, ", arm_32a_sh(apTextSegBase[*apSegOffset] & 96));
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "%u) ", (apTextSegBase[*apSegOffset] >> 7) + ((apTextSegBase[*apSegOffset + 1] & 15) << 1));
		}
		else
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u ", apTextSegBase[*apSegOffset] & 15);
		}

		*apSegOffset += 4;
	}
	else if ((apTextSegBase[*apSegOffset + 3] & 13) == 1 && (apTextSegBase[*apSegOffset + 2] & 240) == 16)
	{
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "tst%s ", arm_32a_c4(apTextSegBase[*apSegOffset + 3] >> 4));
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", apTextSegBase[*apSegOffset + 2] & 15);

		if (apTextSegBase[*apSegOffset + 3] & 2)
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "%u ", apTextSegBase[*apSegOffset] + ((apTextSegBase[*apSegOffset + 1] & 15) << 8));
		}
		else if (apTextSegBase[*apSegOffset] & 16)
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", apTextSegBase[*apSegOffset] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(%s, ", arm_32a_sh(apTextSegBase[*apSegOffset] & 96));
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u) ", apTextSegBase[*apSegOffset + 1] & 15);
		}
		else if ((apTextSegBase[*apSegOffset] & 96) || ((apTextSegBase[*apSegOffset] >> 7) + ((apTextSegBase[*apSegOffset + 1] & 15) << 1)))
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", apTextSegBase[*apSegOffset] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(%s, ", arm_32a_sh(apTextSegBase[*apSegOffset] & 96));
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "%u) ", (apTextSegBase[*apSegOffset] >> 7) + ((apTextSegBase[*apSegOffset + 1] & 15) << 1));
		}
		else
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u ", apTextSegBase[*apSegOffset] & 15);
		}

		*apSegOffset += 4;
	}
	else if ((apTextSegBase[*apSegOffset + 3] & 13) == 1 && (apTextSegBase[*apSegOffset + 2] & 240) == 48)
	{
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "teq%s ", arm_32a_c4(apTextSegBase[*apSegOffset + 3] >> 4));
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", apTextSegBase[*apSegOffset + 2] & 15);

		if (apTextSegBase[*apSegOffset + 3] & 2)
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "%u ", apTextSegBase[*apSegOffset] + ((apTextSegBase[*apSegOffset + 1] & 15) << 8));
		}
		else if (apTextSegBase[*apSegOffset] & 16)
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", apTextSegBase[*apSegOffset] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(%s, ", arm_32a_sh(apTextSegBase[*apSegOffset] & 96));
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u) ", apTextSegBase[*apSegOffset + 1] & 15);
		}
		else if ((apTextSegBase[*apSegOffset] & 96) || ((apTextSegBase[*apSegOffset] >> 7) + ((apTextSegBase[*apSegOffset + 1] & 15) << 1)))
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", apTextSegBase[*apSegOffset] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(%s, ", arm_32a_sh(apTextSegBase[*apSegOffset] & 96));
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "%u) ", (apTextSegBase[*apSegOffset] >> 7) + ((apTextSegBase[*apSegOffset + 1] & 15) << 1));
		}
		else
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u ", apTextSegBase[*apSegOffset] & 15);
		}

		*apSegOffset += 4;
	}
	else if ((apTextSegBase[*apSegOffset + 3] & 13) == 1 && (apTextSegBase[*apSegOffset + 2] & 240) == 80)
	{
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "cmp%s ", arm_32a_c4(apTextSegBase[*apSegOffset + 3] >> 4));
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", apTextSegBase[*apSegOffset + 2] & 15);

		if (apTextSegBase[*apSegOffset + 3] & 2)
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "%u ", apTextSegBase[*apSegOffset] + ((apTextSegBase[*apSegOffset + 1] & 15) << 8));
		}
		else if (apTextSegBase[*apSegOffset] & 16)
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", apTextSegBase[*apSegOffset] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(%s, ", arm_32a_sh(apTextSegBase[*apSegOffset] & 96));
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u) ", apTextSegBase[*apSegOffset + 1] & 15);
		}
		else if ((apTextSegBase[*apSegOffset] & 96) || ((apTextSegBase[*apSegOffset] >> 7) + ((apTextSegBase[*apSegOffset + 1] & 15) << 1)))
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", apTextSegBase[*apSegOffset] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(%s, ", arm_32a_sh(apTextSegBase[*apSegOffset] & 96));
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "%u) ", (apTextSegBase[*apSegOffset] >> 7) + ((apTextSegBase[*apSegOffset + 1] & 15) << 1));
		}
		else
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u ", apTextSegBase[*apSegOffset] & 15);
		}

		*apSegOffset += 4;
	}
	else if ((apTextSegBase[*apSegOffset + 3] & 13) == 1 && (apTextSegBase[*apSegOffset + 2] & 240) == 112)
	{
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "cmn%s ", arm_32a_c4(apTextSegBase[*apSegOffset + 3] >> 4));
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", apTextSegBase[*apSegOffset + 2] & 15);

		if (apTextSegBase[*apSegOffset + 3] & 2)
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "%u ", apTextSegBase[*apSegOffset] + ((apTextSegBase[*apSegOffset + 1] & 15) << 8));
		}
		else if (apTextSegBase[*apSegOffset] & 16)
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", apTextSegBase[*apSegOffset] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(%s, ", arm_32a_sh(apTextSegBase[*apSegOffset] & 96));
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u) ", apTextSegBase[*apSegOffset + 1] & 15);
		}
		else if ((apTextSegBase[*apSegOffset] & 96) || ((apTextSegBase[*apSegOffset] >> 7) + ((apTextSegBase[*apSegOffset + 1] & 15) << 1)))
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", apTextSegBase[*apSegOffset] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(%s, ", arm_32a_sh(apTextSegBase[*apSegOffset] & 96));
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "%u) ", (apTextSegBase[*apSegOffset] >> 7) + ((apTextSegBase[*apSegOffset + 1] & 15) << 1));
		}
		else
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u ", apTextSegBase[*apSegOffset] & 15);
		}

		*apSegOffset += 4;
	}
	else if ((apTextSegBase[*apSegOffset + 3] & 13) == 1 && (apTextSegBase[*apSegOffset + 2] & 224) == 128)
	{
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "orr%s%s ", arm_32a_s(apTextSegBase[*apSegOffset + 2] & 16), arm_32a_c4(apTextSegBase[*apSegOffset + 3] >> 4));
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", apTextSegBase[*apSegOffset + 1] >> 4);
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", apTextSegBase[*apSegOffset + 2] & 15);

		if (apTextSegBase[*apSegOffset + 3] & 2)
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "%u ", apTextSegBase[*apSegOffset] + ((apTextSegBase[*apSegOffset + 1] & 15) << 8));
		}
		else if (apTextSegBase[*apSegOffset] & 16)
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", apTextSegBase[*apSegOffset] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(%s, ", arm_32a_sh(apTextSegBase[*apSegOffset] & 96));
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u) ", apTextSegBase[*apSegOffset + 1] & 15);
		}
		else if ((apTextSegBase[*apSegOffset] & 96) || ((apTextSegBase[*apSegOffset] >> 7) + ((apTextSegBase[*apSegOffset + 1] & 15) << 1)))
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", apTextSegBase[*apSegOffset] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "(%s, ", arm_32a_sh(apTextSegBase[*apSegOffset] & 96));
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "%u) ", (apTextSegBase[*apSegOffset] >> 7) + ((apTextSegBase[*apSegOffset + 1] & 15) << 1));
		}
		else
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u ", apTextSegBase[*apSegOffset] & 15);
		}

		*apSegOffset += 4;
	}
	else if ((apTextSegBase[*apSegOffset + 3] & 13) == 1 && (apTextSegBase[*apSegOffset + 2] & 224) == 160 && (apTextSegBase[*apSegOffset] & 96) == 0 && !((apTextSegBase[*apSegOffset] >> 7) + ((apTextSegBase[*apSegOffset + 1] & 15) << 1)))
	{
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "mov%s%s ", arm_32a_s(apTextSegBase[*apSegOffset + 2] & 16), arm_32a_c4(apTextSegBase[*apSegOffset + 3] >> 4));
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", apTextSegBase[*apSegOffset + 1] >> 4);

		if (apTextSegBase[*apSegOffset + 3] & 2)
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "%u ", apTextSegBase[*apSegOffset] + ((apTextSegBase[*apSegOffset + 1] & 15) << 8));
		}
		else
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u ", apTextSegBase[*apSegOffset] & 15);
		}

		*apSegOffset += 4;
	}
	else if ((apTextSegBase[*apSegOffset + 3] & 13) == 1 && (apTextSegBase[*apSegOffset + 2] & 224) == 160 && (apTextSegBase[*apSegOffset] & 96) == 0)
	{
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "lsl%s%s ", arm_32a_s(apTextSegBase[*apSegOffset + 2] & 16), arm_32a_c4(apTextSegBase[*apSegOffset + 3] >> 4));
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", apTextSegBase[*apSegOffset + 1] >> 4);
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", apTextSegBase[*apSegOffset] & 15);

		if (apTextSegBase[*apSegOffset] & 16)
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u ", apTextSegBase[*apSegOffset + 1] & 15);
		}
		else
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "%u", (apTextSegBase[*apSegOffset] >> 7) + ((apTextSegBase[*apSegOffset + 1] & 15) << 1));
		}

		*apSegOffset += 4;
	}
	else if ((apTextSegBase[*apSegOffset + 3] & 13) == 1 && (apTextSegBase[*apSegOffset + 2] & 224) == 160 && (apTextSegBase[*apSegOffset] & 96) == 32)
	{
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "lsr%s%s ", arm_32a_s(apTextSegBase[*apSegOffset + 2] & 16), arm_32a_c4(apTextSegBase[*apSegOffset + 3] >> 4));
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", apTextSegBase[*apSegOffset + 1] >> 4);
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", apTextSegBase[*apSegOffset] & 15);

		if (apTextSegBase[*apSegOffset] & 16)
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u ", apTextSegBase[*apSegOffset + 1] & 15);
		}
		else
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "%u", (apTextSegBase[*apSegOffset] >> 7) + ((apTextSegBase[*apSegOffset + 1] & 15) << 1));
		}

		*apSegOffset += 4;
	}
	else if ((apTextSegBase[*apSegOffset + 3] & 13) == 1 && (apTextSegBase[*apSegOffset + 2] & 224) == 160 && (apTextSegBase[*apSegOffset] & 96) == 64)
	{
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "asr%s%s ", arm_32a_s(apTextSegBase[*apSegOffset + 2] & 16), arm_32a_c4(apTextSegBase[*apSegOffset + 3] >> 4));
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", apTextSegBase[*apSegOffset + 1] >> 4);
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", apTextSegBase[*apSegOffset] & 15);

		if (apTextSegBase[*apSegOffset] & 16)
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u ", apTextSegBase[*apSegOffset + 1] & 15);
		}
		else
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "%u", (apTextSegBase[*apSegOffset] >> 7) + ((apTextSegBase[*apSegOffset + 1] & 15) << 1));
		}

		*apSegOffset += 4;
	}
	else if ((apTextSegBase[*apSegOffset + 3] & 13) == 1 && (apTextSegBase[*apSegOffset + 2] & 224) == 160 && (apTextSegBase[*apSegOffset] & 96) == 96)
	{
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "ror%s%s ", arm_32a_s(apTextSegBase[*apSegOffset + 2] & 16), arm_32a_c4(apTextSegBase[*apSegOffset + 3] >> 4));
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", apTextSegBase[*apSegOffset + 1] >> 4);
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", apTextSegBase[*apSegOffset] & 15);

		if (apTextSegBase[*apSegOffset] & 16)
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u ", apTextSegBase[*apSegOffset + 1] & 15);
		}
		else
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "%u", (apTextSegBase[*apSegOffset] >> 7) + ((apTextSegBase[*apSegOffset + 1] & 15) << 1));
		}

		*apSegOffset += 4;
	}
	else if ((apTextSegBase[*apSegOffset + 3] & 13) == 1 && (apTextSegBase[*apSegOffset + 2] & 224) == 192)
	{
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "bic%s%s ", arm_32a_s(apTextSegBase[*apSegOffset + 2] & 16), arm_32a_c4(apTextSegBase[*apSegOffset + 3] >> 4));
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", apTextSegBase[*apSegOffset + 1] >> 4);
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", apTextSegBase[*apSegOffset + 2] & 15);

		if (apTextSegBase[*apSegOffset + 3] & 2)
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "%u ", apTextSegBase[*apSegOffset] + ((apTextSegBase[*apSegOffset + 1] & 15) << 8));
		}
		else if (apTextSegBase[*apSegOffset] & 16)
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", apTextSegBase[*apSegOffset] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "%s, ", arm_32a_sh(apTextSegBase[*apSegOffset] & 96));
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u ", apTextSegBase[*apSegOffset + 1] & 15);
		}
		else if ((apTextSegBase[*apSegOffset] & 96) || ((apTextSegBase[*apSegOffset] >> 7) + ((apTextSegBase[*apSegOffset + 1] & 15) << 1)))
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", apTextSegBase[*apSegOffset] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "%s, %u", arm_32a_sh(apTextSegBase[*apSegOffset] & 96), (apTextSegBase[*apSegOffset] >> 7) + ((apTextSegBase[*apSegOffset + 1] & 15) << 1));
		}
		else
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u ", apTextSegBase[*apSegOffset] & 15);
		}

		*apSegOffset += 4;
	}
	else if ((apTextSegBase[*apSegOffset + 3] & 13) == 1 && (apTextSegBase[*apSegOffset + 2] & 224) == 224)
	{
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "mvn%s%s ", arm_32a_s(apTextSegBase[*apSegOffset + 2] & 16), arm_32a_c4(apTextSegBase[*apSegOffset + 3] >> 4));
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", apTextSegBase[*apSegOffset + 1] >> 4);
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", apTextSegBase[*apSegOffset + 2] & 15);

		if (apTextSegBase[*apSegOffset + 3] & 2)
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "%u ", apTextSegBase[*apSegOffset] + ((apTextSegBase[*apSegOffset + 1] & 15) << 8));
		}
		else if (apTextSegBase[*apSegOffset] & 16)
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", apTextSegBase[*apSegOffset] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "%s, ", arm_32a_sh(apTextSegBase[*apSegOffset] & 96));
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u ", apTextSegBase[*apSegOffset + 1] & 15);
		}
		else if ((apTextSegBase[*apSegOffset] & 96) || ((apTextSegBase[*apSegOffset] >> 7) + ((apTextSegBase[*apSegOffset + 1] & 15) << 1)))
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", apTextSegBase[*apSegOffset] & 15);
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "%s, %u", arm_32a_sh(apTextSegBase[*apSegOffset] & 96), (apTextSegBase[*apSegOffset] >> 7) + ((apTextSegBase[*apSegOffset + 1] & 15) << 1));
		}
		else
		{
			aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u ", apTextSegBase[*apSegOffset] & 15);
		}

		*apSegOffset += 4;
	}
	else if ((apTextSegBase[*apSegOffset + 3] & 15) == 0 && (apTextSegBase[*apSegOffset + 2] & 224) == 0 && (apTextSegBase[*apSegOffset] & 240) == 144)
	{
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "mul%s%s ", arm_32a_s(apTextSegBase[*apSegOffset + 2] & 16), arm_32a_c4(apTextSegBase[*apSegOffset + 3] >> 4));
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", apTextSegBase[*apSegOffset + 2] & 15);
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", apTextSegBase[*apSegOffset] & 15);
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u ", apTextSegBase[*apSegOffset + 1] & 15);
		*apSegOffset += 4;
	}
	else if ((apTextSegBase[*apSegOffset + 3] & 15) == 0 && (apTextSegBase[*apSegOffset + 2] & 224) == 32 && (apTextSegBase[*apSegOffset] & 240) == 144)
	{
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "mla%s%s ", arm_32a_s(apTextSegBase[*apSegOffset + 2] & 16), arm_32a_c4(apTextSegBase[*apSegOffset + 3] >> 4));
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", apTextSegBase[*apSegOffset + 2] & 15);
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", apTextSegBase[*apSegOffset] & 15);
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u ", apTextSegBase[*apSegOffset + 1] & 15);
		*apSegOffset += 4;
	}
	else if ((apTextSegBase[*apSegOffset + 3] & 15) == 0 && (apTextSegBase[*apSegOffset + 2] & 240) == 64 && (apTextSegBase[*apSegOffset] & 240) == 144)
	{
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "umaal%s ", arm_32a_c4(apTextSegBase[*apSegOffset + 3] >> 4));
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", (apTextSegBase[*apSegOffset + 1] >> 4) & 15);
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", apTextSegBase[*apSegOffset + 2] & 15);
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", apTextSegBase[*apSegOffset] & 15);
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u ", apTextSegBase[*apSegOffset + 1] & 15);
		*apSegOffset += 4;
	}
	else if ((apTextSegBase[*apSegOffset + 3] & 15) == 0 && (apTextSegBase[*apSegOffset + 2] & 240) == 96 && (apTextSegBase[*apSegOffset] & 240) == 144)
	{
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "mls%s ", arm_32a_c4(apTextSegBase[*apSegOffset + 3] >> 4));
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", apTextSegBase[*apSegOffset + 2] & 15);
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", apTextSegBase[*apSegOffset] & 15);
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", apTextSegBase[*apSegOffset + 1] & 15);
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u ", (apTextSegBase[*apSegOffset + 1] >> 4) & 15);
		*apSegOffset += 4;
	}
	else if ((apTextSegBase[*apSegOffset + 3] & 15) == 0 && (apTextSegBase[*apSegOffset + 2] & 224) == 128 && (apTextSegBase[*apSegOffset] & 240) == 144)
	{
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "umull%s%s ", arm_32a_s(apTextSegBase[*apSegOffset + 2] & 16), arm_32a_c4(apTextSegBase[*apSegOffset + 3] >> 4));
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", (apTextSegBase[*apSegOffset + 1] >> 4) & 15);
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", apTextSegBase[*apSegOffset + 2] & 15);
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", apTextSegBase[*apSegOffset] & 15);
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u ", apTextSegBase[*apSegOffset + 1] & 15);
		*apSegOffset += 4;
	}
	else if ((apTextSegBase[*apSegOffset + 3] & 15) == 0 && (apTextSegBase[*apSegOffset + 2] & 224) == 160 && (apTextSegBase[*apSegOffset] & 240) == 144)
	{
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "umlal%s%s ", arm_32a_s(apTextSegBase[*apSegOffset + 2] & 16), arm_32a_c4(apTextSegBase[*apSegOffset + 3] >> 4));
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", (apTextSegBase[*apSegOffset + 1] >> 4) & 15);
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", apTextSegBase[*apSegOffset + 2] & 15);
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", apTextSegBase[*apSegOffset] & 15);
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u ", apTextSegBase[*apSegOffset + 1] & 15);
		*apSegOffset += 4;
	}
	else if ((apTextSegBase[*apSegOffset + 3] & 15) == 0 && (apTextSegBase[*apSegOffset + 2] & 224) == 192 && (apTextSegBase[*apSegOffset] & 240) == 144)
	{
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "smull%s%s ", arm_32a_s(apTextSegBase[*apSegOffset + 2] & 16), arm_32a_c4(apTextSegBase[*apSegOffset + 3] >> 4));
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", (apTextSegBase[*apSegOffset + 1] >> 4) & 15);
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", apTextSegBase[*apSegOffset + 2] & 15);
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", apTextSegBase[*apSegOffset] & 15);
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u ", apTextSegBase[*apSegOffset + 1] & 15);
		*apSegOffset += 4;
	}
	else if ((apTextSegBase[*apSegOffset + 3] & 15) == 0 && (apTextSegBase[*apSegOffset + 2] & 224) == 224 && (apTextSegBase[*apSegOffset] & 240) == 144)
	{
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "smlal%s%s ", arm_32a_s(apTextSegBase[*apSegOffset + 2] & 16), arm_32a_c4(apTextSegBase[*apSegOffset + 3] >> 4));
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", (apTextSegBase[*apSegOffset + 1] >> 4) & 15);
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", apTextSegBase[*apSegOffset + 2] & 15);
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", apTextSegBase[*apSegOffset] & 15);
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u ", apTextSegBase[*apSegOffset + 1] & 15);
		*apSegOffset += 4;
	}
	else if ((apTextSegBase[*apSegOffset + 3] & 15) == 1 && (apTextSegBase[*apSegOffset + 2] & 240) == 0 && (apTextSegBase[*apSegOffset] & 240) == 80)
	{
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "qadd%s ", arm_32a_c4(apTextSegBase[*apSegOffset + 3] >> 4));
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", (apTextSegBase[*apSegOffset + 1] >> 4) & 15);
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", apTextSegBase[*apSegOffset + 2] & 15);
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u ", apTextSegBase[*apSegOffset] & 15);
		*apSegOffset += 4;
	}
	else if ((apTextSegBase[*apSegOffset + 3] & 15) == 1 && (apTextSegBase[*apSegOffset + 2] & 240) == 32 && (apTextSegBase[*apSegOffset] & 240) == 80)
	{
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "qsub%s ", arm_32a_c4(apTextSegBase[*apSegOffset + 3] >> 4));
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", (apTextSegBase[*apSegOffset + 1] >> 4) & 15);
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", apTextSegBase[*apSegOffset + 2] & 15);
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u ", apTextSegBase[*apSegOffset] & 15);
		*apSegOffset += 4;
	}
	else if ((apTextSegBase[*apSegOffset + 3] & 15) == 1 && (apTextSegBase[*apSegOffset + 2] & 240) == 64 && (apTextSegBase[*apSegOffset] & 240) == 80)
	{
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "qdadd%s ", arm_32a_c4(apTextSegBase[*apSegOffset + 3] >> 4));
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", (apTextSegBase[*apSegOffset + 1] >> 4) & 15);
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", apTextSegBase[*apSegOffset + 2] & 15);
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u ", apTextSegBase[*apSegOffset] & 15);
		*apSegOffset += 4;
	}
	else if ((apTextSegBase[*apSegOffset + 3] & 15) == 1 && (apTextSegBase[*apSegOffset + 2] & 240) == 96 && (apTextSegBase[*apSegOffset] & 240) == 80)
	{
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "qdsub%s ", arm_32a_c4(apTextSegBase[*apSegOffset + 3] >> 4));
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", (apTextSegBase[*apSegOffset + 1] >> 4) & 15);
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", apTextSegBase[*apSegOffset + 2] & 15);
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u ", apTextSegBase[*apSegOffset] & 15);
		*apSegOffset += 4;
	}
	else if ((apTextSegBase[*apSegOffset + 3] & 15) == 1 && (apTextSegBase[*apSegOffset + 2] & 240) == 0 && (apTextSegBase[*apSegOffset] & 240) == 128)
	{
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "smlabb%s ", arm_32a_c4(apTextSegBase[*apSegOffset + 3] >> 4));
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", apTextSegBase[*apSegOffset + 2] & 15);
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", apTextSegBase[*apSegOffset] & 15);
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", apTextSegBase[*apSegOffset + 1] & 15);
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u ", (apTextSegBase[*apSegOffset + 1] >> 4) & 15);
		*apSegOffset += 4;
	}
	else if ((apTextSegBase[*apSegOffset + 3] & 15) == 1 && (apTextSegBase[*apSegOffset + 2] & 240) == 0 && (apTextSegBase[*apSegOffset] & 240) == 160)
	{
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "smlatb%s ", arm_32a_c4(apTextSegBase[*apSegOffset + 3] >> 4));
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", apTextSegBase[*apSegOffset + 2] & 15);
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", apTextSegBase[*apSegOffset] & 15);
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", apTextSegBase[*apSegOffset + 1] & 15);
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u ", (apTextSegBase[*apSegOffset + 1] >> 4) & 15);
		*apSegOffset += 4;
	}
	else if ((apTextSegBase[*apSegOffset + 3] & 15) == 1 && (apTextSegBase[*apSegOffset + 2] & 240) == 0 && (apTextSegBase[*apSegOffset] & 240) == 192)
	{
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "smlabt%s ", arm_32a_c4(apTextSegBase[*apSegOffset + 3] >> 4));
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", apTextSegBase[*apSegOffset + 2] & 15);
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", apTextSegBase[*apSegOffset] & 15);
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", apTextSegBase[*apSegOffset + 1] & 15);
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u ", (apTextSegBase[*apSegOffset + 1] >> 4) & 15);
		*apSegOffset += 4;
	}
	else if ((apTextSegBase[*apSegOffset + 3] & 15) == 1 && (apTextSegBase[*apSegOffset + 2] & 240) == 0 && (apTextSegBase[*apSegOffset] & 240) == 224)
	{
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "smlatt%s ", arm_32a_c4(apTextSegBase[*apSegOffset + 3] >> 4));
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", apTextSegBase[*apSegOffset + 2] & 15);
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", apTextSegBase[*apSegOffset] & 15);
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", apTextSegBase[*apSegOffset + 1] & 15);
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u ", (apTextSegBase[*apSegOffset + 1] >> 4) & 15);
		*apSegOffset += 4;
	}
	else if ((apTextSegBase[*apSegOffset + 3] & 15) == 1 && (apTextSegBase[*apSegOffset + 2] & 240) == 32 && (apTextSegBase[*apSegOffset] & 240) == 128)
	{
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "smlawb%s ", arm_32a_c4(apTextSegBase[*apSegOffset + 3] >> 4));
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", apTextSegBase[*apSegOffset + 2] & 15);
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", apTextSegBase[*apSegOffset] & 15);
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", apTextSegBase[*apSegOffset + 1] & 15);
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u ", (apTextSegBase[*apSegOffset + 1] >> 4) & 15);
		*apSegOffset += 4;
	}
	else if ((apTextSegBase[*apSegOffset + 3] & 15) == 1 && (apTextSegBase[*apSegOffset + 2] & 240) == 32 && (apTextSegBase[*apSegOffset] & 240) == 192)
	{
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "smlawt%s ", arm_32a_c4(apTextSegBase[*apSegOffset + 3] >> 4));
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", apTextSegBase[*apSegOffset + 2] & 15);
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", apTextSegBase[*apSegOffset] & 15);
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", apTextSegBase[*apSegOffset + 1] & 15);
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u ", (apTextSegBase[*apSegOffset + 1] >> 4) & 15);
		*apSegOffset += 4;
	}
	else if ((apTextSegBase[*apSegOffset + 3] & 15) == 1 && (apTextSegBase[*apSegOffset + 2] & 240) == 32 && (apTextSegBase[*apSegOffset] & 240) == 160)
	{
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "smulwb%s ", arm_32a_c4(apTextSegBase[*apSegOffset + 3] >> 4));
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", apTextSegBase[*apSegOffset + 2] & 15);
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", apTextSegBase[*apSegOffset] & 15);
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", apTextSegBase[*apSegOffset + 1] & 15);
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u ", (apTextSegBase[*apSegOffset + 1] >> 4) & 15);
		*apSegOffset += 4;
	}
	else if ((apTextSegBase[*apSegOffset + 3] & 15) == 1 && (apTextSegBase[*apSegOffset + 2] & 240) == 32 && (apTextSegBase[*apSegOffset] & 240) == 224)
	{
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "smulwt%s ", arm_32a_c4(apTextSegBase[*apSegOffset + 3] >> 4));
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", apTextSegBase[*apSegOffset + 2] & 15);
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", apTextSegBase[*apSegOffset] & 15);
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", apTextSegBase[*apSegOffset + 1] & 15);
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u ", (apTextSegBase[*apSegOffset + 1] >> 4) & 15);
		*apSegOffset += 4;
	}
	else if ((apTextSegBase[*apSegOffset + 3] & 15) == 1 && (apTextSegBase[*apSegOffset + 2] & 240) == 32 && (apTextSegBase[*apSegOffset] & 240) == 128)
	{
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "smlawb%s ", arm_32a_c4(apTextSegBase[*apSegOffset + 3] >> 4));
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", apTextSegBase[*apSegOffset + 2] & 15);
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", apTextSegBase[*apSegOffset] & 15);
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", apTextSegBase[*apSegOffset + 1] & 15);
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u ", (apTextSegBase[*apSegOffset + 1] >> 4) & 15);
		*apSegOffset += 4;
	}
	else if ((apTextSegBase[*apSegOffset + 3] & 15) == 1 && (apTextSegBase[*apSegOffset + 2] & 240) == 64 && (apTextSegBase[*apSegOffset] & 240) == 128)
	{
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "smlalbb%s ", arm_32a_c4(apTextSegBase[*apSegOffset + 3] >> 4));
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", (apTextSegBase[*apSegOffset + 1] >> 4) & 15);
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", apTextSegBase[*apSegOffset + 2] & 15);
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", apTextSegBase[*apSegOffset] & 15);
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u ", apTextSegBase[*apSegOffset + 1] & 15);
		*apSegOffset += 4;
	}
	else if ((apTextSegBase[*apSegOffset + 3] & 15) == 1 && (apTextSegBase[*apSegOffset + 2] & 240) == 64 && (apTextSegBase[*apSegOffset] & 240) == 160)
	{
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "smlaltb%s ", arm_32a_c4(apTextSegBase[*apSegOffset + 3] >> 4));
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", (apTextSegBase[*apSegOffset + 1] >> 4) & 15);
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", apTextSegBase[*apSegOffset + 2] & 15);
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", apTextSegBase[*apSegOffset] & 15);
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u ", apTextSegBase[*apSegOffset + 1] & 15);
		*apSegOffset += 4;
	}
	else if ((apTextSegBase[*apSegOffset + 3] & 15) == 1 && (apTextSegBase[*apSegOffset + 2] & 240) == 64 && (apTextSegBase[*apSegOffset] & 240) == 192)
	{
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "smlalbt%s ", arm_32a_c4(apTextSegBase[*apSegOffset + 3] >> 4));
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", (apTextSegBase[*apSegOffset + 1] >> 4) & 15);
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", apTextSegBase[*apSegOffset + 2] & 15);
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", apTextSegBase[*apSegOffset] & 15);
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u ", apTextSegBase[*apSegOffset + 1] & 15);
		*apSegOffset += 4;
	}
	else if ((apTextSegBase[*apSegOffset + 3] & 15) == 1 && (apTextSegBase[*apSegOffset + 2] & 240) == 64 && (apTextSegBase[*apSegOffset] & 240) == 224)
	{
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "smlaltt%s ", arm_32a_c4(apTextSegBase[*apSegOffset + 3] >> 4));
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", (apTextSegBase[*apSegOffset + 1] >> 4) & 15);
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", apTextSegBase[*apSegOffset + 2] & 15);
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", apTextSegBase[*apSegOffset] & 15);
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u ", apTextSegBase[*apSegOffset + 1] & 15);
		*apSegOffset += 4;
	}
	else if ((apTextSegBase[*apSegOffset + 3] & 15) == 1 && (apTextSegBase[*apSegOffset + 2] & 240) == 96 && (apTextSegBase[*apSegOffset] & 240) == 128)
	{
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "smulbb%s ", arm_32a_c4(apTextSegBase[*apSegOffset + 3] >> 4));
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", apTextSegBase[*apSegOffset + 2] & 15);
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", apTextSegBase[*apSegOffset] & 15);
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u ", apTextSegBase[*apSegOffset + 1] & 15);
		*apSegOffset += 4;
	}
	else if ((apTextSegBase[*apSegOffset + 3] & 15) == 1 && (apTextSegBase[*apSegOffset + 2] & 240) == 96 && (apTextSegBase[*apSegOffset] & 240) == 160)
	{
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "smultb%s ", arm_32a_c4(apTextSegBase[*apSegOffset + 3] >> 4));
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", apTextSegBase[*apSegOffset + 2] & 15);
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", apTextSegBase[*apSegOffset] & 15);
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u ", apTextSegBase[*apSegOffset + 1] & 15);
		*apSegOffset += 4;
	}
	else if ((apTextSegBase[*apSegOffset + 3] & 15) == 1 && (apTextSegBase[*apSegOffset + 2] & 240) == 96 && (apTextSegBase[*apSegOffset] & 240) == 192)
	{
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "smulbt%s ", arm_32a_c4(apTextSegBase[*apSegOffset + 3] >> 4));
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", apTextSegBase[*apSegOffset + 2] & 15);
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", apTextSegBase[*apSegOffset] & 15);
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u ", apTextSegBase[*apSegOffset + 1] & 15);
		*apSegOffset += 4;
	}
	else if ((apTextSegBase[*apSegOffset + 3] & 15) == 1 && (apTextSegBase[*apSegOffset + 2] & 240) == 96 && (apTextSegBase[*apSegOffset] & 240) == 224)
	{
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "smultt%s ", arm_32a_c4(apTextSegBase[*apSegOffset + 3] >> 4));
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", apTextSegBase[*apSegOffset + 2] & 15);
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", apTextSegBase[*apSegOffset] & 15);
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u ", apTextSegBase[*apSegOffset + 1] & 15);
		*apSegOffset += 4;
	}
	else if ((apTextSegBase[*apSegOffset + 3] & 15) == 1 && (apTextSegBase[*apSegOffset + 2] & 240) == 0 && (apTextSegBase[*apSegOffset] & 240) == 144)
	{
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "swp%s ", arm_32a_c4(apTextSegBase[*apSegOffset + 3] >> 4));
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", (apTextSegBase[*apSegOffset + 1] >> 4) & 15);
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", apTextSegBase[*apSegOffset] & 15);
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u ", apTextSegBase[*apSegOffset + 2] & 15);
		*apSegOffset += 4;
	}
	else if ((apTextSegBase[*apSegOffset + 3] & 15) == 1 && (apTextSegBase[*apSegOffset + 2] & 240) == 64 && (apTextSegBase[*apSegOffset] & 240) == 144)
	{
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "swpb%s ", arm_32a_c4(apTextSegBase[*apSegOffset + 3] >> 4));
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", (apTextSegBase[*apSegOffset + 1] >> 4) & 15);
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", apTextSegBase[*apSegOffset] & 15);
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u ", apTextSegBase[*apSegOffset + 2] & 15);
		*apSegOffset += 4;
	}
	else if ((apTextSegBase[*apSegOffset + 3] & 15) == 1 && (apTextSegBase[*apSegOffset + 2] & 240) == 128 && (apTextSegBase[*apSegOffset] & 240) == 144)
	{
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "strex%s ", arm_32a_c4(apTextSegBase[*apSegOffset + 3] >> 4));
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", (apTextSegBase[*apSegOffset + 1] >> 4) & 15);
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", apTextSegBase[*apSegOffset] & 15);
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u ", apTextSegBase[*apSegOffset + 2] & 15);
		*apSegOffset += 4;
	}
	else if ((apTextSegBase[*apSegOffset + 3] & 15) == 1 && (apTextSegBase[*apSegOffset + 2] & 240) == 144 && (apTextSegBase[*apSegOffset] & 240) == 144)
	{
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "ldrex%s ", arm_32a_c4(apTextSegBase[*apSegOffset + 3] >> 4));
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", (apTextSegBase[*apSegOffset + 1] >> 4) & 15);
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u ", apTextSegBase[*apSegOffset + 2] & 15);
		*apSegOffset += 4;
	}
	else if ((apTextSegBase[*apSegOffset + 3] & 15) == 1 && (apTextSegBase[*apSegOffset + 2] & 240) == 160 && (apTextSegBase[*apSegOffset] & 240) == 144)
	{
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "strexd%s ", arm_32a_c4(apTextSegBase[*apSegOffset + 3] >> 4));
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", (apTextSegBase[*apSegOffset + 1] >> 4) & 15);
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", apTextSegBase[*apSegOffset] & 15);
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u ", apTextSegBase[*apSegOffset + 2] & 15);
		*apSegOffset += 4;
	}
	else if ((apTextSegBase[*apSegOffset + 3] & 15) == 1 && (apTextSegBase[*apSegOffset + 2] & 240) == 176 && (apTextSegBase[*apSegOffset] & 240) == 144)
	{
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "ldrexd%s ", arm_32a_c4(apTextSegBase[*apSegOffset + 3] >> 4));
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", (apTextSegBase[*apSegOffset + 1] >> 4) & 15);
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", apTextSegBase[*apSegOffset] & 15);
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u ", apTextSegBase[*apSegOffset + 2] & 15);
		*apSegOffset += 4;
	}
	else if ((apTextSegBase[*apSegOffset + 3] & 15) == 1 && (apTextSegBase[*apSegOffset + 2] & 240) == 192 && (apTextSegBase[*apSegOffset] & 240) == 144)
	{
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "strexb%s ", arm_32a_c4(apTextSegBase[*apSegOffset + 3] >> 4));
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", (apTextSegBase[*apSegOffset + 1] >> 4) & 15);
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", apTextSegBase[*apSegOffset] & 15);
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u ", apTextSegBase[*apSegOffset + 2] & 15);
		*apSegOffset += 4;
	}
	else if ((apTextSegBase[*apSegOffset + 3] & 15) == 1 && (apTextSegBase[*apSegOffset + 2] & 240) == 208 && (apTextSegBase[*apSegOffset] & 240) == 144)
	{
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "ldrexb%s ", arm_32a_c4(apTextSegBase[*apSegOffset + 3] >> 4));
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", (apTextSegBase[*apSegOffset + 1] >> 4) & 15);
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", apTextSegBase[*apSegOffset] & 15);
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u ", apTextSegBase[*apSegOffset + 2] & 15);
		*apSegOffset += 4;
	}
	else if ((apTextSegBase[*apSegOffset + 3] & 15) == 1 && (apTextSegBase[*apSegOffset + 2] & 240) == 224 && (apTextSegBase[*apSegOffset] & 240) == 144)
	{
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "strexh%s ", arm_32a_c4(apTextSegBase[*apSegOffset + 3] >> 4));
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", (apTextSegBase[*apSegOffset + 1] >> 4) & 15);
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", apTextSegBase[*apSegOffset] & 15);
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u ", apTextSegBase[*apSegOffset + 2] & 15);
		*apSegOffset += 4;
	}
	else if ((apTextSegBase[*apSegOffset + 3] & 15) == 1 && (apTextSegBase[*apSegOffset + 2] & 240) == 240 && (apTextSegBase[*apSegOffset] & 240) == 144)
	{
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "ldrexh%s ", arm_32a_c4(apTextSegBase[*apSegOffset + 3] >> 4));
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", (apTextSegBase[*apSegOffset + 1] >> 4) & 15);
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", apTextSegBase[*apSegOffset] & 15);
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u ", apTextSegBase[*apSegOffset + 2] & 15);
		*apSegOffset += 4;
	}
	else if ((apTextSegBase[*apSegOffset + 3] & 15) == 3 && (apTextSegBase[*apSegOffset + 2] & 240) == 16)
	{
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "movt%s ", arm_32a_c4(apTextSegBase[*apSegOffset + 3] >> 4));
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", (apTextSegBase[*apSegOffset + 1] >> 4) & 15);
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "%u ", apTextSegBase[*apSegOffset] + ((apTextSegBase[*apSegOffset + 1] & 15) << 8) + ((apTextSegBase[*apSegOffset + 2] & 15) << 12));
		*apSegOffset += 4;
	}
	else if ((apTextSegBase[*apSegOffset + 3] & 15) == 3 && (apTextSegBase[*apSegOffset + 2] & 240) == 32 && apTextSegBase[*apSegOffset] == 0)
	{
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "nop%s ", arm_32a_c4(apTextSegBase[*apSegOffset + 3] >> 4));
		*apSegOffset += 4;
	}
	else if ((apTextSegBase[*apSegOffset + 3] & 15) == 3 && (apTextSegBase[*apSegOffset + 2] & 240) == 32 && apTextSegBase[*apSegOffset] == 1)
	{
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "yield%s ", arm_32a_c4(apTextSegBase[*apSegOffset + 3] >> 4));
		*apSegOffset += 4;
	}
	else if ((apTextSegBase[*apSegOffset + 3] & 15) == 3 && (apTextSegBase[*apSegOffset + 2] & 240) == 32 && apTextSegBase[*apSegOffset] == 2)
	{
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "wfe%s ", arm_32a_c4(apTextSegBase[*apSegOffset + 3] >> 4));
		*apSegOffset += 4;
	}
	else if ((apTextSegBase[*apSegOffset + 3] & 15) == 3 && (apTextSegBase[*apSegOffset + 2] & 240) == 32 && apTextSegBase[*apSegOffset] == 3)
	{
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "wfi%s ", arm_32a_c4(apTextSegBase[*apSegOffset + 3] >> 4));
		*apSegOffset += 4;
	}
	else if ((apTextSegBase[*apSegOffset + 3] & 15) == 3 && (apTextSegBase[*apSegOffset + 2] & 240) == 32 && apTextSegBase[*apSegOffset] == 4)
	{
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "sev%s ", arm_32a_c4(apTextSegBase[*apSegOffset + 3] >> 4));
		*apSegOffset += 4;
	}
	else if ((apTextSegBase[*apSegOffset + 3] & 15) == 1 && (apTextSegBase[*apSegOffset + 2] == 0x2F) && (apTextSegBase[*apSegOffset + 1] == 0xFF) && ((apTextSegBase[*apSegOffset] & 0xF0) == 0x10))
	{
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "bx%s ", arm_32a_c4(apTextSegBase[*apSegOffset + 3] >> 4));
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u ", apTextSegBase[*apSegOffset] & 15);
		*apSegOffset += 4;
	}
	else if ((apTextSegBase[*apSegOffset + 3] & 15) == 3 && (apTextSegBase[*apSegOffset + 2] & 240) == 0x40)
	{
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "movt%s ", arm_32a_c4(apTextSegBase[*apSegOffset + 3] >> 4));
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", (apTextSegBase[*apSegOffset + 1] & 240) >> 4);
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "0x%04X ", (((UINT32)(apTextSegBase[*apSegOffset + 2] & 15)) << 12) + (((UINT32)(apTextSegBase[*apSegOffset + 1] & 15)) << 8) + (UINT32)(apTextSegBase[*apSegOffset] & 0xFF));
		*apSegOffset += 4;
	}
	else if ((apTextSegBase[*apSegOffset + 3] & 15) == 3 && (apTextSegBase[*apSegOffset + 2] & 240) == 0)
	{
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "movw%s ", arm_32a_c4(apTextSegBase[*apSegOffset + 3] >> 4));
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "r%u, ", (apTextSegBase[*apSegOffset + 1] & 240) >> 4);
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "0x%04X ", (((UINT32)(apTextSegBase[*apSegOffset + 2] & 15)) << 12) + (((UINT32)(apTextSegBase[*apSegOffset + 1] & 15)) << 8) + (UINT32)(apTextSegBase[*apSegOffset] & 0xFF));
		*apSegOffset += 4;
		}
	else
	{
		aMaxLen -= v; apOutput += v; v = K2ASC_PrintfLen(apOutput, aMaxLen, "?(0x%02X%02X%02X%02X) ", apTextSegBase[*apSegOffset + 3], apTextSegBase[*apSegOffset + 2], apTextSegBase[*apSegOffset + 1], apTextSegBase[*apSegOffset]);
		*apSegOffset += 4;
	}
}
