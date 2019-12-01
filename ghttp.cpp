/**
	Author: Techmo
	Last modified: 8 February 2019

	This module allows for the use of a secure https connection when performing
	requests. This is essential for the transfer of sensitive information such as access
	tokens, api keys, and password hashes.
*/

#define CURL_STATICLIB
#define GMMODULE

#define VERSION "v1.1-curl"


#include "Interface.h"

extern "C" {
#include <curl/curl.h>
#include <string.h>
}

#include <stdio.h>
#include <vector>
#include <string>
#include <stdio.h>

// A macro to push functions to the global table and keep my sanity
#define AddFunction(func) {\
	LUA->PushCFunction(func);\
	LUA->SetField(-2, #func);\
}

#define print(str) {\
	LUA->PushSpecial(GarrysMod::Lua::SPECIAL_GLOB);\
	LUA->GetField(-1, "print");\
	LUA->PushString(str);\
	LUA->Call(1, 0);\
	LUA->Pop();\
}

using namespace GarrysMod::Lua;

// Flags and preferences
std::string cert_path = "";
bool skip_peer_verification = false;
bool skip_hostname_verification = false;
bool disabled = false;

/*
	Callback function for libcurl to use when it recieves data
*/
size_t curl_write_stdstring(void *contents, size_t size, size_t nmemb, std::string *s)
{
	size_t newLength = size * nmemb;
	try
	{
		s->append((char*)contents, newLength);
	}
	catch (std::bad_alloc &e)
	{
		return 0;
	}
	return newLength;
}

struct WriteThis {
  const char *readptr;
  size_t sizeleft;
};

/* 
   Creates an HTTPS post request

   SSLPost(url, parameters, response_callback, error_callback)
*/
int Post(lua_State* state) {

	if (disabled) return 0;

	// Check function parameters passed by lua
	LUA->CheckType(1, Type::STRING);
	LUA->CheckType(2, Type::TABLE);
	LUA->CheckType(3, Type::FUNCTION);
	LUA->CheckType(4, Type::FUNCTION);

	std::string req = "{";
	std::string resp;
	std::string url = LUA->GetString(1);
	bool first = true;
	print("Parsing table");
	// Iterate over table in stack
	// Find all the keys and values and add them to the json data
	LUA->PushNil();
	while (LUA->Next(2) != 0) {
		print("Adding table element");
		int key_type = LUA->GetType(-2);
		int val_type = LUA->GetType(-1);

		if (key_type == Type::STRING) {
			std::string key = LUA->GetString(-2);

			print((std::string("Adding key: ") + key).c_str());

			// Handle string table values
			if (val_type == Type::STRING) {
				if (first) {
					first = false;
				}
				else {
					req += ",";
				}

				std::string val = LUA->GetString(-1);
				print((std::string("Adding value: ") + val).c_str());
				req += "\"" + key + "\":\"" + val + "\"";
			}

			// Handle number table values
			else if (val_type == Type::NUMBER) {
				if (first) {
					first = false;
				}
				else {
					req += ",";
				}

				double val = LUA->GetNumber(-1);
				req += "\"" + key + "\":" + std::to_string(val);
			}

			// Handle boolean table values
			else if (val_type == Type::BOOL) {
				if (first) {
					first = false;
				}
				else {
					req += ",";
				}

				bool val = LUA->GetBool(-1);
				req += "\"" + key + "\":";

				if (val)
					req += "true";
				else
					req += "false";
			}
			else {
				// Sorry, if its not a number, string, or bool, you're SOL
				// TODO: Use lua api to convert any useless value into a string for some reason

			}
		}
		else {
			// Invalid table, key types are not correct for this application
		}

		// Pop value, keep key for next iteration
		LUA->Pop(1);
	}

	// Finish json data, and pray I didn't screw this up somehow
	req += "}";

	print((std::string("Constructed request: ") + req).c_str());

	//printf("%s\n", req.c_str());
	CURLcode res;
	CURL* curl;

	// Init CURL
	curl = curl_easy_init();

	// Set request headers
	struct curl_slist *headers = NULL;
	headers = curl_slist_append(headers, "Accept: application/json");
	headers = curl_slist_append(headers, "Content-Type: application/json");
	headers = curl_slist_append(headers, "charsets: utf-8");

	// Set URL
	curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

	// Build custom request
	curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, req.c_str());
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcrp/0.1");

	// Tell curl where to write response data
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_stdstring);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &resp);

    /* Set the expected POST size. If you want to POST large amounts of data,
       consider CURLOPT_POSTFIELDSIZE_LARGE */ 
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)strlen(req.c_str()));


	// Perform the request
	res = curl_easy_perform(curl);

	// Error check, calls error callback on error
	if (res != CURLE_OK) {
		//printf("GSSL:ERR SSLPost request failed: %s\n", curl_easy_strerror(res));

		// Push error callback function (which is at stack pos 4)
		LUA->Push(4);
		
		// Push error string
		LUA->PushString(curl_easy_strerror(res));

		// Call error callback
		// 1 argument on the stack, push 0 return values on the stack
		LUA->Call(1, 0);
	}
	else {
		// Push response callback function (which is at stack pos 3)
		LUA->Push(3);

		// Push response body to stack
		LUA->PushString(resp.c_str());


		// Call response callback
		LUA->Call(1, 0);
	}

	// Clean up
	curl_easy_cleanup(curl);

	return 0;
}


GMOD_MODULE_OPEN() {

	LUA->PushSpecial(GarrysMod::Lua::SPECIAL_GLOB);
	LUA->CreateTable();

	LUA->PushString(VERSION);
	LUA->SetField(-2, "VERSION");
	
	AddFunction(Post)

	LUA->SetField(-2, "GHTTP");
	LUA->Pop();


	/* Init CURL */
	if (curl_global_init(CURL_GLOBAL_DEFAULT)) {
		printf("GHTTP:ERR Global initialization failed. GHTTP requests will do nothing.\n");
		curl_global_cleanup();
		disabled = true;
	}

	return 0;
}

GMOD_MODULE_CLOSE() {

	LUA->PushNil();
	LUA->SetField(GarrysMod::Lua::SPECIAL_GLOB, "GHTTP");

	if(!disabled)
		curl_global_cleanup();

	return 0;
}
