#include "Common.h"
#include "ProfilerServer.h"

#include "Socket.h"
#include "Message.h"

#pragma comment( lib, "ws2_32.lib" )

namespace Profiler
{
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static const short DEFAULT_PORT = 31313;
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Server::Server(short port) : socket(new Socket())
{
	socket->Bind(port, 8);
	socket->Listen();
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void Server::Update()
{
	CRITICAL_SECTION(lock);

	InitConnection();

	int length = -1;
	while ( (length = socket->Receive( buffer, BIFFER_SIZE ) ) > 0 )
	{
		networkStream.Append(buffer, length);
	}

	while (IMessage *message = IMessage::Create(networkStream))
	{
		message->Apply();
		delete message;
	}
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void Server::Send(DataResponse::Type type, OutputDataStream& stream)
{
	CRITICAL_SECTION(lock);

	std::string data = stream.GetData();

	DataResponse response(type, (uint32)data.size());
	socket->Send((char*)&response, sizeof(response));
	socket->Send(data.c_str(), data.size());
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool Server::InitConnection()
{
	if (!acceptThread)
	{
		acceptThread.Create(&Server::AsyncAccept, this);
		return true;
	}
	return false;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Server::~Server()
{
	acceptThread.Terminate();

	if (socket)
	{
		delete socket;
		socket = nullptr;
	}
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Server & Server::Get()
{
	static Server instance(DEFAULT_PORT);
	return instance;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool Server::Accept()
{
	socket->Accept();
	return true;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
DWORD WINAPI Server::AsyncAccept( LPVOID lpParam )
{
	Server* server = (Server*)lpParam;

	while (server->Accept())
	{
		ThreadSleep(1000);
	}

	return 0;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

}