capiparity.exe ^
       	--api-header-prefix="%CD%/ShardScript/include" ^
       	"%CD%\ShardScript\include\shard\ShardScriptExtern.hpp" ^
       	-- ^
	-std=c++20 ^
       	-I%CD%/ShardScript/include ^
       	-isystem "%CD%/ShardScript/third_party/libuv/include" ^
       	-isystem "%CD%/ShardScript/third_party/mimalloc/include" ^
       	-DSHARD_API_EXPORT

pause