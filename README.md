##GHTTP
GHTTP is a simple binary binary module to make post requests on linux. GHTTP Automatically converts a provided table into json, and performs a post request.

A simple example:
```lua
-- Maybe notify a web server that a player joined a server? You can probably think of a better use.
-- We use SSL because of the api key. SSL would also make the transfer of other sensitive information more secure.

require('ghttp')

local function notify(ply)
	GHTTP.Post("http://coolgameservers.com/community/prophunt/snotify", { SteamID = ply:SteamID(), apikey = key }, function(response)
		
		-- GHTTP returns the entire response string
		
		if response != "ok" then
			print("something went wrong")
		end

	end, function(error)
		-- At its core, GHTTP uses libcurl for its functionality. It's error statements are simply forwarded.
		-- More general error codes are in the works
		print(error)
	end )
end

hook.Add("PlayerSpawn", "CommunityPlayerNotify", notify)
```
