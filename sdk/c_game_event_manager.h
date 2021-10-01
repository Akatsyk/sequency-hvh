#pragma once

#include <cstdint>
#include "macros.h"
#include "c_client_state.h"
class c_game_event
{
public:
	/*virtual ~c_game_event() = default;
	virtual const char* get_name() const = 0;

	virtual bool is_reliable() const = 0;
	virtual bool is_local() const = 0;
	virtual bool is_empty(const char* keyname = nullptr) = 0;

	virtual bool get_bool(const char* keyname = nullptr, bool default_value = false) = 0;
	virtual int GetInt(const char* keyname = nullptr, int default_value = 0) = 0;
	virtual uint64_t get_uint64(const char* keyname = nullptr, uint64_t default_value = 0) = 0;
	virtual float get_float(const char* keyname = nullptr, float default_value = 0.0f) = 0;
	virtual const char* get_string(const char* keyname = nullptr, const char* default_value = _("")) = 0;*/
	virtual ~c_game_event() {};
	virtual const char* GetName() const = 0; // get event name

	virtual bool IsReliable() const = 0; // if event handled reliable
	virtual bool IsLocal() const = 0; // if event is never networked
	virtual bool IsEmpty(const char* keyName = NULL) = 0; // check if data field exists

														  // Data access
	virtual bool GetBool(const char* keyName = NULL, bool defaultValue = false) = 0;
	virtual int GetInt(const char* keyName = NULL, int defaultValue = 0) = 0;
	virtual unsigned long long GetUint64(char const* keyName = NULL, unsigned long long defaultValue = 0) = 0;
	virtual float GetFloat(const char* keyName = NULL, float defaultValue = 0.0f) = 0;

	virtual const char* GetString(const char* keyName = NULL, const char* defaultValue = "") = 0;
	virtual const wchar_t* GetWString(char const* keyName = NULL, const wchar_t* defaultValue = L"") = 0;

	virtual void SetBool(const char* keyName, bool value) = 0;
	virtual void SetInt(const char* keyName, int value) = 0;
	virtual void SetUInt64(const char* keyName, unsigned long long value) = 0;
	virtual void SetFloat(const char* keyName, float value) = 0;
	virtual void SetString(const char* keyName, const char* value) = 0;
	virtual void SetWString(const char* keyName, const wchar_t* value) = 0;
};

class c_game_event_listener
{
public:
	virtual ~c_game_event_listener() {}
	virtual void fire_game_event(c_game_event* Event) = 0;

	virtual int GetEventDebugID()
	{
		return 42;
	}
};

class c_game_event_manager
{
public:
	/*virtual ~c_game_event_manager() = 0;
	virtual int load_events_from_file(const char* filename) = 0;
	virtual void reset() = 0;
	virtual bool add_listener(c_game_event_listener* listener, const char* name, bool serverside) = 0;
	virtual bool find_listener(c_game_event_listener* listener, const char* name) = 0;
	virtual void remove_listener(c_game_event_listener* listener) = 0;
	virtual void add_listener_global(c_game_event_listener* listener, bool serverside) = 0;*/


	virtual int __Unknown_1(int* dwUnknown) = 0;

	// load game event descriptions from a file eg "resource\gameevents.res"
	virtual int LoadEventsFromFile(const char* filename) = 0;

	// removes all and anything
	virtual void Reset() = 0;

	// adds a listener for a particular event
	virtual bool AddListener(c_game_event_listener* listener, const char* name, bool bServerSide) = 0;

	// returns true if this listener is listens to given event
	virtual bool FindListener(c_game_event_listener* listener, const char* name) = 0;

	// removes a listener 
	virtual int RemoveListener(c_game_event_listener* listener) = 0;

	// create an event by name, but doesn't fire it. returns NULL is event is not
	// known or no listener is registered for it. bForce forces the creation even if no listener is active
	virtual c_game_event* CreateEvent(const char* name, bool bForce, unsigned int dwUnknown) = 0;

	// fires a server event created earlier, if bDontBroadcast is set, event is not send to clients
	virtual bool FireEvent(c_game_event* event, bool bDontBroadcast = false) = 0;

	// fires an event for the local client only, should be used only by client code
	virtual bool FireEventClientSide(c_game_event* event) = 0;

	// create a new copy of this event, must be free later
	virtual c_game_event* DuplicateEvent(c_game_event* event) = 0;

	// if an event was created but not fired for some reason, it has to bee freed, same UnserializeEvent
	virtual void FreeEvent(c_game_event* event) = 0;

	// write/read event to/from bitbuffer
	virtual bool SerializeEvent(c_game_event* event, bf_write* buf) = 0;

	// create new KeyValues, must be deleted
	virtual c_game_event* UnserializeEvent(bf_read* buf) = 0;
};

interface_var(c_game_event_manager, game_event_manager, "engine.dll", "GAMEEVENTSMANAGER")
