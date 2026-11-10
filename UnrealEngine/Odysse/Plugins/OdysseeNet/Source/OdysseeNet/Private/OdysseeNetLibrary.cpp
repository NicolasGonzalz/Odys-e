#include "OdysseeNetLibrary.h"

#include <cstdint>
#include <string>

#include "odyssee/core/Config.hpp"
#include "odyssee/net/ByteBuffer.hpp"

FString UOdysseeNetLibrary::GetSharedVersion()
{
	return FString::Printf(TEXT("shared %s, protocole %d"), UTF8_TO_TCHAR(ODYSSEE_VERSION_STRING),
		static_cast<int32>(odyssee::kProtocolVersion));
}

bool UOdysseeNetLibrary::RoundTripThroughByteBuffer(int32 Number, const FString& Text,
	int32& OutNumber, FString& OutText)
{
	odyssee::net::ByteWriter writer;
	writer.writeU32(static_cast<std::uint32_t>(Number));
	writer.writeString(std::string(TCHAR_TO_UTF8(*Text)));

	odyssee::net::ByteReader reader(writer.data());
	const auto number = reader.readU32();
	const auto text = reader.readString();
	if (!number || !text)
	{
		return false;
	}

	OutNumber = static_cast<int32>(*number);
	OutText = UTF8_TO_TCHAR(text->c_str());
	return OutNumber == Number && OutText == Text;
}
