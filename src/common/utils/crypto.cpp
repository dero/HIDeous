#include "crypto.h"
#include <windows.h>
#include <wincrypt.h>
#include <string>

/**
 * Get a short hash of a longer string, such as a device name.
 */
std::wstring GetShortHash(const std::wstring &longString)
{
	HCRYPTPROV hProv = 0;
	HCRYPTHASH hHash = 0;
	BYTE rgbHash[16];
	DWORD cbHash = 16;
	CHAR rgbDigits[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	std::string hashStr;

	if (CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT))
	{
		if (CryptCreateHash(hProv, CALG_MD5, 0, 0, &hHash))
		{
			if (CryptHashData(hHash, (BYTE *)longString.c_str(), longString.length() * sizeof(wchar_t), 0))
			{
				if (CryptGetHashParam(hHash, HP_HASHVAL, rgbHash, &cbHash, 0))
				{
					for (DWORD i = 0; i < 3; i++) // Only take the first 3 bytes to create a 6 character hash
					{
						hashStr.append(1, rgbDigits[rgbHash[i] >> 4]);
						hashStr.append(1, rgbDigits[rgbHash[i] & 0xf]);
					}
				}
			}
			CryptDestroyHash(hHash);
		}
		CryptReleaseContext(hProv, 0);
	}

	return std::wstring(hashStr.begin(), hashStr.end());
}
