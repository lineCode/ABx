#include "stdafx.h"
#include "NetworkMessage.h"
#include <abcrypto.hpp>
#include <AB/ProtocolCodes.h>

namespace Net {

std::string NetworkMessage::GetString(uint16_t len /* = 0 */)
{
    if (len == 0)
    {
        len = Get<uint16_t>();
        if (len >= (NETWORKMESSAGE_MAXSIZE - info_.position))
            return std::string();
    }
    if (!CanRead(len))
        return std::string();

    char* v = reinterpret_cast<char*>(buffer_) + info_.position;
    info_.position += len;
    return std::string(v, len);
}

std::string NetworkMessage::GetStringEncrypted()
{
    std::string encString = GetString();
    if (encString.empty())
        return encString;

    size_t len = encString.length();
    char* buff = new char[len + 1];
    memset(buff, 0, len + 1);
    memcpy_s((char*)(buff), len, encString.data(), len);
    uint32_t* buffer = (uint32_t*)(buff);
    xxtea_dec(buffer, (uint32_t)(len / 4), AB::ENC_KEY);
    std::string result(buff);
    delete[] buff;
    return result;
}

void NetworkMessage::AddPaddingBytes(uint32_t n)
{
    if (!CanAdd(n))
        return;

    memset((void*)&buffer_[info_.position], 0x33, n);
    info_.length += static_cast<MsgSize_t>(n);
}

void NetworkMessage::AddBytes(const char* bytes, uint32_t size)
{
    if (!CanAdd(size) || size > 8192)
        return;

    memcpy_s(buffer_ + info_.position, NETWORKMESSAGE_MAXSIZE, bytes, size);
    info_.position += static_cast<MsgSize_t>(size);
    info_.length += static_cast<MsgSize_t>(size);
}

void NetworkMessage::AddString(const std::string& value)
{
    uint16_t len = static_cast<uint16_t>(value.length());
    if (!CanAdd(len + 2) || len > 8192)
        return;
    Add<uint16_t>(len);
    // Allows also \0
    memcpy_s(buffer_ + info_.position, NETWORKMESSAGE_MAXSIZE, value.data(), len);
    info_.position += static_cast<MsgSize_t>(len);
    info_.length += static_cast<MsgSize_t>(len);
}

void NetworkMessage::AddString(const char* value)
{
    uint16_t len = static_cast<uint16_t>(strlen(value));
    if (!CanAdd(len + 2) || len > 8192)
        return;

    Add<uint16_t>(len);
    memcpy_s(buffer_ + info_.position, NETWORKMESSAGE_MAXSIZE, value, len);
    info_.position += static_cast<MsgSize_t>(len);
    info_.length += static_cast<MsgSize_t>(len);
}

void NetworkMessage::AddStringEncrypted(const std::string& value)
{
    uint16_t len = static_cast<uint16_t>(value.length());
    if (!CanAdd(len + 2) || len > 8192)
        return;

    if (value.empty())
    {
        AddString(value);
        return;
    }

    //add bytes until reach 8 multiple
    if ((len % 8) != 0)
    {
        uint16_t n = 8 - (len % 8);
        len += n;
    }
    char* buff = new char[len];
    memset(buff, 0, len);
    memcpy_s((char*)(buff), len, value.data(), len);
    uint32_t* buffer = (uint32_t*)(buff);
    xxtea_enc(buffer, len / 4, AB::ENC_KEY);
    std::string encString(buff, len);
    AddString(encString);
    delete[] buff;
}

}