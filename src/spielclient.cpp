/**
 * spielclient.cpp
 * Autor: Sascha Hlusiak
 *
 * Klasse fuer einen Netzwerk-Client.
 **/

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#ifdef WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>
#endif

#include "spielclient.h"



/**
 * Leeren SpielClient initialisieren
 **/
CSpielClient::CSpielClient()
{
	start_new_game();
	client_socket=0;
	status.clients=status.player=status.computer=0;
	status.width=status.height=20;
	set_stone_numbers(0,0,0,0,0);
	for (int i=0;i<STONE_SIZE_MAX;i++)
		status.stone_numbers[i]=0;
}

CSpielClient::~CSpielClient()
{
	/* Verbindung trennen */
	Disconnect();
}

/**
 * Verbindet dem SpielClient mit einem SpielServer
 * host: Hostname oder IP des Servers
 * port: Port, auf dem Verbunden werden soll
 *
 * Rueckgabe: NULL bei Erfolg, sonst Zeiger auf Zeichenkette mit der Fehlermeldung
 **/

const char* CSpielClient::Connect(const char* host,int port, int blocking)
{
#ifndef WIN32
	if (port == 0)
	{
		sockaddr_un addr;

		errno=0;
		addr.sun_family=AF_UNIX;
		strcpy(addr.sun_path, host);
	
		/* Ein Socket erstellen */
		client_socket=socket(PF_UNIX,SOCK_STREAM,0);

		if (client_socket==-1)return strerror(errno);
		/* Jetzt versuchen, eine Verbindung zum Server aufzubauen */
		if (connect(client_socket,(sockaddr*)&addr,sizeof(addr))==-1)
		{
			/* Verbindungsaufbau Fehlgeschlagen */
			closesocket(client_socket);
			client_socket=0;
			return strerror(errno);
		}
		if (blocking == 0)
		{
			fcntl(client_socket,F_SETFL,O_NONBLOCK);
		}

		return NULL;
	}
#endif


#if (defined HAVE_GETADDRINFO) || (defined WIN32)
	/* Dies beinhaltet auch IPv6 */
	sockaddr_in *addr=NULL;
	struct addrinfo *addr_info,*p;
	int addr_len;
	const char* errormessage;

	errno=0;
	errormessage = NULL;
	/* Hostname in IP wandeln */
	errno=getaddrinfo(host,NULL,NULL,&addr_info);
	if (errno)return gai_strerror(errno);

	p=addr_info;
	client_socket = 0;
	while (p)
	{
		if (p->ai_protocol==0 || p->ai_protocol==IPPROTO_TCP)
		{
			addr=(sockaddr_in*)malloc(p->ai_addrlen);
			memcpy(addr,p->ai_addr,p->ai_addrlen);

			addr->sin_port=htons(port);
			addr_len=p->ai_addrlen;
			/* Ein Socket erstellen */
			client_socket=socket(p->ai_family,SOCK_STREAM,0);
			if (connect(client_socket,(sockaddr*)addr,addr_len) == 0)
			{
				errormessage = NULL;
				free(addr);
				break;
			}
			/* Verbindungsaufbau Fehlgeschlagen */
			closesocket(client_socket);
			client_socket=0;
		#ifdef WIN32
			errormessage = "Connection refused";
		#else
			errormessage = strerror(errno);
		#endif
			free(addr);
		}
		
		p=p->ai_next;
	}
	freeaddrinfo(addr_info);
	
	if (errormessage) return errormessage;
	if (p==NULL)
	{
		return "No IPv4 or IPv6 adresses found";
	}

#else
	sockaddr_in _addr,*addr=&_addr;
	int addr_len=sizeof(_addr);
	hostent *he;

	errno=0;
	_addr.sin_family=AF_INET;
	_addr.sin_port=htons(port);

	/* Hostname in IP wandeln */
	he=gethostbyname(host);
	if (he==NULL)return "Host not found";
	
	/* Host Adresse speichern */
	memcpy(&_addr.sin_addr,he->h_addr,he->h_length);
	/* Ein Socket erstellen */
	client_socket=socket(AF_INET,SOCK_STREAM,0);

	if (client_socket==-1)return strerror(errno);
	/* Jetzt versuchen, eine Verbindung zum Server aufzubauen */
	if (connect(client_socket,(sockaddr*)addr,addr_len)==-1)
	{
		/* Verbindungsaufbau Fehlgeschlagen */
		closesocket(client_socket);
		client_socket=0;
	#ifdef WIN32
		return "Connection refused";
	#else
		return strerror(errno);
	#endif
	}
#endif
	
	/* Den Socket nicht-blockierend machen, damit recv() Aufrufe sofort
	 * zurueckkehren, wenn keine Daten vorliegen. 
	 * Der Socket des Clients wird periodisch "gepollt".
	 */
	if (blocking == 0)
	{
#ifdef WIN32
		unsigned long block=1;
		ioctlsocket(client_socket,FIONBIO,&block);
#else
		fcntl(client_socket,F_SETFL,O_NONBLOCK);
#endif
	}

	return NULL;
}


void CSpielClient::Connect(int client_socket, int blocking)
{
	this->client_socket = client_socket;
	if (blocking == 0)
	{
		fcntl(client_socket,F_SETFL,O_NONBLOCK);
	}
}

/** 
 *Verbindung zum Server trennen
 */
void CSpielClient::Disconnect()
{
	if (client_socket)closesocket(client_socket);
	client_socket=0;
}

/**
 * Alle anstehenden Netzwerknachrichten abholen und verarbeiten.
 * Gibt NULL bei Erfolg zurueck, sonst Zeiger auf Fehlermeldung, die anzeigt
 * dass die Verbindung zum Server verloren wurde
 **/
const char* CSpielClient::poll()
{
	const char *err;
	/* Puffer fuer eine Netzwerknachricht. */
	static char buffer[512];
	do
	{
		/* Lese eine Nachricht komplett aus dem Socket in buffer */
		err=read_network_message(client_socket,(NET_HEADER*)buffer,sizeof(buffer));
		/* Bei 0 ist eine Nachricht erfolgreicht gelesen worden. Verarbeiten! */
		if (err==NULL)process_message((NET_HEADER*)buffer);
		/* bei -1 liegen keine Daten mehr vor */
		else if (err!=(char*)(-1))
		{
			/* Ansonsten ist ein Lesefehler aufgetreten 
			   und die Verbindung wird getrennt */
			Disconnect();
			return err;
		}
		/* Solange wiederholen, wie erfolgreich Nachrichten gelesen wurden */
	}while (err==NULL);
	return NULL;
}

/**
 * Erbitte Spielserver um einen lokalen Spieler.
 * Server schickt eine Spielernummer an den Client zurueck.
 **/
void CSpielClient::request_player(int wish_player, char* name)const
{
	NET_REQUEST_PLAYER data;
	data.player = wish_player;
	if (name == NULL)
		data.name[0] = '\0';
	else
		strncpy((char*)&data.name[0], name, sizeof(data.name));
	data.name[sizeof(data.name) - 1] = '\0';
	send_message((NET_HEADER*)&data,sizeof(data),MSG_REQUEST_PLAYER);
}

/**
 * Verarbeitet eine einzelne Netzwerknachricht
 **/
void CSpielClient::process_message(NET_HEADER* data)
{
	int i;
	switch(data->msg_type)
	{
		/* Der Server gewaehrt dem Client einen lokalen Spieler */
		case MSG_GRANT_PLAYER:i=((NET_GRANT_PLAYER*)data)->player;
			/* Merken, dass es sich bei i um einen lokalen Spieler handelt */
			spieler[i]=PLAYER_LOCAL;
			break;

		/* Der Server hat einen aktuellen Spieler festgelegt */
		case MSG_CURRENT_PLAYER: m_current_player=((NET_CURRENT_PLAYER*)data)->player;
			newCurrentPlayer(m_current_player);
			break;

		/* Nachricht des Servers ueber ein endgueltiges Setzen eines Steins auf das Feld */
		case MSG_SET_STONE: {
			NET_SET_STONE *s=(NET_SET_STONE*)data;
			/* Entsprechenden Stein des Spielers holen */
			CStone *stone=get_player(s->player)->get_stone(s->stone);
			/* Stein in richtige Position drehen */
			stone->mirror_rotate_to(s->mirror_count,s->rotate_count);
			/* Stein aufs echte Spielfeld setzen */
			if ((CSpiel::is_valid_turn(stone, s->player, s->y, s->x) == FIELD_DENIED) ||
			   (CSpiel::set_stone(stone, s->player,s->y,s->x)!=FIELD_ALLOWED))
			{	// Spiel scheint nicht mehr synchron zu sein
				// GAANZ schlecht!!
				printf("Game not in sync!\n");
				exit(1);
			}
			/* Zug der History anhaengen */
			addHistory(s->player,stone,s->y,s->x);
			stoneWasSet(s);
			break;
		}
		case MSG_STONE_HINT: {
			NET_SET_STONE *s=(NET_SET_STONE*)data;
			hintReceived(s);
			break;
		}

		/* Server hat entschlossen, dass das Spiel vorbei ist */
		case MSG_GAME_FINISH:{
			gameFinished();
			break;
		}

		/* Ein Server-Status Paket ist eingetroffen, Inhalt merken */
		case MSG_SERVER_STATUS:
		{
			NET_SERVER_STATUS *s=(NET_SERVER_STATUS*)data;
			status.clients=s->clients;
			status.player=s->player;
			status.computer=s->computer;
			/* Wenn Spielfeldgroesse sich von Server unterscheidet,
			   lokale Spielfeldgroesse hier anpassen */
			status.width=s->width;
			status.height=s->height;
			if (status.width!=get_field_size_x() || status.height!=get_field_size_y())
				set_field_size_and_new(status.height,status.width);
			{
				bool changed=false;
				for (int i=0;i<STONE_SIZE_MAX;i++)
				{
					changed |= (status.stone_numbers[i] != s->stone_numbers[i]);
					status.stone_numbers[i]=s->stone_numbers[i];
				}
				if (changed)set_stone_numbers(status.stone_numbers[0],status.stone_numbers[1],status.stone_numbers[2],status.stone_numbers[3],status.stone_numbers[4]);
			}
			m_gamemode=(GAMEMODE)s->gamemode;
			if (m_gamemode==GAMEMODE_4_COLORS_2_PLAYERS)
				set_teams(0,2,1,3);
			if (m_gamemode==GAMEMODE_2_COLORS_2_PLAYERS || m_gamemode == GAMEMODE_DUO)
			{
				for (int n = 0 ; n < STONE_COUNT_ALL_SHAPES; n++){
					get_player(1)->get_stone(n)->set_available(0);
					get_player(3)->get_stone(n)->set_available(0);
				}
			}
			if (ntohs(data->data_length) == sizeof(NET_SERVER_STATUS)) {
				int i;
				memcpy(status.client_names, s->client_names, sizeof(s->client_names));
				for (i = 0; i < PLAYER_MAX; i++) {
					status.spieler[i] = s->spieler[i];
					printf("Spieler %d: %d\n", i, status.spieler[i]);
				}
				for (int i = 0; i < CLIENTS_MAX; i++) {
					if (strlen((char*)status.client_names[i]) > 0)
						printf("Client %d: %s\n", i, (char*)status.client_names[i]);
				}
			}
		}
		break;

		/* Server hat eine Chat-Nachricht geschickt. */
		case MSG_CHAT: {
			NET_CHAT* chat=(NET_CHAT*)data;
			chatReceived(chat);
			break;
		}
		/* Der Server hat eine neue Runde gestartet. Spiel zuruecksetzen */
		case MSG_START_GAME: {
			CSpiel::start_new_game();
			/* Unbedingt history leeren. */
			if (history)history->delete_all_turns();

			set_stone_numbers(status.stone_numbers[0],status.stone_numbers[1],status.stone_numbers[2],status.stone_numbers[3],status.stone_numbers[4]);
			if (m_gamemode==GAMEMODE_4_COLORS_2_PLAYERS)
				set_teams(0,2,1,3);
			if (m_gamemode==GAMEMODE_2_COLORS_2_PLAYERS || m_gamemode==GAMEMODE_DUO)
			{
				for (int n = 0 ; n < STONE_COUNT_ALL_SHAPES; n++){
					get_player(1)->get_stone(n)->set_available(0);
					get_player(3)->get_stone(n)->set_available(0);
				}
			}
			m_current_player=-1;
			gameStarted();
			break;
		}

		/* Server laesst den letzten Zug rueckgaengig machen */
		case MSG_UNDO_STONE: {
			CTurn *t=history->get_last_turn();
			CStone *stone=get_player(t->get_playernumber())->get_stone(t->get_stone_number());
			stoneUndone(stone, t);
			undo_turn(history);
			break;
		}
		default: printf("FEHLER: unbekannte Nachricht empfangen: #%d\n",data->msg_type);
			break;
	}
}

/**
 * Wird von der GUI aufgerufen, wenn ein Spieler einen Stein setzen will
 * Die Aktion wird nur an den Server geschickt, der Stein wird NICHT lokal gesetzt
 **/
TSingleField CSpielClient::set_stone(CStone* stone, int stone_number, int y, int x)
{
	NET_SET_STONE data;
	if (m_current_player==-1)return FIELD_DENIED;

	/* Datenstruktur mit Daten der Aktion fuellen */
	data.player=m_current_player;
	data.stone=stone_number;
	data.mirror_count=stone->get_mirror_counter();
	data.rotate_count=stone->get_rotate_counter();
	data.x=x;
	data.y=y;

	/* Nachricht ueber den Stein an den Server schicken */
	send_message((NET_HEADER*)&data,sizeof(data),MSG_SET_STONE);

	/* Lokal keinen Spieler als aktiv setzen.
	   Der Server schickt uns nachher den neuen aktiven Spieler zu */
	set_noplayer();
	return FIELD_ALLOWED;
}

/**
 * Erbittet den Spielstart beim Server
 **/
void CSpielClient::request_start()const
{
	NET_START_GAME data;
	send_message((NET_HEADER*)&data,sizeof(data),MSG_START_GAME);
}

/**
 * Erbittet eine Zugzuruecknahme beim Server
 **/
void CSpielClient::request_undo()const
{
	NET_REQUEST_UNDO data;
	send_message((NET_HEADER*)&data,sizeof(data),MSG_REQUEST_UNDO);
}

/**
 * Schickt eine Chat-Nachricht an den Server. Dieser wird sie schnellstmoeglich
 * an alle Clients weiterleiten (darunter auch dieser Client selbst).
 **/
void CSpielClient::chat(const char *text)
{
	/* Bei Textlaenge von 0 wird nix verschickt */
	if (strlen(text)<1)return;
	/* Berechne Platz der NET_CHAT Nachricht, dass der Text exakt ans Ende passt */
	int len=sizeof(NET_CHAT)+sizeof(char)*strlen(text)+1;
	/* Reserviere ordentlich Speicher */
	NET_CHAT *chat=(NET_CHAT*)malloc(len);
	/* Merke Laenge des Textes in der Nachricht */
	chat->length=strlen(text);
	/* text in die Nachricht kopieren. */
	strcpy((char*)&chat->text[0],text);
	/* Nachricht an den Server schicken */
	send_message((NET_HEADER*)chat,len,MSG_CHAT);
	/* Reservierten Speicher der Nachricht wieder freigeben */
	free(chat);
}

/**
 * Gibt true zurueck, wenn der Spieler kein Computerspieler ist
 **/
const bool CSpielClient::is_local_player(const int player)const
{
	/* Bei keinem aktuellem Spieler, ist der aktuelle natuerlich nicht lokal. */
	if (player==-1)return false;
	return (spieler[player]!=PLAYER_COMPUTER);
}
