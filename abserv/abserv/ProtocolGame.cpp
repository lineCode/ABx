#include "stdafx.h"
#include "ProtocolGame.h"

#include "DebugNew.h"

namespace Net {

ProtocolGame::ProtocolGame(std::shared_ptr<Connection> connection) :
    Protocol(connection)
{
}


ProtocolGame::~ProtocolGame()
{
}

void ProtocolGame::OnRecvFirstMessage(NetworkMessage& msg)
{
}

void ProtocolGame::OnConnect()
{
}

}
